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
#ifndef _JOB_H_
#define _JOB_H_

#include "time.h"

#define JOB_ASAP				0

typedef struct job {
	mcu_time_t time;
	void (*cb)(struct job *);
	struct job *next;
} job_t;


void job_schedule(job_t *job, void (*cb)(job_t *), mcu_time_t time);
void job_cancel(job_t *job);

job_t * job_get_next(void);

void job_mainloop(void);

#endif /* _JOB_H_ */
