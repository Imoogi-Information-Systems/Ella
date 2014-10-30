/*
 * ella.h
 *
 * Copyright (C) 2014 - Imoogi Information Systems
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ELLA_H_
#define _ELLA_H_

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "paths.h"

FILE *counter_file;
pid_t me;
char path_buffer[PATH_BUFFER];
char path_buffer_locked[PATH_BUFFER + 10];

/* The waiting interval structure for the sleepnano function */
struct timespec interval = { .tv_sec = 0, .tv_nsec = 20 };

void
open_counter(int count);

long long unsigned int
increment_counter();

int
add_new_count(int id);

int
reset_counter();

void
print_usage();

void
set_path_buffer(int count);


#endif