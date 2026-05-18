#ifndef BOREALOS_FILESYSTEM_H
#define BOREALOS_FILESYSTEM_H

#include <Definitions.h>
#include "Allocator.h"
#include "Utility/HashMap.h"
#include "Utility/String.h"
#include "Utility/StringView.h"

// Mode bits (same as in uapi/linux/stat.h):

#define S_IFMT 0017000 // bitmask for the file type bitfields
#define S_IFSOCK 0140000 // socket
#define S_IFLNK 0120000 // symbolic link (unsupported in BorealOS, but we include it for completeness)
#define S_IFREG 0100000 // regular file
#define S_IFBLK 0060000 // block device
#define S_IFDIR 0040000 // directory
#define S_IFCHR 0020000 // character device
#define S_IFIFO 0010000 // FIFO
#define S_ISUID 0004000 // set-user-ID bit
#define S_ISGID 0002000 // set-group-ID bit
#define S_ISVTX 0001000 // sticky bit

// Permission bits:
#define S_IRWXU 0000700 // owner has read, write, and execute permission
#define S_IRUSR 0000400 // owner has read permission
#define S_IWUSR 0000200 // owner has write permission
#define S_IXUSR 0000100 // owner has execute permission

#define S_IRWXG 0000070 // group has read, write, and execute permission
#define S_IRGRP 0000040 // group has read permission
#define S_IWGRP 0000020 // group has write permission
#define S_IXGRP 0000010 // group has execute permission

#define S_IRWXO 0000007 // others have read, write, and execute permission
#define S_IROTH 0000004 // others have read permission
#define S_IWOTH 0000002 // others have write permission
#define S_IXOTH 0000001 // others have execute permission


/// Abstract interface for a file system.
namespace FileSystem {
    enum class FSResult : uint32_t {
        SUCCESS = 0,

        WRITE_PERMISSION = 1, // No write permission for the file
        READ_PERMISSION = 2, // No read permission for the file
        MISSING_MOUNT = 3, // No file system mounted for the given prefix
        MISSING_ENTRY = 4, // The specified file or directory does not exist in the file system
        EXEC_PERMISSION = 5, // No execute permission for the file
        OUT_OF_MOUNT = 6, // The specified path tried to access outside the mounted file system (e.g. "partition_a:/../other_partition/file.txt")
        NOT_A_DIRECTORY = 7, // Tried to access a file as a directory
        ALREADY_EXISTS = 8, // Tried to create a file or directory that already exists
        INVALID_PATH = 9, // The format of the path is invalid
        UNSUPPORTED = 10, // The requested operation is not supported by this file system (e.g. writing to a read-only file system)
        INVALID_DESCRIPTOR = 11, // The provided file descriptor is not valid

        UNKNOWN = uint32_t(-1) // An unknown error occurred
    };

    enum class OpenFlags : uint32_t {
        None = 0,
        Read = 1 << 0, // Open the file for reading
        Write = 1 << 1, // Open the file for writing
        Create = 1 << 2, // Create the file if it doesn't exist (implies Write)
        Truncate = 1 << 3, // Truncate the file to zero length if it already exists (implies Write)
        Append = 1 << 4, // Move the file offset to the end of the file before each write (implies Write)
    };

    static OpenFlags operator| (OpenFlags a, OpenFlags b) {
        return static_cast<OpenFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    static OpenFlags operator& (OpenFlags a, OpenFlags b) {
        return static_cast<OpenFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    enum class MountFlags : uint32_t {
        NONE = 0,
        READ_ONLY = 1 << 0,
        NO_EXEC = 1 << 1,
        SYNC = 1 << 2,
    };

    static MountFlags operator| (MountFlags a, MountFlags b) {
        return static_cast<MountFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    static MountFlags operator& (MountFlags a, MountFlags b) {
        return static_cast<MountFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    enum class FileMode : uint32_t {
        File = S_IFREG,
        Directory = S_IFDIR,
        CharacterDevice = S_IFCHR,
        BlockDevice = S_IFBLK,
        FIFO = S_IFIFO,
        Socket = S_IFSOCK,
    };

    class FileSystemInterface;

    struct File; // Opaque file struct, the actual definition is up to the implementation of the FileSystem.
    struct Descriptor {
        File* file; // The file that this descriptor refers to.
        FileSystemInterface* fs; // The file system that this descriptor belongs to, used to perform operations on the file.
        size_t offset; // The current offset in the file for read/write operations.
        OpenFlags flags;
    };

    struct FileInfo {
        size_t size = 0; // The size of the file in bytes.
        uint32_t mode = 0;
        uint32_t ownerUserId = 0; // The user ID of the owner of the file.
        uint32_t ownerGroupId = 0; // The group ID of the owner of the file.
    };

    struct Capabilities {
        bool canRead = false;
        bool canWrite = false;
    };

    struct DirectoryInfo {
        size_t entryCount = 0;
        const char** entries = nullptr; // Array of null-terminated strings representing the names of the entries in the directory.
    };

    class FileSystemInterface {
    public:
        explicit FileSystemInterface(Allocator* allocator) : _allocator(allocator) {}
        virtual ~FileSystemInterface() = default;

        /// Returns the capabilities of this file system, such as whether it supports reading and writing.
        [[nodiscard]] virtual Capabilities GetCapabilities() const = 0;

        /// Opens a file at the given path and returns a pointer to a File struct representing it, or nullptr if the file doesn't exist or couldn't be opened.
        [[nodiscard]] virtual FSResult Open(Utility::StringView path, OpenFlags flags, File **outFile, FileMode mode = FileMode::File) = 0;

        /// Reads up to size bytes from the given file into the provided buffer, and returns the number of bytes actually read, or -1 if there was an error.
        virtual FSResult Read(File* file, void* buffer, size_t size, size_t offset, size_t *outReadBytes) = 0;

        /// Writes size bytes from the provided buffer to the given file, and returns the number of bytes actually written, or -1 if there was an error.
        virtual FSResult Write(File* file, const void* buffer, size_t size, size_t offset, size_t *outWrittenBytes) = 0;

        /// Returns information about the given file, such as its size and whether it's a directory. Returns true on success, or false if there was an error.
        virtual FSResult GetFileInfo(File *file, FileInfo *info) = 0;

        /// If the given file is a directory, fills the provided DirectoryInfo struct with information about the directory.
        virtual FSResult GetDirectoryInfo(File *file, DirectoryInfo *info) = 0;

        /// Frees any resources associated with the given DirectoryInfo struct, such as the entries array.
        virtual void FreeDirectoryInfo(DirectoryInfo* info) = 0;

        /// Closes the given file and releases any resources associated with it.
        virtual FSResult Close(File *file) = 0;

        // QoL functions that crash the kernel on error.
        File* OpenOrPanic(Utility::StringView path) {
            File* file = nullptr;
            auto result = Open(path, OpenFlags::Read | OpenFlags::Write, &file);
            if (result != FSResult::SUCCESS) {
                LOG_ERROR("Failed to open file \"%s\". Error code: %u32", path.ToString().CStr(), static_cast<uint32_t>(result));
                PANIC("Failed to open file.");
            }

            return file;
        }

        size_t ReadOrPanic(File* file, void* buffer, size_t size) {
            size_t bytesRead = 0;
            auto result = Read(file, buffer, size, 0, &bytesRead);
            if (result != FSResult::SUCCESS) {
                LOG_ERROR("Failed to read from file. Error code: %u32", static_cast<uint32_t>(result));
                PANIC("Failed to read from file.");
            }

            return bytesRead;
        }

        size_t WriteOrPanic(File* file, const void* buffer, size_t size) {
            size_t bytesWritten = 0;
            auto result = Write(file, buffer, size, 0, &bytesWritten);
            if (result != FSResult::SUCCESS) {
                LOG_ERROR("Failed to write to file. Error code: %u32", static_cast<uint32_t>(result));
                PANIC("Failed to write to file.");
            }

            return bytesWritten;
        }

        FileInfo GetFileInfoOrPanic(File* file) {
            FileInfo info = {};
            auto result = GetFileInfo(file, &info);
            if (result != FSResult::SUCCESS) {
                LOG_ERROR("Failed to get file info. Error code: %u32", static_cast<uint32_t>(result));
                PANIC("Failed to get file info.");
            }

            return info;
        }

        DirectoryInfo GetDirectoryInfoOrPanic(File* file) {
            DirectoryInfo info = {};
            auto result = GetDirectoryInfo(file, &info);
            if (result != FSResult::SUCCESS) {
                LOG_ERROR("Failed to get directory info. Error code: %u32", static_cast<uint32_t>(result));
                PANIC("Failed to get directory info.");
            }

            return info;
        }

    protected:
        Allocator *_allocator;
    };

    // The VFS acts as a collection of mounted file systems, and is responsible for routing file operations to the correct file system based on the file path. It also provides a unified interface for performing file operations across all mounted file systems.
    // The VFS uses a Windows like path system, where disks are represented by a prefix, like "partition_a", or "ramfs".
    // You can access them using paths like "partition_a:/path/to/file" or "ramfs:/path/to/file". The VFS will route the operation to the correct file system based on the prefix before the colon.
    class VirtualFileSystem {
    public:
        explicit VirtualFileSystem();

        FSResult Mount(const Utility::String& prefix, FileSystemInterface* fs, MountFlags flags = MountFlags::NONE);
        FSResult Unmount(const Utility::String& prefix);

        // Open a file with an absolute path including prefix.
        // Valid: path="partition_a:/path/to/file.txt"
        // Valid: path="partition_b:/something/something/../file.txt"
        FSResult Open(const Utility::String& path, OpenFlags flags, Descriptor* outDescriptor, FileMode mode = FileMode::File);

        // Open a file relative to a current working directory. The path can include special navigation components like "." and "..", which will be resolved relative to the current working directory. The current working directory must be an absolute path including prefix.
        // Valid: path="file.txt", cwd="partition_a:/path/to"
        // Invalid: path="../file.txt", cwd="partition_a:/path/to", path="/path/to/file.txt", cwd="/path/to"
        FSResult Open(const Utility::String& path, const Utility::String& currentWorkingDirectory, OpenFlags flags, Descriptor* outDescriptor, FileMode mode = FileMode::File);

        FSResult Read(Descriptor* descriptor, void* buffer, size_t size, size_t* bytesRead);
        FSResult Write(Descriptor* descriptor, const void* buffer, size_t size, size_t* bytesWritten);
        FSResult GetFileInfo(Descriptor* descriptor, FileInfo* outInfo);
        FSResult GetDirectoryInfo(Descriptor* descriptor, DirectoryInfo* outInfo);
        void FreeDirectoryInfo(Descriptor *directory, DirectoryInfo* info);
        FSResult Close(Descriptor* descriptor);
        FSResult CanonicalizePath(const Utility::String& path, const Utility::String& cwd, Utility::String& outCanonicalPath);

    private:
        struct MountedFileSystem {
            Utility::String prefix = "";
            FileSystemInterface *fs = nullptr;
            MountFlags flags = MountFlags::NONE;
        };

        Utility::HashMap<Utility::String, MountedFileSystem> _mountedFileSystems;
        FSResult InternalOpen(OpenFlags flags, Descriptor *outDescriptor, FileMode mode, const Utility::String &path) const;
    };
}// FileSystem

#endif //BOREALOS_FILESYSTEM_H