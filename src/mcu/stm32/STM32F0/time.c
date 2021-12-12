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

static TIM_HandleTypeDef htim;


void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
	/* TIM Break input event */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_BREAK) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim, TIM_IT_BREAK) != RESET) {
			__HAL_TIM_CLEAR_IT(&htim, TIM_FLAG_BREAK);
		}
	}

	/* TIM Trigger detection event */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_TRIGGER) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim, TIM_IT_TRIGGER) != RESET) {
			__HAL_TIM_CLEAR_IT(&htim, TIM_FLAG_TRIGGER);
		}
	}

	/* TIM commutation event */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_COM) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim, TIM_IT_COM) != RESET) {
			__HAL_TIM_CLEAR_IT(&htim, TIM_FLAG_COM);
		}
	}

	/* TIM Update event */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_UPDATE) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim, TIM_IT_UPDATE) != RESET) {
			__HAL_TIM_CLEAR_IT(&htim, TIM_FLAG_UPDATE);
			ticks_h++;
		}
	}
}

void TIM1_CC_IRQHandler(void)
{
	/* Capture compare 1 event */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_CC1) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim, TIM_IT_CC1) != RESET) {
			__HAL_TIM_CLEAR_IT(&htim, TIM_FLAG_CC1);
			/* Just wake the CPU */
		}
	}

	/* Capture compare 2 event */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_CC2) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim, TIM_IT_CC2) != RESET) {
			__HAL_TIM_CLEAR_IT(&htim, TIM_FLAG_CC2);
		}
	}

	/* Capture compare 3 event */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_CC3) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim, TIM_IT_CC3) != RESET) {
			__HAL_TIM_CLEAR_IT(&htim, TIM_FLAG_CC3);
		}
	}

	/* Capture compare 4 event */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_CC4) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim, TIM_IT_CC4) != RESET) {
			__HAL_TIM_CLEAR_IT(&htim, TIM_FLAG_CC4);
		}
	}
}

void time_init(void)
{
	/* Enable and set TIM Interrupts to the highest priority */
	HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
	HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);

	/* Enable TIM clock */
	__HAL_RCC_TIM1_CLK_ENABLE();

	/* Reset TIM interface */
	__HAL_RCC_TIM1_FORCE_RESET();
	__HAL_RCC_TIM1_RELEASE_RESET();

	htim.Instance               = TIM1;
	htim.Init.Period            = (0x10000 - 1);
	htim.Init.Prescaler         = ((48 << 7) - 1);
	htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim.Init.RepetitionCounter = 0x0;
	htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	/* Initialize TIM peripheral according to the given parameters */
	HAL_TIM_Base_Init(&htim);

	/* Start counting */
	HAL_TIM_Base_Start_IT(&htim);
}

mcu_time_t time_get(void)
{
	mcu_time_t t;
	uint32_t cnt;

	system_disable_irq();

	t = ticks_h;
	/* Check if an overflow is not already pending */
	if (__HAL_TIM_GET_FLAG(&htim, TIM_FLAG_UPDATE) != RESET) {
		t++;
	}

	cnt = (htim.Instance)->CNT;

	system_enable_irq();

	return (t << 16) | cnt;
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
	static TIM_OC_InitTypeDef config = {
		.OCMode       = TIM_OCMODE_TIMING,
		.OCPolarity   = TIM_OCPOLARITY_HIGH,
		.OCNPolarity  = TIM_OCNPOLARITY_HIGH,
		.OCIdleState  = TIM_OCIDLESTATE_SET,
		.OCNIdleState = TIM_OCNIDLESTATE_RESET,  
		.OCFastMode   = TIM_OCFAST_DISABLE,
	};

	if (delta < SLEEP_S1_THRESHOLD || max_state == STATE_RUN) {
		/* Job is now/very soon, no time to sleep */
		/* Disable the comparator and its interrupt */
		__HAL_TIM_DISABLE_IT(&htim, TIM_IT_CC1);
		//HAL_TIM_OC_Stop_IT(&htim, TIM_CHANNEL_1);
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
		//__HAL_TIM_COMPARE_SET(&htim, (uint16_t) (delta + cnt - latency));
		config.Pulse = (uint16_t) (delta + cnt - latency);
		HAL_TIM_OC_ConfigChannel(&htim, &config, TIM_CHANNEL_1);
		/* Enable comparator interrupt */
		__HAL_TIM_ENABLE_IT(&htim, TIM_IT_CC1);
		//HAL_TIM_OC_Start_IT(&htim, TIM_CHANNEL_1);
	}

	return state;
}
