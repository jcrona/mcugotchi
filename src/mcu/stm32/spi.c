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

static SPI_HandleTypeDef hspi1;


void spi_init(void)
{
	__HAL_RCC_SPI1_CLK_ENABLE();

	hspi1.Instance               = SPI1;
	hspi1.Init.Mode              = SPI_MODE_MASTER;
	hspi1.Init.Direction         = SPI_DIRECTION_1LINE;
	hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
	hspi1.Init.NSS               = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode            = SPI_TIMODE_DISABLED;
	hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
	hspi1.Init.CRCPolynomial     = 7;
	HAL_SPI_Init(&hspi1);

	/* Enable 1LINE TX mode */
	SPI_1LINE_TX(&hspi1);

	__HAL_SPI_ENABLE(&hspi1);
}

void spi_write(uint8_t data)
{
	/* Bypass all the ckecks from HAL and write directly to SPI1 */
	*(__IO uint8_t *) (&(hspi1.Instance)->DR) = data;
	while(__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY));
}
