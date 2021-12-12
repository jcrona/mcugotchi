/*
 * MCUGotchi - A Tamagotchi P1 emulator for microcontrollers
 *
 * Copyright (C) 2021 Jean-Christophe Rona <jc@rona.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdint.h>

#include "stm32_hal.h"

#include "system.h"
#include "time.h"

static volatile uint32_t ticks_h = 0;

static LPTIM_HandleTypeDef hlptim;


void LPTIM1_IRQHandler(void)
{
	/* Counter direction change up event */
	if (__HAL_LPTIM_GET_FLAG(&hlptim, LPTIM_FLAG_DOWN) != RESET) {
		if (__HAL_LPTIM_GET_IT_SOURCE(&hlptim, LPTIM_IT_DOWN) != RESET) {
			__HAL_LPTIM_CLEAR_FLAG(&hlptim, LPTIM_FLAG_DOWN);
		}
	}

	/* Counter direction change down to up event */
	if (__HAL_LPTIM_GET_FLAG(&hlptim, LPTIM_FLAG_UP) != RESET) {
		if (__HAL_LPTIM_GET_IT_SOURCE(&hlptim, LPTIM_IT_UP) != RESET) {
			__HAL_LPTIM_CLEAR_FLAG(&hlptim, LPTIM_FLAG_UP);
		}
	}

	/* Autoreload register update OK event */
	if (__HAL_LPTIM_GET_FLAG(&hlptim, LPTIM_FLAG_ARROK) != RESET) {
		if (__HAL_LPTIM_GET_IT_SOURCE(&hlptim, LPTIM_IT_ARROK) != RESET) {
			__HAL_LPTIM_CLEAR_FLAG(&hlptim, LPTIM_FLAG_ARROK);
		}
	}

	/* Compare register update OK event */
	if (__HAL_LPTIM_GET_FLAG(&hlptim, LPTIM_FLAG_CMPOK) != RESET) {
		if (__HAL_LPTIM_GET_IT_SOURCE(&hlptim, LPTIM_IT_CMPOK) != RESET) {
			__HAL_LPTIM_CLEAR_FLAG(&hlptim, LPTIM_FLAG_CMPOK);
		}
	}

	/* External trigger edge event */
	if (__HAL_LPTIM_GET_FLAG(&hlptim, LPTIM_FLAG_EXTTRIG) != RESET) {
		if (__HAL_LPTIM_GET_IT_SOURCE(&hlptim, LPTIM_IT_EXTTRIG) != RESET) {
			__HAL_LPTIM_CLEAR_FLAG(&hlptim, LPTIM_FLAG_EXTTRIG);
		}
	}

	/* Compare match event */
	if (__HAL_LPTIM_GET_FLAG(&hlptim, LPTIM_FLAG_CMPM) != RESET) {
		if (__HAL_LPTIM_GET_IT_SOURCE(&hlptim, LPTIM_IT_CMPM) != RESET) {
			__HAL_LPTIM_CLEAR_FLAG(&hlptim, LPTIM_FLAG_CMPM);
			/* Just wake the CPU */
		}
	}

	/* Autoreload match event */
	if (__HAL_LPTIM_GET_FLAG(&hlptim, LPTIM_FLAG_ARRM) != RESET) {
		if (__HAL_LPTIM_GET_IT_SOURCE(&hlptim, LPTIM_IT_ARRM) != RESET) {
			__HAL_LPTIM_CLEAR_FLAG(&hlptim, LPTIM_FLAG_ARRM);
			ticks_h++;
		}
	}
}

void time_init(void)
{
	/* Enable and set LPTIM Interrupts to the highest priority */
	HAL_NVIC_SetPriority(LPTIM1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(LPTIM1_IRQn);

	/* Enable LPTIM clock */
	__HAL_RCC_LPTIM1_CLK_ENABLE();

	/* Reset LPTIM interface */
	__HAL_RCC_LPTIM1_FORCE_RESET();
	__HAL_RCC_LPTIM1_RELEASE_RESET();

	/* Internal clock source (LSE) with /4 prescaler and no filter (8,192 kHz) */
	hlptim.Instance                = LPTIM1;
	hlptim.Init.Clock.Source       = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
	hlptim.Init.Clock.Prescaler    = LPTIM_PRESCALER_DIV4;
	hlptim.Init.CounterSource      = LPTIM_COUNTERSOURCE_INTERNAL;
	hlptim.Init.Trigger.Source     = LPTIM_TRIGSOURCE_SOFTWARE;
	hlptim.Init.Trigger.ActiveEdge = LPTIM_ACTIVEEDGE_RISING;

	/* Initialize LPTIM peripheral according to the given parameters */
	HAL_LPTIM_Init(&hlptim);

	/* Start counting */
	HAL_LPTIM_Counter_Start_IT(&hlptim, 0xFFFF);
}

mcu_time_t time_get(void)
{
	mcu_time_t t;
	uint32_t cnt;

	system_disable_irq();

	t = ticks_h;
	/* Check if an overflow is not already pending */
	if (__HAL_LPTIM_GET_FLAG(&hlptim, LPTIM_FLAG_ARRM) != RESET) {
		t++;
	}

	cnt = (hlptim.Instance)->CNT;

	system_enable_irq();

	return (cnt | (t << 16));
}

void time_wait_until(mcu_time_t time)
{
	while ((int32_t) (time - time_get()) > 0) {
		__asm__ __volatile__ ("nop");
	}
}

void time_delay(mcu_time_t time)
{
	time_wait_until(time_get() + time);
}

exec_state_t time_configure_wakeup(mcu_time_t time)
{
	mcu_time_t t = time_get();
	int32_t delta = time - t;
	uint32_t cnt = t & 0xFFFF;
	exec_state_t max_state = system_get_max_state();
	exec_state_t state;
	uint32_t latency;

	if (delta < SLEEP_S1_THRESHOLD || max_state == STATE_RUN) {
		/* Job is now/very soon, no time to sleep */
		/* Disable the comparator and its interrupt */
		__HAL_LPTIM_DISABLE_IT(&hlptim, LPTIM_IT_CMPM);
		return STATE_RUN;
	} else if (delta < SLEEP_S2_THRESHOLD || max_state == STATE_SLEEP_S1) {
		latency = EXIT_SLEEP_S1_LATENCY;
		state = STATE_SLEEP_S1;
	} else if (delta < SLEEP_S3_THRESHOLD || max_state == STATE_SLEEP_S2) {
		latency = EXIT_SLEEP_S2_LATENCY;
		state = STATE_SLEEP_S2;
	} else {
		latency = EXIT_SLEEP_S3_LATENCY;
		state = STATE_SLEEP_S3;
	}

	if (delta + cnt - latency <= 0xFFFF) {
		/* Job is soon enough, configure the comparator in order compensate the CPU wakeup latency */
		__HAL_LPTIM_COMPARE_SET(&hlptim, (uint16_t) (delta + cnt - latency));
		/* Enable comparator interrupt */
		__HAL_LPTIM_ENABLE_IT(&hlptim, LPTIM_IT_CMPM);
	}

	return state;
}
