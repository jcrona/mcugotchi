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

#include "stm32_hal.h"

#include "board.h"
#include "system.h"
#include "job.h"
#include "time.h"
#include "gpio.h"
#include "battery.h"

#define ADC_DATA_SIZE					32

#define ADC_V_FULL_SCALE				3300 // mV
#define ADC_REG_FULL_SCALE				4095 // 12bits

static uint16_t adc_data[ADC_DATA_SIZE];

static ADC_HandleTypeDef AdcHandle;
static DMA_HandleTypeDef DmaHandle;

static void (*battery_cb)(uint16_t) = NULL;

static job_t battery_processing_job;

static uint8_t measurement_ongoing = 0;

static uint8_t state_lock = 0;


void battery_init(void)
{
}

#ifdef BOARD_VBATT_ANA_ADC_CHANNEL
static void adc_init(void)
{
	ADC_ChannelConfTypeDef sConfig;

	/* Initialize DMA */
	__HAL_RCC_DMA1_CLK_ENABLE();

	DmaHandle.Instance                 = DMA1_Channel1;
	DmaHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	DmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
	DmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
	DmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	DmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
	DmaHandle.Init.Mode                = DMA_NORMAL;
	DmaHandle.Init.Priority            = DMA_PRIORITY_MEDIUM;
	DmaHandle.Init.Request             = DMA_REQUEST_0;
	HAL_DMA_Init(&DmaHandle);

	/* Associate the DMA handle */
	__HAL_LINKDMA(&AdcHandle, DMA_Handle, DmaHandle);

	/* NVIC configuration for DMA Input data interrupt */
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	/* Initialize ADC */
	__HAL_RCC_ADC1_CLK_ENABLE();

	AdcHandle.Instance                   = ADC1;
	AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;      /* Synchronous clock mode, input ADC clock with prscaler 2 */
	AdcHandle.Init.Resolution            = ADC_RESOLUTION_12B;            /* 12-bit resolution for converted data */
	AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;           /* Right-alignment for converted data */
	AdcHandle.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;    /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
	AdcHandle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;           /* EOC flag picked-up to indicate conversion end */
	AdcHandle.Init.LowPowerAutoPowerOff  = DISABLE;
	AdcHandle.Init.LowPowerFrequencyMode = DISABLE;
	AdcHandle.Init.LowPowerAutoWait      = DISABLE;                       /* Auto-delayed conversion feature disabled */
	AdcHandle.Init.ContinuousConvMode    = ENABLE;                        /* Continuous mode enabled (automatic conversion restart after each conversion) */
	AdcHandle.Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
	AdcHandle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
	AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE; /* Parameter discarded because software trigger chosen */
	AdcHandle.Init.DMAContinuousRequests = DISABLE;                       /* ADC DMA one shot request to match with DMA normal mode */
	AdcHandle.Init.Overrun               = ADC_OVR_DATA_PRESERVED;        /* DR register is preserved in case of overrun */
	AdcHandle.Init.OversamplingMode      = DISABLE;                       /* No oversampling */
	AdcHandle.Init.SamplingTime          = ADC_SAMPLETIME_7CYCLES_5;

	/* Initialize ADC peripheral according to the passed parameters */
	if (HAL_ADC_Init(&AdcHandle) != HAL_OK) {
		system_fatal_error();
	}

	/* Calibration */
	if (HAL_ADCEx_Calibration_Start(&AdcHandle, ADC_SINGLE_ENDED) !=  HAL_OK) {
		system_fatal_error();
	}

	/* Channel configuration */
	sConfig.Channel      = BOARD_VBATT_ANA_ADC_CHANNEL;               /* Channel to be converted */
	sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) {
		system_fatal_error();
	}
}

static void adc_deinit(void)
{
	/* Disable ADC */
	HAL_ADC_DeInit(&AdcHandle);
	__HAL_RCC_ADC1_CLK_DISABLE();

	/* Disable DMA */
	HAL_DMA_DeInit(&DmaHandle);
	__HAL_RCC_DMA1_CLK_DISABLE();
}
#endif

void battery_register_cb(void (*cb)(uint16_t))
{
	battery_cb = cb;
}

void battery_start_meas(void)
{
#ifdef BOARD_VBATT_ANA_ADC_CHANNEL
	if (measurement_ongoing) {
		/* The ADC is already performing a conversion sequence, no need to start another one */
		return;
	}

	measurement_ongoing = 1;

	/* The ADC does not work in low-power modes, thus those modes are not allowed */
	system_lock_max_state(STATE_SLEEP_S1, &state_lock);

#ifdef BOARD_VBATT_MEAS_PIN
	gpio_set(BOARD_VBATT_MEAS_PORT, BOARD_VBATT_MEAS_PIN);
#endif

	adc_init();

	if (HAL_ADC_Start_DMA(&AdcHandle, (uint32_t *) adc_data, ADC_DATA_SIZE) != HAL_OK) {
		system_fatal_error();
	}
#endif
}

void battery_stop_meas(void)
{
#ifdef BOARD_VBATT_ANA_ADC_CHANNEL
	if (!measurement_ongoing) {
		/* The ADC is not running, nothing to do */
		return;
	}

	HAL_ADC_Stop_DMA(&AdcHandle);

	adc_deinit();

#ifdef BOARD_VBATT_MEAS_PIN
	gpio_clear(BOARD_VBATT_MEAS_PORT, BOARD_VBATT_MEAS_PIN);
#endif

	system_unlock_max_state(STATE_SLEEP_S1, &state_lock);
#endif

	measurement_ongoing = 0;
}

static void battery_processing_job_fn(job_t *job)
{
	uint32_t battery_v = 0;
	uint8_t i;

	/* Process the raw ADC values
	 * TODO: Use oversampling and thus disable DMA
	 */
	for (i = 0; i < ADC_DATA_SIZE; i++) {
		battery_v += adc_data[i];
	}

	battery_v = (battery_v * BOARD_VBATT_ANA_RATIO * ADC_V_FULL_SCALE)/(ADC_DATA_SIZE * ADC_REG_FULL_SCALE);

	if (battery_cb != NULL) {
		battery_cb(battery_v);
	}
}

void DMA1_Channel1_IRQHandler(void)
{
	/* We have all the data needed, stop measuring */
	if ((0U != (AdcHandle.DMA_Handle->DmaBaseAddress->ISR & (DMA_FLAG_TC1 << (AdcHandle.DMA_Handle->ChannelIndex & 0x1cU)))) && (0U != (AdcHandle.DMA_Handle->Instance->CCR & DMA_IT_TC))) {
		battery_stop_meas();
		job_schedule(&battery_processing_job, &battery_processing_job_fn, JOB_ASAP);
	}

	HAL_DMA_IRQHandler(AdcHandle.DMA_Handle);
}
