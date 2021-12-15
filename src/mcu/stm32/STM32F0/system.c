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
#include "stm32_hal.h"

#include "dfu.h"
#include "system.h"

static uint8_t state_lock_counters[STATE_NUM] = {0};


void system_disable_irq(void)
{
	__disable_irq();
}

void system_enable_irq(void)
{
	__enable_irq();
}

static void system_clock_config(void)
{
	/* The system Clock is configured as follow :
	 *  System Clock source            = PLL (HSI48)
	 *  SYSCLK(Hz)                     = 48000000
	 *  HCLK(Hz)                       = 48000000
	 *  AHB Prescaler                  = 1
	 *  APB1 Prescaler                 = 1
	 *  HSI Frequency(Hz)              = 48000000
	 *  PREDIV                         = 2
	 *  PLLMUL                         = 2
	 *  Flash Latency(WS)              = 1
	 */
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	static RCC_CRSInitTypeDef RCC_CRSInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	/* Select HSI48 Oscillator as PLL source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI48;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK) {
		system_fatal_error();
	}

	/* Select HSI48 as USB clock source */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
	PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct)!= HAL_OK) {
		system_fatal_error();
	}

	/* Select PLL as system clock source and configure the HCLK and PCLK1 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1)!= HAL_OK) {
		system_fatal_error();
	}

	/* Configure the clock recovery system (CRS) */

	/* Enable CRS Clock */
	__HAL_RCC_CRS_CLK_ENABLE();

	/* Default Synchro Signal division factor (not divided) */
	RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
	/* Set the SYNCSRC[1:0] bits according to CRS_Source value */
	RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_USB;
	/* HSI48 is synchronized with USB SOF at 1KHz rate */
	RCC_CRSInitStruct.ReloadValue =  __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
	RCC_CRSInitStruct.ErrorLimitValue = RCC_CRS_ERRORLIMIT_DEFAULT;
	/* Set the TRIM[5:0] to the default value */
	RCC_CRSInitStruct.HSI48CalibrationValue = 0x20;
	/* Start automatic synchronization */
	HAL_RCCEx_CRSConfig (&RCC_CRSInitStruct);
}

void system_init(void)
{
	HAL_Init();

	system_clock_config();
}

void system_enter_state(exec_state_t state)
{
	/* TODO: implement low-power sleep states */
	switch (state) {
		case STATE_SLEEP_S3:
		case STATE_SLEEP_S2:
		case STATE_SLEEP_S1:
			/* Sleep until the next IRQ */
			__WFI();
			break;

		default:
			break;
	}
}

exec_state_t system_get_max_state(void)
{
	uint32_t i;

	for (i = 0; i < HIGHEST_ALLOWED_STATE; i++) {
		if (state_lock_counters[i]) {
			return (exec_state_t) i;
		}
	}

	return HIGHEST_ALLOWED_STATE;
}

void system_lock_max_state(exec_state_t state, uint8_t *lock)
{
	if (!(*lock)) {
		state_lock_counters[(uint32_t) state]++;
		*lock = 1;
	}
}

void system_unlock_max_state(exec_state_t state, uint8_t *lock)
{
	if (*lock) {
		state_lock_counters[(uint32_t) state]--;
		*lock = 0;
	}
}

void system_reset(void)
{
	NVIC_SystemReset();
}

void system_dfu_reset(void)
{
	SET_DFU_FLAG();
	system_reset();
}

void system_fatal_error(void)
{
	while(1) __asm__ __volatile__ ("nop");
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
	return HAL_OK;
}
