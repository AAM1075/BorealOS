#include <Core/ProcessManager.h>
#include <Formats/ELF.h>

#include "Kernel.h"
#include "KernelData.h"
#include "Interrupts/Syscall.h"
#include "Utility/Math.h"

namespace Core {
    constexpr uintptr_t STACK_TOP = 0x7FFFFFFFF000; // Just below the top of the user space virtual memory, we will grow down from here.

    struct PlatformData {
        Memory::Paging::PagingState* pagingState;
    };

    ProcessManager::ProcessManager() : _processes(256) {
        _kernel = Kernel<KernelData>::GetInstance();
        _paging = &_kernel->ArchitectureData->Paging;
    }

    ProcessManager::Process *ProcessManager::CreateProcess(const uint8_t *binaryData, size_t size) {
        auto process = new Process{
            .pid = choosePid(),
            .binary = Formats::ELF(binaryData, size),
            .entryPoint = 0,
            .currentWorkingDirectory = nullptr,
            .platformData = new PlatformData{
                .pagingState = nullptr,
            },
            .mainThread = nullptr,
            .threads = Utility::List<Thread*>(1), // we will assume most processes start with 1 thread.
            .openFiles = Utility::List<FileSystem::Descriptor*>(4), // we will assume most processes start with a few open files (stdin, stdout, stderr, and maybe one more).
        };

        if (!process->binary.IsValid()) {
            LOG_ERROR("Failed to create process, invalid ELF binary!");
            delete process;
            return nullptr;
        }

        process->entryPoint = process->binary.GetEntryPoint();
        _processes.Add(process);

        // For now we will assume the process wants no stdin, framebuffer stdout and stderr.
        // This is not actually our responsibility, should be done by the init process or something, but for testing purposes this is fine.
        process->openFiles.Add(new FileSystem::Descriptor {
            .file = _kernel->ArchitectureData->Stdio->GetStdin(),
            .fs = _kernel->ArchitectureData->Stdio,
            .offset = 0,
        });

        process->openFiles.Add(new FileSystem::Descriptor {
            .file = _kernel->ArchitectureData->Stdio->GetStdout(),
            .fs = _kernel->ArchitectureData->Stdio,
            .offset = 0,
        });

        process->openFiles.Add(new FileSystem::Descriptor {
            .file = _kernel->ArchitectureData->Stdio->GetStderr(),
            .fs = _kernel->ArchitectureData->Stdio,
            .offset = 0,
        });

        return process;
    }

    void ProcessManager::BeginProcess(Process *process, const ProcessArguments &arguments) {
        if (arguments.currentWorkingDirectory) {
            auto cwdLength = Utility::StringFormatter::strlen(arguments.currentWorkingDirectory);
            process->currentWorkingDirectory = new char[cwdLength + 1];
            memcpy(process->currentWorkingDirectory, arguments.currentWorkingDirectory, cwdLength + 1);
            process->currentWorkingDirectory[cwdLength] = '\0'; // Ensure null termination
        } else {
            process->currentWorkingDirectory = new char[2] {'/', '\0'}; // Default to root directory if not specified
        }

        auto platformData = static_cast<PlatformData *>(process->platformData);
        platformData->pagingState = _paging->CreatePagingStateForProcess();

        Thread* mainThread = CreateThread(process, process->entryPoint, STACK_TOP);

        LoadProcessIntoMemory(process);
        SetupThreadStack(process, mainThread, arguments);

        mainThread->isRunning = false;
        mainThread->isPaused = false;
        process->mainThread = mainThread;
    }

    ProcessManager::Thread * ProcessManager::CreateThread(Process *process, uintptr_t instructionPointer,
        uintptr_t stackPointer) {
        auto thread = new Thread{
            .tid = chooseTid(),
            .priority = UINT16_MAX / 2,
            .isRunning = false,
            .isPaused = true,
            .stackPointer = stackPointer,
            .stackBase = stackPointer,
            .instructionPointer = instructionPointer,
        };

        process->threads.Add(thread);
        return thread;
    }

    void ProcessManager::ContinueThread(Process *process, Thread *thread, void *platformRegisters) {
        thread->isRunning = true;
        thread->isPaused = false;

        auto platformData = reinterpret_cast<PlatformData *>(process->platformData);
        _paging->SwitchToPageTable(platformData->pagingState);
        Interrupts::Syscall::ResumeThread((Interrupts::IDT::Registers*)platformRegisters);
    }

    void ProcessManager::BeginThread(Process *process, Thread *thread) {
        thread->isRunning = true;
        thread->isPaused = false;

        auto platformData = reinterpret_cast<PlatformData *>(process->platformData);
        _paging->SwitchToPageTable(platformData->pagingState);
        Interrupts::Syscall::EnterUserspace(thread->instructionPointer, thread->stackPointer);
    }

    void ProcessManager::BreakThread(Thread *thread, uintptr_t instructionPointer, uintptr_t stackPointer) {
        thread->isRunning = false;
        thread->isPaused = true;
        thread->instructionPointer = instructionPointer;
        thread->stackPointer = stackPointer;
    }

    void ProcessManager::ExitThread(Process *process, Thread *thread) {
        thread->isRunning = false;
        thread->isPaused = false;
        process->threads.Remove(thread);
        delete thread;

        if (process->threads.Size() == 0) {
            ShutdownProcess(process);
        } else if (process->mainThread == thread) {
            process->mainThread = process->threads[0]; // If the main thread exited, just pick another thread to be the main thread.
        }
    }

    void ProcessManager::ShutdownProcess(Process *process) {
        for (size_t i = 0; i < process->threads.Size(); i++) {
            auto thread = process->threads[i];
            if (thread->isRunning) {
                BreakThread(thread, thread->instructionPointer, thread->stackPointer);
            }
            delete thread;
        }

        process->threads.Clear();

        for (size_t i = 0; i < process->openFiles.Size(); i++) {
            auto descriptor = process->openFiles[i];
            if (descriptor && descriptor->fs && descriptor->file) {
                descriptor->fs->Close(descriptor->file);
            }
            delete descriptor;
        }

        process->openFiles.Clear();

        // We should also free the memory used by the process, but since we don't have a way to do that yet, we will just leak it for now.
        auto platformData = reinterpret_cast<PlatformData *>(process->platformData);
        _paging->DestroyPagingState(platformData->pagingState);

        delete platformData;

        delete process;
    }

    void ProcessManager::KillProcess(Process *process) {
        // TODO: Make.
    }

    void ProcessManager::LoadProcessIntoMemory(Process *process) {
        auto platformData = reinterpret_cast<PlatformData *>(process->platformData);

        auto data = process->binary.GetData();
        auto header = reinterpret_cast<const Formats::Elf64_Ehdr *>(data);
        auto programHeaders = reinterpret_cast<const Formats::Elf64_Phdr *>(data + header->e_phoff);
        auto current = _paging->GetCurrentPagingState();

        _paging->SwitchToPageTable(platformData->pagingState);
        for (size_t i = 0; i < header->e_phnum; i++) {
            auto &ph = programHeaders[i];
            if (ph.p_type != PT_LOAD) continue;

            Memory::PageFlags flags = Memory::PageFlags::Present | Memory::PageFlags::User;
            if (ph.p_flags & PF_W)  flags |= Memory::PageFlags::ReadWrite;
            if (!(ph.p_flags & PF_X)) flags |= Memory::PageFlags::NoExecute;

            const size_t pageSize = Architecture::KernelPageSize;
            const uint64_t segFileEnd = ph.p_vaddr + ph.p_filesz;
            const uint64_t segMemEnd = ph.p_vaddr + ph.p_memsz;
            const uint64_t pageBase = ph.p_vaddr & ~(pageSize - 1);
            const uint64_t pageEnd = (segMemEnd + pageSize - 1) & ~(pageSize - 1);

            for (uint64_t pageAddr = pageBase; pageAddr < pageEnd; pageAddr += pageSize) {
                auto physicalAddress = _kernel->ArchitectureData->Pmm.AllocatePages(1);
                if (!physicalAddress) PANIC("Failed to allocate memory for process segment!");

                _paging->MapPage(pageAddr, physicalAddress, Memory::PageFlags::ReadWrite | Memory::PageFlags::User);

                // Zero the full page upfront — covers BSS and any partial page gaps
                memset((void *)pageAddr, 0, pageSize);

                // Clamp the file copy range to what actually intersects this page
                const uint64_t copyStart = Utility::Math::Max(pageAddr, ph.p_vaddr);
                const uint64_t copyEnd = Utility::Math::Min(pageAddr + pageSize, segFileEnd);
                if (copyStart < copyEnd)
                    memcpy((void *)copyStart, data + ph.p_offset + (copyStart - ph.p_vaddr), copyEnd - copyStart);

                _paging->UnmapPage(pageAddr);
                _paging->MapPage(pageAddr, physicalAddress, flags);
            }
        }

        process->entryPoint = header->e_entry;
        _paging->SwitchToPageTable(current);
    }

    void ProcessManager::SetupThreadStack(Process *process, Thread *thread, const ProcessArguments &arguments) {
        auto platformData = reinterpret_cast<PlatformData *>(process->platformData);

        const size_t stackPages = 4;
        const uint64_t stackBase = STACK_TOP - (stackPages * Architecture::KernelPageSize);

        auto current = _paging->GetCurrentPagingState();
        _paging->SwitchToPageTable(platformData->pagingState);

        for (uint64_t addr = stackBase; addr < STACK_TOP; addr += Architecture::KernelPageSize) {
            auto phys = _kernel->ArchitectureData->Pmm.AllocatePages(1);
            if (!phys) PANIC("Failed to allocate stack page!");

            _paging->MapPage(addr, phys, Memory::PageFlags::Present | Memory::PageFlags::ReadWrite | Memory::PageFlags::User | Memory::PageFlags::NoExecute);
            memset((void *)addr, 0, Architecture::KernelPageSize);
        }

        uint64_t sp = STACK_TOP;

        auto pushString = [&](const char* str) -> uint64_t {
            size_t len = Utility::StringFormatter::strlen(str) + 1; // Include null terminator
            sp -= len;
            memcpy((void*)sp, str, len);
            return sp;
        };

        auto pushPointer = [&](uint64_t ptr) -> uint64_t {
            sp -= sizeof(uint64_t);
            *reinterpret_cast<uint64_t*>(sp) = ptr;
            return sp;
        };

        uint64_t argvPointers[arguments.argc + 1];
        argvPointers[arguments.argc] = 0; // Null-terminate the argv array
        for (int i = arguments.argc - 1; i >= 0; i--) {
            argvPointers[i] = pushString(arguments.argv[i]);
        }

        sp = ALIGN_DOWN(sp - (arguments.argc * sizeof(uint64_t)), 16); // Align the stack to a 16-byte boundary and make room for argv pointers

        pushPointer(0);

        for (int i = arguments.argc - 1; i >= 0; i--) {
            pushPointer(argvPointers[i]);
        }

        pushPointer(arguments.argc);

        thread->stackPointer = sp;
        thread->stackBase = STACK_TOP;
        _paging->SwitchToPageTable(current);
    }

    void ProcessManager::UnloadProcessFromMemory(Process *process) {
    }

    void ProcessManager::ContextSwitch(Process *from, Process *to) {
    }

    void ProcessManager::CleanupProcess(Process *process) {
    }
}
