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
#ifndef _UC1701X_H_
#define _UC1701X_H_

#include <stdint.h>

#define REG_COL_ADDR_LSB				0x00
#define REG_COL_ADDR_MSB				0x10
#define REG_POWER_CTRL					0x28
#define POWER_CTRL_BOOST				(1 << 0)
#define POWER_CTRL_V_REG				(1 << 1)
#define POWER_CTRL_V_FOL				(1 << 2)
#define REG_SCROLL_LINE					0x40
#define REG_PAGE_ADDR					0xB0
#define REG_VLCD_RES_RATIO				0x20
#define REG_ELEC_VOLUME					0x81 // double-byte
#define REG_ALL_PIX_ON					0xA4
#define REG_INV_DISP					0xA6
#define REG_DISP_EN					0xAE
#define REG_SEG_DIR					0xA0
#define REG_COM_DIR					0xC0
#define REG_RESET					0xE2
#define REG_LCD_BIAS_RATIO				0xA2
#define REG_SET_CUR_UPDATE				0xE0
#define REG_RES_CUR_UPDATE				0xEE
#define REG_ADV_PRG_CTRL0				0xFA // double-byte

typedef enum {
	DISP_MODE_NORMAL,
	DISP_MODE_INVERTED,
} disp_mode_t;

typedef enum{
	PWR_MODE_SLEEP,
	PWR_MODE_ON,
} pwr_mode_t;


void uc1701x_init(void);

void uc1701x_set_display_mode(disp_mode_t mode);
void uc1701x_set_power_mode(pwr_mode_t mode);

void uc1701x_send_cmd_1b(uint8_t reg, uint8_t data);
void uc1701x_send_cmd_2b(uint8_t reg, uint8_t data);

void uc1701x_send_data(uint8_t *data, uint16_t length);

#endif /* _UC1701X_H_ */
