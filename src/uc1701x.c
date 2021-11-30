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

#include "time.h"
#include "spi.h"
#include "gpio.h"
#include "board.h"
#include "uc1701x.h"

void uc1701x_init(void)
{
	spi_init();

	/* Power-up sequence */
	gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
	gpio_clear(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	time_delay(MS_TO_MCU_TIME(1));
	gpio_set(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	time_delay(MS_TO_MCU_TIME(5));
	gpio_set(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);

	/* Configuration */
	uc1701x_send_cmd_1b(REG_RESET, 0);
	uc1701x_send_cmd_1b(REG_POWER_CTRL, POWER_CTRL_BOOST | POWER_CTRL_V_REG | POWER_CTRL_V_FOL);
	uc1701x_send_cmd_1b(REG_ADV_PRG_CTRL0, 0x93);
	uc1701x_send_cmd_1b(REG_SCROLL_LINE, 0);
	uc1701x_send_cmd_1b(REG_SEG_DIR, 1);
	uc1701x_send_cmd_1b(REG_COM_DIR, 0);

	time_delay(MS_TO_MCU_TIME(120));
	uc1701x_send_cmd_1b(REG_LCD_BIAS_RATIO, 0);
	uc1701x_send_cmd_2b(REG_ELEC_VOLUME, 50);
	uc1701x_send_cmd_1b(REG_VLCD_RES_RATIO, 3);
}

void uc1701x_set_display_mode(disp_mode_t mode)
{
	uc1701x_send_cmd_1b(REG_INV_DISP, (mode == DISP_MODE_NORMAL) ? 0 : 1);
}

void uc1701x_set_power_mode(pwr_mode_t mode)
{
	switch (mode) {
		case PWR_MODE_SLEEP:
			uc1701x_send_cmd_1b(REG_DISP_EN, 0);
			uc1701x_send_cmd_1b(REG_ALL_PIX_ON, 1);
			break;

		case PWR_MODE_ON:
			uc1701x_send_cmd_1b(REG_ALL_PIX_ON, 0);
			uc1701x_send_cmd_1b(REG_DISP_EN, 1);
			break;
	}
}

void uc1701x_send_cmd_1b(uint8_t reg, uint8_t data)
{
	gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);

	spi_write(reg | data);
	time_delay(US_TO_MCU_TIME(1));

	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}

void uc1701x_send_cmd_2b(uint8_t reg, uint8_t data)
{
	gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);

	spi_write(reg);
	spi_write(data);
	time_delay(US_TO_MCU_TIME(1));

	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}

void uc1701x_send_data(uint8_t *data, uint16_t length)
{
	uint16_t i;
	uint16_t page = 0;
	uint16_t block_len;

	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);

	while (length > 0) {
		if (length < 128) {
			block_len = length;
		} else {
			block_len = 128;
		}

		gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);

		spi_write(REG_PAGE_ADDR | page++);
		spi_write(REG_COL_ADDR_LSB | 4); // The logical line is actually 132 pixels, so a shift of 4 rows is needed since MX is enabled
		spi_write(REG_COL_ADDR_MSB | 0);

		time_delay(US_TO_MCU_TIME(1));

		gpio_set(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);

		for (i = 0; i < block_len; i++) {
			spi_write(data[i]);
		}

		time_delay(US_TO_MCU_TIME(1));

		length -= block_len;
		data += block_len;
	}

	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}
