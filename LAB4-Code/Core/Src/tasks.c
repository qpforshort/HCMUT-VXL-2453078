#include "tasks.h"
#include <stddef.h>

extern UART_HandleTypeDef huart1;

/* A bounded, interrupt-driven UART queue. No blocking UART calls in tasks. */
#define LOG_DEPTH 16U
#define LOG_WIDTH 64U
static uint8_t log_data[LOG_DEPTH][LOG_WIDTH];
static uint16_t log_length[LOG_DEPTH];
static volatile uint8_t log_head, log_tail, log_count, log_busy;
volatile uint32_t App_TickCallbackCount;
volatile uint32_t App_LogDropped;
volatile uint32_t App_UartErrors;
static uint32_t cancelled_id;
#if APP_VISIBLE_DELETE_DEMO
static uint32_t led5_id;
#endif

/* Called with interrupts masked, or from the USART1 TX completion ISR. */
static void start_tx(void)
{
    if (log_busy == 0U && log_count != 0U) {
        log_busy = 1U;
        if (HAL_UART_Transmit_IT(&huart1, log_data[log_head],
                                log_length[log_head]) != HAL_OK) {
            log_busy = 0U;
            ++App_UartErrors;
        }
    }
}

/* Producer runs only in main/tasks; USART IRQ consumes complete messages. */
static void log_event(const char *tag)
{
    uint8_t line[LOG_WIDTH];
    uint16_t n = 0U, i;
    uint32_t value = get_time();
    uint32_t divisor = 1000000000UL;
    uint32_t saved;
    line[n++] = 'T'; line[n++] = '=';
    do {
        line[n++] = (uint8_t)('0' + (value / divisor) % 10U);
        divisor /= 10U;
    } while (divisor != 0U);
    line[n++] = ' ';
    while (*tag != '\0' && n < LOG_WIDTH - 2U) {
        line[n++] = (uint8_t)*tag++;
    }
    line[n++] = '\r'; line[n++] = '\n';

    saved = __get_PRIMASK();
    __disable_irq();
    if (log_count == LOG_DEPTH) {
        ++App_LogDropped;
    } else {
        for (i = 0U; i < n; ++i) log_data[log_tail][i] = line[i];
        log_length[log_tail] = n;
        log_tail = (uint8_t)((log_tail + 1U) % LOG_DEPTH);
        ++log_count;
        start_tx();
    }
    __set_PRIMASK(saved);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1 && log_busy != 0U) {
        log_head = (uint8_t)((log_head + 1U) % LOG_DEPTH);
        --log_count;
        log_busy = 0U;
        start_tx();
    }
}

void task1(void) { HAL_GPIO_TogglePin(led0_GPIO_Port, led0_Pin); log_event("LED1 PERIOD=500ms"); }
void task2(void) { HAL_GPIO_TogglePin(led1_GPIO_Port, led1_Pin); log_event("LED2 PERIOD=1000ms"); }
void task3(void) { HAL_GPIO_TogglePin(led2_GPIO_Port, led2_Pin); log_event("LED3 PERIOD=1500ms"); }
void task4(void) { HAL_GPIO_TogglePin(led3_GPIO_Port, led3_Pin); log_event("LED4 PERIOD=2000ms"); }
void task5(void) { HAL_GPIO_TogglePin(led4_GPIO_Port, led4_Pin); log_event("LED5 PERIOD=2500ms"); }

static void trace_tick(void)
{
    ++App_TickCallbackCount;
#if APP_TRACE_10MS
    log_event("TICK");
#endif
}
static void shot_a(void) { log_event("ONCE_A_300MS"); }
static void shot_b(void) { log_event("ONCE_B_100MS"); }
static void shot_c(void) { log_event("ONCE_C_200MS"); }
static void should_not_run(void) { log_event("FAIL_CANCELLED_TASK_RAN"); }
static void cancel_task(void)
{
    log_event(SCH_Delete_Task(cancelled_id) == SCH_OK ? "CANCEL_OK" : "CANCEL_FAIL");
}

static uint32_t add_checked(void (*fn)(void), uint32_t delay, uint32_t period)
{
    uint32_t id = SCH_Add_Task(fn, delay, period);
    if (id == SCH_NO_TASK_ID) Error_Handler();
    return id;
}

#if APP_VISIBLE_DELETE_DEMO
static void stop_led5(void)
{
    if (SCH_Delete_Task(led5_id) == SCH_OK) {
        HAL_GPIO_WritePin(led4_GPIO_Port, led4_Pin, GPIO_PIN_SET);
        log_event("LED5_STOPPED");
    } else log_event("LED5_DELETE_FAIL");
}
static void restart_led5(void)
{
    led5_id = add_checked(task5, 250U, 250U);
    log_event("LED5_RESTART_SCHEDULED");
}
#endif

void App_Init(void)
{
    /* Called once after UART init, before starting TIM2. LEDs are active-low. */
    log_head = log_tail = log_count = log_busy = 0U;
    App_TickCallbackCount = App_LogDropped = App_UartErrors = 0U;
    HAL_GPIO_WritePin(GPIOA, led0_Pin | led1_Pin | led2_Pin |
                     led3_Pin | led4_Pin, GPIO_PIN_SET);
    SCH_Init();
    add_checked(task1,  50U,  50U);  /* First at 0.5 s, then every 0.5 s. */
    add_checked(task2, 100U, 100U);
    add_checked(task3, 150U, 150U);
    add_checked(task4, 200U, 200U);
#if APP_VISIBLE_DELETE_DEMO
    led5_id = add_checked(task5, 250U, 250U);
#else
    add_checked(task5, 250U, 250U);
#endif
    add_checked(trace_tick, 1U, 1U);
    /* Register deliberately out of deadline order. Expect B, C, A. */
    add_checked(shot_a, 30U, 0U);
    add_checked(shot_b, 10U, 0U);
    add_checked(shot_c, 20U, 0U);
    cancelled_id = add_checked(should_not_run, 70U, 0U);
    add_checked(cancel_task, 60U, 0U);
#if APP_VISIBLE_DELETE_DEMO
    add_checked(stop_led5, 1000U, 0U);
    add_checked(restart_led5, 1200U, 0U);
#endif
    log_event("LAB4 START: TICK=10MS; UART1=115200");
    log_event("EXPECT B@100 C@200 A@300 CANCEL_OK@600");
}
