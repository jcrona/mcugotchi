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

#include "job.h"
#include "board.h"
#include "input_ll.h"
#include "input.h"

#define INPUT_NUM					3

#define DEBOUNCE_DURATION				100 //ms
#define LONG_PRESS_DURATION				1000 //ms

typedef struct {
	input_state_t state;
	job_t debounce_job;
	job_t long_press_job;
	EXTI_HandleTypeDef handle;
	uint32_t exti_port;
	GPIO_TypeDef* port;
	uint16_t pin;
} input_data_t;

static input_data_t inputs[INPUT_NUM] = {
	[INPUT_BTN_LEFT] = {
		.state = INPUT_STATE_LOW,
	},
	[INPUT_BTN_MIDDLE] = {
		.state = INPUT_STATE_LOW,
	},
	[INPUT_BTN_RIGHT] = {
		.state = INPUT_STATE_LOW,
	},
};

static void (*input_handler)(input_t, input_state_t, uint8_t) = NULL;


static void config_int_line(EXTI_HandleTypeDef *h, uint32_t port, uint8_t trigger)
{
	EXTI_ConfigTypeDef e;

	e.Line = h->Line;
	e.Mode = EXTI_MODE_INTERRUPT;
	e.Trigger = trigger;
	e.GPIOSel = port;
	HAL_EXTI_SetConfigLine(h, &e);
}

void input_init(void)
{
	/* Left button */
	inputs[INPUT_BTN_LEFT].handle.Line = BOARD_LEFT_BTN_EXTI_LINE;
	inputs[INPUT_BTN_LEFT].exti_port = BOARD_LEFT_BTN_EXTI_PORT;
	inputs[INPUT_BTN_LEFT].port = BOARD_LEFT_BTN_PORT;
	inputs[INPUT_BTN_LEFT].pin = BOARD_LEFT_BTN_PIN;

	/* Middle button */
	inputs[INPUT_BTN_MIDDLE].handle.Line = BOARD_MIDDLE_BTN_EXTI_LINE;
	inputs[INPUT_BTN_MIDDLE].exti_port = BOARD_MIDDLE_BTN_EXTI_PORT;
	inputs[INPUT_BTN_MIDDLE].port = BOARD_MIDDLE_BTN_PORT;
	inputs[INPUT_BTN_MIDDLE].pin = BOARD_MIDDLE_BTN_PIN;

	/* Right button */
	inputs[INPUT_BTN_RIGHT].handle.Line = BOARD_RIGHT_BTN_EXTI_LINE;
	inputs[INPUT_BTN_RIGHT].exti_port = BOARD_RIGHT_BTN_EXTI_PORT;
	inputs[INPUT_BTN_RIGHT].port = BOARD_RIGHT_BTN_PORT;
	inputs[INPUT_BTN_RIGHT].pin = BOARD_RIGHT_BTN_PIN;
}

void input_register_handler(void (*handler)(input_t, input_state_t, uint8_t))
{
	input_handler = handler;
}

static input_state_t get_input_hw_state(input_t input)
{
	return (HAL_GPIO_ReadPin(inputs[input].port, inputs[input].pin) == GPIO_PIN_SET) ? INPUT_STATE_HIGH : INPUT_STATE_LOW;
}

static void long_press_job_fn(job_t *job)
{
	input_t input;

	/* Lookup the input */
	if (job == &(inputs[INPUT_BTN_LEFT].long_press_job)) {
		input = INPUT_BTN_LEFT;
	} else if (job == &(inputs[INPUT_BTN_MIDDLE].long_press_job)) {
		input = INPUT_BTN_MIDDLE;
	} else if (job == &(inputs[INPUT_BTN_RIGHT].long_press_job)) {
		input = INPUT_BTN_RIGHT;
	} else {
		return;
	}

	if (input_handler != NULL) {
		input_handler(input, inputs[input].state, 1);
	}
}

static void debounce_job_fn(job_t *job)
{
	input_t input;

	/* Lookup the input */
	if (job == &(inputs[INPUT_BTN_LEFT].debounce_job)) {
		input = INPUT_BTN_LEFT;
	} else if (job == &(inputs[INPUT_BTN_MIDDLE].debounce_job)) {
		input = INPUT_BTN_MIDDLE;
	} else if (job == &(inputs[INPUT_BTN_RIGHT].debounce_job)) {
		input = INPUT_BTN_RIGHT;
	} else {
		return;
	}

	if (inputs[input].state == INPUT_STATE_LOW && get_input_hw_state(input) == INPUT_STATE_HIGH) {
		job_schedule(&(inputs[input].long_press_job), &long_press_job_fn, time_get() + MS_TO_MCU_TIME(LONG_PRESS_DURATION));
		config_int_line(&(inputs[input].handle), inputs[input].exti_port, EXTI_TRIGGER_FALLING);
	} else if (inputs[input].state == INPUT_STATE_HIGH && get_input_hw_state(input) == INPUT_STATE_LOW) {
		job_cancel(&(inputs[input].long_press_job));
		config_int_line(&(inputs[input].handle), inputs[input].exti_port, EXTI_TRIGGER_RISING);
	} else {
		/* The input has been toggled during debounce, make sure we handle it properly */
		job_schedule(&(inputs[input].debounce_job), &debounce_job_fn, JOB_ASAP);
	}

	inputs[input].state = !inputs[input].state;

	if (input_handler != NULL) {
		input_handler(input, inputs[input].state, 0);
	}
}

void input_ll_irq_handler(input_t input)
{
	if (HAL_EXTI_GetPending(&(inputs[input].handle), EXTI_TRIGGER_RISING_FALLING)) {
		HAL_EXTI_ClearPending(&(inputs[input].handle), EXTI_TRIGGER_RISING_FALLING);

		job_schedule(&(inputs[input].debounce_job), &debounce_job_fn, time_get() + MS_TO_MCU_TIME(DEBOUNCE_DURATION));
	}
}
