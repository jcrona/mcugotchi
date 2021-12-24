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
#ifndef _TIME_H_
#define _TIME_H_

#include <stdint.h>

#include "system.h"
#include "mcu.h"

#define US_TO_MCU_TIME(t)				((t * MCU_TIME_FREQ_NUM + MCU_TIME_FREQ_DEN - 1)/MCU_TIME_FREQ_DEN)
#define MS_TO_MCU_TIME(t)				(US_TO_MCU_TIME(t * 1000ULL))

#define MCU_TIME_FREQ_X1000 				((1000000000ULL/MCU_TIME_FREQ_DEN) * MCU_TIME_FREQ_NUM)

typedef uint32_t mcu_time_t;


void time_init(void);

mcu_time_t time_get(void);
void time_wait_until(mcu_time_t time);
void time_delay(mcu_time_t time);

exec_state_t time_configure_wakeup(mcu_time_t time);

#endif /* _TIME_H_ */
