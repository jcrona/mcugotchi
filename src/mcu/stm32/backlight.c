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

#include "board.h"
#include "system.h"
#include "backlight.h"

#define TIMER_PERIOD					0x100

#ifdef BOARD_SCREEN_BL_PWM_TIMER
static TIM_HandleTypeDef htim;
#endif

static uint8_t state_lock = 0;


void backlight_init(void)
{
#ifdef BOARD_SCREEN_BL_PWM_TIMER
	/* Enable TIM clock */
	BOARD_SCREEN_BL_CLK_ENABLE();

	/* Reset TIM interface */
	BOARD_SCREEN_BL_FORCE_RESET();
	BOARD_SCREEN_BL_RELEASE_RESET();

	htim.Instance               = BOARD_SCREEN_BL_PWM_TIMER;
	htim.Init.Period            = (TIMER_PERIOD - 1);
	htim.Init.Prescaler         = (16 - 1);
	htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	/* Initialize TIM peripheral according to the given parameters */
	HAL_TIM_PWM_Init(&htim);
#endif
}

void backlight_set(uint8_t v)
{
#ifdef BOARD_SCREEN_BL_PWM_TIMER
	static TIM_OC_InitTypeDef config = {
		.OCMode       = TIM_OCMODE_PWM1,
		.Pulse        = 0x00,
		.OCPolarity   = TIM_OCPOLARITY_HIGH,
		.OCFastMode   = TIM_OCFAST_DISABLE,
	};

#ifdef BOARD_SCREEN_BL_PWM_CHANNEL
	config.Pulse = v;

	/* Stop the timer */
	HAL_TIM_PWM_Stop(&htim, BOARD_SCREEN_BL_PWM_CHANNEL);

	/* Configure the PWM */
	HAL_TIM_PWM_ConfigChannel(&htim, &config, BOARD_SCREEN_BL_PWM_CHANNEL);

	/* Start the timer */
	HAL_TIM_PWM_Start(&htim, BOARD_SCREEN_BL_PWM_CHANNEL);
#endif
#endif

	if (v == 0) {
		/* Low-power modes introduce noise, thus are not allowed */
		system_unlock_max_state(STATE_SLEEP_S1, &state_lock);
	} else {
		/* Stop mode is not allowed */
		system_lock_max_state(STATE_SLEEP_S1, &state_lock);
	}
}
