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
#ifndef _SSD1306_H_
#define _SSD1306_H_

#include <stdint.h>

#define REG_CONTRAST					0x81
#define REG_DISP_ON					0xA4
#define REG_DISP_MODE					0xA6
#define REG_DISP_EN					0xAE

#define REG_LOW_ADDR					0x00
#define REG_HIGH_ADDR					0x10
#define REG_MEM_ADDR_MODE				0x20
#define MEM_ADDR_MODE_H					0x00
#define MEM_ADDR_MODE_V					0x01
#define MEM_ADDR_MODE_P					0x02
#define REG_PAGE_START_ADDR				0xB0
#define REG_COL_ADDR					0x21
#define REG_PAGE_ADDR					0x22

#define REG_DISP_START_LINE				0x40
#define REG_SEG_REMAP					0xA0
#define REG_MUX_RATIO					0xA8
#define REG_COM_SCAN_DIR				0xC0
#define REG_DISP_OFFSET					0xD3
#define REG_COM_PINS_CFG				0xDA
#define REG_DISP_CLK_CFG				0xD5
#define REG_PRE_CHRG_PERIOD				0xD9
#define REG_CHRG_PUMP					0x8D
#define REG_VCOMH_LVL					0xDB

typedef enum {
	DISP_MODE_NORMAL,
	DISP_MODE_INVERTED,
} disp_mode_t;

typedef enum{
	PWR_MODE_SLEEP,
	PWR_MODE_ON,
} pwr_mode_t;


void ssd1306_init(void);

void ssd1306_set_display_mode(disp_mode_t mode);
void ssd1306_set_power_mode(pwr_mode_t mode);

void ssd1306_send_cmd_1b(uint8_t reg, uint8_t data);
void ssd1306_send_cmd_2b(uint8_t reg, uint8_t data);
void ssd1306_send_cmd_3b(uint8_t reg, uint8_t data1, uint8_t data2);

void ssd1306_send_data(uint8_t *data, uint16_t length);

#endif /* _SSD1306_H_ */
