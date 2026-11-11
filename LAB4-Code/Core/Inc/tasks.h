#ifndef INC_TASKS_H_
#define INC_TASKS_H_

#include "main.h"
#include "sch.h"

/* Set to 0 for a quieter terminal: the 10 ms callback still runs. */
#define APP_TRACE_10MS 0

/* Optional visible delete demo: D5 is cancelled at 10 s and re-registered
 * at 12 s. Its first resumed toggle is at 14.5 s. Default keeps all LEDs running. */
#define APP_VISIBLE_DELETE_DEMO 0

void App_Init(void);
void task1(void);
void task2(void);
void task3(void);
void task4(void);
void task5(void);
extern volatile uint32_t App_TickCallbackCount;
extern volatile uint32_t App_LogDropped;
extern volatile uint32_t App_UartErrors;

#endif
