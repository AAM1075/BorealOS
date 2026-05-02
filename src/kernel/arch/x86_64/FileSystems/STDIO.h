#ifndef BOREALOS_STDIO_H
#define BOREALOS_STDIO_H

#include <Definitions.h>
#include <FileSystemInterface.h>

namespace FileSystem {
    // kernel stdin, stdout, and stderr which will be (for now) the first 3 file descriptors in every process.
    // Shared between all processes.
    // All caps title because CLion thought i meant <stdio> from the cstdlib
    class STDIO final : public FileSystemInterface {
    public:
        explicit STDIO(Allocator *allocator);

        [[nodiscard]] Capabilities GetCapabilities() const override;

        [[nodiscard]] File *Open(const char *path) override;

        size_t Read(File *file, void *buffer, size_t size) override;

        size_t Write(File *file, const void *buffer, size_t size) override;

        bool GetFileInfo(File *file, FileInfo *info) override;

        bool GetDirectoryInfo(File *file, DirectoryInfo *info) override;

        void FreeDirectoryInfo(DirectoryInfo *info) override;

        void Close(File *file) override;

        [[nodiscard]] File* GetStdin() const { return _stdin; }
        [[nodiscard]] File* GetStdout() const { return _stdout; }
        [[nodiscard]] File* GetStderr() const { return _stderr; }

    private:
        File* _stdin;
        File* _stdout;
        File* _stderr;
    };
} // FileSystems

#endif //BOREALOS_STDIO_H
