#ifndef BOREALOS_TIMESCHEDULER_H
#define BOREALOS_TIMESCHEDULER_H

#include <Definitions.h>

namespace Core::Time {
    /// Schedules tasks to be run at a specific time in the future, measured in nanoseconds. The actual implementation of how the time is measured and how the tasks are executed is left to the derived class, this is just an interface for scheduling tasks.
    class TimeScheduler {
    public:
        typedef void (*TaskFunction)(void* context, void* platformData);

        virtual ~TimeScheduler() = default;

        /// Schedule a task to be run after a certain delay, specified in nanoseconds. The function will be called with the provided context when the task is executed.
        virtual void ScheduleTask(TaskFunction function, void* context, uint64_t delayNs, bool repeat) = 0;

        /// This should be called periodically, it will check if any scheduled tasks are ready to be executed and execute them. The actual timing mechanism is left to the derived class, but it should ensure that tasks are executed as close to their scheduled time as possible.
        virtual void Tick(void *platformData) = 0;
    };
}

#endif //BOREALOS_TIMESCHEDULER_H
