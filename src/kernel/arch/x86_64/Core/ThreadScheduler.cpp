#include <Core/ThreadScheduler.h>

#include "Kernel.h"
#include "KernelData.h"
#include "Core/ProcessManager.h"

void scheduledTask(void* context, void* platformData) {
    auto scheduler = reinterpret_cast<Core::ThreadScheduler*>(context);
    scheduler->Tick(platformData);
}

namespace Core {
    ThreadScheduler::ThreadScheduler(ProcessManager *processManager, Time::TimeScheduler *timeScheduler) : _processManager(processManager), _timeScheduler(timeScheduler), _threadQueue(256), _scheduledProcesses(256) {
        timeScheduler->ScheduleTask(scheduledTask, this, SCHEDULER_DELAY, true); // Schedule the scheduler to run in 10ms.
    }

    void ThreadScheduler::ScheduleProcess(ProcessManager::Process *process, const ProcessManager::ProcessArguments& arguments) {
        ScheduledProcess scheduledProcess{
            .process = process,
            .arguments = arguments,
        };

        _processManager->BeginProcess(process, arguments); // build the process, and its main thread.
        _scheduledProcesses.Add(scheduledProcess);

        _threadQueue.Add({
            .thread = process->mainThread,
            .parentProcess = _scheduledProcesses.Size() - 1,
            .hasRunBefore = false,
        });
    }

    void ThreadScheduler::Tick(void* platformData) {
        auto registers = reinterpret_cast<Interrupts::IDT::Registers*>(platformData);

        if (_threadQueue.Size() == 0) {
            // No processes to schedule, just return and wait for the next tick.
            return;
        }

        if (_currentThread) { // If we are actively running a process, break it and switch to the next one.
            _processManager->BreakThread(_currentThread, registers->rip, registers->user_rsp);
        }

        _processIndex = (_processIndex + 1) % _threadQueue.Size();
        auto& nextThreadInfo = _threadQueue[_processIndex];
        _currentThread = nextThreadInfo.thread;

        if (nextThreadInfo.hasRunBefore)
            _processManager->ContinueThread(_scheduledProcesses[nextThreadInfo.parentProcess].process, _currentThread, registers);
        else {
            nextThreadInfo.hasRunBefore = true; // done BEFORE starting the thread, since the below function does not return.
            _processManager->BeginThread(_scheduledProcesses[nextThreadInfo.parentProcess].process, _currentThread);
        }
    }

    ProcessManager::Thread * ThreadScheduler::GetCurrentThread() const {
        return _currentThread;
    }

    ProcessManager::Process * ThreadScheduler::GetCurrentProcess() {
        if (!_currentThread) {
            return nullptr;
        }

        auto& threadInfo = _threadQueue[_processIndex];
        return _scheduledProcesses[threadInfo.parentProcess].process;
    }

    void ThreadScheduler::ExitThread(ProcessManager::Process *process, ProcessManager::Thread *thread) {
        _processManager->ExitThread(process, thread);

        // Remove the thread from the scheduling queue.
        for (size_t i = 0; i < _threadQueue.Size(); i++) {
            if (_threadQueue[i].thread == thread) {
                _threadQueue.Remove(i);
                break;
            }
        }

        if (_currentThread == thread) {
            _currentThread = nullptr; // The currently running thread has exited, so we set it to nullptr. The next time Tick is called, it will switch to the next thread in the queue.
        }

        _processIndex = (_processIndex - 1) % _threadQueue.Size(); // Move the process index back by one, since we removed the current thread from the queue.
    }
}
