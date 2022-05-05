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

#include "ff_gen_drv.h"

#include "lib/tamalib.h"
#include "state.h"

#define STATE_SLOT_SIZE					821 // in bytes

#define STATE_FILE_MAGIC				"TLST"
#define STATE_FILE_VERSION				2

static uint8_t state_buf[STATE_SLOT_SIZE];

static char state_file_name[] = "saveX.bin";


void state_save(uint8_t slot)
{
	FIL f;
	UINT num;
	state_t *state;
	uint8_t *ptr = state_buf;
	uint32_t i;

	if (slot >= STATE_SLOTS_NUM) {
		return;
	}

	state = tamalib_get_state();

	/* First the magic, then the version, and finally the fields of
	 * the state_t struct written as u8, u16 little-endian or u32
	 * little-endian following the struct order
	 */
	ptr[0] = (uint8_t) STATE_FILE_MAGIC[0];
	ptr[1] = (uint8_t) STATE_FILE_MAGIC[1];
	ptr[2] = (uint8_t) STATE_FILE_MAGIC[2];
	ptr[3] = (uint8_t) STATE_FILE_MAGIC[3];
	ptr += 4;

	ptr[0] = STATE_FILE_VERSION & 0xFF;
	ptr += 1;

	ptr[0] = *(state->pc) & 0xFF;
	ptr[1] = (*(state->pc) >> 8) & 0x1F;
	ptr += 2;

	ptr[0] = *(state->x) & 0xFF;
	ptr[1] = (*(state->x) >> 8) & 0xF;
	ptr += 2;

	ptr[0] = *(state->y) & 0xFF;
	ptr[1] = (*(state->y) >> 8) & 0xF;
	ptr += 2;

	ptr[0] = *(state->a) & 0xF;
	ptr += 1;

	ptr[0] = *(state->b) & 0xF;
	ptr += 1;

	ptr[0] = *(state->np) & 0x1F;
	ptr += 1;

	ptr[0] = *(state->sp) & 0xFF;
	ptr += 1;

	ptr[0] = *(state->flags) & 0xF;
	ptr += 1;

	ptr[0] = *(state->tick_counter) & 0xFF;
	ptr[1] = (*(state->tick_counter) >> 8) & 0xFF;
	ptr[2] = (*(state->tick_counter) >> 16) & 0xFF;
	ptr[3] = (*(state->tick_counter) >> 24) & 0xFF;
	ptr += 4;

	ptr[0] = *(state->clk_timer_timestamp) & 0xFF;
	ptr[1] = (*(state->clk_timer_timestamp) >> 8) & 0xFF;
	ptr[2] = (*(state->clk_timer_timestamp) >> 16) & 0xFF;
	ptr[3] = (*(state->clk_timer_timestamp) >> 24) & 0xFF;
	ptr += 4;

	ptr[0] = *(state->prog_timer_timestamp) & 0xFF;
	ptr[1] = (*(state->prog_timer_timestamp) >> 8) & 0xFF;
	ptr[2] = (*(state->prog_timer_timestamp) >> 16) & 0xFF;
	ptr[3] = (*(state->prog_timer_timestamp) >> 24) & 0xFF;
	ptr += 4;

	ptr[0] = *(state->prog_timer_enabled) & 0x1;
	ptr += 1;

	ptr[0] = *(state->prog_timer_data) & 0xFF;
	ptr += 1;

	ptr[0] = *(state->prog_timer_rld) & 0xFF;
	ptr += 1;

	ptr[0] = *(state->call_depth) & 0xFF;
	ptr[1] = (*(state->call_depth) >> 8) & 0xFF;
	ptr[2] = (*(state->call_depth) >> 16) & 0xFF;
	ptr[3] = (*(state->call_depth) >> 24) & 0xFF;
	ptr += 4;

	for (i = 0; i < INT_SLOT_NUM; i++) {
		ptr[0] = state->interrupts[i].factor_flag_reg & 0xF;
		ptr += 1;

		ptr[0] = state->interrupts[i].mask_reg & 0xF;
		ptr += 1;

		ptr[0] = state->interrupts[i].triggered & 0x1;
		ptr += 1;
	}

	/* First 640 half bytes correspond to the RAM */
	for (i = 0; i < MEM_RAM_SIZE; i++) {
		ptr[i] = GET_RAM_MEMORY(state->memory, i + MEM_RAM_ADDR) & 0xF;
	}
	ptr += MEM_RAM_SIZE;

	/* I/Os are from 0xF00 to 0xF7F */
	for (i = 0; i < MEM_IO_SIZE; i++) {
		ptr[i] = GET_RAM_MEMORY(state->memory, i + MEM_IO_ADDR) & 0xF;
	}
	ptr += MEM_IO_SIZE;

	state_file_name[4] = slot + '0';

	if (f_open(&f, state_file_name, FA_CREATE_ALWAYS | FA_WRITE)) {
		/* Error */
		return;
	}

        if (f_write(&f, state_buf, sizeof(state_buf), &num) || (num < sizeof(state_buf))) {
		/* Error */
		f_close(&f);
		return;
	}

	f_close(&f);
}

void state_load(uint8_t slot)
{
	FIL f;
	UINT num;
	state_t *state;
	uint8_t *ptr = state_buf;
	uint32_t i;

	if (slot >= STATE_SLOTS_NUM) {
		return;
	}

	state = tamalib_get_state();

	state_file_name[4] = slot + '0';

	if (f_open(&f, state_file_name, FA_OPEN_EXISTING | FA_READ)) {
		/* Error */
		return;
	}

	if (f_read(&f, state_buf, sizeof(state_buf), &num) || (num < sizeof(state_buf))) {
		/* Error */
		f_close(&f);
		return;
	}

	f_close(&f);

	/* First the magic, then the version, and finally the fields of
	 * the state_t struct written as u8, u16 little-endian or u32
	 * little-endian following the struct order
	 */
	if (ptr[0] != (uint8_t) STATE_FILE_MAGIC[0] || ptr[1] != (uint8_t) STATE_FILE_MAGIC[1] ||
		ptr[2] != (uint8_t) STATE_FILE_MAGIC[2] || ptr[3] != (uint8_t) STATE_FILE_MAGIC[3]) {
		return;
	}
	ptr += 4;

	if (ptr[0] != STATE_FILE_VERSION) {
		/* TODO: Handle migration at a point */
		return;
	}
	ptr += 1;

	*(state->pc) = ptr[0] | ((ptr[1] & 0x1F) << 8);
	ptr += 2;

	*(state->x) = ptr[0] | ((ptr[1] & 0xF) << 8);
	ptr += 2;

	*(state->y) = ptr[0] | ((ptr[1] & 0xF) << 8);
	ptr += 2;

	*(state->a) = ptr[0] & 0xF;
	ptr += 1;

	*(state->b) = ptr[0] & 0xF;
	ptr += 1;

	*(state->np) = ptr[0] & 0x1F;
	ptr += 1;

	*(state->sp) = ptr[0];
	ptr += 1;

	*(state->flags) = ptr[0] & 0xF;
	ptr += 1;

	*(state->tick_counter) = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
	ptr += 4;

	*(state->clk_timer_timestamp) = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
	ptr += 4;

	*(state->prog_timer_timestamp) = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
	ptr += 4;

	*(state->prog_timer_enabled) = ptr[0] & 0x1;
	ptr += 1;

	*(state->prog_timer_data) = ptr[0];
	ptr += 1;

	*(state->prog_timer_rld) = ptr[0];
	ptr += 1;

	*(state->call_depth) = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
	ptr += 4;

	for (i = 0; i < INT_SLOT_NUM; i++) {
		state->interrupts[i].factor_flag_reg = ptr[0] & 0xF;
		ptr += 1;

		state->interrupts[i].mask_reg = ptr[0] & 0xF;
		ptr += 1;

		state->interrupts[i].triggered = ptr[0] & 0x1;
		ptr += 1;
	}

	/* First 640 half bytes correspond to the RAM */
	for (i = 0; i < MEM_RAM_SIZE; i++) {
		SET_RAM_MEMORY(state->memory, i + MEM_RAM_ADDR, ptr[i] & 0xF);
	}
	ptr += MEM_RAM_SIZE;

	/* I/Os are from 0xF00 to 0xF7F */
	for (i = 0; i < MEM_IO_SIZE; i++) {
		SET_RAM_MEMORY(state->memory, i + MEM_IO_ADDR, ptr[i] & 0xF);
	}
	ptr += MEM_IO_SIZE;

	tamalib_refresh_hw();
}

void state_erase(uint8_t slot)
{
	if (slot >= STATE_SLOTS_NUM) {
		return;
	}

	state_file_name[4] = slot + '0';

	f_unlink(state_file_name);
}

uint8_t state_stat(uint8_t slot)
{
	if (slot >= STATE_SLOTS_NUM) {
		return 0;
	}

	state_file_name[4] = slot + '0';

	/* Check if the slot is used */
	return (f_stat(state_file_name, NULL) == FR_OK);
}
