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
#include "gpio.h"
#include "speaker.h"

#define TIMER_PERIOD					0x10

#ifdef BOARD_SPEAKER_PWM_TIMER
static TIM_HandleTypeDef htim;

static uint8_t state_lock = 0;
#endif


void speaker_init(void)
{
#ifdef BOARD_SPEAKER_PWM_TIMER
	static TIM_OC_InitTypeDef config = {
		.OCMode       = TIM_OCMODE_PWM1,
		.Pulse        = TIMER_PERIOD/2, // 50% duty cycle
		.OCPolarity   = TIM_OCPOLARITY_HIGH,
		.OCFastMode   = TIM_OCFAST_DISABLE,
	};

	/* Enable TIM clock */
	BOARD_SPEAKER_CLK_ENABLE();

	/* Reset TIM interface */
	BOARD_SPEAKER_FORCE_RESET();
	BOARD_SPEAKER_RELEASE_RESET();

	htim.Instance               = BOARD_SPEAKER_PWM_TIMER;
	htim.Init.Period            = (TIMER_PERIOD - 1);
	htim.Init.Prescaler         = 0;
	htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	/* Initialize TIM peripheral according to the given parameters */
	HAL_TIM_PWM_Init(&htim);

	/* Stop the timer */
	HAL_TIM_PWM_Stop(&htim, BOARD_SPEAKER_PWM_CHANNEL);

	/* Configure the PWM */
	HAL_TIM_PWM_ConfigChannel(&htim, &config, BOARD_SPEAKER_PWM_CHANNEL);
#endif
}

void speaker_set_frequency(uint32_t freq)
{
#ifdef BOARD_SPEAKER_PWM_TIMER
	/* Update the prescaler (the frequency is in dHz) */
	htim.Init.Prescaler = (uint16_t) ((SystemCoreClock * 10)/(TIMER_PERIOD * freq));
	htim.Instance->PSC = htim.Init.Prescaler;
#endif
}

void speaker_enable(uint8_t en)
{
#ifdef BOARD_SPEAKER_PWM_TIMER
#ifdef BOARD_SPEAKER_PWM_CHANNEL
	GPIO_InitTypeDef g;

	if (en) {
		/* Configure the PIN as PWM output */
		g.Pin = BOARD_SPEAKER_PIN;
		g.Mode = GPIO_MODE_AF_PP;
		g.Alternate = GPIO_AF4_TIM22;
		HAL_GPIO_Init(BOARD_SPEAKER_PORT, &g);

		/* Start the timer */
		HAL_TIM_PWM_Start(&htim, BOARD_SPEAKER_PWM_CHANNEL);

		/* Low-power modes introduce noise, thus are not allowed */
		system_lock_max_state(STATE_SLEEP_S1, &state_lock);
	} else {
		/* Stop the timer */
		HAL_TIM_PWM_Stop(&htim, BOARD_SPEAKER_PWM_CHANNEL);

		/* Connect the PIN to GND to avoid unwanted noise */
		g.Pin = BOARD_SPEAKER_PIN;
		g.Mode = GPIO_MODE_OUTPUT_PP;
		g.Pull = GPIO_PULLDOWN;
		g.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(BOARD_SPEAKER_PORT, &g);
		gpio_clear(BOARD_SPEAKER_PORT, BOARD_SPEAKER_PIN);

		system_unlock_max_state(STATE_SLEEP_S1, &state_lock);
	}
#endif
#endif
}
