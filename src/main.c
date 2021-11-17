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
 *
 * The expected connections are the following:
 * | Name            | PIN     |
 * |-----------------|---------|
 * | SSD1306 CLK PIN | PA5     |
 * | SSD1306 DIN PIN | PA7     |
 * | SSD1306 DC PIN  | PA1     |
 * | SSD1306 CE PIN  | PA2     |
 * | SSD1306 RES PIN | PA6     |
 * | SSD1306 VCC PIN | VDD     |
 * | SSD1306 GND PIN | GND     |
 * | Left Button     | PB3/VDD |
 * | Middle Button   | PA0/VDD |
 * | Right Button    | PB2/VDD |
 */
#include <stdint.h>

#include "ssd1306.h"
#include "gfx.h"
#include "menu.h"
#include "job.h"
#include "time.h"
#include "storage.h"
#include "state.h"
#include "button.h"
#include "usb.h"
#include "fs_ll.h"
#include "rom.h"
#include "board.h"

#include "lib/tamalib.h"

#define FIRMWARE_VERSION				"v0.1"

#define PIXEL_SIZE					3
#define ICON_SIZE					8
#define ICON_STRIDE_X					24
#define ICON_STRIDE_Y					56
#define ICON_OFFSET_X					24
#define ICON_OFFSET_Y					0
#define LCD_OFFET_X					16
#define LCD_OFFET_Y					8

#define PAUSED_X					34
#define PAUSED_Y					24
#define PAUSED_STR					"Paused"

#define USBON_X						24
#define USBON_Y						24
#define USBON_STR					"USB Mode"

#define FRAMERATE 					30

#define TAMALIB_FREQ					32768 // Hz

#define MAIN_JOB_PERIOD					1 //ms

static volatile u12_t *g_program = (volatile u12_t *) (STORAGE_BASE_ADDRESS + (STORAGE_ROM_OFFSET << 2));

static bool_t matrix_buffer[LCD_HEIGHT][LCD_WIDTH] = {{0}};
static bool_t icon_buffer[ICON_NUM] = {0};

static uint16_t time_shift = 0;

static bool_t tamalib_is_late = 0;

static job_t cpu_job;
static job_t render_job;

static bool_t lcd_inverted = 0;
static uint8_t speed_ratio = 1;
static bool_t emulation_paused = 0;
static bool_t usb_enabled = 0;
static bool_t rom_loaded = 1;

static const bool_t icons[ICON_NUM][ICON_SIZE][ICON_SIZE] = {
	{
		{1, 0, 1, 0, 1, 0, 0, 1},
		{1, 0, 1, 0, 1, 0, 1, 1},
		{1, 1, 1, 1, 1, 0, 1, 1},
		{0, 1, 1, 1, 0, 1, 1, 1},
		{0, 0, 1, 0, 0, 1, 1, 1},
		{0, 0, 1, 0, 0, 0, 1, 1},
		{0, 0, 1, 0, 0, 0, 0, 1},
		{0, 0, 1, 0, 0, 0, 0, 1},
	},
	{
		{0, 1, 0, 0, 0, 0, 1, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{1, 0, 1, 0, 0, 1, 0, 1},
		{0, 0, 1, 0, 0, 1, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 1, 0, 0, 0, 0, 1, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
	},
	{
		{1, 1, 1, 0, 0, 1, 1, 1},
		{1, 0, 1, 0, 1, 1, 1, 1},
		{1, 1, 1, 0, 1, 1, 1, 1},
		{0, 0, 0, 1, 1, 1, 1, 0},
		{0, 0, 1, 1, 1, 0, 0, 0},
		{0, 0, 1, 1, 0, 0, 0, 0},
		{1, 1, 0, 0, 0, 0, 0, 0},
		{1, 1, 0, 0, 0, 0, 0, 0},
	},
	{
		{0, 0, 0, 1, 1, 1, 0, 0},
		{0, 0, 0, 0, 1, 1, 1, 0},
		{0, 0, 0, 1, 1, 1, 1, 1},
		{0, 0, 1, 1, 0, 1, 1, 1},
		{0, 1, 1, 0, 1, 1, 0, 1},
		{0, 1, 1, 1, 1, 0, 0, 0},
		{1, 1, 1, 1, 0, 0, 0, 0},
		{1, 1, 0, 0, 0, 0, 0, 0},
	},
	{
		{0, 1, 1, 0, 0, 0, 0, 0},
		{1, 0, 0, 1, 0, 0, 1, 1},
		{1, 0, 0, 1, 1, 1, 0, 1},
		{0, 1, 0, 1, 0, 0, 1, 1},
		{0, 1, 0, 0, 1, 1, 0, 1},
		{0, 1, 0, 0, 0, 0, 0, 1},
		{0, 1, 1, 0, 0, 0, 1, 1},
		{0, 0, 1, 1, 1, 1, 1, 0},
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 0},
		{1, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 1, 0, 1, 0, 1, 1},
		{1, 0, 1, 0, 1, 0, 1, 1},
		{1, 0, 0, 1, 0, 0, 0, 1},
		{0, 1, 0, 0, 1, 0, 1, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 1, 0},
		{0, 0, 0, 0, 0, 1, 0, 1},
		{0, 1, 1, 1, 0, 1, 0, 1},
		{1, 1, 1, 0, 0, 1, 0, 1},
		{1, 1, 0, 0, 1, 0, 1, 0},
		{1, 1, 1, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 0, 0, 0, 0},
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 0, 0, 0, 0},
		{1, 0, 0, 0, 1, 0, 0, 0},
		{1, 1, 0, 1, 1, 1, 1, 0},
		{1, 0, 1, 0, 0, 1, 0, 1},
		{0, 1, 1, 1, 1, 0, 1, 1},
		{0, 0, 0, 1, 0, 0, 0, 1},
		{0, 0, 0, 0, 1, 1, 1, 0},
	},
};


/* No need to support breakpoints */
static void * hal_malloc(u32_t size)
{
	return NULL;
}

static void hal_free(void *ptr)
{
}

static void hal_halt(void)
{
}

/* No need to support logs */
static bool_t hal_is_log_enabled(log_level_t level)
{
	return 0;
}

static void hal_log(log_level_t level, char *buff, ...)
{
}

static timestamp_t hal_get_timestamp(void)
{
	return (timestamp_t) (time_get() << time_shift);
}

static void hal_sleep_until(timestamp_t ts)
{
	/* Since TamaLIB is always late in implementations without mainloop,
	 * notify the cpu job that TamaLIB catched up instead of waiting
	 */
	if ((int32_t) (ts - hal_get_timestamp()) > 0) {
		tamalib_is_late = 0;
	}
}

static void draw_icon(uint8_t x, uint8_t y, uint8_t num, uint8_t v)
{
	uint8_t i, j;

	if (v) {
		for (j = 0; j < ICON_SIZE; j++) {
			for (i = 0; i < ICON_SIZE; i++) {
				if(icons[num][j][i]) {
					SetPix(x + i, y + j);
				}
			}
		}
	} else {
		for (j = 0; j < ICON_SIZE; j++) {
			for (i = 0; i < ICON_SIZE; i++) {
				if(icons[num][j][i]) {
					ClrPix(x + i, y + j);
				}
			}
		}
	}
}

static void draw_square(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t v)
{
	uint8_t i, j;

	if (v) {
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				SetPix(x + i, y + j);
			}
		}
	} else {
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				ClrPix(x + i, y + j);
			}
		}
	}
}

static void hal_update_screen(void)
{
	u8_t i, j;

	if (menu_is_visible()) {
		return;
	}

	ClrBuf();

	/* Dot matrix */
	for (j = 0; j < LCD_HEIGHT; j++) {
		for (i = 0; i < LCD_WIDTH; i++) {
			if (matrix_buffer[j][i]) {
				draw_square(i * PIXEL_SIZE + LCD_OFFET_X, j * PIXEL_SIZE + LCD_OFFET_Y, PIXEL_SIZE, PIXEL_SIZE, 1);
			}
		}
	}

	/* Icons */
	for (i = 0; i < ICON_NUM; i++) {
		if (icon_buffer[i]) {
			draw_icon((i % 4) * ICON_STRIDE_X + ICON_OFFSET_X, (i / 4) * ICON_STRIDE_Y + ICON_OFFSET_Y, i, 1);
		}
	}

	if (usb_enabled) {
		PStr(USBON_STR, USBON_X, USBON_Y, 1, PixNorm);
	} else if (emulation_paused) {
		PStr(PAUSED_STR, PAUSED_X, PAUSED_Y, 1, PixNorm);
	}

	PScrn();
}

static void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
	matrix_buffer[y][x] = val;
}

static void hal_set_lcd_icon(u8_t icon, bool_t val)
{
	if (icon_buffer[icon] == 0 && val == 1) {
		/* The Tamagotchi is calling */
		if (menu_is_visible()) {
			menu_close();
		}
	}

	icon_buffer[icon] = val;
}

/* TODO: sound support */
static void hal_set_frequency(u32_t freq)
{
}

static void hal_play_frequency(bool_t en)
{
}

static int hal_handler(void)
{
	return 0;
}

static hal_t hal = {
	.malloc = &hal_malloc,
	.free = &hal_free,
	.halt = &hal_halt,
	.is_log_enabled = &hal_is_log_enabled,
	.log = &hal_log,
	.sleep_until = &hal_sleep_until,
	.get_timestamp = &hal_get_timestamp,
	.update_screen = &hal_update_screen,
	.set_lcd_matrix = &hal_set_lcd_matrix,
	.set_lcd_icon = &hal_set_lcd_icon,
	.set_frequency = &hal_set_frequency,
	.play_frequency = &hal_play_frequency,
	.handler = &hal_handler,
};

static void menu_screen_mode(uint8_t pos, menu_parent_t *parent)
{
	lcd_inverted = !lcd_inverted;
	ssd1306_set_display_mode(lcd_inverted ? DISP_MODE_INVERTED : DISP_MODE_NORMAL);
}

static char * menu_screen_mode_arg(uint8_t pos, menu_parent_t *parent)
{
	switch (lcd_inverted) {
		case 0:
			return "Inv.";

		default:
		case 1:
			return "Norm.";
	}
}

static void menu_toggle_speed(uint8_t pos, menu_parent_t *parent)
{
	speed_ratio = !speed_ratio;
	tamalib_set_speed(speed_ratio);
}

static char * menu_toggle_speed_arg(uint8_t pos, menu_parent_t *parent)
{
	switch (speed_ratio) {
		case 0:
			return "x1";

		default:
		case 1:
			return "Max";
	}
}

static void menu_pause(uint8_t pos, menu_parent_t *parent)
{
	emulation_paused = !emulation_paused;
	tamalib_set_exec_mode(emulation_paused ? EXEC_MODE_PAUSE : EXEC_MODE_RUN);
}

static char * menu_pause_arg(uint8_t pos, menu_parent_t *parent)
{
	switch (emulation_paused) {
		case 0:
			return "Pause";

		default:
		case 1:
			return "Run";
	}
}

static void menu_reset_cpu(uint8_t pos, menu_parent_t *parent)
{
	cpu_reset();
	menu_close();
}

static void menu_factory_reset(uint8_t pos, menu_parent_t *parent)
{
	fs_ll_umount();
	storage_erase();
	system_reset();
}

static void menu_slots(uint8_t pos, menu_parent_t *parent)
{
	if (parent->pos == 0) {
		/* Load */
		state_load(pos);
		menu_close();
	} else if (parent->pos == 1) {
		/* Save */
		state_save(pos);
		menu_close();
	} else {
		/* Clear */
		state_erase(pos);
	}
}

static char * menu_slots_arg(uint8_t pos, menu_parent_t *parent)
{
	switch (state_stat(pos)) {
		default:
		case 0:
			return "";

		case 1:
			return " *";
	}
}

static void menu_clear_states(uint8_t pos, menu_parent_t *parent)
{
	uint8_t i;

	for (i = 0; i < STATE_SLOTS_NUM; i++) {
		state_erase(i);
	}
}

static void menu_roms(uint8_t pos, menu_parent_t *parent)
{
	if (rom_load(pos) < 0) {
		return;
	}

	cpu_reset();
	menu_close();
}

static char * menu_roms_arg(uint8_t pos, menu_parent_t *parent)
{
	switch (rom_stat(pos)) {
		default:
		case 0:
			return "";

		case 1:
			return " *";
	}
}

static void menu_usb(uint8_t pos, menu_parent_t *parent)
{
	menu_pause(0, NULL);
	fs_ll_umount();
	usb_start();
	menu_close();
	usb_enabled = 1;
}

static menu_item_t options_menu[] = {
	{"Screen ", &menu_screen_mode_arg, &menu_screen_mode, 0, NULL},
	{"Speed ", &menu_toggle_speed_arg, &menu_toggle_speed, 0, NULL},
	{"", &menu_pause_arg, &menu_pause, 0, NULL},
	{"Reset CPU", NULL, &menu_reset_cpu, 1, NULL},
	{"Fact. Reset", NULL, &menu_factory_reset, 1, NULL},
	{"FW. "FIRMWARE_VERSION, NULL, NULL, 0, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t slots_menu[] = {
	{"Slot 0", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 1", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 2", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 3", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 4", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 5", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 6", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 7", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 8", &menu_slots_arg, &menu_slots, 0, NULL},
	{"Slot 9", &menu_slots_arg, &menu_slots, 0, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t states_menu[] = {
	{"Load", NULL, NULL, 0, slots_menu},
	{"Save", NULL, NULL, 0, slots_menu},
	{"Clear", NULL, NULL, 0, slots_menu},
	{"Clear All", NULL, &menu_clear_states, 1, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t roms_menu[] = {
	{"ROM 0", &menu_roms_arg, &menu_roms, 1, NULL},
	{"ROM 1", &menu_roms_arg, &menu_roms, 1, NULL},
	{"ROM 2", &menu_roms_arg, &menu_roms, 1, NULL},
	{"ROM 3", &menu_roms_arg, &menu_roms, 1, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t main_menu[] = {
	{"Options", NULL, NULL, 0, options_menu},
	{"States", NULL, NULL, 0, states_menu},
	{"ROMs", NULL, NULL, 0, roms_menu},
	{"USB Mode", NULL, &menu_usb, 0, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static void ll_init(void)
{
	system_init();

	board_init();

	time_init();

	button_init();

	ssd1306_init();
	ssd1306_set_power_mode(PWR_MODE_ON);
	ssd1306_set_display_mode(lcd_inverted ? DISP_MODE_INVERTED : DISP_MODE_NORMAL);

	gfx_register_display(&ssd1306_send_data);
}

static void render_job_fn(job_t *job)
{
	job_schedule(&render_job, &render_job_fn, time_get() + MS_TO_MCU_TIME(1000)/FRAMERATE);

	hal_update_screen();
}

static void cpu_job_fn(job_t *job)
{
	job_t *next_job;

	job_schedule(&cpu_job, &cpu_job_fn, time_get() + MS_TO_MCU_TIME(MAIN_JOB_PERIOD));

	tamalib_is_late = 1;

	/* Execute all the missed steps at once */
	while (tamalib_is_late) {
		tamalib_step();

		next_job = job_get_next();
		if (next_job != NULL && next_job->time <= time_get()) {
			/* No more time to execute instructions */
			job_schedule(&cpu_job, &cpu_job_fn, next_job->time);
			break;
		}
	}
}
static void no_rom_btn_handler(button_t btn, btn_state_t state, bool_t long_press)
{
	system_reset();
}

static void usb_mode_btn_handler(button_t btn, btn_state_t state, bool_t long_press)
{
	if (state == BTN_STATE_PRESSED && !long_press) {
		usb_enabled = 0;
		usb_stop();
		fs_ll_mount();
		menu_pause(0, NULL);
	}
}

static void menu_btn_handler(button_t btn, btn_state_t state, bool_t long_press)
{
	if (state == BTN_STATE_PRESSED) {
		switch (btn) {
			case BTN_LEFT:
				menu_next();
				break;

			case BTN_MIDDLE:
				menu_enter();
				break;

			case BTN_RIGHT:
				menu_back();
				break;

		}
	}
}

static void default_btn_handler(button_t btn, btn_state_t state, bool_t long_press)
{
	if (long_press) {
		if (btn == BTN_RIGHT) {
			menu_open();

			/* Make sure TamaLIB receives a release since it received a press */
			tamalib_set_button(btn, BTN_STATE_RELEASED);
		}
	} else {
		tamalib_set_button(btn, state);
	}
}

static void btn_handler(button_t btn, btn_state_t state, bool_t long_press)
{
	/* Dispatch the event */
	if (!rom_loaded) {
		no_rom_btn_handler(btn, state, long_press);
	} else if (usb_enabled) {
		usb_mode_btn_handler(btn, state, long_press);
	} else if (menu_is_visible()) {
		menu_btn_handler(btn, state, long_press);
	} else {
		default_btn_handler(btn, state, long_press);
	}
}

int main(void)
{
	ll_init();

	fs_ll_init();
	fs_ll_mount();

	usb_init();

	button_register_handler(&btn_handler);

	/* Try to load the default ROM from the filesystem if it is not loaded */
	if (!rom_is_loaded() && rom_load(DEFAULT_ROM_SLOT) < 0) {
		rom_loaded = 0;

		PStr("No ROM found !", 0, 0, 0, PixNorm);
		PStr("Connect me to a computer", 0, 16, 0, PixNorm);
		PStr("and copy a Tamagotchi ROM", 0, 24, 0, PixNorm);
		PStr("named rom0.bin.", 0, 32, 0, PixNorm);
		PStr("Once done, press any", 0, 48, 0, PixNorm);
		PStr("button to continue.", 0, 56, 0, PixNorm);
		PScrn();

		fs_ll_umount();
		usb_start();
	} else {
		menu_register(main_menu);

		tamalib_register_hal(&hal);

		/* TamaLIB must use an integer time base of at least 32768 Hz,
		 * so shift the one provided by the MCU until it fits.
		 */
		while (((MCU_TIME_FREQ_X1000 << time_shift) < TAMALIB_FREQ * 1000) || (MCU_TIME_FREQ_X1000 << time_shift) % 1000) {
			time_shift++;
		}

		if (tamalib_init((const u12_t *) g_program, NULL, (MCU_TIME_FREQ_X1000 << time_shift)/1000)) {
			system_fatal_error();
		}

		job_schedule(&render_job, &render_job_fn, JOB_ASAP);
		job_schedule(&cpu_job, &cpu_job_fn, JOB_ASAP);
	}

	job_mainloop();

	tamalib_release();

	fs_ll_umount();
}
