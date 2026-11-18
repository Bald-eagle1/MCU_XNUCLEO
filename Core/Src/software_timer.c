/*
 * software_timer.c
 *
 *  Created on: Nov 17, 2026
 *      Author: no
 */

#include "software_timer.h"
#include <stddef.h>

/* ---- Private State for Index-based Timers ------------------------------- */

static uint32_t timer_counter[MAX_TIMERS] = {0};
static uint8_t  timer_flag[MAX_TIMERS]    = {0};
static uint8_t  timer_enabled[MAX_TIMERS] = {0};

/* ---- Index-based Timer API ---------------------------------------------- */

void setTimer(uint8_t index, uint32_t duration)
{
    if (index >= MAX_TIMERS)
    {
        return;
    }
    timer_counter[index] = duration;
    timer_flag[index]    = 0;
    timer_enabled[index] = 1;
}

uint8_t isTimerTimeout(uint8_t index)
{
    if (index >= MAX_TIMERS)
    {
        return 0;
    }
    if (timer_flag[index] == 1)
    {
        timer_flag[index] = 0;
        return 1;
    }
    return 0;
}

void stopTimer(uint8_t index)
{
    if (index >= MAX_TIMERS)
    {
        return;
    }
    timer_enabled[index] = 0;
    timer_flag[index]    = 0;
    timer_counter[index] = 0;
}

void timer_run(void)
{
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (timer_enabled[i])
        {
            if (timer_counter[i] > 0)
            {
                timer_counter[i]--;
            }
            if (timer_counter[i] == 0)
            {
                timer_flag[i]    = 1;
                timer_enabled[i] = 0; /* one-shot by default for FSM transitions */
            }
        }
    }
}

/* ---- Struct-based Timer API --------------------------------------------- */

void SoftTimer_Init(SoftTimer_TypeDef *timer)
{
    if (timer == NULL) return;
    timer->counter     = 0;
    timer->duration    = 0;
    timer->flag        = 0;
    timer->enabled     = 0;
    timer->auto_reload = 0;
}

void SoftTimer_Set(SoftTimer_TypeDef *timer, uint32_t duration_ticks, uint8_t auto_reload)
{
    if (timer == NULL) return;
    timer->duration    = duration_ticks;
    timer->auto_reload = auto_reload;
}

void SoftTimer_Start(SoftTimer_TypeDef *timer, uint32_t duration_ticks, uint8_t auto_reload)
{
    if (timer == NULL) return;
    timer->duration    = duration_ticks;
    timer->counter     = duration_ticks;
    timer->auto_reload = auto_reload;
    timer->flag        = 0;
    timer->enabled     = 1;
}

void SoftTimer_Stop(SoftTimer_TypeDef *timer)
{
    if (timer == NULL) return;
    timer->enabled = 0;
    timer->flag    = 0;
    timer->counter = 0;
}

uint8_t SoftTimer_IsExpired(SoftTimer_TypeDef *timer)
{
    if (timer == NULL) return 0;
    if (timer->flag)
    {
        timer->flag = 0;
        return 1;
    }
    return 0;
}

void SoftTimer_Update(SoftTimer_TypeDef *timer)
{
    if (timer == NULL || !timer->enabled) return;

    if (timer->counter > 0)
    {
        timer->counter--;
    }

    if (timer->counter == 0)
    {
        timer->flag = 1;
        if (timer->auto_reload)
        {
            timer->counter = timer->duration;
        }
        else
        {
            timer->enabled = 0;
        }
    }
}
