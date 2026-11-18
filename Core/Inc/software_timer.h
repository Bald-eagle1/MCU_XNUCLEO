/*
 * software_timer.h
 *
 *  Created on: Nov 17, 2026
 *      Author: no
 *
 *  Software Timer Module for State Machines & Periodic Delays (CO3009 Standard).
 *  Updated by calling timer_run() periodically (e.g. every 10 ms).
 */

#ifndef INC_SOFTWARE_TIMER_H_
#define INC_SOFTWARE_TIMER_H_

#include <stdint.h>

/* ---- Configuration ------------------------------------------------------ */

#define TIMER_TICK_MS       10      /* Hardware timer tick period in ms         */
#define MAX_TIMERS          10      /* Maximum number of index-based timers     */

/* Convenience macro: convert milliseconds to ticks */
#define MS_TO_TICKS(ms)     ((uint32_t)((ms) / TIMER_TICK_MS))

/* ---- Object/Struct-based Software Timer --------------------------------- */

typedef struct {
    uint32_t counter;       /* Remaining ticks                          */
    uint32_t duration;      /* Reload duration in ticks                 */
    uint8_t  flag;          /* Expiration flag (1 when expired)         */
    uint8_t  enabled;       /* 1 = running, 0 = stopped                 */
    uint8_t  auto_reload;   /* 1 = repeating, 0 = one-shot              */
} SoftTimer_TypeDef;

/* ---- Index-based Timer API (CO3009 Standard for FSMs) ------------------- */

/**
 * @brief  Configure and start an index-based software timer.
 * @param  index        Timer index (0 to MAX_TIMERS - 1).
 * @param  duration     Duration in ticks (use MS_TO_TICKS(ms) for ms).
 */
void setTimer(uint8_t index, uint32_t duration);

/**
 * @brief  Check whether the specified timer has expired.
 * @param  index        Timer index (0 to MAX_TIMERS - 1).
 * @return 1 if timeout occurred (flag is automatically cleared), 0 otherwise.
 */
uint8_t isTimerTimeout(uint8_t index);

/**
 * @brief  Stop an index-based timer.
 */
void stopTimer(uint8_t index);

/**
 * @brief  Periodic countdown function.
 *         Decrements all active index-based and registered struct timers.
 *         Call from an SCH task running every 10 ms or from TIM2 callback.
 */
void timer_run(void);

/* ---- Struct-based Timer API (For modular libraries like DHT20) ----------- */

/**
 * @brief  Initialise/reset a SoftTimer_TypeDef structure.
 */
void SoftTimer_Init(SoftTimer_TypeDef *timer);

/**
 * @brief  Set duration and reload mode without starting.
 */
void SoftTimer_Set(SoftTimer_TypeDef *timer, uint32_t duration_ticks, uint8_t auto_reload);

/**
 * @brief  Start or restart a timer.
 */
void SoftTimer_Start(SoftTimer_TypeDef *timer, uint32_t duration_ticks, uint8_t auto_reload);

/**
 * @brief  Stop a timer.
 */
void SoftTimer_Stop(SoftTimer_TypeDef *timer);

/**
 * @brief  Check if timer expired (reads and clears flag).
 */
uint8_t SoftTimer_IsExpired(SoftTimer_TypeDef *timer);

/**
 * @brief  Update single struct timer (ticks down by 1).
 */
void SoftTimer_Update(SoftTimer_TypeDef *timer);

#endif /* INC_SOFTWARE_TIMER_H_ */
