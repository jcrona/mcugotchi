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
#include "system_ll.h"
#include "system.h"

typedef struct {
	uint32_t moder;
	uint32_t ospeedr;
	uint32_t otyper;
	uint32_t pupdr;
	uint8_t enabled;
} gpio_config_t;

typedef struct {
	uint16_t one;
	uint32_t two;
} gpio_masks_t;

static uint8_t state_lock_counters[STATE_NUM] = {0};

static gpio_config_t gpio_a, gpio_b, gpio_c, gpio_d, gpio_e, gpio_h;
static gpio_masks_t gpio_a_msk = {0}, gpio_b_msk = {0}, gpio_c_msk = {0}, gpio_d_msk = {0}, gpio_e_msk = {0}, gpio_h_msk = {0};


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
	GPIO_InitTypeDef g;

	HAL_Init();

	system_clock_config();

	/* Configure all GPIOs in Analog mode */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

	g.Pin  = GPIO_PIN_All;
	g.Mode  = GPIO_MODE_ANALOG;
	g.Pull  = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(GPIOA, &g);
	HAL_GPIO_Init(GPIOB, &g);
	HAL_GPIO_Init(GPIOC, &g);
	HAL_GPIO_Init(GPIOD, &g);
	HAL_GPIO_Init(GPIOE, &g);
	HAL_GPIO_Init(GPIOH, &g);

	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
	__HAL_RCC_GPIOE_CLK_DISABLE();
	__HAL_RCC_GPIOH_CLK_DISABLE();
}

void system_register_lp_pin(GPIO_TypeDef *port, uint16_t pin)
{
	gpio_masks_t *gpio_msk = NULL;

	if (port == GPIOA) {
		gpio_msk = &gpio_a_msk;
	} else if (port == GPIOB) {
		gpio_msk = &gpio_b_msk;
	} else if (port == GPIOC) {
		gpio_msk = &gpio_c_msk;
	} else if (port == GPIOD) {
		gpio_msk = &gpio_d_msk;
	} else if (port == GPIOE) {
		gpio_msk = &gpio_e_msk;
	} else if (port == GPIOH) {
		gpio_msk = &gpio_h_msk;
	} else {
		return;
	}

	gpio_msk->one |= pin;
	gpio_msk->two |= (pin * pin) | ((pin * pin) << 1);
}

static void save_gpio_config(void)
{
	/* Keep track of which port is enabled */
	gpio_a.enabled = __HAL_RCC_GPIOA_IS_CLK_ENABLED();
	gpio_b.enabled = __HAL_RCC_GPIOB_IS_CLK_ENABLED();
	gpio_c.enabled = __HAL_RCC_GPIOC_IS_CLK_ENABLED();
	gpio_d.enabled = __HAL_RCC_GPIOD_IS_CLK_ENABLED();
	gpio_e.enabled = __HAL_RCC_GPIOE_IS_CLK_ENABLED();
	gpio_h.enabled = __HAL_RCC_GPIOH_IS_CLK_ENABLED();

	/* Enable all clocks */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

	/* Save the GPIO configuration */
	gpio_a.moder = GPIOA->MODER;
	gpio_b.moder = GPIOB->MODER;
	gpio_c.moder = GPIOC->MODER;
	gpio_d.moder = GPIOD->MODER;
	gpio_e.moder = GPIOE->MODER;
	gpio_h.moder = GPIOH->MODER;

	gpio_a.ospeedr = GPIOA->OSPEEDR;
	gpio_b.ospeedr = GPIOB->OSPEEDR;
	gpio_c.ospeedr = GPIOC->OSPEEDR;
	gpio_d.ospeedr = GPIOD->OSPEEDR;
	gpio_e.ospeedr = GPIOE->OSPEEDR;
	gpio_h.ospeedr = GPIOH->OSPEEDR;

	gpio_a.otyper = GPIOA->OTYPER;
	gpio_b.otyper = GPIOB->OTYPER;
	gpio_c.otyper = GPIOC->OTYPER;
	gpio_d.otyper = GPIOD->OTYPER;
	gpio_e.otyper = GPIOE->OTYPER;
	gpio_h.otyper = GPIOH->OTYPER;

	gpio_a.pupdr = GPIOA->PUPDR;
	gpio_b.pupdr = GPIOB->PUPDR;
	gpio_c.pupdr = GPIOC->PUPDR;
	gpio_d.pupdr = GPIOD->PUPDR;
	gpio_e.pupdr = GPIOE->PUPDR;
	gpio_h.pupdr = GPIOH->PUPDR;

	/* Configure all GPIO port pins in Analog input mode
	 * (Analog, 400kHz, PP, NOPULL), except the registered
	 * low-power pins
	 */
	GPIOA->MODER = (GPIOA->MODER & gpio_a_msk.two) | ~gpio_a_msk.two;
	GPIOA->OSPEEDR = (GPIOA->OSPEEDR & gpio_a_msk.two);
	GPIOA->OTYPER = (GPIOA->OTYPER & gpio_a_msk.one);
	GPIOA->PUPDR = (GPIOA->PUPDR & gpio_a_msk.two);

	GPIOB->MODER = (GPIOB->MODER & gpio_b_msk.two) | ~gpio_b_msk.two;
	GPIOB->OSPEEDR = (GPIOB->OSPEEDR & gpio_b_msk.two);
	GPIOB->OTYPER = (GPIOB->OTYPER & gpio_b_msk.one);
	GPIOB->PUPDR = (GPIOB->PUPDR & gpio_b_msk.two);

	GPIOC->MODER = (GPIOC->MODER & gpio_c_msk.two) | ~gpio_c_msk.two;
	GPIOC->OSPEEDR = (GPIOC->OSPEEDR & gpio_c_msk.two);
	GPIOC->OTYPER = (GPIOC->OTYPER & gpio_c_msk.one);
	GPIOC->PUPDR = (GPIOC->PUPDR & gpio_c_msk.two);

	GPIOD->MODER = (GPIOC->MODER & gpio_d_msk.two) | ~gpio_d_msk.two;
	GPIOD->OSPEEDR = (GPIOC->OSPEEDR & gpio_d_msk.two);
	GPIOD->OTYPER = (GPIOC->OTYPER & gpio_d_msk.one);
	GPIOD->PUPDR = (GPIOC->PUPDR & gpio_d_msk.two);

	GPIOE->MODER = (GPIOC->MODER & gpio_e_msk.two) | ~gpio_e_msk.two;
	GPIOE->OSPEEDR = (GPIOC->OSPEEDR & gpio_e_msk.two);
	GPIOE->OTYPER = (GPIOC->OTYPER & gpio_e_msk.one);
	GPIOE->PUPDR = (GPIOC->PUPDR & gpio_e_msk.two);

	GPIOH->MODER = (GPIOH->MODER & gpio_h_msk.two) | ~gpio_h_msk.two;
	GPIOH->OSPEEDR = (GPIOH->OSPEEDR & gpio_h_msk.two);
	GPIOH->OTYPER = (GPIOH->OTYPER & gpio_h_msk.one);
	GPIOH->PUPDR = (GPIOH->PUPDR & gpio_h_msk.two);

	/* Disable unused clocks */
	if (!gpio_a_msk.one) {
		__HAL_RCC_GPIOA_CLK_DISABLE();
	}

	if (!gpio_b_msk.one) {
		__HAL_RCC_GPIOB_CLK_DISABLE();
	}

	if (!gpio_c_msk.one) {
		__HAL_RCC_GPIOC_CLK_DISABLE();
	}

	if (!gpio_d_msk.one) {
		__HAL_RCC_GPIOD_CLK_DISABLE();
	}

	if (!gpio_e_msk.one) {
		__HAL_RCC_GPIOE_CLK_DISABLE();
	}

	if (!gpio_h_msk.one) {
		__HAL_RCC_GPIOH_CLK_DISABLE();
	}
}

static void restore_gpio_config(void)
{
	/* Enable all clocks */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

	/* Restore the GPIO configuration */
	GPIOA->MODER = gpio_a.moder;
	GPIOB->MODER = gpio_b.moder;
	GPIOC->MODER = gpio_c.moder;
	GPIOD->MODER = gpio_d.moder;
	GPIOE->MODER = gpio_e.moder;
	GPIOH->MODER = gpio_h.moder;

	GPIOA->OSPEEDR = gpio_a.ospeedr;
	GPIOB->OSPEEDR = gpio_b.ospeedr;
	GPIOC->OSPEEDR = gpio_c.ospeedr;
	GPIOD->OSPEEDR = gpio_d.ospeedr;
	GPIOE->OSPEEDR = gpio_e.ospeedr;
	GPIOH->OSPEEDR = gpio_h.ospeedr;

	GPIOA->OTYPER = gpio_a.otyper;
	GPIOB->OTYPER = gpio_b.otyper;
	GPIOC->OTYPER = gpio_c.otyper;
	GPIOD->OTYPER = gpio_d.otyper;
	GPIOE->OTYPER = gpio_e.otyper;
	GPIOH->OTYPER = gpio_h.otyper;

	GPIOA->PUPDR = gpio_a.pupdr;
	GPIOB->PUPDR = gpio_b.pupdr;
	GPIOC->PUPDR = gpio_c.pupdr;
	GPIOD->PUPDR = gpio_d.pupdr;
	GPIOE->PUPDR = gpio_e.pupdr;
	GPIOH->PUPDR = gpio_h.pupdr;

	/* Disable unused clocks */
	if (!gpio_a.enabled) {
		__HAL_RCC_GPIOA_CLK_DISABLE();
	}

	if (!gpio_b.enabled) {
		__HAL_RCC_GPIOB_CLK_DISABLE();
	}

	if (!gpio_c.enabled) {
		__HAL_RCC_GPIOC_CLK_DISABLE();
	}

	if (!gpio_d.enabled) {
		__HAL_RCC_GPIOD_CLK_DISABLE();
	}

	if (!gpio_e.enabled) {
		__HAL_RCC_GPIOE_CLK_DISABLE();
	}

	if (!gpio_h.enabled) {
		__HAL_RCC_GPIOH_CLK_DISABLE();
	}
}

static void system_lp_sleep_stop(uint8_t stop)
{
	save_gpio_config();

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

	restore_gpio_config();

	/* Clear Wake Up flag */
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}

static void system_sleep(void)
{
	__HAL_FLASH_SLEEP_POWERDOWN_ENABLE();

	/* Sleep until the next IRQ */
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

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
