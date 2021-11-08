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
#ifndef _MCU_H_
#define _MCU_H_

/* MCU time frequency is 7,8125 kHz = ((1000000/MCU_TIME_FREQ_DEN) * MCU_TIME_FREQ_NUM) */
#define MCU_TIME_FREQ_NUM					1
#define MCU_TIME_FREQ_DEN					128

/* Storage related offsets and sizes */
#define STORAGE_BASE_ADDRESS					0x800D000

#define STORAGE_SIZE						0x13000
#define STORAGE_PAGE_SIZE					512 // 2KB in words (sizeof(uint32_t))

#define STORAGE_ROM_OFFSET					0x0
#define STORAGE_ROM_SIZE					0xC00 // 12KB in words (sizeof(uint32_t))

#define STORAGE_FS_OFFSET					0xC00
#define STORAGE_FS_SIZE						0x4000 // 64KB in words (sizeof(uint32_t))

#endif /* _MCU_H_ */
