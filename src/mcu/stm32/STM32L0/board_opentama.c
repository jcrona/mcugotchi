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

#include "input_ll.h"
#include "system_ll.h"
#include "board.h"


void board_init(void)
{
	GPIO_InitTypeDef g;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Buttons */
	g.Pin = BOARD_LEFT_BTN_PIN;
	g.Mode = GPIO_MODE_IT_RISING;
	g.Pull = GPIO_PULLDOWN;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_LEFT_BTN_PORT, &g);
	system_register_lp_pin(BOARD_LEFT_BTN_PORT, BOARD_LEFT_BTN_PIN);

	g.Pin = BOARD_MIDDLE_BTN_PIN;
	g.Mode = GPIO_MODE_IT_RISING;
	g.Pull = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_MIDDLE_BTN_PORT, &g);
	system_register_lp_pin(BOARD_MIDDLE_BTN_PORT, BOARD_MIDDLE_BTN_PIN);

	g.Pin = BOARD_RIGHT_BTN_PIN;
	g.Mode = GPIO_MODE_IT_RISING;
	g.Pull = GPIO_PULLDOWN;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_RIGHT_BTN_PORT, &g);
	system_register_lp_pin(BOARD_RIGHT_BTN_PORT, BOARD_RIGHT_BTN_PIN);

	/* SPI 1 */
	g.Pin = BOARD_SCREEN_SCLK_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Alternate = GPIO_AF0_SPI1;
	HAL_GPIO_Init(BOARD_SCREEN_SCLK_PORT, &g);
#if defined(BOARD_HAS_SSD1306) && defined(BOARD_SSD1306_NO_CS_PIN)
	system_register_lp_pin(BOARD_SCREEN_SCLK_PORT, BOARD_SCREEN_SCLK_PIN);
#endif

	g.Pin = BOARD_SCREEN_MOSI_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Alternate = GPIO_AF0_SPI1;
	HAL_GPIO_Init(BOARD_SCREEN_MOSI_PORT, &g);

	/* Screen */
	g.Pin  = BOARD_SCREEN_DC_PIN;
	g.Mode  = GPIO_MODE_OUTPUT_PP;
	g.Pull  = GPIO_PULLUP;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_SCREEN_DC_PORT, &g);

	g.Pin  = BOARD_SCREEN_NSS_PIN;
	g.Mode  = GPIO_MODE_OUTPUT_PP;
	g.Pull  = GPIO_PULLUP;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_SCREEN_NSS_PORT, &g);

	g.Pin  = BOARD_SCREEN_RST_PIN;
	g.Mode  = GPIO_MODE_OUTPUT_PP;
	g.Pull  = GPIO_PULLUP;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_SCREEN_RST_PORT, &g);
	system_register_lp_pin(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);

	g.Pin = BOARD_SCREEN_BL_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Alternate = GPIO_AF2_TIM3;
	HAL_GPIO_Init(BOARD_SCREEN_BL_PORT, &g);

	/* RGB LED */
	g.Pin = BOARD_LED_RED_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Alternate = GPIO_AF2_TIM2;
	HAL_GPIO_Init(BOARD_LED_RED_PORT, &g);

	g.Pin = BOARD_LED_GREEN_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Alternate = GPIO_AF2_TIM2;
	HAL_GPIO_Init(BOARD_LED_GREEN_PORT, &g);

	g.Pin = BOARD_LED_BLUE_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Alternate = GPIO_AF2_TIM2;
	HAL_GPIO_Init(BOARD_LED_BLUE_PORT, &g);

	/* Speaker */
	g.Pin = BOARD_SPEAKER_PIN;
	g.Mode = GPIO_MODE_OUTPUT_PP;
	g.Pull = GPIO_PULLDOWN;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_SPEAKER_PORT, &g);

	/* Charger */
	g.Pin = BOARD_NCHARGE_PIN;
	g.Mode = GPIO_MODE_IT_RISING;
	g.Pull = GPIO_PULLUP;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_NCHARGE_PORT, &g);
	system_register_lp_pin(BOARD_NCHARGE_PORT, BOARD_NCHARGE_PIN);

	/* Battery voltage measurement */
	g.Pin = BOARD_VBATT_MEAS_PIN;
	g.Mode = GPIO_MODE_OUTPUT_PP;
	g.Pull = GPIO_PULLDOWN;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_VBATT_MEAS_PORT, &g);
	system_register_lp_pin(BOARD_VBATT_MEAS_PORT, BOARD_VBATT_MEAS_PIN);

	g.Pin = BOARD_VBATT_ANA_PIN;
	g.Mode = GPIO_MODE_ANALOG;
	g.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BOARD_VBATT_ANA_PORT, &g);

	/* VBUS sensing */
	g.Pin = BOARD_VBUS_SENSE_PIN;
	g.Mode = GPIO_MODE_IT_RISING;
	g.Pull = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_VBUS_SENSE_PORT, &g);
	system_register_lp_pin(BOARD_VBUS_SENSE_PORT, BOARD_VBUS_SENSE_PIN);

	/* USB */
	g.Pin = BOARD_USB_DP_PIN;
	g.Mode = GPIO_MODE_INPUT;
	g.Pull = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(BOARD_USB_DP_PORT, &g);
	system_register_lp_pin(BOARD_USB_DP_PORT, BOARD_USB_DP_PIN);

	g.Pin = BOARD_USB_DM_PIN;
	g.Mode = GPIO_MODE_INPUT;
	g.Pull = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(BOARD_USB_DM_PORT, &g);
	system_register_lp_pin(BOARD_USB_DM_PORT, BOARD_USB_DM_PIN);
}

void board_init_irq(void)
{
	/* Buttons */
	HAL_NVIC_SetPriority(EXTI2_3_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

	/* Charger and VBUS */
	HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void EXTI0_1_IRQHandler(void)
{
	input_ll_irq_handler(INPUT_BTN_MIDDLE);
}

void EXTI2_3_IRQHandler(void)
{
	input_ll_irq_handler(INPUT_BTN_LEFT);
	input_ll_irq_handler(INPUT_BTN_RIGHT);
}

void EXTI4_15_IRQHandler(void)
{
	input_ll_irq_handler(INPUT_BATTERY_CHARGING);
	input_ll_irq_handler(INPUT_VBUS_SENSING);
}
