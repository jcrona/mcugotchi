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

volatile u32_t ticks = 0;

static bool_t matrix_buffer[LCD_HEIGTH][LCD_WIDTH] = {{0}};
static bool_t icon_buffer[ICON_NUM] = {0};

static btn_state_t left_state = BTN_STATE_RELEASED;
static btn_state_t middle_state = BTN_STATE_RELEASED;
static btn_state_t right_state = BTN_STATE_RELEASED;

static timestamp_t right_ts = 0;

static bool_t lcd_inverted = 0;

static EXTI_HandleTypeDef left_btn_handle, middle_btn_handle, right_btn_handle;

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
	return (timestamp_t) ticks;
}

static void hal_sleep_until(timestamp_t ts)
{
	while ((int32_t) (ts - ticks) > 0) {
		__asm__ __volatile__ ("nop");
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
	for (j = 0; j < LCD_HEIGTH; j++) {
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

static void menu_invert_screen(void)
{
	lcd_inverted = !lcd_inverted;
	LCDScreenMode(lcd_inverted ? LCDInv : LCDNorm);
}

static menu_item_t menu_items[] = {
	{"Inv. Screen", &menu_invert_screen, NULL},

	{NULL, NULL, NULL},
};

static menu_t menu = {
	.items = menu_items,
	.parent = NULL,
};

void SysTick_Handler(void)
{
	ticks += 100;
}

void fatal_error(void)
{
	while(1) __asm__ __volatile__ ("nop");
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

static void config_int_line(EXTI_HandleTypeDef *h, uint32_t line, uint32_t port, uint8_t trigger)
{
	EXTI_ConfigTypeDef e;

	e.Line = line;
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

	SysTick_Config(SystemCoreClock/10000); //100us timebase

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Left and right buttons */
	g.Pin  = GPIO_PIN_2 | GPIO_PIN_3;
	g.Mode  = GPIO_MODE_IT_RISING;
	g.Pull  = GPIO_PULLDOWN;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &g);

	right_btn_handle.Line = EXTI_LINE_2;
	left_btn_handle.Line = EXTI_LINE_3;

	HAL_NVIC_SetPriority(EXTI2_3_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

	/* Middle button (user button) */
	g.Pin  = GPIO_PIN_0;
	g.Mode  = GPIO_MODE_IT_RISING;
	g.Pull  = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &g);

	middle_btn_handle.Line = EXTI_LINE_0;

	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

	SSD1306_InitSetup();
	LCDSleepMode(LCDWake);
	LCDScreenMode(lcd_inverted ? LCDInv : LCDNorm);
}

int main(void)
{
	board_init();

	menu_register(&menu);

	tamalib_register_hal(&hal);

	if (tamalib_init(g_program, NULL, 1000000)) {
		fatal_error();
	}

	tamalib_mainloop();

	tamalib_release();
}

void EXTI0_1_IRQHandler(void)
{
	if (HAL_EXTI_GetPending(&middle_btn_handle, EXTI_TRIGGER_RISING_FALLING)) {
		/* Middle button */
		HAL_EXTI_ClearPending(&middle_btn_handle, EXTI_TRIGGER_RISING_FALLING);

		if (middle_state == BTN_STATE_RELEASED) {
			middle_state = BTN_STATE_PRESSED;
			config_int_line(&middle_btn_handle, EXTI_LINE_0, EXTI_GPIOA, EXTI_TRIGGER_FALLING);

			if (menu_is_visible()) {
				menu_enter();
			}
		} else {
			middle_state = BTN_STATE_RELEASED;
			config_int_line(&middle_btn_handle, EXTI_LINE_0, EXTI_GPIOA, EXTI_TRIGGER_RISING);
		}

		if (!menu_is_visible()) {
			tamalib_set_button(BTN_MIDDLE, middle_state);
		}
	}
}

void EXTI2_3_IRQHandler(void)
{
	if (HAL_EXTI_GetPending(&left_btn_handle, EXTI_TRIGGER_RISING_FALLING)) {
		/* Left button */
		HAL_EXTI_ClearPending(&left_btn_handle, EXTI_TRIGGER_RISING_FALLING);

		if (left_state == BTN_STATE_RELEASED) {
			left_state = BTN_STATE_PRESSED;
			config_int_line(&left_btn_handle, EXTI_LINE_3, EXTI_GPIOB, EXTI_TRIGGER_FALLING);

			if (menu_is_visible()) {
				menu_next();
			}
		} else {
			left_state = BTN_STATE_RELEASED;
			config_int_line(&left_btn_handle, EXTI_LINE_3, EXTI_GPIOB, EXTI_TRIGGER_RISING);
		}

		if (!menu_is_visible()) {
			tamalib_set_button(BTN_LEFT, left_state);
		}
	}

	if (HAL_EXTI_GetPending(&right_btn_handle, EXTI_TRIGGER_RISING_FALLING)) {
		/* Right button */
		HAL_EXTI_ClearPending(&right_btn_handle, EXTI_TRIGGER_RISING_FALLING);

		if (right_state == BTN_STATE_RELEASED) {
			right_ts = ticks;
			right_state = BTN_STATE_PRESSED;
			config_int_line(&right_btn_handle, EXTI_LINE_2, EXTI_GPIOB, EXTI_TRIGGER_FALLING);

			if (menu_is_visible()) {
				menu_back();
			}
		} else {
			right_ts = ticks - right_ts;
			right_state = BTN_STATE_RELEASED;
			config_int_line(&right_btn_handle, EXTI_LINE_2, EXTI_GPIOB, EXTI_TRIGGER_RISING);
		}

		if (!menu_is_visible()) {
			tamalib_set_button(BTN_RIGHT, right_state);

			if (right_state == BTN_STATE_RELEASED && right_ts >= 1000000) {
				menu_open();
			}
		}
	}
}
