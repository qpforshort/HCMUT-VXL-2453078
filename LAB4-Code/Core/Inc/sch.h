#ifndef INC_SCH_H_
#define INC_SCH_H_

#include <stdint.h>

#define SCH_TICK_MS          10U
#define SCH_MAX_TASKS        16U
#define SCH_NO_TASK_ID       0U
#define SCH_OK               0U
#define SCH_ERROR            1U
#define SCH_MAX_INTERVAL     0x7FFFFFFFUL

/* DELAY and PERIOD are in TICKS, not milliseconds. 50 ticks = 500 ms.
 * PERIOD == 0: one-shot. DELAY == 0: ready on the next dispatcher call.
 * Only SCH_Update may be called from an ISR. All other API calls are made
 * from main/tasks. Tasks must return promptly; nested dispatch is forbidden.
 * Overdue periodic releases are caught up in deadline order, without drift.
 * Deadlines must be serviced within half of the uint32_t tick range.
 */
typedef enum {
    SCH_ERR_NONE = 0,
    SCH_ERR_FULL,
    SCH_ERR_NULL_TASK,
    SCH_ERR_INTERVAL,
    SCH_ERR_ID_NOT_FOUND,
    SCH_ERR_ID_EXHAUSTED
} SCH_Error;

void SCH_Init(void);
void SCH_Update(void);
void SCH_Dispatch_Tasks(void);
uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD);
uint8_t SCH_Delete_Task(uint32_t taskID);
uint32_t get_time(void);              /* Milliseconds, resolution 10 ms. */
uint32_t SCH_GetTicks(void);
uint8_t SCH_GetTaskCount(void);
SCH_Error SCH_GetLastError(void);
void deleteAllTasks(void);            /* Keep the clock and ID sequence. */

#endif
