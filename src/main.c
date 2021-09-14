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
#include "stm32f0xx_hal.h"

#include "ssd1306.h"
#include "gfx.h"
#include "menu.h"
#include "job.h"
#include "time.h"
#include "storage.h"
#include "state.h"

#include "lib/tamalib.h"

#include "rom.h"

#define PIXEL_SIZE					3
#define ICON_SIZE					8
#define ICON_STRIDE_X					24
#define ICON_STRIDE_Y					56
#define ICON_OFFSET_X					24
#define ICON_OFFSET_Y					0
#define LCD_OFFET_X					16
#define LCD_OFFET_Y					8

#define FRAMERATE 					30

#define MAIN_JOB_PERIOD					1000 //us

#define BTN_NUM						3

#define DEBOUNCE_DURATION				100000 //us
#define LONG_PRESS_DURATION				1000000 //us

#define STORAGE_MAGIC					0x1A3A60C1
#define STORAGE_VERSION					0

#define STORAGE_MAGIC_OFFSET				0 // in words (sizeof(uint32_t))
#define STORAGE_VERSION_OFFSET				1 // in words (sizeof(uint32_t))

typedef struct {
	btn_state_t state;
	job_t debounce_job;
	job_t long_press_job;
	EXTI_HandleTypeDef handle;
	uint32_t exti_port;
	GPIO_TypeDef* port;
	uint16_t pin;
} btn_data_t;

static bool_t matrix_buffer[LCD_HEIGHT][LCD_WIDTH] = {{0}};
static bool_t icon_buffer[ICON_NUM] = {0};

static bool_t tamalib_is_late = 0;

static btn_data_t buttons[BTN_NUM] = {
	{
		.state = BTN_STATE_RELEASED,
	},
	{
		.state = BTN_STATE_RELEASED,
	},
	{
		.state = BTN_STATE_RELEASED,
	},
};

static job_t cpu_job;
static job_t render_job;

static bool_t lcd_inverted = 0;
static uint8_t speed_ratio = 1;
static bool_t emulation_paused = 0;

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
	return (timestamp_t) time_get();
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

static void storage_init(void)
{
	uint32_t tmp;

	storage_erase();

	/* Write magic */
	tmp = STORAGE_MAGIC;
	storage_write(STORAGE_MAGIC_OFFSET, &tmp, 1);

	/* Write version */
	tmp = STORAGE_VERSION;
	storage_write(STORAGE_VERSION_OFFSET, &tmp, 1);
}

static void storage_sanity_check(void)
{
	uint32_t tmp;

	/* Check the magic of the storage */
	storage_read(STORAGE_MAGIC_OFFSET, &tmp, 1);
	if (tmp != STORAGE_MAGIC) {
		/* Re-init storage */
		storage_init();
		return;
	}

	/* Check the version of the storage */
	storage_read(STORAGE_VERSION_OFFSET, &tmp, 1);
	if (tmp != STORAGE_VERSION) {
		/* Re-init storage (TODO: Migration instead) */
		storage_init();
	}
}

static void menu_screen_mode(uint8_t pos, menu_parent_t *parent)
{
	lcd_inverted = !lcd_inverted;
	LCDScreenMode(lcd_inverted ? LCDInv : LCDNorm);
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

static void menu_reset(uint8_t pos, menu_parent_t *parent)
{
	cpu_reset();
	menu_close();
}

static void menu_slots(uint8_t pos, menu_parent_t *parent)
{
	if (parent->pos == 0) {
		/* Load */
		state_load(pos + 1);
		menu_close();
	} else if (parent->pos == 1) {
		/* Save */
		state_save(pos + 1);
		menu_close();
	} else {
		/* Clear */
		state_erase(pos + 1);
	}
}

static char * menu_slots_arg(uint8_t pos, menu_parent_t *parent)
{
	switch (state_check_if_used(pos + 1)) {
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
		state_erase(i + 1);
	}
}

static menu_t options_menu[] = {
	{"Screen ", &menu_screen_mode_arg, &menu_screen_mode, NULL},
	{"Speed ", &menu_toggle_speed_arg, &menu_toggle_speed, NULL},
	{"", &menu_pause_arg, &menu_pause, NULL},
	{"Reset", NULL, &menu_reset, NULL},

	{NULL, NULL, NULL},
};

static menu_t slots_menu[] = {
	{"Slot 1", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 2", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 3", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 4", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 5", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 6", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 7", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 8", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 9", &menu_slots_arg, &menu_slots, NULL},
	{"Slot 10", &menu_slots_arg, &menu_slots, NULL},

	{NULL, NULL, NULL},
};

static menu_t states_menu[] = {
	{"Load", NULL, NULL, slots_menu},
	{"Save", NULL, NULL, slots_menu},
	{"Clear", NULL, NULL, slots_menu},
	{"Clear All", NULL, &menu_clear_states, NULL},

	{NULL, NULL, NULL},
};

static menu_t main_menu[] = {
	{"Options", NULL, NULL, options_menu},
	{"States", NULL, NULL, states_menu},

	{NULL, NULL, NULL},
};

void fatal_error(void)
{
	while(1) __asm__ __volatile__ ("nop");
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
	return HAL_OK;
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSI48)
  *            SYSCLK(Hz)                     = 48000000
  *            HCLK(Hz)                       = 48000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            HSI Frequency(Hz)              = 48000000
  *            PREDIV                         = 2
  *            PLLMUL                         = 2
  *            Flash Latency(WS)              = 1
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Select HSI48 Oscillator as PLL source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI48;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK) {
		fatal_error();
	}

	/* Select PLL as system clock source and configure the HCLK and PCLK1 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1)!= HAL_OK) {
		fatal_error();
	}
}

static void config_int_line(EXTI_HandleTypeDef *h, uint32_t port, uint8_t trigger)
{
	EXTI_ConfigTypeDef e;

	e.Line = h->Line;
	e.Mode = EXTI_MODE_INTERRUPT;
	e.Trigger = trigger;
	e.GPIOSel = port;
	HAL_EXTI_SetConfigLine(h, &e);
}

static void board_init(void)
{
	GPIO_InitTypeDef g;

	HAL_Init();

	SystemClock_Config();

	time_init();

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Left and right buttons */
	g.Pin  = GPIO_PIN_2 | GPIO_PIN_3;
	g.Mode  = GPIO_MODE_IT_RISING;
	g.Pull  = GPIO_PULLDOWN;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &g);

	buttons[BTN_RIGHT].handle.Line = EXTI_LINE_2;
	buttons[BTN_RIGHT].exti_port = EXTI_GPIOB;
	buttons[BTN_RIGHT].port = GPIOB;
	buttons[BTN_RIGHT].pin = GPIO_PIN_2;
	buttons[BTN_LEFT].handle.Line = EXTI_LINE_3;
	buttons[BTN_LEFT].exti_port = EXTI_GPIOB;
	buttons[BTN_LEFT].port = GPIOB;
	buttons[BTN_LEFT].pin = GPIO_PIN_3;

	HAL_NVIC_SetPriority(EXTI2_3_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

	/* Middle button (user button) */
	g.Pin  = GPIO_PIN_0;
	g.Mode  = GPIO_MODE_IT_RISING;
	g.Pull  = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &g);

	buttons[BTN_MIDDLE].handle.Line = EXTI_LINE_0;
	buttons[BTN_MIDDLE].exti_port = EXTI_GPIOA;
	buttons[BTN_MIDDLE].port = GPIOA;
	buttons[BTN_MIDDLE].pin = GPIO_PIN_0;

	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

	SSD1306_InitSetup();
	LCDSleepMode(LCDWake);
	LCDScreenMode(lcd_inverted ? LCDInv : LCDNorm);

	storage_sanity_check();
}

static void render_job_fn(job_t *job)
{
	job_schedule(&render_job, &render_job_fn, time_get() + 1000000/FRAMERATE);

	hal_update_screen();
}

static void cpu_job_fn(job_t *job)
{
	job_t *next_job;

	job_schedule(&cpu_job, &cpu_job_fn, time_get() + MAIN_JOB_PERIOD);

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

int main(void)
{
	board_init();

	menu_register(main_menu);

	tamalib_register_hal(&hal);

	if (tamalib_init(g_program, NULL, 1000000)) {
		fatal_error();
	}

	job_schedule(&render_job, &render_job_fn, JOB_ASAP);
	job_schedule(&cpu_job, &cpu_job_fn, JOB_ASAP);

	job_mainloop();

	tamalib_release();
}

static void btn_handler(button_t btn, btn_state_t state, bool_t long_press)
{
	if (long_press) {
		if (btn == BTN_RIGHT && !menu_is_visible()) {
			menu_open();
		}
	} else if (state == BTN_STATE_PRESSED) {
		if (menu_is_visible()) {
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

	if (!menu_is_visible()) {
		tamalib_set_button(btn, buttons[btn].state);
	}
}

static btn_state_t get_btn_hw_state(button_t btn)
{
	return (HAL_GPIO_ReadPin(buttons[btn].port, buttons[btn].pin) == GPIO_PIN_SET) ? BTN_STATE_PRESSED : BTN_STATE_RELEASED;
}

static void btn_long_press_job_fn(job_t *job)
{
	button_t btn;

	/* Lookup the button */
	if (job == &(buttons[BTN_LEFT].long_press_job)) {
		btn = BTN_LEFT;
	} else if (job == &(buttons[BTN_MIDDLE].long_press_job)) {
		btn = BTN_MIDDLE;
	} else if (job == &(buttons[BTN_RIGHT].long_press_job)) {
		btn = BTN_RIGHT;
	} else {
		return;
	}

	btn_handler(btn, buttons[btn].state, 1);
}

static void btn_debounce_job_fn(job_t *job)
{
	button_t btn;

	/* Lookup the button */
	if (job == &(buttons[BTN_LEFT].debounce_job)) {
		btn = BTN_LEFT;
	} else if (job == &(buttons[BTN_MIDDLE].debounce_job)) {
		btn = BTN_MIDDLE;
	} else if (job == &(buttons[BTN_RIGHT].debounce_job)) {
		btn = BTN_RIGHT;
	} else {
		return;
	}

	if (buttons[btn].state == BTN_STATE_RELEASED && get_btn_hw_state(btn) == BTN_STATE_PRESSED) {
		job_schedule(&(buttons[btn].long_press_job), &btn_long_press_job_fn, time_get() + LONG_PRESS_DURATION);
		config_int_line(&(buttons[btn].handle), buttons[btn].exti_port, EXTI_TRIGGER_FALLING);
	} else if (buttons[btn].state == BTN_STATE_PRESSED && get_btn_hw_state(btn) == BTN_STATE_RELEASED) {
		job_cancel(&(buttons[btn].long_press_job));
		config_int_line(&(buttons[btn].handle), buttons[btn].exti_port, EXTI_TRIGGER_RISING);
	} else {
		/* The button has been pressed or released during debounce, make sure we handle it properly */
		job_schedule(&(buttons[btn].debounce_job), &btn_debounce_job_fn, JOB_ASAP);
	}

	buttons[btn].state = !buttons[btn].state;
	btn_handler(btn, buttons[btn].state, 0);
}

static void btn_irq_handler(button_t btn)
{
	if (HAL_EXTI_GetPending(&(buttons[btn].handle), EXTI_TRIGGER_RISING_FALLING)) {
		HAL_EXTI_ClearPending(&(buttons[btn].handle), EXTI_TRIGGER_RISING_FALLING);

		job_schedule(&(buttons[btn].debounce_job), &btn_debounce_job_fn, time_get() + DEBOUNCE_DURATION);
	}
}

void EXTI0_1_IRQHandler(void)
{
	btn_irq_handler(BTN_MIDDLE);
}

void EXTI2_3_IRQHandler(void)
{
	btn_irq_handler(BTN_LEFT);
	btn_irq_handler(BTN_RIGHT);
}
