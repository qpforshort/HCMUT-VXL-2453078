#include "sch.h"
#include <stddef.h>
#include <limits.h>

typedef struct {
    void (*pTask)(void);
    uint32_t deadline;
    uint32_t period;
    uint32_t id;
} SCH_Task;

static SCH_Task queue[SCH_MAX_TASKS];
static uint8_t task_count;
static uint32_t next_id;
static volatile uint32_t tick_count;
static SCH_Error last_error;

/* Modulo comparison: intervals and outstanding lateness must be < 2^31 ticks. */
static uint8_t time_before(uint32_t a, uint32_t b)
{
    return (uint8_t)((int32_t)(a - b) < 0);
}

/* Stable order: earlier deadline first, then earlier registration ID. */
static void insert_task(SCH_Task task)
{
    uint8_t pos = task_count;
    while (pos > 0U) {
        SCH_Task previous = queue[pos - 1U];
        if (!time_before(task.deadline, previous.deadline) &&
            !(task.deadline == previous.deadline && task.id < previous.id)) {
            break;
        }
        queue[pos] = previous;
        --pos;
    }
    queue[pos] = task;
    ++task_count;
}

static void remove_at(uint8_t index)
{
    uint8_t i;
    for (i = index; i + 1U < task_count; ++i) {
        queue[i] = queue[i + 1U];
    }
    --task_count;
    queue[task_count] = (SCH_Task){0};
}

void SCH_Init(void)
{
    uint8_t i;
    /* Call before starting TIM2. Reset starts a new ID/clock session. */
    for (i = 0U; i < SCH_MAX_TASKS; ++i) {
        queue[i] = (SCH_Task){0};
    }
    task_count = 0U;
    next_id = 1U;
    tick_count = 0U;
    last_error = SCH_ERR_NONE;
}

void SCH_Update(void)
{
    /* O(1): no array scan, callbacks, UART or queue edits in the ISR. */
    ++tick_count;
}

uint32_t SCH_GetTicks(void)
{
    /* Aligned 32-bit read is atomic on this Cortex-M3 target. */
    return tick_count;
}

uint32_t get_time(void)
{
    /* Unsigned millisecond timestamp wraps after approximately 49.7 days. */
    return SCH_GetTicks() * SCH_TICK_MS;
}

uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD)
{
    SCH_Task task;
    if (pFunction == NULL) {
        last_error = SCH_ERR_NULL_TASK;
        return SCH_NO_TASK_ID;
    }
    if (DELAY > SCH_MAX_INTERVAL || PERIOD > SCH_MAX_INTERVAL) {
        last_error = SCH_ERR_INTERVAL;
        return SCH_NO_TASK_ID;
    }
    if (task_count >= SCH_MAX_TASKS) {
        last_error = SCH_ERR_FULL;
        return SCH_NO_TASK_ID;
    }
    if (next_id == 0U) {
        last_error = SCH_ERR_ID_EXHAUSTED;
        return SCH_NO_TASK_ID;
    }
    task.pTask = pFunction;
    task.deadline = SCH_GetTicks() + DELAY;
    task.period = PERIOD;
    task.id = next_id++;
    insert_task(task);
    return task.id;
}

void SCH_Dispatch_Tasks(void)
{
    SCH_Task task;
    if (task_count == 0U || time_before(SCH_GetTicks(), queue[0].deadline)) {
        return;
    }
    task = queue[0];
    remove_at(0U);
    if (task.period != 0U) {
        /* Use the planned deadline, not callback completion time. */
        task.deadline += task.period;
        insert_task(task);
    }
    /* Queue is consistent before the callback: it can add/delete tasks,
     * including deleting itself. No interrupt lock surrounds user code.
     * One callback per dispatch call; main immediately calls us again. */
    task.pTask();
}

uint8_t SCH_Delete_Task(uint32_t taskID)
{
    uint8_t i;
    for (i = 0U; i < task_count; ++i) {
        if (queue[i].id == taskID) {
            remove_at(i);
            return SCH_OK;
        }
    }
    last_error = SCH_ERR_ID_NOT_FOUND;
    return SCH_ERROR;
}

uint8_t SCH_GetTaskCount(void)
{
    return task_count;
}

SCH_Error SCH_GetLastError(void)
{
    return last_error; /* Sticky until SCH_Init, for debugging. */
}

void deleteAllTasks(void)
{
    task_count = 0U;
}

#ifdef SCH_TEST
/* Host-test seam, omitted from firmware builds. */
void SCH_TestSetTicks(uint32_t value)
{
    tick_count = value;
}
#endif
