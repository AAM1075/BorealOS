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

    File *STDIO::Open(const char *path) {
        return nullptr;
    }

    size_t STDIO::Read(File *file, void *buffer, size_t size) {
        return -1; // Reading is not supported yet
        // TODO: When the HID service is available, we can use that to read input.
    }

    size_t STDIO::Write(File *file, const void *buffer, size_t size) {
        if (file->canWrite == false) {
            return -1; // Can't write to this file
        }

        auto kernel = Kernel<KernelData>::GetInstance();
        auto console = &kernel->ArchitectureData->Console;
        if (file == _stderr) {
            console->PrintString(ANSI::Colors::Foreground::Red);
        }

        console->Write((char*)buffer, size);
        return size;
    }

    bool STDIO::GetFileInfo(File *file, FileInfo *info) {
        return false;
    }

    bool STDIO::GetDirectoryInfo(File *file, DirectoryInfo *info) {
        return false;
    }

    void STDIO::FreeDirectoryInfo(DirectoryInfo *info) {

    }

    void STDIO::Close(File *file) {

    }
}
