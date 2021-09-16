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
#include "stm32f0xx_hal.h"

#include "lib/tamalib.h"
#include "storage.h"
#include "state.h"

#define STORAGE_SLOTS_OFFSET				64 // in words (sizeof(uint32_t))

#define STATE_SLOT_SIZE					256 // in words (sizeof(uint32_t))

static uint32_t state_buf[STATE_SLOT_SIZE];


void state_save(uint8_t slot)
{
	state_t *state;
	uint32_t slot_offset;
	uint32_t pos = 0;
	uint32_t i;

	if (slot >= STATE_SLOTS_NUM) {
		return;
	}

	/* Find the right offset for that slot */
	slot_offset = STORAGE_SLOTS_OFFSET + (slot - 1) * STATE_SLOT_SIZE;

	state = tamalib_get_state();

	/* Set the slot as used */
	state_buf[pos++] = 1;

	/* All fields are written following the struct order */
	state_buf[pos++] = *(state->pc) & 0x1FFF;
	state_buf[pos++] = *(state->x) & 0xFFF;
	state_buf[pos++] = *(state->y) & 0xFFF;
	state_buf[pos++] = *(state->a) & 0xF;
	state_buf[pos++] = *(state->b) & 0xF;
	state_buf[pos++] = *(state->np) & 0x1F;
	state_buf[pos++] = *(state->sp) & 0xFF;
	state_buf[pos++] = *(state->flags) & 0xF;
	state_buf[pos++] = *(state->tick_counter);
	state_buf[pos++] = *(state->clk_timer_timestamp);
	state_buf[pos++] = *(state->prog_timer_timestamp);
	state_buf[pos++] = *(state->prog_timer_enabled) & 0x1;
	state_buf[pos++] = *(state->prog_timer_data) & 0xFF;
	state_buf[pos++] = *(state->prog_timer_rld) & 0xFF;
	state_buf[pos++] = *(state->call_depth);

	for (i = 0; i < INT_SLOT_NUM; i++) {
		state_buf[pos++] = state->interrupts[i].factor_flag_reg & 0xF;
		state_buf[pos++] = state->interrupts[i].mask_reg & 0xF;
		state_buf[pos++] = state->interrupts[i].triggered & 0x1;
	}

	/* First 640 half bytes correspond to the RAM */
	for (i = 0; i < 640; i++) {
		((uint8_t *) &state_buf[pos])[i] = state->memory[i];
	}
	pos += (640 + sizeof(uint32_t) - 1)/sizeof(uint32_t);

	/* I/Os are from 0xF00 to 0xF7F */
	for (i = 0; i < 160; i++) {
		((uint8_t *) &state_buf[pos])[i] = state->memory[i + 0xF00];
	}
	pos += (160 + sizeof(uint32_t) - 1)/sizeof(uint32_t);

	storage_write(slot_offset, state_buf, STATE_SLOT_SIZE);
}

void state_load(uint8_t slot)
{
	state_t *state;
	uint32_t slot_offset;
	uint32_t pos = 0;
	uint32_t i;

	if (slot >= STATE_SLOTS_NUM) {
		return;
	}

	/* Find the right offset for that slot */
	slot_offset = STORAGE_SLOTS_OFFSET + (slot - 1) * STATE_SLOT_SIZE;

	storage_read(slot_offset, state_buf, STATE_SLOT_SIZE);

	state = tamalib_get_state();

	/* Check is the slot is used */
	if (!state_buf[pos++]) {
		return;
	}

	/* All fields are written following the struct order */
	*(state->pc) = state_buf[pos++] & 0x1FFF;
	*(state->x) = state_buf[pos++] & 0xFFF;
	*(state->y) = state_buf[pos++] & 0xFFF;
	*(state->a) = state_buf[pos++] & 0xF;
	*(state->b) = state_buf[pos++] & 0xF;
	*(state->np) = state_buf[pos++] & 0x1F;
	*(state->sp) = state_buf[pos++] & 0xFF;
	*(state->flags) = state_buf[pos++] & 0xF;
	*(state->tick_counter) = state_buf[pos++];
	*(state->clk_timer_timestamp) = state_buf[pos++];
	*(state->prog_timer_timestamp) = state_buf[pos++];
	*(state->prog_timer_enabled) = state_buf[pos++] & 0x1;
	*(state->prog_timer_data) = state_buf[pos++] & 0xFF;
	*(state->prog_timer_rld) = state_buf[pos++] & 0xFF;
	*(state->call_depth) = state_buf[pos++];

	for (i = 0; i < INT_SLOT_NUM; i++) {
		state->interrupts[i].factor_flag_reg = state_buf[pos++] & 0xF;
		state->interrupts[i].mask_reg = state_buf[pos++] & 0xF;
		state->interrupts[i].triggered = state_buf[pos++] & 0x1;
	}

	/* First 640 half bytes correspond to the RAM */
	for (i = 0; i < 640; i++) {
		state->memory[i] = ((uint8_t *) &state_buf[pos])[i];
	}
	pos += (640 + sizeof(uint32_t) - 1)/sizeof(uint32_t);

	/* I/Os are from 0xF00 to 0xF7F */
	for (i = 0; i < 160; i++) {
		state->memory[i + 0xF00] = ((uint8_t *) &state_buf[pos])[i];
	}
	pos += (160 + sizeof(uint32_t) - 1)/sizeof(uint32_t);

	tamalib_refresh_hw();
}

void state_erase(uint8_t slot)
{
	uint32_t slot_offset;
	uint32_t i;

	if (slot >= STATE_SLOTS_NUM) {
		return;
	}

	/* Find the right offset for that slot */
	slot_offset = STORAGE_SLOTS_OFFSET + (slot - 1) * STATE_SLOT_SIZE;

	storage_read(slot_offset, state_buf, STATE_SLOT_SIZE);

	/* Check is the slot is used */
	if (!state_buf[0]) {
		return;
	}

	for (i = 0; i < STATE_SLOT_SIZE; i++) {
		state_buf[i] = 0;
	}

	storage_write(slot_offset, state_buf, STATE_SLOT_SIZE);
}

uint8_t state_check_if_used(uint8_t slot)
{
	uint32_t slot_offset;

	/* Find the right offset for that slot */
	slot_offset = STORAGE_SLOTS_OFFSET + (slot - 1) * STATE_SLOT_SIZE;

	storage_read(slot_offset, state_buf, STATE_SLOT_SIZE);

	/* Check is the slot is used */
	return state_buf[0];
}
