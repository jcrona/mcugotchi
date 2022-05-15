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

#include "gfx.h"
#include "menu.h"
#include "job.h"
#include "time.h"
#include "storage.h"
#include "state.h"
#include "input.h"
#include "led.h"
#include "speaker.h"
#include "backlight.h"
#include "battery.h"
#include "usb.h"
#include "fs_ll.h"
#include "rom.h"
#include "config.h"
#include "board.h"
#if defined(BOARD_HAS_SSD1306)
#include "ssd1306.h"
#elif defined(BOARD_HAS_UC1701X)
#include "uc1701x.h"
#endif

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

#define PLEASE_WAIT_X					9
#define PLEASE_WAIT_Y					24
#define PLEASE_WAIT_STR					"Please Wait"

#define AUTOSAVING_X					14
#define AUTOSAVING_Y					24
#define AUTOSAVING_STR					"Autosaving"

#define BATTERY_ON_X					114
#define BATTERY_ON_Y					21
#define BATTERY_OFF_X					58
#define BATTERY_OFF_Y					21
#define BATTERY_W					12
#define BATTERY_THICKNESS				2
#define BATTERY_LVL_THICKNESS				2

#define FRAMERATE 					30

#define TAMALIB_FREQ					32768 // Hz

#define MAIN_JOB_PERIOD					10 //ms
#define BATTERY_JOB_PERIOD				60000 //ms
#define BACKLIGHT_OFF_PERIOD				5000 //ms
#define AUTOSAVE_PERIOD					3600000 //ms
#define AUTOOFF_PERIOD					30000 //ms

#define BATTERY_MIN					3500 // mV
#define BATTERY_MAX					4200 // mV
#define BATTERY_LOW					3650 // mV
#define BATTERY_MAX_LEVEL				5

#define AUTOSAVE_SLOT					0

static volatile u12_t *g_program = (volatile u12_t *) (STORAGE_BASE_ADDRESS + (STORAGE_ROM_OFFSET << 2));

static bool_t matrix_buffer[LCD_HEIGHT][LCD_WIDTH] = {{0}};
static bool_t icon_buffer[ICON_NUM] = {0};

static uint16_t time_shift = 0;

static bool_t tamalib_is_late = 0;

static job_t cpu_job;
static job_t render_job;
static job_t battery_job;
static job_t backlight_job;
static job_t autosave_job;
static job_t autooff_job;

static uint8_t speed_ratio = 1;
static bool_t emulation_paused = 0;
static bool_t usb_enabled = 0;
static bool_t rom_loaded = 1;
static bool_t power_off_mode = 0;
static bool_t is_backlight_on = 0;
static bool_t is_charging = 0;
static bool_t is_calling = 0;
static bool_t is_vbus = 0;
static uint16_t current_battery = BATTERY_MAX;

/* Default config values */
static config_t config = {
	.lcd_inverted = 0,
	.backlight_always_on = 0,
	.backlight_level = 2,
	.speaker_enabled = 1,
	.led_enabled = 1,
	.battery_enabled = 0,
	.autosave_enabled = 1,
};

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

static void battery_job_fn(job_t *job);
static void autosave_job_fn(job_t *job);
static void autooff_job_fn(job_t *job);


static void update_led(void)
{
	uint8_t r = 0, g = 0, b = 0;

	if (!config.led_enabled) {
		led_set(0, 0, 0);
		return;
	}

	if (is_charging) {
		/* Yellow */
		r = 0xFF;
		g = 0xFF;
	} else if (is_vbus) {
		/* Green */
		g = 0xFF;
	} else if (current_battery < BATTERY_LOW) {
		/* Red */
		r = 0xFF;
	}

	if (is_calling) {
		/* Blue */
		b = 0xFF;
	}

	led_set(r, g, b);
}

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

static void hal_update_screen(void)
{
}

static void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
	matrix_buffer[y][x] = val;
}

static void hal_set_lcd_icon(u8_t icon, bool_t val)
{
	if (icon == 7 && icon_buffer[icon] != val) {
		/* The Tamagotchi started or stopped calling */
		if (val && menu_is_visible()) {
			menu_close();
		}

		is_calling = val;

		update_led();
	}

	icon_buffer[icon] = val;
}

static void hal_set_frequency(u32_t freq)
{
	speaker_set_frequency(freq);
}

static void hal_play_frequency(bool_t en)
{
	speaker_enable((uint8_t) (en && config.speaker_enabled));
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

static void draw_icon(uint8_t x, uint8_t y, uint8_t num, color_t color)
{
	uint8_t i, j;

	for (j = 0; j < ICON_SIZE; j++) {
		for (i = 0; i < ICON_SIZE; i++) {
			if(icons[num][j][i]) {
				gfx_pixel(x + i, y + j, color);
			}
		}
	}
}

static void draw_battery(uint8_t x, uint8_t y, uint8_t w, uint8_t thickness, uint8_t level_thickness, uint8_t max_level, uint8_t level)
{
	uint8_t h = max_level * (level_thickness + 1) + 1 + 3 * thickness;
	uint8_t i;

	/* Body */
	gfx_square(x, y + thickness, thickness, h - thickness, COLOR_ON_BLACK);
	gfx_square(x + w - thickness, y + thickness, thickness, h - thickness, COLOR_ON_BLACK);
	gfx_square(x + thickness, y + thickness, w - thickness * 2, thickness, COLOR_ON_BLACK);
	gfx_square(x + thickness, y + h - thickness, w - thickness * 2, thickness, COLOR_ON_BLACK);
	gfx_square(x + w/4, y, w/2, thickness, COLOR_ON_BLACK);

	/* Level */
	for (i = 0; i < level; i++) {
		gfx_square(x + thickness + 1, y + h - thickness - (i + 1) * (level_thickness + 1), w - thickness * 2 - 2, level_thickness, COLOR_ON_BLACK);
	}
}

static void draw_battery_full(uint8_t x, uint8_t y)
{
	int8_t level = (((int32_t) current_battery - BATTERY_MIN) * (BATTERY_MAX_LEVEL + 1))/(BATTERY_MAX - BATTERY_MIN);

	if (level > BATTERY_MAX_LEVEL) {
		level = BATTERY_MAX_LEVEL;
	}

	if (level < 0) {
		level = 0;
	}

	draw_battery(x, y, BATTERY_W, BATTERY_THICKNESS, BATTERY_LVL_THICKNESS, BATTERY_MAX_LEVEL, level);

	if (is_charging) {
		gfx_square(x + BATTERY_W/2 - 2, y - 1 - 3 * 2, 2, 4, COLOR_ON_BLACK);
		gfx_square(x + BATTERY_W/2, y - 1 - 2 * 2, 2, 4, COLOR_ON_BLACK);
	}
}

static void tamalib_screen(void)
{
	u8_t i, j;

	/* Dot matrix */
	for (j = 0; j < LCD_HEIGHT; j++) {
		for (i = 0; i < LCD_WIDTH; i++) {
			if (matrix_buffer[j][i]) {
				gfx_square(i * PIXEL_SIZE + LCD_OFFET_X, j * PIXEL_SIZE + LCD_OFFET_Y, PIXEL_SIZE, PIXEL_SIZE, COLOR_ON_BLACK);
			}
		}
	}

	/* Icons */
	for (i = 0; i < ICON_NUM; i++) {
		if (icon_buffer[i]) {
			draw_icon((i % 4) * ICON_STRIDE_X + ICON_OFFSET_X, (i / 4) * ICON_STRIDE_Y + ICON_OFFSET_Y, i, COLOR_ON_BLACK);
		}
	}
}

static void please_wait_screen(void)
{
	gfx_clear();

	gfx_string(PLEASE_WAIT_STR, PLEASE_WAIT_X, PLEASE_WAIT_Y, 1, COLOR_ON_BLACK, BACKGROUND_ON);

	gfx_print_screen();
}

static void autosaving_screen(void)
{
	gfx_clear();

	gfx_string(AUTOSAVING_STR, AUTOSAVING_X, AUTOSAVING_Y, 1, COLOR_ON_BLACK, BACKGROUND_ON);

	gfx_print_screen();
}

static void no_rom_screen(void)
{
	gfx_string("No ROM found !", 0, 0, 0, COLOR_ON_BLACK, BACKGROUND_ON);
	gfx_string("Connect me to a PC", 0, 16, 0, COLOR_ON_BLACK, BACKGROUND_ON);

	/* Keep some space for the battery icon */
	gfx_string("and copy a Tamagotchi", 0, 24, 0, COLOR_ON_BLACK, BACKGROUND_ON);
	gfx_string("ROM named \"rom0.bin\".", 0, 32, 0, COLOR_ON_BLACK, BACKGROUND_ON);

	gfx_string("Once done, press any", 0, 48, 0, COLOR_ON_BLACK, BACKGROUND_ON);
	gfx_string("button to continue.", 0, 56, 0, COLOR_ON_BLACK, BACKGROUND_ON);
}

static void backlight_job_fn(job_t *job)
{
	backlight_set(0);
	is_backlight_on = 0;
}

static void turn_on_backlight(bool_t force)
{
	if (!is_backlight_on || force) {
		backlight_set((config.backlight_level < 16) ? config.backlight_level * 16 : 255);
		is_backlight_on = 1;
	}

	if (!config.backlight_always_on) {
		job_schedule(&backlight_job, &backlight_job_fn, time_get() + MS_TO_MCU_TIME(BACKLIGHT_OFF_PERIOD));
	}
}

static void user_feedback(void)
{
	/* Delay auto-power-off (only if no ROM is loaded) */
	if (!rom_loaded && !usb_enabled && !power_off_mode) {
		job_schedule(&autooff_job, &autooff_job_fn, time_get() + MS_TO_MCU_TIME(AUTOOFF_PERIOD));
	}

	/* Turn ON the backlight for few seconds */
	turn_on_backlight(0);
}

static void enable_usb(void)
{
	/* Disable auto-power-off when USB is enabled */
	if (!rom_loaded) {
		job_cancel(&autooff_job);
	}

	emulation_paused = 1;
	tamalib_set_exec_mode(emulation_paused ? EXEC_MODE_PAUSE : EXEC_MODE_RUN);

	fs_ll_umount();

	usb_init();
	usb_start();

	usb_enabled = 1;
}

static void disable_usb(void)
{
	usb_enabled = 0;

	usb_stop();
	usb_deinit();

	fs_ll_mount();

	emulation_paused = 0;
	tamalib_set_exec_mode(emulation_paused ? EXEC_MODE_PAUSE : EXEC_MODE_RUN);

	/* Enable auto-power-off if needed */
	if (!rom_loaded) {
		job_schedule(&autooff_job, &autooff_job_fn, time_get() + MS_TO_MCU_TIME(AUTOOFF_PERIOD));
	}
}

static void power_on(void)
{
	power_off_mode = 0;

	/* Just a reset for now */
	system_reset();
}

static void power_on_charge_only(void)
{
	/* Just enable the display */
#if defined(BOARD_HAS_SSD1306)
	ssd1306_set_power_mode(PWR_MODE_ON);
#elif defined(BOARD_HAS_UC1701X)
	uc1701x_set_power_mode(PWR_MODE_ON);
#endif

	/* And battery measurement */
	job_schedule(&battery_job, &battery_job_fn, JOB_ASAP);
}

static void power_off(void)
{
	/* Disable everything so that the device goes to Stop mode */
	if (!power_off_mode) {
		please_wait_screen();

		/* Save the current configuration */
		config_save(&config);

		if (config.autosave_enabled) {
			/* Save the current state and disable autosave */
			state_save(AUTOSAVE_SLOT);
			job_cancel(&autosave_job);
		}

		emulation_paused = 1;
		tamalib_set_exec_mode(emulation_paused ? EXEC_MODE_PAUSE : EXEC_MODE_RUN);

		fs_ll_umount();

		if (usb_enabled) {
			disable_usb();
		}

		config.speaker_enabled = 0;

		config.led_enabled = 0;
		update_led();

		backlight_set(0);
		is_backlight_on = 0;

		job_cancel(&render_job);
		job_cancel(&cpu_job);
	}

	/* Turn OFF backlight */
	backlight_set(0);
	is_backlight_on = 0;
	job_cancel(&backlight_job);

	job_cancel(&battery_job);

#if defined(BOARD_HAS_SSD1306)
	ssd1306_set_power_mode(PWR_MODE_SLEEP);
#elif defined(BOARD_HAS_UC1701X)
	uc1701x_set_power_mode(PWR_MODE_SLEEP);
#endif

	power_off_mode = 1;

	if (is_vbus) {
		/* Enter charge only mode */
		power_on_charge_only();
	}
}

static char * menu_toggle_arg(uint8_t v)
{
	switch (v) {
		case 0:
			return "[ ]";

		default:
		case 1:
			return "[*]";
	}
}

static void menu_screen_mode(uint8_t pos, menu_parent_t *parent)
{
	config.lcd_inverted = !config.lcd_inverted;
#if defined(BOARD_HAS_SSD1306)
	ssd1306_set_display_mode(config.lcd_inverted ? DISP_MODE_INVERTED : DISP_MODE_NORMAL);
#elif defined(BOARD_HAS_UC1701X)
	uc1701x_set_display_mode(config.lcd_inverted ? DISP_MODE_INVERTED : DISP_MODE_NORMAL);
#endif
}

static char * menu_screen_mode_arg(uint8_t pos, menu_parent_t *parent)
{
	switch (config.lcd_inverted) {
		case 0:
			return "Norm.";

		default:
		case 1:
			return "Inv.";
	}
}

static void menu_backlight_inc(uint8_t pos, menu_parent_t *parent)
{
	if (config.backlight_level < 16) {
		config.backlight_level++;
		turn_on_backlight(1);
	}
}

static void menu_backlight_dec(uint8_t pos, menu_parent_t *parent)
{
	if (config.backlight_level > 0) {
		config.backlight_level--;
		turn_on_backlight(1);
	}
}

static char * menu_backlight_arg(uint8_t pos, menu_parent_t *parent)
{
	static char str[] = "00";

	str[0] = '0' + config.backlight_level/10;
	str[1] = '0' + config.backlight_level % 10;

	return str;
}

static void menu_backlight_mode(uint8_t pos, menu_parent_t *parent)
{
	config.backlight_always_on = !config.backlight_always_on;

	turn_on_backlight(0);

	if (config.backlight_always_on) {
		job_cancel(&backlight_job);
	}
}

static char * menu_backlight_mode_arg(uint8_t pos, menu_parent_t *parent)
{
	switch (config.backlight_always_on) {
		case 0:
			return "Dynamic";

		default:
		case 1:
			return "Always ON";
	}
}

static void menu_sound(uint8_t pos, menu_parent_t *parent)
{
	config.speaker_enabled = !config.speaker_enabled;
}

static char * menu_sound_arg(uint8_t pos, menu_parent_t *parent)
{
	return menu_toggle_arg(config.speaker_enabled);
}

static void menu_led(uint8_t pos, menu_parent_t *parent)
{
	config.led_enabled = !config.led_enabled;
	update_led();
}

static char * menu_led_arg(uint8_t pos, menu_parent_t *parent)
{
	return menu_toggle_arg(config.led_enabled);
}

static void menu_battery(uint8_t pos, menu_parent_t *parent)
{
	config.battery_enabled = !config.battery_enabled;
}

static char * menu_battery_arg(uint8_t pos, menu_parent_t *parent)
{
	return menu_toggle_arg(config.battery_enabled);
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
			return "[Max]";

		default:
		case 1:
			return " [x1]";
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
			return "Running";

		default:
		case 1:
			return "Paused";
	}
}

static char * menu_vbat_arg(uint8_t pos, menu_parent_t *parent)
{
	static char str[] = "0.00 V";

	str[0] = '0' + current_battery/1000;
	str[2] = '0' + (current_battery/100) % 10;
	str[3] = '0' + (current_battery/10) % 10;

	return str;
}

static void menu_power_off(uint8_t pos, menu_parent_t *parent)
{
	power_off();
	menu_close();
}

static void menu_reset_cpu(uint8_t pos, menu_parent_t *parent)
{
	cpu_reset();
	menu_close();
}

static void menu_factory_reset(uint8_t pos, menu_parent_t *parent)
{
	please_wait_screen();

	fs_ll_umount();
	storage_erase();
	system_reset();
}

static void menu_firmware_update(uint8_t pos, menu_parent_t *parent)
{
	/* Save the current configuration */
	please_wait_screen();
	config_save(&config);

	system_dfu_reset();
}

static void menu_reset_device(uint8_t pos, menu_parent_t *parent)
{
	/* Save the current configuration */
	please_wait_screen();
	config_save(&config);

	system_reset();
}

static void menu_slots(uint8_t pos, menu_parent_t *parent)
{
	please_wait_screen();

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
			return " ";

		case 1:
			return "*";
	}
}

static void menu_clear_states(uint8_t pos, menu_parent_t *parent)
{
	uint8_t i;

	please_wait_screen();

	for (i = 0; i < STATE_SLOTS_NUM; i++) {
		state_erase(i);
	}
}

static void menu_autosave(uint8_t pos, menu_parent_t *parent)
{
	config.autosave_enabled = !config.autosave_enabled;

	if (config.autosave_enabled) {
		job_schedule(&autosave_job, &autosave_job_fn, time_get() + MS_TO_MCU_TIME(AUTOSAVE_PERIOD));
	} else {
		job_cancel(&autosave_job);
	}
}

static char * menu_autosave_arg(uint8_t pos, menu_parent_t *parent)
{
	return menu_toggle_arg(config.autosave_enabled);
}

static void menu_roms(uint8_t pos, menu_parent_t *parent)
{
	please_wait_screen();

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
			return " ";

		case 1:
			return "*";
	}
}

static void menu_usb(uint8_t pos, menu_parent_t *parent)
{
	if (is_vbus) {
		enable_usb();
		menu_close();
	}
}

static menu_item_t backlight_menu[] = {
	{"", &menu_backlight_arg, NULL, 0, NULL},
	{"+", NULL, &menu_backlight_inc, 0, NULL},
	{"-", NULL, &menu_backlight_dec, 0, NULL},
	{"", &menu_backlight_mode_arg, &menu_backlight_mode, 0, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t interface_menu[] = {
	{"Screen ", &menu_screen_mode_arg, &menu_screen_mode, 0, NULL},
#ifdef BOARD_HAS_UC1701X
	{"Backlight", NULL, NULL, 0, backlight_menu},
#endif
	{"Sound    ", &menu_sound_arg, &menu_sound, 0, NULL},
	{"RGB LED  ", &menu_led_arg, &menu_led, 0, NULL},
	{"Battery  ", &menu_battery_arg, &menu_battery, 0, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t emulation_menu[] = {
	{"Speed  ", &menu_toggle_speed_arg, &menu_toggle_speed, 0, NULL},
	{"", &menu_pause_arg, &menu_pause, 0, NULL},
	{"Reset CPU", NULL, &menu_reset_cpu, 1, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t system_menu[] = {
	{"Batt. ", &menu_vbat_arg, NULL, 0, NULL},
	{"FW. "FIRMWARE_VERSION, NULL, NULL, 0, NULL},
	{"FW. Update", NULL, &menu_firmware_update, 1, NULL},
	{"Power OFF", NULL, &menu_power_off, 1, NULL},
	{"Reset", NULL, &menu_reset_device, 1, NULL},
	{"Fact. Reset", NULL, &menu_factory_reset, 1, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t slots_menu[] = {
	{"Autosave   ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 1     ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 2     ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 3     ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 4     ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 5     ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 6     ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 7     ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 8     ", &menu_slots_arg, &menu_slots, 1, NULL},
	{"Slot 9     ", &menu_slots_arg, &menu_slots, 1, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t states_menu[] = {
	{"Load", NULL, NULL, 0, slots_menu},
	{"Save", NULL, NULL, 0, slots_menu},
	{"Clear", NULL, NULL, 0, slots_menu},
	{"Clear All", NULL, &menu_clear_states, 1, NULL},
	{"Autosave ", &menu_autosave_arg, &menu_autosave, 0, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t roms_menu[] = {
	{"ROM 0      ", &menu_roms_arg, &menu_roms, 1, NULL},
	{"ROM 1      ", &menu_roms_arg, &menu_roms, 1, NULL},
	{"ROM 2      ", &menu_roms_arg, &menu_roms, 1, NULL},
	{"ROM 3      ", &menu_roms_arg, &menu_roms, 1, NULL},

	{NULL, NULL, NULL, 0, NULL},
};

static menu_item_t main_menu[] = {
	{"States", NULL, NULL, 0, states_menu},
	{"ROMs", NULL, NULL, 0, roms_menu},
	{"Interface", NULL, NULL, 0, interface_menu},
	{"Emulation", NULL, NULL, 0, emulation_menu},
	{"USB Mode", NULL, &menu_usb, 0, NULL},
	{"System", NULL, NULL, 0, system_menu},

	{NULL, NULL, NULL, 0, NULL},
};

static void ll_init(void)
{
	system_init();

	board_init();

	time_init();

	led_init();

	backlight_init();

	speaker_init();

	battery_init();

#if defined(BOARD_HAS_SSD1306)
	ssd1306_init();
	ssd1306_set_power_mode(PWR_MODE_ON);
	ssd1306_set_display_mode(DISP_MODE_NORMAL);

	gfx_register_display(&ssd1306_send_data);
#elif defined(BOARD_HAS_UC1701X)
	uc1701x_init();
	uc1701x_set_power_mode(PWR_MODE_ON);
	uc1701x_set_display_mode(DISP_MODE_NORMAL);

	gfx_register_display(&uc1701x_send_data);
#endif

	/* Wait a little bit to make sure all I/Os are stable */
	time_delay(MS_TO_MCU_TIME(10));

	input_init();

	board_init_irq();
}

static void autooff_job_fn(job_t *job)
{
	power_off();
}

static void autosave_job_fn(job_t *job)
{
	job_schedule(&autosave_job, &autosave_job_fn, time_get() + MS_TO_MCU_TIME(AUTOSAVE_PERIOD));

	autosaving_screen();

	/* Save to autosave slot */
	state_save(AUTOSAVE_SLOT);

	if (menu_is_visible()) {
		menu_draw();
	}
}

static void render_job_fn(job_t *job)
{
	job_schedule(&render_job, &render_job_fn, time_get() + MS_TO_MCU_TIME(1000)/FRAMERATE);

	if (menu_is_visible()) {
		return;
	}

	gfx_clear();

	if (!rom_loaded) {
		no_rom_screen();
	} else {
		tamalib_screen();
	}

	if (usb_enabled) {
		gfx_string(USBON_STR, USBON_X, USBON_Y, 1, COLOR_ON_BLACK, BACKGROUND_ON);
	} else if (emulation_paused) {
		gfx_string(PAUSED_STR, PAUSED_X, PAUSED_Y, 1, COLOR_ON_BLACK, BACKGROUND_ON);
	}

	/* Battery */
	if (config.battery_enabled || current_battery < BATTERY_LOW || is_vbus) {
		draw_battery_full(BATTERY_ON_X, BATTERY_ON_Y);
	}

	gfx_print_screen();
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

static void battery_job_fn(job_t *job)
{
	job_schedule(&battery_job, &battery_job_fn, time_get() + MS_TO_MCU_TIME(BATTERY_JOB_PERIOD));

	battery_start_meas();
}

static void battery_cb(uint16_t v)
{
	/* The battery voltage is allowed to rise only if the device is charging */
	if (v < current_battery || is_charging) {
		current_battery = v;
	}

	if (power_off_mode) {
		/* The device is "OFF", update the battery icon */
		gfx_clear();

		draw_battery_full(BATTERY_OFF_X, BATTERY_OFF_Y);

		gfx_print_screen();
	}

	if (current_battery <= BATTERY_MIN && !power_off_mode) {
		/* Battery is critical */
		power_off();
	}
}

static void power_off_handler(input_t btn, input_state_t state, uint8_t long_press)
{
	if (is_vbus) {
		/* Charge only mode */
		user_feedback();
	}

	if (long_press && btn == INPUT_BTN_MIDDLE) {
		power_on();
	}
}

static void usb_mode_btn_handler(input_t btn, input_state_t state, uint8_t long_press)
{
	user_feedback();

	if (state == INPUT_STATE_HIGH && !long_press) {
		disable_usb();

		if (!rom_loaded) {
			/* Save the current configuration */
			please_wait_screen();
			config_save(&config);

			/* Try to load the ROM by resetting the device */
			system_reset();
		}
	}
}

static void menu_btn_handler(input_t btn, input_state_t state, uint8_t long_press)
{
	user_feedback();

	if (state == INPUT_STATE_HIGH) {
		switch (btn) {
			case INPUT_BTN_LEFT:
				menu_next();
				break;

			case INPUT_BTN_MIDDLE:
				menu_enter();
				break;

			case INPUT_BTN_RIGHT:
				menu_back();
				break;

			default:
				break;
		}
	}
}

static void default_btn_handler(input_t btn, input_state_t state, uint8_t long_press)
{
	user_feedback();

	if (long_press) {
		if (btn == INPUT_BTN_RIGHT) {
			menu_open();

			/* Make sure TamaLIB receives a release since it received a press */
			tamalib_set_button(btn, BTN_STATE_RELEASED);
		}
	} else {
		tamalib_set_button(btn, state);
	}
}

static void battery_charging_handler(input_state_t state)
{
	is_charging = (state == INPUT_STATE_LOW);

	/* The battery voltage has probably changed, let's update it */
	job_schedule(&battery_job, &battery_job_fn, JOB_ASAP);

	update_led();
}

static void vbus_sensing_handler(input_state_t state)
{
	user_feedback();

	is_vbus = (state == INPUT_STATE_HIGH);

	update_led();

	if (!is_vbus) {
		if (power_off_mode) {
			/* We were in charge only mode */
			power_off();
		} else if (usb_enabled) {
			disable_usb();
		}
	} else {
		if (power_off_mode) {
			/* Let's go in charge only mode */
			power_on_charge_only();
		} else if (!rom_loaded) {
			/* In no_rom mode, enable USB as soon as
			 * the device is connected to a computer
			 */
			enable_usb();
		}
	}
}

static void input_handler(input_t input, input_state_t state, uint8_t long_press)
{
	/* Dispatch the event */
	switch(input) {
		case INPUT_BTN_LEFT:
		case INPUT_BTN_MIDDLE:
		case INPUT_BTN_RIGHT:
			if (power_off_mode) {
				power_off_handler(input, state, long_press);
			} else if (usb_enabled) {
				usb_mode_btn_handler(input, state, long_press);
			} else if (menu_is_visible()) {
				menu_btn_handler(input, state, long_press);
			} else {
				default_btn_handler(input, state, long_press);
			}
			break;

		case INPUT_BATTERY_CHARGING:
			battery_charging_handler(state);
			break;

		case INPUT_VBUS_SENSING:
			vbus_sensing_handler(state);
			break;
	}
}

static void states_init(void)
{
#if defined(BOARD_HAS_SSD1306)
	ssd1306_set_display_mode(config.lcd_inverted ? DISP_MODE_INVERTED : DISP_MODE_NORMAL);
#elif defined(BOARD_HAS_UC1701X)
	uc1701x_set_display_mode(config.lcd_inverted ? DISP_MODE_INVERTED : DISP_MODE_NORMAL);
#endif

	turn_on_backlight(1);

	is_calling = icon_buffer[7]; // No need for update_led() since that call will be made by the following handlers

	battery_charging_handler(input_get_state(INPUT_BATTERY_CHARGING));

	vbus_sensing_handler(input_get_state(INPUT_VBUS_SENSING));
}

int main(void)
{
	ll_init();

	/* Make sure the RGB LED is off */
	led_set(0, 0, 0);

	/* Clear any remaining data in RAM */
	gfx_clear();
	gfx_print_screen();

	please_wait_screen();

	fs_ll_init();
	fs_ll_mount();

	tamalib_register_hal(&hal);

	/* Try to load the configuration */
	if (config_load(&config) < 0) {
		config_save(&config);
	}

	/* Try to load the default ROM from the filesystem if it is not loaded */
	if (!rom_is_loaded() && rom_load(DEFAULT_ROM_SLOT) < 0) {
		job_schedule(&autooff_job, &autooff_job_fn, time_get() + MS_TO_MCU_TIME(AUTOOFF_PERIOD));

		rom_loaded = 0;
	} else {
		/* TamaLIB must use an integer time base of at least 32768 Hz,
		 * so shift the one provided by the MCU until it fits.
		 */
		while (((MCU_TIME_FREQ_X1000 << time_shift) < TAMALIB_FREQ * 1000) || (MCU_TIME_FREQ_X1000 << time_shift) % 1000) {
			time_shift++;
		}

		if (tamalib_init((const u12_t *) g_program, NULL, (MCU_TIME_FREQ_X1000 << time_shift)/1000)) {
			system_fatal_error();
		}

		if (config.autosave_enabled) {
			/* Try to load the autosave slot and schedule the next autosave */
			state_load(AUTOSAVE_SLOT);
			job_schedule(&autosave_job, &autosave_job_fn, time_get() + MS_TO_MCU_TIME(AUTOSAVE_PERIOD));
		}

		job_schedule(&cpu_job, &cpu_job_fn, JOB_ASAP);
	}

	states_init();

	input_register_handler(&input_handler);

	battery_register_cb(&battery_cb);

	menu_register(main_menu);

	job_schedule(&render_job, &render_job_fn, JOB_ASAP);
	job_schedule(&battery_job, &battery_job_fn, JOB_ASAP);

	job_mainloop();

	tamalib_release();

	fs_ll_umount();
}
