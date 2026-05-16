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

        [[nodiscard]] FSResult Open(Utility::StringView path, OpenFlags flags, File **outFile, FileMode mode) override;

        FSResult Read(File *file, void *buffer, size_t size, size_t offset, size_t *outReadBytes) override;

        FSResult Write(File *file, const void *buffer, size_t size, size_t offset, size_t *outWrittenBytes) override;

        FSResult GetFileInfo(File *file, FileInfo *info) override;

        FSResult GetDirectoryInfo(File *file, DirectoryInfo *info) override;

        void FreeDirectoryInfo(DirectoryInfo *info) override;

        FSResult Close(File *file) override;

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
