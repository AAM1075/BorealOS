#ifndef BOREALOS_PROCESSSCHEDULER_H
#define BOREALOS_PROCESSSCHEDULER_H

#include <Definitions.h>
#include "ProcessManager.h"
#include "TimeScheduler.h"

namespace Core {
    /// Schedules processes to run on the CPU, handles context switching between processes, and manages process states (running, paused, etc). It will call ProcessManager to actually run the processes, but it is responsible for deciding when to run which process.
    class ThreadScheduler {
    public:
        ThreadScheduler(ProcessManager* processManager, Time::TimeScheduler *timeScheduler);

        void ScheduleProcess(ProcessManager::Process* process, const ProcessManager::ProcessArguments& arguments);
        void Tick(void* platformData);

        [[nodiscard]] ProcessManager::Thread* GetCurrentThread() const;
        [[nodiscard]] ProcessManager::Process* GetCurrentProcess();

        void ExitThread(ProcessManager::Process * process, ProcessManager::Thread * thread);

    private:
        static constexpr uint64_t SCHEDULER_DELAY = 1'000'000;

        ProcessManager* _processManager;
        uint64_t _processIndex{};
        ProcessManager::Thread* _currentThread{};
        Time::TimeScheduler* _timeScheduler;

        struct ScheduledProcess {
            ProcessManager::Process* process = nullptr;
            ProcessManager::ProcessArguments arguments = {};
        };

        struct ScheduledThread {
            ProcessManager::Thread* thread = nullptr;
            size_t parentProcess = 0; // Index into _scheduledProcesses for the process that owns this thread.
            bool hasRunBefore = false;
        };

        Utility::List<ScheduledThread> _threadQueue;
        Utility::List<ScheduledProcess> _scheduledProcesses;
    };
}

#endif //BOREALOS_PROCESSSCHEDULER_H
