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

#include "lib/tamalib.h"
#include "job.h"
#include "board.h"
#include "button_ll.h"
#include "button.h"

#define BTN_NUM						3

#define DEBOUNCE_DURATION				100 //ms
#define LONG_PRESS_DURATION				1000 //ms

typedef struct {
	btn_state_t state;
	job_t debounce_job;
	job_t long_press_job;
	EXTI_HandleTypeDef handle;
	uint32_t exti_port;
	GPIO_TypeDef* port;
	uint16_t pin;
} btn_data_t;

static btn_data_t buttons[BTN_NUM] = {
	{
		.state = BTN_STATE_RELEASED,
	},
	{
		.state = BTN_STATE_RELEASED,
	},
	{
		.state = BTN_STATE_RELEASED,
	},
};

static void (*btn_handler)(button_t, btn_state_t, bool_t) = NULL;


static void config_int_line(EXTI_HandleTypeDef *h, uint32_t port, uint8_t trigger)
{
	EXTI_ConfigTypeDef e;

	e.Line = h->Line;
	e.Mode = EXTI_MODE_INTERRUPT;
	e.Trigger = trigger;
	e.GPIOSel = port;
	HAL_EXTI_SetConfigLine(h, &e);
}

void button_init(void)
{
	/* Left button */
	buttons[BTN_LEFT].handle.Line = BOARD_LEFT_BTN_EXTI_LINE;
	buttons[BTN_LEFT].exti_port = BOARD_LEFT_BTN_EXTI_PORT;
	buttons[BTN_LEFT].port = BOARD_LEFT_BTN_PORT;
	buttons[BTN_LEFT].pin = BOARD_LEFT_BTN_PIN;

	/* Middle button */
	buttons[BTN_MIDDLE].handle.Line = BOARD_MIDDLE_BTN_EXTI_LINE;
	buttons[BTN_MIDDLE].exti_port = BOARD_MIDDLE_BTN_EXTI_PORT;
	buttons[BTN_MIDDLE].port = BOARD_MIDDLE_BTN_PORT;
	buttons[BTN_MIDDLE].pin = BOARD_MIDDLE_BTN_PIN;

	/* Right button */
	buttons[BTN_RIGHT].handle.Line = BOARD_RIGHT_BTN_EXTI_LINE;
	buttons[BTN_RIGHT].exti_port = BOARD_RIGHT_BTN_EXTI_PORT;
	buttons[BTN_RIGHT].port = BOARD_RIGHT_BTN_PORT;
	buttons[BTN_RIGHT].pin = BOARD_RIGHT_BTN_PIN;
}

void button_register_handler(void (*handler)(button_t, btn_state_t, bool_t))
{
	btn_handler = handler;
}

static btn_state_t get_btn_hw_state(button_t btn)
{
	return (HAL_GPIO_ReadPin(buttons[btn].port, buttons[btn].pin) == GPIO_PIN_SET) ? BTN_STATE_PRESSED : BTN_STATE_RELEASED;
}

static void long_press_job_fn(job_t *job)
{
	button_t btn;

	/* Lookup the button */
	if (job == &(buttons[BTN_LEFT].long_press_job)) {
		btn = BTN_LEFT;
	} else if (job == &(buttons[BTN_MIDDLE].long_press_job)) {
		btn = BTN_MIDDLE;
	} else if (job == &(buttons[BTN_RIGHT].long_press_job)) {
		btn = BTN_RIGHT;
	} else {
		return;
	}

	if (btn_handler != NULL) {
		btn_handler(btn, buttons[btn].state, 1);
	}
}

static void debounce_job_fn(job_t *job)
{
	button_t btn;

	/* Lookup the button */
	if (job == &(buttons[BTN_LEFT].debounce_job)) {
		btn = BTN_LEFT;
	} else if (job == &(buttons[BTN_MIDDLE].debounce_job)) {
		btn = BTN_MIDDLE;
	} else if (job == &(buttons[BTN_RIGHT].debounce_job)) {
		btn = BTN_RIGHT;
	} else {
		return;
	}

	if (buttons[btn].state == BTN_STATE_RELEASED && get_btn_hw_state(btn) == BTN_STATE_PRESSED) {
		job_schedule(&(buttons[btn].long_press_job), &long_press_job_fn, time_get() + MS_TO_MCU_TIME(LONG_PRESS_DURATION));
		config_int_line(&(buttons[btn].handle), buttons[btn].exti_port, EXTI_TRIGGER_FALLING);
	} else if (buttons[btn].state == BTN_STATE_PRESSED && get_btn_hw_state(btn) == BTN_STATE_RELEASED) {
		job_cancel(&(buttons[btn].long_press_job));
		config_int_line(&(buttons[btn].handle), buttons[btn].exti_port, EXTI_TRIGGER_RISING);
	} else {
		/* The button has been pressed or released during debounce, make sure we handle it properly */
		job_schedule(&(buttons[btn].debounce_job), &debounce_job_fn, JOB_ASAP);
	}

	buttons[btn].state = !buttons[btn].state;

	if (btn_handler != NULL) {
		btn_handler(btn, buttons[btn].state, 0);
	}
}

void button_ll_irq_handler(button_t btn)
{
	if (HAL_EXTI_GetPending(&(buttons[btn].handle), EXTI_TRIGGER_RISING_FALLING)) {
		HAL_EXTI_ClearPending(&(buttons[btn].handle), EXTI_TRIGGER_RISING_FALLING);

		job_schedule(&(buttons[btn].debounce_job), &debounce_job_fn, time_get() + MS_TO_MCU_TIME(DEBOUNCE_DURATION));
	}
}
