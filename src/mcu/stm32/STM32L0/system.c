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
	/* The system Clock is configured as follow:
	 * HSI48 used as USB clock source
	 *  - System Clock source            = PLL
	 *  - HSI Frequency(Hz)              = 16000000
	 *  - PLL Frequency(Hz)              = 32000000
	 *  - SYSCLK(Hz)                     = 32000000
	 *  - HCLK(Hz)                       = 32000000
	 *  - AHB Prescaler                  = 1
	 *  - APB1 Prescaler                 = 1
	 *  - APB2 Prescaler                 = 1
	 *  - Flash Latency(WS)              = 1
	 *  - Main regulator output voltage  = Scale1 mode
	 */
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct = {0};
	static RCC_CRSInitTypeDef RCC_CRSInitStruct;
	RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

	/* Enable HSI Oscillator to be used as System clock source
	 * Enable HSI48 Oscillator to be used as USB clock source
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL6;
	RCC_OscInitStruct.PLL.PLLDIV     = RCC_PLL_DIV3;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		system_fatal_error();
	}

	/* Select HSI48 as USB clock source */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
	PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
		system_fatal_error();
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clock dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
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

	/* Enable Power Controller clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* The voltage scaling allows optimizing the power consumption when the device is
	 * clocked below the maximum system frequency, to update the voltage scaling value
	 * regarding system frequency refer to product datasheet.
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/* Select the LSE clock as LPTIM and LPUART1 peripheral clock */
	RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
	/* LSE must be used as LPUART1 peripheral clock in order to have the same baudrate in sleep and run mode */
	RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_LPUART1;
	RCC_PeriphCLKInitStruct.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_LSE;
	RCC_PeriphCLKInitStruct.LptimClockSelection = RCC_LPTIM1CLKSOURCE_LSE;
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

}

void system_init(void)
{
	HAL_Init();

	system_clock_config();
}

static void system_lp_sleep_stop(uint8_t stop)
{
	/* Enable the fast wake up from Ultra low power mode */
	HAL_PWREx_EnableFastWakeUp();

	/* Enable Ultra low power mode (up to 3ms wake up time) */
	HAL_PWREx_EnableUltraLowPower();

	/* Reset RCC (MSI as System Clock) */
	/* Set MSION bit */
	RCC->CR |= (uint32_t) 0x00000100U;
	/* Reset SW[1:0], HPRE[3:0], PPRE1[2:0], PPRE2[2:0], MCOSEL[2:0] and MCOPRE[2:0] bits */
	RCC->CFGR &= (uint32_t) 0x88FF400CU;
	/* Reset HSION, HSIDIVEN, HSEON, CSSON and PLLON bits */
	RCC->CR &= (uint32_t) 0xFEF6FFF6U;
	/* Reset HSI48ON  bit */
	RCC->CRRCR &= (uint32_t) 0xFFFFFFFEU;
	/* Reset HSEBYP bit */
	RCC->CR &= (uint32_t) 0xFFFBFFFFU;
	/* Reset PLLSRC, PLLMUL[3:0] and PLLDIV[1:0] bits */
	RCC->CFGR &= (uint32_t) 0xFF02FFFFU;
	/* Disable all interrupts */
	RCC->CIER = 0x00000000U;


	/* Wait until MSI is used as system clock source */
	while ((RCC->CFGR & (uint32_t) RCC_CFGR_SWS) != (uint32_t) RCC_CFGR_SWS_MSI);

	/* Disable LSI clock */
	RCC->CSR &= (uint32_t) ((uint32_t) ~RCC_CSR_LSION);


	/* Switch Flash mode to no latency (WS 0) */
	__HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);

	/* Disable Prefetch Buffer */
	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();

	/* Disable FLASH during Sleep */
	__HAL_FLASH_SLEEP_POWERDOWN_ENABLE();


	/* Select the Voltage Range 3 (1.2V) */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

	/* Wait Until the Voltage Regulator is ready */
	while ((PWR->CSR & PWR_CSR_VOSF) != RESET);


	/* Suspend execution until IRQ */
	if (stop) {
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	} else {
		HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	}


	/* Enable HSI Clock */
	RCC->CR |= ((uint32_t) RCC_CR_HSION);

	/* Wait until HSI is ready */
	while ((RCC->CR & RCC_CR_HSIRDY) == RESET);

	/* Enable HSI48 Clock */
	RCC->CRRCR |= ((uint32_t) RCC_CRRCR_HSI48ON);

	/* Wait until HSI48 is ready */
	while ((RCC->CRRCR & RCC_CRRCR_HSI48RDY) == RESET);

	/* Enable Prefetch Buffer */
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();

	/* Switch Flash mode to 1 cycle latency (WS 1) */
	__HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

	/* Select the Voltage Range 1 (1.8V) */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/* Wait Until the Voltage Regulator is ready */
	while ((PWR->CSR & PWR_CSR_VOSF) != RESET);

	/* PLL configuration */
	RCC->CFGR &= (uint32_t) ((uint32_t) ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL | RCC_CFGR_PLLDIV));
	RCC->CFGR |= (uint32_t) (RCC_CFGR_PLLSRC_HSI | RCC_CFGR_PLLMUL6 | RCC_CFGR_PLLDIV3);

	/* Enable PLL Clock */
	RCC->CR |= ((uint32_t) RCC_CR_PLLON);

	/* Wait until PLL is ready */
	while ((RCC->CR & RCC_CR_PLLRDY) == RESET);

	/* Select PLL as system clock source */
	RCC->CFGR = (RCC->CFGR & (uint32_t) ((uint32_t) ~(RCC_CFGR_SW))) | (uint32_t) RCC_CFGR_SW_PLL;

	/* Wait until PLL is used as system clock source */
	while ((RCC->CFGR & (uint32_t) RCC_CFGR_SWS) != (uint32_t) RCC_CFGR_SWS_PLL);

	/* HCLK = SYSCLK/1 */
	RCC->CFGR = (RCC->CFGR & (uint32_t) ((uint32_t) ~RCC_CFGR_HPRE)) | (uint32_t) RCC_CFGR_HPRE_DIV1;

	/* PCLK2 = HCLK/1 */
	RCC->CFGR = (RCC->CFGR & (uint32_t) ((uint32_t) ~RCC_CFGR_PPRE2)) | (uint32_t) RCC_CFGR_PPRE2_DIV1;

	/* PCLK1 = HCLK/1 */
	RCC->CFGR = (RCC->CFGR & (uint32_t) ((uint32_t) ~RCC_CFGR_PPRE1)) | (uint32_t) RCC_CFGR_PPRE1_DIV1;

	/* Disable Ultra low power mode */
	HAL_PWREx_DisableUltraLowPower();

	/* Enable FLASH during Sleep */
	__HAL_FLASH_SLEEP_POWERDOWN_DISABLE();

	/* Clear Wake Up flag */
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}

static void system_sleep(void)
{
	__HAL_FLASH_SLEEP_POWERDOWN_ENABLE();
	HAL_PWREx_EnableUltraLowPower();
	HAL_PWREx_EnableFastWakeUp();

	/* Sleep until the next IRQ */
	__WFI();

	/* Disable Ultra low power mode */
	HAL_PWREx_DisableUltraLowPower();

	/* Enable FLASH during Sleep */
	__HAL_FLASH_SLEEP_POWERDOWN_DISABLE();
}

void system_enter_state(exec_state_t state)
{
	switch (state) {
		case STATE_SLEEP_S3:
			/* Stop */
			system_lp_sleep_stop(1);
			break;

		case STATE_SLEEP_S2:
			/* Low-power Sleep */
			system_lp_sleep_stop(0);
			break;

		case STATE_SLEEP_S1:
			/* Sleep */
			system_sleep();
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
