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

#include "spi.h"
#include "board.h"

static SPI_HandleTypeDef hspi;


void spi_init(void)
{
	BOARD_SCREEN_SPI_CLK_ENABLE();

	hspi.Instance               = BOARD_SCREEN_SPI;
	hspi.Init.Mode              = SPI_MODE_MASTER;
	hspi.Init.Direction         = SPI_DIRECTION_1LINE;
	hspi.Init.DataSize          = SPI_DATASIZE_8BIT;
	hspi.Init.CLKPolarity       = SPI_POLARITY_LOW;
	hspi.Init.CLKPhase          = SPI_PHASE_1EDGE;
	hspi.Init.NSS               = SPI_NSS_SOFT;
	hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	hspi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	hspi.Init.TIMode            = SPI_TIMODE_DISABLED;
	hspi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
	hspi.Init.CRCPolynomial     = 7;
	HAL_SPI_Init(&hspi);

	/* Enable 1LINE TX mode */
	SPI_1LINE_TX(&hspi);

	__HAL_SPI_ENABLE(&hspi);
}

void spi_write(uint8_t data)
{
	/* Bypass all the ckecks from HAL and write directly to SPI */
	*(__IO uint8_t *) (&(hspi.Instance)->DR) = data;
	while(__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_BSY));
}
