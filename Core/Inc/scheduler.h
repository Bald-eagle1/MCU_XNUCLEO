/*
 * scheduler.h
 *
 *  Created on: Nov 17, 2026
 *      Author: no
 *
 *  O(1) Cooperative Task Scheduler (CO3009 Course Project Standard)
 *  Based on relative delay (delta queue) insertion.
 *  SCH_Update() runs in O(1) time complexity inside the timer ISR.
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include <stdint.h>

/* ---- Configuration ------------------------------------------------------ */

#define SCH_MAX_TASKS       10      /* Maximum concurrent tasks in delta queue */
#define SCH_TICK_MS         10      /* Scheduler base tick in milliseconds     */

/* ---- Data Types --------------------------------------------------------- */

typedef struct {
    void     (*pTask)(void);        /* Pointer to the task function            */
    uint32_t Delay;                 /* Remaining ticks relative to prev task   */
    uint32_t Period;                /* Period in ticks (0 = one-shot task)     */
    uint8_t  RunMe;                 /* Flag: 1 when task is ready to run       */
    uint32_t TaskID;                /* Unique task identifier                  */
} sTask;

/* ---- Public API --------------------------------------------------------- */

/**
 * @brief  Initialise the scheduler and clear the task queue.
 */
void SCH_Init(void);

/**
 * @brief  Insert a task into the relative-delay queue.
 * @param  pFunc   Pointer to task function (void func(void)).
 * @param  DELAY   Initial delay before first run (in ticks).
 * @param  PERIOD  Interval between runs (in ticks). 0 = one-shot.
 * @return TaskID (> 0) on success, 0 if queue is full or param invalid.
 */
uint32_t SCH_Add_Task(void (*pFunc)(void), uint32_t DELAY, uint32_t PERIOD);

/**
 * @brief  Delete a task from the scheduler by its TaskID.
 *         Maintains relative delays of subsequent tasks.
 * @param  taskID  Identifier returned by SCH_Add_Task.
 * @return 1 on success, 0 if not found.
 */
uint8_t SCH_Delete_Task(uint32_t taskID);

/**
 * @brief  O(1) tick update function.
 *         Must be called inside the hardware timer ISR (e.g. TIM2).
 *         Only inspects the head of the relative-delay list.
 */
void SCH_Update(void);

/**
 * @brief  Task dispatcher.
 *         Must be called repeatedly in the main while(1) super-loop.
 *         Executes ready tasks and handles periodic re-insertion or
 *         one-shot deletion.
 */
void SCH_Dispatch_Tasks(void);

#endif /* INC_SCHEDULER_H_ */
