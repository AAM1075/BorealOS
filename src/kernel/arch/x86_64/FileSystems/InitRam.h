#ifndef BOREALOS_INITRAMFILESYSTEM_H
#define BOREALOS_INITRAMFILESYSTEM_H

#include "FileSystemInterface.h"
#include "Boot/c_limine.h"

namespace FileSystem {
    class InitRam final : public FileSystemInterface {
    public:
        explicit InitRam(limine_file* cpioArchive, Allocator *allocator);

        [[nodiscard]] Capabilities GetCapabilities() const override;
        [[nodiscard]] FSResult Open(Utility::StringView path, OpenFlags flags, File **outFile, FileMode mode) override;

        FSResult Read(File *file, void *buffer, size_t size, size_t offset, size_t *outReadBytes) override;

        FSResult Write(File *file, const void *buffer, size_t size, size_t offset, size_t *outWrittenBytes) override;

        FSResult GetFileInfo(File *file, FileInfo *info) override;

        FSResult GetDirectoryInfo(File *file, DirectoryInfo *info) override;
        void FreeDirectoryInfo(DirectoryInfo *info) override;

        FSResult Close(File *file) override;

    private:
        static constexpr char CpioNewcMagic[] = "070701";
        static constexpr char CpioNewcTrailer[] = "TRAILER!!!";
        static constexpr size_t CpioNewcHeaderSize = 110;

        limine_file* _cpioArchive;
        File** _files; // For now, we just preload the entire archive into memory
        size_t _fileCount;
    };
} // FileSystems

#endif //BOREALOS_INITRAMFILESYSTEM_H