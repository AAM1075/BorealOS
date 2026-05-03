#include "Scheduler.h"
#include "Scheduler.h"

namespace Core::Time {
    Scheduler::Scheduler(TSC *tsc) : _tsc(tsc), _taskHeap(256) {

    }

    void Scheduler::ScheduleTask(TaskFunction function, void *context, uint64_t delayNs, bool repeat) {
        uint64_t currentNs = _tsc->GetNanoseconds();
        uint64_t endNs = currentNs + delayNs;

        Task newTask {
            .endTime = endNs,
            .delay = delayNs,
            .function = function,
            .context = context,
            .repeat = repeat
        };

        if (endNs < currentNs) {
            // The end time is in the past, so we should execute this task immediately. We can do this by setting endTime to 0, which will cause it to be executed on the next tick.
            newTask.endTime = 0;
        }

        InsertTask(newTask);
    }

    void Scheduler::Tick(void *platformData) {
        uint64_t currentNs = _tsc->GetNanoseconds();
        while (!_taskHeap.IsEmpty()) {
            Task* nextTask = _taskHeap.Peek();
            if (nextTask->endTime > currentNs) {
                // The next task is scheduled for the future, so we're done for this tick.
                break;
            }

            // The next task is ready to be executed, so pop it from the heap and execute it.
            Task taskToExecute = *_taskHeap.Pop();

            if (taskToExecute.repeat) {
                ScheduleTask(taskToExecute.function, taskToExecute.context, taskToExecute.delay, true);
            }

            taskToExecute.function(taskToExecute.context, platformData);
        }
    }

    void Scheduler::InsertTask(Task &task) {
        if (!_taskHeap.Insert(task)) {
            PANIC("Scheduler task heap is full, cannot schedule more tasks!");
        }
    }
}
