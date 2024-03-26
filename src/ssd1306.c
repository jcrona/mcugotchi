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
#include "ssd1306.h"

void ssd1306_init(void)
{
	spi_init();

	/* Power-up sequence */
	gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
	gpio_clear(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	time_delay(MS_TO_MCU_TIME(1));
	gpio_set(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	time_delay(MS_TO_MCU_TIME(1));
	gpio_clear(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	time_delay(MS_TO_MCU_TIME(1));
	gpio_set(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	gpio_set(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
	time_delay(MS_TO_MCU_TIME(10));

	/* Configuration */
	ssd1306_send_cmd_2b(REG_MUX_RATIO, 0x3F);
	ssd1306_send_cmd_2b(REG_DISP_OFFSET, 0x00);
	ssd1306_send_cmd_1b(REG_DISP_START_LINE, 0);
	ssd1306_send_cmd_1b(REG_SEG_REMAP, 0);
	ssd1306_send_cmd_2b(REG_COM_PINS_CFG, 0x12);
	ssd1306_send_cmd_1b(REG_COM_SCAN_DIR, 0);

	ssd1306_send_cmd_2b(REG_CONTRAST, 0x7F);
	ssd1306_send_cmd_2b(REG_MEM_ADDR_MODE, MEM_ADDR_MODE_H);
	ssd1306_send_cmd_3b(REG_COL_ADDR, 0x00, 0x7F);
	ssd1306_send_cmd_3b(REG_PAGE_ADDR, 0x00, 0x07);

	ssd1306_send_cmd_1b(REG_DISP_MODE, 0);
	ssd1306_send_cmd_2b(REG_VCOMH_LVL, 0x00);

	ssd1306_send_cmd_1b(REG_DISP_ON, 0);
	ssd1306_send_cmd_2b(REG_DISP_CLK_CFG, 0x80);
	ssd1306_send_cmd_2b(REG_CHRG_PUMP, 0x14);
	ssd1306_send_cmd_1b(REG_DISP_EN, 1);
}

void ssd1306_set_display_mode(disp_mode_t mode)
{
	ssd1306_send_cmd_1b(REG_DISP_MODE, (mode == DISP_MODE_NORMAL) ? 0 : 1);
}

void ssd1306_set_power_mode(pwr_mode_t mode)
{
	switch (mode) {
		case PWR_MODE_SLEEP:
			ssd1306_send_cmd_1b(REG_DISP_EN, 0);
			ssd1306_send_cmd_2b(REG_CHRG_PUMP, 0x10);
			break;

		case PWR_MODE_ON:
			ssd1306_send_cmd_2b(REG_CHRG_PUMP, 0x14);
			ssd1306_send_cmd_1b(REG_DISP_EN, 1);
			break;
	}
}

void ssd1306_send_cmd_1b(uint8_t reg, uint8_t data)
{
	gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);

	spi_write(reg | data);
	time_delay(US_TO_MCU_TIME(1));

	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}

void ssd1306_send_cmd_2b(uint8_t reg, uint8_t data)
{
	gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);

	spi_write(reg);
	spi_write(data);
	time_delay(US_TO_MCU_TIME(1));

	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}

void ssd1306_send_cmd_3b(uint8_t reg, uint8_t data1, uint8_t data2)
{
	gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);

	spi_write(reg);
	spi_write(data1);
	spi_write(data2);
	time_delay(US_TO_MCU_TIME(1));

	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}

void ssd1306_send_data(uint8_t *data, uint16_t length)
{
	uint16_t i;

	gpio_set(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);

	for (i = 0; i < length; i++) {
		spi_write(data[i]);
	}
	time_delay(US_TO_MCU_TIME(1));

	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}
