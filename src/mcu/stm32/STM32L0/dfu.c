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
	/* Disable the SysTick if any */
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	/* Remap 0x0 to System Memory */
	MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE, SYSCFG_CFGR1_MEM_MODE_0);

	SCB->VTOR = 0;

	/* WA: Invalidate the beginning of the code in flash to make
	 * sure the bootloader will not jump back to flash
	 */
	WRITE_REG(FLASH->PEKEYR, 0x89ABCDEFU); // FLASH_PEKEY1
	WRITE_REG(FLASH->PEKEYR, 0x02030405U); // FLASH_PEKEY2
	WRITE_REG(FLASH->PRGKEYR, 0x8C9DAEBFU); // FLASH_PRGKEY1
	WRITE_REG(FLASH->PRGKEYR, 0x13141516U); // FLASH_PRGKEY2
	/* Set the ERASE bit */
	SET_BIT(FLASH->PECR, FLASH_PECR_ERASE);
	/* Set PROG bit */
	SET_BIT(FLASH->PECR, FLASH_PECR_PROG);
	/* Write 00000000h to the first word of the program page to erase */
	*(__IO uint32_t *)(uint32_t)(0x08000000 & ~(128U - 1)) = 0x00000000;
	/* Wait for last operation to be completed */
	while (((FLASH->SR) & (FLASH_SR_BSY)) == (FLASH_SR_BSY));
	/* If the erase operation is completed, disable the ERASE Bit */
	CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);
	CLEAR_BIT(FLASH->PECR, FLASH_PECR_ERASE);
	/* Set the PRGLOCK Bit to lock the FLASH Registers access */
	SET_BIT(FLASH->PECR, FLASH_PECR_PRGLOCK);
	/* Set the PELOCK Bit to lock the PECR Register access */
	SET_BIT(FLASH->PECR, FLASH_PECR_PELOCK);

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
		:: "r" (SCB->VTOR));
}
