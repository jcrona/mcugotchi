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

#include "dfu.h"

/* The flag that will be kept across reboots */
volatile __attribute__((used, section(".bss_noinit"))) uint32_t dfu_flag;


void dfu_jump(void)
{
	uint32_t vtor = 0;

	/* Disable the SysTick if any */
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	/* Remap 0x0 to System Memory */
	MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE, SYSCFG_CFGR1_MEM_MODE_0);

	//SCB->VTOR = 0;

	/* Sync */
	__DSB();
	__ISB();

	/* Set the Stack Pointer and jump */
	asm volatile (
		/* Update SP (offset 0 of the vector table) */
		"ldr     r1, [%0];"
		"mov     sp, r1;"
		/* Jump to the entry point (offset 1 of the vector table) */
		"ldr     r0, [%0, #4];"
		"bx      r0;"
		:: "r" (vtor));
}
