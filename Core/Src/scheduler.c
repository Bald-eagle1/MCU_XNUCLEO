/*
 * scheduler.c
 *
 *  Created on: Nov 17, 2026
 *      Author: no
 *
 *  O(1) Relative Delay (Delta Queue) Task Scheduler Implementation
 */

#include "scheduler.h"
#include <stddef.h>

/* ---- Private State ------------------------------------------------------ */

static sTask SCH_tasks_G[SCH_MAX_TASKS];
static uint32_t SCH_count = 0;
static uint32_t next_task_id = 1;

/* ---- Public API --------------------------------------------------------- */

void SCH_Init(void)
{
    for (uint32_t i = 0; i < SCH_MAX_TASKS; i++)
    {
        SCH_tasks_G[i].pTask  = NULL;
        SCH_tasks_G[i].Delay  = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe  = 0;
        SCH_tasks_G[i].TaskID = 0;
    }
    SCH_count = 0;
    next_task_id = 1;
}

uint32_t SCH_Add_Task(void (*pFunc)(void), uint32_t DELAY, uint32_t PERIOD)
{
    if (pFunc == NULL || SCH_count >= SCH_MAX_TASKS)
    {
        return 0; /* Queue full or invalid pointer */
    }

    uint32_t task_id = next_task_id++;
    if (next_task_id == 0)
    {
        next_task_id = 1; /* Avoid 0 (reserved for invalid ID) */
    }

    /*
     * Insertion into Delta Queue:
     * Find insertion position where remaining delay fits.
     * Subtract preceding relative delays.
     */
    uint32_t new_delay = DELAY;
    uint32_t i = 0;

    for (i = 0; i < SCH_count; i++)
    {
        if (new_delay < SCH_tasks_G[i].Delay)
        {
            /* Insert before task i: update task i's relative delay */
            SCH_tasks_G[i].Delay -= new_delay;
            break;
        }
        else
        {
            new_delay -= SCH_tasks_G[i].Delay;
        }
    }

    /* Shift items from i to SCH_count - 1 to make space at index i */
    for (int32_t j = (int32_t)SCH_count; j > (int32_t)i; j--)
    {
        SCH_tasks_G[j] = SCH_tasks_G[j - 1];
    }

    /* Place new task at index i */
    SCH_tasks_G[i].pTask  = pFunc;
    SCH_tasks_G[i].Delay  = new_delay;
    SCH_tasks_G[i].Period = PERIOD;
    SCH_tasks_G[i].RunMe  = 0;
    SCH_tasks_G[i].TaskID = task_id;

    SCH_count++;
    return task_id;
}

uint8_t SCH_Delete_Task(uint32_t taskID)
{
    if (SCH_count == 0 || taskID == 0)
    {
        return 0;
    }

    for (uint32_t i = 0; i < SCH_count; i++)
    {
        if (SCH_tasks_G[i].TaskID == taskID)
        {
            /*
             * If deleting an intermediate task, its relative delay
             * must be added to the next task to preserve absolute timing.
             */
            if (i < SCH_count - 1)
            {
                SCH_tasks_G[i + 1].Delay += SCH_tasks_G[i].Delay;
            }

            /* Shift subsequent tasks left */
            for (uint32_t j = i; j < SCH_count - 1; j++)
            {
                SCH_tasks_G[j] = SCH_tasks_G[j + 1];
            }

            SCH_count--;
            SCH_tasks_G[SCH_count].pTask  = NULL;
            SCH_tasks_G[SCH_count].Delay  = 0;
            SCH_tasks_G[SCH_count].Period = 0;
            SCH_tasks_G[SCH_count].RunMe  = 0;
            SCH_tasks_G[SCH_count].TaskID = 0;

            return 1;
        }
    }

    return 0; /* TaskID not found */
}

/*
 * O(1) Complexity:
 * Only updates the task at the head of the sorted relative-delay list.
 */
void SCH_Update(void)
{
    if (SCH_count == 0)
    {
        return;
    }

    if (SCH_tasks_G[0].Delay > 0)
    {
        SCH_tasks_G[0].Delay--;
    }

    /*uint32_t so no need to handle negative*/
    if (SCH_tasks_G[0].Delay == 0)
    {
        SCH_tasks_G[0].RunMe = 1;
    }
}

void SCH_Dispatch_Tasks(void)
{
    while (SCH_count > 0 && SCH_tasks_G[0].RunMe > 0)
    {
        void (*pFunction)(void) = SCH_tasks_G[0].pTask;
        uint32_t period = SCH_tasks_G[0].Period;

        /* Execute task callback */
        if (pFunction != NULL)
        {
            (*pFunction)();
        }

        /* Remove task from head of queue */
        for (uint32_t j = 0; j < SCH_count - 1; j++)
        {
            SCH_tasks_G[j] = SCH_tasks_G[j + 1];
        }

        SCH_count--;
        SCH_tasks_G[SCH_count].pTask  = NULL;
        SCH_tasks_G[SCH_count].Delay  = 0;
        SCH_tasks_G[SCH_count].Period = 0;
        SCH_tasks_G[SCH_count].RunMe  = 0;
        SCH_tasks_G[SCH_count].TaskID = 0;

        /* If the new head task has Delay == 0, it was due at the same tick */
        if (SCH_count > 0 && SCH_tasks_G[0].Delay == 0)
        {
            SCH_tasks_G[0].RunMe = 1;
        }

        /* If periodic task, re-insert with Delay = Period */
        if (period > 0 && pFunction != NULL)
        {
            SCH_Add_Task(pFunction, period, period);
        }
    }
}
