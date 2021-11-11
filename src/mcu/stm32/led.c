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
#include "led.h"

#define TIMER_PERIOD					0x400

#ifdef BOARD_LED_RGB_PWM_TIMER
static TIM_HandleTypeDef htim;
#endif


void led_init(void)
{
#ifdef BOARD_LED_RGB_PWM_TIMER
	/* Enable TIM clock */
	BOARD_LED_RGB_CLK_ENABLE();

	/* Reset TIM interface */
	BOARD_LED_RGB_FORCE_RESET();
	BOARD_LED_RGB_RELEASE_RESET();

	htim.Instance               = BOARD_LED_RGB_PWM_TIMER;
	htim.Init.Period            = (TIMER_PERIOD - 1);
	htim.Init.Prescaler         = (20 - 1);
	htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	/* Initialize TIM peripheral according to the given parameters */
	HAL_TIM_PWM_Init(&htim);
#endif
}

void led_set(uint8_t r, uint8_t g, uint8_t b)
{
#ifdef BOARD_LED_RGB_PWM_TIMER
	static TIM_OC_InitTypeDef config = {
		.OCMode       = TIM_OCMODE_PWM1,
		.Pulse        = 0x00,
		.OCPolarity   = TIM_OCPOLARITY_HIGH,
		.OCFastMode   = TIM_OCFAST_DISABLE,
	};

#ifdef BOARD_LED_RED_PWM_CHANNEL
	/* Red */
	if (r > 0) {
		config.Pulse = (r * (BOARD_LED_RED_CALIBRATION - 1))/(0x100 - 1);

		/* Stop the timer */
		HAL_TIM_PWM_Stop(&htim, BOARD_LED_RED_PWM_CHANNEL);

		/* Configure the PWM */
		HAL_TIM_PWM_ConfigChannel(&htim, &config, BOARD_LED_RED_PWM_CHANNEL);

		/* Start the timer */
		HAL_TIM_PWM_Start(&htim, BOARD_LED_RED_PWM_CHANNEL);
	} else {
		/* Stop the timer */
		HAL_TIM_PWM_Stop(&htim, BOARD_LED_RED_PWM_CHANNEL);
	}
#endif

#ifdef BOARD_LED_GREEN_PWM_CHANNEL
	/* Green */
	if (g > 0) {
		config.Pulse = (g * (BOARD_LED_GREEN_CALIBRATION - 1))/(0x100 - 1);

		/* Stop the timer */
		HAL_TIM_PWM_Stop(&htim, BOARD_LED_GREEN_PWM_CHANNEL);

		/* Configure the PWM */
		HAL_TIM_PWM_ConfigChannel(&htim, &config, BOARD_LED_GREEN_PWM_CHANNEL);

		/* Start the timer */
		HAL_TIM_PWM_Start(&htim, BOARD_LED_GREEN_PWM_CHANNEL);
	} else {
		/* Stop the timer */
		HAL_TIM_PWM_Stop(&htim, BOARD_LED_GREEN_PWM_CHANNEL);
	}
#endif

#ifdef BOARD_LED_BLUE_PWM_CHANNEL
	/* Blue */
	if (b > 0) {
		config.Pulse = (b * (BOARD_LED_BLUE_CALIBRATION - 1))/(0x100 - 1);

		/* Stop the timer */
		HAL_TIM_PWM_Stop(&htim, BOARD_LED_BLUE_PWM_CHANNEL);

		/* Configure the PWM */
		HAL_TIM_PWM_ConfigChannel(&htim, &config, BOARD_LED_BLUE_PWM_CHANNEL);

		/* Start the timer */
		HAL_TIM_PWM_Start(&htim, BOARD_LED_BLUE_PWM_CHANNEL);
	} else {
		/* Stop the timer */
		HAL_TIM_PWM_Stop(&htim, BOARD_LED_BLUE_PWM_CHANNEL);
	}
#endif
#endif
}
