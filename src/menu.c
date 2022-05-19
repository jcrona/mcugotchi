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
#include <stddef.h>

#include "gfx.h"
#include "menu.h"

#define MENU_OFFSET_X				0
#define MENU_OFFSET_Y				0
#define TEXT_OFFSET_X				1
#define TEXT_OFFSET_Y				1
#define MENU_ITEM_STRIDE_Y			16
#define MENU_ITEM_SIZE				1
#define ITEMS_ON_SCREEN				((DISPLAY_HEIGHT - MENU_OFFSET_Y)/MENU_ITEM_STRIDE_Y)

#define MAX_DEPTH				9

static uint8_t is_visible = 0;
static menu_item_t *g_menu = NULL;
static menu_item_t *current_menu = NULL;
static int8_t current_item = -1;
static uint8_t current_depth = 0;

static menu_parent_t parents[MAX_DEPTH + 1] = { 0 }; // parents[0] will always be NULL


static void menu_confirm_yes(uint8_t pos, menu_parent_t *parent)
{
	menu_back();

	/* Execute the callback */
	current_menu[current_item].cb(current_item, &parents[current_depth]);
}

static void menu_confirm_no(uint8_t pos, menu_parent_t *parent)
{
	menu_back();
}

static menu_item_t confirm_menu[] = {
	{"I am sure", NULL, &menu_confirm_yes, 0, NULL},
	{"Go back ", NULL, &menu_confirm_no, 0, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

void menu_draw(void)
{
	uint8_t i;
	uint8_t top_item = 0;
	uint8_t y = MENU_OFFSET_Y, x;

	if (!is_visible) {
		return;
	}

	if (current_item >= ITEMS_ON_SCREEN) {
		top_item = current_item - ITEMS_ON_SCREEN + 1;
	}

	gfx_clear();

	gfx_square(0, y + (current_item - top_item) * MENU_ITEM_STRIDE_Y, DISPLAY_WIDTH, MENU_ITEM_STRIDE_Y, COLOR_ON_BLACK);

	for (i = 0; i < ITEMS_ON_SCREEN; i++) {
		if (current_menu[top_item + i].name == NULL) {
			break;
		}

		x = gfx_string(current_menu[top_item + i].name, MENU_OFFSET_X + TEXT_OFFSET_X, y + TEXT_OFFSET_Y, MENU_ITEM_SIZE, (top_item + i == current_item) ? COLOR_OFF_WHITE : COLOR_ON_BLACK, BACKGROUND_OFF);

		if (current_menu[top_item + i].arg_cb != NULL) {
			gfx_string(current_menu[top_item + i].arg_cb(top_item + i, &parents[current_depth]), x, y + TEXT_OFFSET_Y, MENU_ITEM_SIZE, (top_item + i == current_item) ? COLOR_OFF_WHITE : COLOR_ON_BLACK, BACKGROUND_OFF);
		}

		y += MENU_ITEM_STRIDE_Y;
	}

	gfx_print_screen();
}

void menu_register(menu_item_t *menu)
{
	g_menu = menu;
}

void menu_open(void)
{
	if (g_menu == NULL) {
		return;
	}

	is_visible = 1;

	current_menu = g_menu;
	current_depth = 0;
	current_item = -1;
	menu_next();

	menu_draw();
}

void menu_close(void)
{
	gfx_clear();
	gfx_print_screen();

	is_visible = 0;
}

uint8_t menu_is_visible(void)
{
	return is_visible;
}

void menu_next(void)
{
	do {
		current_item++;

		if (current_menu[current_item].name == NULL) {
			current_item = 0;
		}
	} while(current_menu[current_item].cb == NULL && current_menu[current_item].sub_menu == NULL);

	menu_draw();
}

void menu_enter(void)
{
	menu_item_t *sub_menu = NULL;

	if (current_menu[current_item].cb != NULL) {
		/* Regular item */
		if (current_menu[current_item].confirm) {
			/* Confirmation sub menu */
			sub_menu = confirm_menu;
		} else {
			/* Execute the callback right away */
			current_menu[current_item].cb(current_item, &parents[current_depth]);
		}
	} else if (current_menu[current_item].sub_menu != NULL) {
		/* Sub menu */
		sub_menu = current_menu[current_item].sub_menu;
	}

	if (sub_menu != NULL && current_depth < MAX_DEPTH) {
		current_depth++;
		parents[current_depth].menu = current_menu;
		parents[current_depth].pos = current_item;
		current_menu = sub_menu;
		current_item = -1;
		menu_next();
	}

	menu_draw();
}

void menu_back(void)
{
	if (current_depth > 0) {
		current_menu = parents[current_depth].menu;
		current_item = parents[current_depth].pos;
		current_depth--;

		menu_draw();
	} else {
		menu_close();
	}
}
