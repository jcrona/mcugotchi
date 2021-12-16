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
#include "job.h"
#include "time.h"
#include "system.h"
#include "led.h"

/* Define this to get a breathing animation instead of a static light */
#define BREATHING_LED

#define BREATHING_RATE					30 // Hz
#define BREATHING_IN_TIME				2 // s
#define BREATHING_HOLD_TIME				1 // s
#define BREATHING_OUT_TIME				2 // s
#define BREATHING_WAIT_TIME				5 // s

#define TIMER_PERIOD					0x400

#ifdef BOARD_LED_RGB_PWM_TIMER
static TIM_HandleTypeDef htim;
#endif

#ifdef BREATHING_LED
static job_t breathing_job;
static uint8_t red = 0, green = 0, blue = 0;
static uint16_t breathing_counter = 0;
#endif

static uint8_t state_lock = 0;


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

static void led_set_raw(uint8_t r, uint8_t g, uint8_t b)
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
	config.Pulse = (r * (BOARD_LED_RED_CALIBRATION - 1))/(0x100 - 1);

	/* Stop the timer */
	HAL_TIM_PWM_Stop(&htim, BOARD_LED_RED_PWM_CHANNEL);

	/* Configure the PWM */
	HAL_TIM_PWM_ConfigChannel(&htim, &config, BOARD_LED_RED_PWM_CHANNEL);

	/* Start the timer */
	HAL_TIM_PWM_Start(&htim, BOARD_LED_RED_PWM_CHANNEL);
#endif

#ifdef BOARD_LED_GREEN_PWM_CHANNEL
	/* Green */
	config.Pulse = (g * (BOARD_LED_GREEN_CALIBRATION - 1))/(0x100 - 1);

	/* Stop the timer */
	HAL_TIM_PWM_Stop(&htim, BOARD_LED_GREEN_PWM_CHANNEL);

	/* Configure the PWM */
	HAL_TIM_PWM_ConfigChannel(&htim, &config, BOARD_LED_GREEN_PWM_CHANNEL);

	/* Start the timer */
	HAL_TIM_PWM_Start(&htim, BOARD_LED_GREEN_PWM_CHANNEL);
#endif

#ifdef BOARD_LED_BLUE_PWM_CHANNEL
	/* Blue */
	config.Pulse = (b * (BOARD_LED_BLUE_CALIBRATION - 1))/(0x100 - 1);

	/* Stop the timer */
	HAL_TIM_PWM_Stop(&htim, BOARD_LED_BLUE_PWM_CHANNEL);

	/* Configure the PWM */
	HAL_TIM_PWM_ConfigChannel(&htim, &config, BOARD_LED_BLUE_PWM_CHANNEL);

	/* Start the timer */
	HAL_TIM_PWM_Start(&htim, BOARD_LED_BLUE_PWM_CHANNEL);
#endif
#endif

	if (r == 0 && g == 0 && b == 0) {
		/* Low-power modes introduce noise, thus are not allowed */
		system_unlock_max_state(STATE_SLEEP_S1, &state_lock);
	} else {
		/* Stop mode is not allowed */
		system_lock_max_state(STATE_SLEEP_S1, &state_lock);
	}
}

#ifdef BREATHING_LED
static void breathing_job_fn(job_t *job)
{
	uint8_t r, g, b;

	job_schedule(&breathing_job, &breathing_job_fn, time_get() + MS_TO_MCU_TIME(1000)/BREATHING_RATE);

	if (breathing_counter < BREATHING_IN_TIME * BREATHING_RATE) {
		r = (red * breathing_counter)/(BREATHING_IN_TIME * BREATHING_RATE);
		g = (green * breathing_counter)/(BREATHING_IN_TIME * BREATHING_RATE);
		b = (blue * breathing_counter)/(BREATHING_IN_TIME * BREATHING_RATE);
	} else if (breathing_counter < BREATHING_IN_TIME * BREATHING_RATE + 1) {
		job_schedule(&breathing_job, &breathing_job_fn, time_get() + MS_TO_MCU_TIME(1000 * BREATHING_HOLD_TIME));
		r = red;
		g = green;
		b = blue;
	} else if (breathing_counter < (BREATHING_IN_TIME + BREATHING_OUT_TIME) * BREATHING_RATE + 1) {
		r = (red * ((BREATHING_IN_TIME + BREATHING_OUT_TIME) * BREATHING_RATE - breathing_counter))/(BREATHING_OUT_TIME * BREATHING_RATE);
		g = (green * ((BREATHING_IN_TIME + BREATHING_OUT_TIME) * BREATHING_RATE - breathing_counter))/(BREATHING_OUT_TIME * BREATHING_RATE);
		b = (blue * ((BREATHING_IN_TIME + BREATHING_OUT_TIME) * BREATHING_RATE - breathing_counter))/(BREATHING_OUT_TIME * BREATHING_RATE);
	} else {
		job_schedule(&breathing_job, &breathing_job_fn, time_get() + MS_TO_MCU_TIME(1000 * BREATHING_WAIT_TIME));
		breathing_counter = 0;
		led_set_raw(0, 0, 0);
		return;
	}

	led_set_raw(r, g, b);

	breathing_counter++;
}
#endif

void led_set(uint8_t r, uint8_t g, uint8_t b)
{
#ifdef BREATHING_LED
	breathing_counter = 0;

	if (r == 0 && g == 0 && b == 0) {
		job_cancel(&breathing_job);
		led_set_raw(0, 0, 0);
	} else {
		job_schedule(&breathing_job, &breathing_job_fn, JOB_ASAP);
		red = r;
		green = g;
		blue = b;
	}
#else
	led_set_raw(r, g, b);
#endif
}
