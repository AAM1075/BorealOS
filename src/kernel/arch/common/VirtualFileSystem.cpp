#include <FileSystemInterface.h>

#include "Utility/Path.h"
#include "Utility/StringFormatter.h"

namespace FileSystem {
    VirtualFileSystem::VirtualFileSystem() : _mountedFileSystems(8) {

    }

    FSResult VirtualFileSystem::Mount(const Utility::String &prefix, FileSystemInterface *fs, MountFlags flags) {
        auto lowered = prefix.ToLower();
        if (_mountedFileSystems.Get(lowered).HasValue()) {
            return FSResult::ALREADY_EXISTS; // A file system is already mounted with this prefix
        }

        const auto& normalizedPrefix = prefix;

        // Check if the prefix is longer than 128, or does not match [a-zA-Z0-9_]{1,128}
        if (normalizedPrefix.Size() == 0 || normalizedPrefix.Size() > 128) {
            return FSResult::INVALID_PATH; // Prefix must be between 1 and 128 characters long
        }

        for (size_t i = 0; i < normalizedPrefix.Size(); i++) {
            char c = normalizedPrefix.CStr()[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
                return FSResult::INVALID_PATH; // Prefix contains invalid characters
            }
        }

        _mountedFileSystems.Insert(Utility::Move(lowered), {Utility::Move(normalizedPrefix), fs, flags});
        return FSResult::SUCCESS;
    }

    FSResult VirtualFileSystem::Unmount(const Utility::String &prefix) {
        auto lowered = prefix.ToLower();
        if (!_mountedFileSystems.Get(lowered).HasValue()) {
            return FSResult::MISSING_MOUNT; // No file system is mounted with this prefix
        }

        _mountedFileSystems.Remove(lowered);
        return FSResult::SUCCESS;
    }

    FSResult VirtualFileSystem::Open(const Utility::String &path, OpenFlags flags, Descriptor *outDescriptor, FileMode mode) {
        Utility::String normalized;
        if (CanonicalizePath(path, "", normalized) != FSResult::SUCCESS) {
            return FSResult::INVALID_PATH; // Failed to canonicalize the path, likely due to invalid format
        }

        return InternalOpen(flags, outDescriptor, mode, normalized);
    }

    FSResult VirtualFileSystem::Open(const Utility::String &path, const Utility::String &currentWorkingDirectory,
        OpenFlags flags, Descriptor *outDescriptor, FileMode mode) {
        Utility::String normalized;
        if (CanonicalizePath(path, currentWorkingDirectory, normalized) != FSResult::SUCCESS) {
            return FSResult::INVALID_PATH; // Failed to canonicalize the path, likely due to invalid format
        }

        return InternalOpen(flags, outDescriptor, mode, normalized);
    }

    FSResult VirtualFileSystem::Read(Descriptor *descriptor, void *buffer, size_t size, size_t *bytesRead) {
        if (descriptor->file == nullptr || descriptor->fs == nullptr) {
            return FSResult::INVALID_DESCRIPTOR;
        }

        if (bytesRead)
            *bytesRead = 0;

        if ((uint32_t)(descriptor->flags & OpenFlags::Read) == 0) {
            return FSResult::READ_PERMISSION;
        }

        size_t actualBytesRead = 0;
        auto result = descriptor->fs->Read(descriptor->file, buffer, size, descriptor->offset, &actualBytesRead);
        if (result != FSResult::SUCCESS) {
            return result; // Failed to read from the underlying file system, return the error code
        }

        descriptor->offset += actualBytesRead; // Move the offset forward by the number of bytes read

        if (bytesRead)
            *bytesRead = actualBytesRead;

        return FSResult::SUCCESS;
    }

    FSResult VirtualFileSystem::Write(Descriptor *descriptor, const void *buffer, size_t size, size_t *bytesWritten) {
        if (descriptor->file == nullptr || descriptor->fs == nullptr) {
            return FSResult::INVALID_DESCRIPTOR;
        }

        if (bytesWritten)
            *bytesWritten = 0;

        if ((uint32_t)(descriptor->flags & OpenFlags::Write) == 0) {
            return FSResult::WRITE_PERMISSION;
        }

        size_t actualBytesWritten = 0;
        auto result = descriptor->fs->Write(descriptor->file, buffer, size, descriptor->offset, &actualBytesWritten);
        if (result != FSResult::SUCCESS) {
            return result;
        }

        descriptor->offset += actualBytesWritten; // Move the offset forward by the number of bytes written

        if (bytesWritten)
            *bytesWritten = actualBytesWritten;

        return FSResult::SUCCESS;
    }

    FSResult VirtualFileSystem::GetFileInfo(Descriptor *descriptor, FileInfo *outInfo) {
        return descriptor->fs->GetFileInfo(descriptor->file, outInfo);
    }

    FSResult VirtualFileSystem::GetDirectoryInfo(Descriptor *descriptor, DirectoryInfo *outInfo) {
        return descriptor->fs->GetDirectoryInfo(descriptor->file, outInfo);
    }

    void VirtualFileSystem::FreeDirectoryInfo(Descriptor *directory, DirectoryInfo *info) {
        directory->fs->FreeDirectoryInfo(info);
    }

    FSResult VirtualFileSystem::Close(Descriptor *descriptor) {
        auto result = descriptor->fs->Close(descriptor->file);
        if (result != FSResult::SUCCESS) {
            return result; // Failed to close the file in the underlying file system, return the error code
        }

        descriptor->file = nullptr;
        descriptor->fs = nullptr;
        descriptor->offset = 0;
        descriptor->flags = OpenFlags::None;
        return FSResult::SUCCESS;
    }

    FSResult VirtualFileSystem::CanonicalizePath(const Utility::String &path,
                                                 const Utility::String &cwd, Utility::String &outCanonicalPath) {

        Utility::String output;
        size_t colonIndex = path.Find(":");
        if (colonIndex == size_t(-1)) {
            if (cwd.IsEmpty())
                return FSResult::INVALID_PATH; // Relative path provided, but current working directory is empty

            output = cwd + "/" + path;
        }
        else {
            output = path; // Absolute path, no need to prepend the current working directory
        }

        colonIndex = output.Find(":");
        if (colonIndex == size_t(-1))
            return FSResult::INVALID_PATH; // if CWD did not include a prefix

        Utility::String prefix = output.SubStringView(0, colonIndex + 1).ToString().ToLower(); // include the colon in the prefix

        auto pathPart = output.SubStringView(colonIndex + 1);
        size_t componentCount = Utility::Path::GetComponentCount(pathPart.Data(), '/');

        auto parts = new Utility::StringView[componentCount];
        Utility::Path::SplitPath(pathPart.Data(), '/', parts);

        auto stack = new Utility::StringView[componentCount];
        size_t stackSize = 0;

        for (size_t i = 0; i < componentCount; i++) {
            const char* data = parts[i].Data();
            size_t size = parts[i].Size();

            if (size == 0) continue;
            if (size == 1 && data[0] == '.') continue; // Current directory, skip
            if (size == 2 && data[0] == '.' && data[1] == '.') {
                if (stackSize == 0) {
                    delete[] parts;
                    delete[] stack;
                    return FSResult::OUT_OF_MOUNT; // Trying to navigate above the root of the mounted file system, which is not allowed
                }

                stackSize--;
                continue;
            }

            stack[stackSize++] = parts[i]; // Normal path component, push onto the stack
        }

        Utility::String result = prefix;
        for (size_t i = 0; i < stackSize; i++)
            result = result + "/" + stack[i].ToString();

        if (stackSize == 0)
            result = result + "/";

        delete[] parts;
        delete[] stack;

        outCanonicalPath = Utility::Move(result);
        return FSResult::SUCCESS;
    }

    FSResult VirtualFileSystem::InternalOpen(OpenFlags flags, Descriptor *outDescriptor, FileMode mode, const Utility::String &path) const {
        size_t colonIndex = path.Find(":");
        if (colonIndex == size_t(-1)) {
            return FSResult::INVALID_PATH; // Path does not contain a colon
        }

        auto prefix = path.SubStringView(0, colonIndex).ToString().ToLower();
        auto relativePath = path.SubStringView(colonIndex + 1);

        auto mountedFS = _mountedFileSystems.Get(prefix);
        if (!mountedFS.HasValue()) {
            return FSResult::MISSING_MOUNT; // No file system is mounted with this prefix
        }

        if ((uint32_t)(mountedFS.Value().flags & MountFlags::READ_ONLY) != 0 && (uint32_t)(flags & OpenFlags::Write) != 0) {
            return FSResult::WRITE_PERMISSION; // The file system is mounted as read-only, but the open flags request write access
        }

        File* file = nullptr;
        auto result = mountedFS.Value().fs->Open(relativePath, flags, &file, mode);
        if (result != FSResult::SUCCESS) {
            return result; // Failed to open the file in the underlying file system, return the error code
        }

        *outDescriptor = {
            .file = file,
            .fs = mountedFS.Value().fs,
            .offset = 0,
            .flags = flags,
        };

        return FSResult::SUCCESS;
    }
}
