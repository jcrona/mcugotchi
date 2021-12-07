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

	g.Pin = BOARD_MIDDLE_BTN_PIN;
	g.Mode = GPIO_MODE_IT_RISING;
	g.Pull = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_MIDDLE_BTN_PORT, &g);

	g.Pin = BOARD_RIGHT_BTN_PIN;
	g.Mode = GPIO_MODE_IT_RISING;
	g.Pull = GPIO_PULLDOWN;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_RIGHT_BTN_PORT, &g);

	/* SPI 1 */
	g.Pin = BOARD_SCREEN_SCLK_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Alternate = GPIO_AF0_SPI1;
	HAL_GPIO_Init(BOARD_SCREEN_SCLK_PORT, &g);

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

	/* USB */
	g.Pin = BOARD_USB_DP_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Pull = GPIO_NOPULL;
	g.Alternate = GPIO_AF2_USB;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_USB_DP_PORT, &g);

	g.Pin = BOARD_USB_DM_PIN;
	g.Mode = GPIO_MODE_AF_PP;
	g.Pull = GPIO_NOPULL;
	g.Alternate = GPIO_AF2_USB;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(BOARD_USB_DM_PORT, &g);
}

void board_init_irq(void)
{
	/* Buttons */
	HAL_NVIC_SetPriority(EXTI2_3_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

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
