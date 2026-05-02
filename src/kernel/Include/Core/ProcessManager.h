#ifndef BOREALOS_PROCESSMANAGER_H
#define BOREALOS_PROCESSMANAGER_H

#include <Definitions.h>

#include "FileSystemInterface.h"
#include "Kernel.h"
#include "../Formats/ELF.h"
#include "../Utility/List.h"
#include "Memory/Paging.h"

namespace FileSystem {
    struct File;
}

typedef struct KernelData KernelData;

namespace Core {
    /// Load & execute processes, does not manage multiple processes, that's the task of the ProcessScheduler, which will call this class.
    class ProcessManager {
    public:
        ProcessManager();

        struct Thread {
            size_t tid;
            uint16_t priority;
            bool isRunning;
            bool isPaused;
            uintptr_t stackPointer;
            uintptr_t stackBase;
            uintptr_t instructionPointer;
        };

        struct Process {
            uint64_t pid;
            Formats::ELF binary;
            uintptr_t entryPoint;
            char* currentWorkingDirectory;
            void* platformData;

            Thread* mainThread;
            Utility::List<Thread*> threads;
            Utility::List<FileSystem::Descriptor*> openFiles;
        };

        /// Create a process from the given ELF binary data. The process will not run, until you, or the scheduler, call RunProcess on it.
        Process *CreateProcess(const uint8_t* binaryData, size_t size);

        struct ProcessArguments {
            const char** argv;
            size_t argc;
            const char* currentWorkingDirectory;
        };

        /// Begin the process, this will load it into memory. The process will be paused after this, you need to call BeginThread on its main thread to start executing it.
        /// This creates the initial thread for the process. But does not start executing it.
        void BeginProcess(Process* process, const ProcessArguments& arguments);

        /// Create a new thread in the given process, this will not start executing the thread, you need to call ContinueThread to do that.
        Thread* CreateThread(Process* process, uintptr_t instructionPointer, uintptr_t stackPointer);

        /// Continue a paused thread, this will resume execution from where it was paused. The thread must be paused, and not currently running.
        [[noreturn]] void ContinueThread(Process *process, Thread *thread, void *platformRegisters);

        /// Starts a thread, this is separate because we need to set some things up before we can start executing a thread.
        [[noreturn]] void BeginThread(Process *process, Thread *thread);

        /// Pause the given thread, this will allow it to be resumed later with ContinueThread.
        void BreakThread(Thread *thread, uintptr_t instructionPointer, uintptr_t stackPointer);

        /// Exit the given thread, this will clean up any resources used by the thread and remove it from the process. If this is the last thread in the process, it will also exit the process.
        void ExitThread(Process *process, Thread *thread);

        /// Ask the program to shut down, this will allow it to do any cleanup it needs to do before exiting. It will not be forced to exit.
        void ShutdownProcess(Process* process);

        /// Forcefully kill the given process, this will not allow it to do any cleanup, so it should only be used if the process is unresponsive.
        void KillProcess(Process* process);

    private:
        Utility::List<Process*> _processes;
        Kernel<KernelData> *_kernel;
        Memory::Paging *_paging;

        void LoadProcessIntoMemory(Process* process);
        void SetupThreadStack(Process* process, Thread *thread, const ProcessArguments &arguments);
        void UnloadProcessFromMemory(Process* process);
        void ContextSwitch(Process* from, Process* to);
        void CleanupProcess(Process* process);

        static uint64_t choosePid() {
            static uint64_t nextPid = 1;
            return nextPid++;
        }

        static uint64_t chooseTid() {
            static uint64_t nextTid = 1;
            return nextTid++;
        }
    };
}

#endif //BOREALOS_PROCESSMANAGER_H
