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
#ifndef _MENU_H_
#define _MENU_H_

#include <stdint.h>

typedef struct menu_item menu_item_t;
typedef struct menu_parent menu_parent_t;

struct menu_item {
	char *name;
	char * (*arg_cb)(uint8_t, menu_parent_t *);
	void (*cb)(uint8_t, menu_parent_t *);
	uint8_t confirm; // 0: no confirmation, 1: confirmations
	struct menu_item *sub_menu;
};

struct menu_parent {
	menu_item_t *menu;
	uint8_t pos;
};

void menu_draw(void);

void menu_register(menu_item_t *items);

void menu_open(void);
void menu_close(void);

uint8_t menu_is_visible(void);

void menu_next(void);
void menu_enter(void);
void menu_back(void);

#endif /* _MENU_H_ */
