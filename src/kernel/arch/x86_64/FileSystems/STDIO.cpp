#include "STDIO.h"

#include "Kernel.h"
#include "KernelData.h"

namespace FileSystem {
    struct File {
        bool canWrite;
        bool canRead;
    };

    STDIO::STDIO(Allocator *allocator) : FileSystemInterface(allocator) {
        _stdin = new File{.canWrite = false, .canRead = true};
        _stdout = new File{.canWrite = true, .canRead = false};
        _stderr = new File{.canWrite = true, .canRead = false};
    }

    Capabilities STDIO::GetCapabilities() const {
        return {true, true};
    }

    FSResult STDIO::Open(Utility::StringView path, OpenFlags flags, File **outFile, FileMode mode) {
        return FSResult::UNSUPPORTED;
    }

    FSResult STDIO::Read(File *file, void *buffer, size_t size, size_t offset, size_t *outReadBytes) {
        if (outReadBytes)
            *outReadBytes = 0;
        return FSResult::UNSUPPORTED;
        // TODO: When the HID service is available, we can use that to read input.
    }

    FSResult STDIO::Write(File *file, const void *buffer, size_t size, size_t offset, size_t *outWrittenBytes) {
        if (file->canWrite == false) {
            return FSResult::WRITE_PERMISSION;
        }

        auto kernel = Kernel<KernelData>::GetInstance();
        auto console = &kernel->ArchitectureData->Console;
        if (file == _stderr) {
            console->PrintString(ANSI::Colors::Foreground::Red);
        }

        console->Write((char*)buffer, size);
        if (outWrittenBytes)
            *outWrittenBytes = size;
        return FSResult::SUCCESS;
    }

    FSResult STDIO::GetFileInfo(File *file, FileInfo *info) {
        return FSResult::UNSUPPORTED;
    }

    FSResult STDIO::GetDirectoryInfo(File *file, DirectoryInfo *info) {
        return FSResult::UNSUPPORTED;
    }

    void STDIO::FreeDirectoryInfo(DirectoryInfo *info) {

    }

    FSResult STDIO::Close(File *file) {
        return FSResult::UNSUPPORTED;
    }
}
