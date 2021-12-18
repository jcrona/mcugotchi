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
#include <stddef.h>

#include "time.h"
#include "system.h"
#include "job.h"

static job_t *jobs = NULL;


void job_schedule(job_t *job, void (*cb)(job_t *), mcu_time_t time)
{
	job_t **j = &jobs;

	/* Disable IRQs handling */
	system_disable_irq();

	/* Remove the job if it is already in the queue */
	job_cancel(job);

	job->cb = cb;
	job->time = time;

	while (*j != NULL && (int32_t) (time - (*j)->time) >= 0) {
		j = &((*j)->next);
	}

	job->next = *j;
	*j = job;

	/* Enable IRQs handling */
	system_enable_irq();
}

void job_cancel(job_t *job)
{
	job_t **j = &jobs;

	/* Disable IRQs handling */
	system_disable_irq();

	while (*j != NULL) {
		if (*j == job) {
			*j = job->next;
			break;
		}

		j = &((*j)->next);
	}

	/* Enable IRQs handling */
	system_enable_irq();
}

job_t * job_get_next(void)
{
	return jobs;
}

void job_mainloop(void)
{
	job_t *j = NULL;
	exec_state_t state;

	while (1) {
		/* Disable IRQs handling */
		system_disable_irq();

		if (jobs != NULL) {
			if (jobs->time == JOB_ASAP) {
				state = STATE_RUN;
			} else {
				state = time_configure_wakeup(jobs->time);
			}
		} else {
			state = system_get_max_state();
		}

		if (state == STATE_RUN) {
			if (jobs != NULL) {
				j = jobs;
				jobs = j->next;
			}
		} else {
			system_enter_state(state);
		}

		/* Enable IRQs handling */
		system_enable_irq();

		if (j != NULL) {
			time_wait_until(j->time);
			j->cb(j);
			j = NULL;
		}
	}
}
