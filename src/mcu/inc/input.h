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
#ifndef _INPUT_H_
#define _INPUT_H_

typedef enum {
	INPUT_STATE_LOW = 0,
	INPUT_STATE_HIGH,
} input_state_t;

typedef enum {
	INPUT_BTN_LEFT = 0,
	INPUT_BTN_MIDDLE,
	INPUT_BTN_RIGHT,
	INPUT_BATTERY_CHARGING,
	INPUT_VBUS_SENSING,
} input_t;


void input_init(void);
input_state_t input_get_state(input_t input);
void input_register_handler(void (*handler)(input_t, input_state_t, uint8_t));

#endif /* _INPUT_H_ */
