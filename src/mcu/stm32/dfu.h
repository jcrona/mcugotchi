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
#ifndef _DFU_H_
#define _DFU_H_

#include <stdint.h>

#define DFU_FLAG_MAGIC				0x12345678

#define SET_DFU_FLAG()				(dfu_flag = DFU_FLAG_MAGIC)
#define CLEAR_DFU_FLAG()			(dfu_flag = 0)
#define IS_DFU_FLAG_SET()			(dfu_flag == DFU_FLAG_MAGIC)

extern volatile uint32_t dfu_flag;


void dfu_jump(void);

#endif /* _DFU_H_ */
