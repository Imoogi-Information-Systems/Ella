/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2014  <Imoogi Information Systems>
 * 
 * ella is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ella is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "ella.h"
#include "paths.h"

int main(int argc, char **argv)
{
	me = getpid();
	
	if (argc == 3) {
		/* Expected commands: ella create <id> or ella reset <id> */
		if (strcasecmp(argv[1], "create") == 0) {
			/* Create a new counter */
			return add_new_count(atoi(argv[2]));
		} else if (strcasecmp(argv[1], "reset") == 0) {
			/* Open the counter file exclusively */
			open_counter(atoi(argv[2]));
			/* Reset existing counter */
			return reset_counter();
		} else {
			print_usage();
			return 1;
		}
	} else if (argc == 2) {
		/* Expected command: ella <id> or ella --help or ella -h */
		if (strcasecmp(argv[1], "--help") == 0 || strcasecmp(argv[1], "-h") == 0) {
			/* Show the usage instructions */
			print_usage();
			return 0;
		} else {
			/* Open the counter file exclusively */
			open_counter(atoi(argv[1]));
			/* Increment the counter */
			long long unsigned int counter = increment_counter();
			if (counter > 0) {
				printf("%lld\n", counter);
				return 0;
			} else {
				printf("Error reading/writing counter.\n");
				return 1;
			}
		}
	} else {
		/* Unknown command, print usage help */
		print_usage();
		return 1;
	}
}

/*
 * Opens the file that holds our current counter. May sleep if we can't get a lock.
 * To emulate atomic operation we change the file name so others can't use it.
 * @param count the count we wish to increment (0 for the default)
 * @return a file pointer to the opened file
 */
void
open_counter(int count)
{
	int renaming_result;
	/* Set the path buffers to hold the full paths to the counter files we are 
	 * interested in */
	set_path_buffer(count);
	/* To lock it to our exclusive use, we try to rename the file to a name
	 * containing our unique (at runtime) process id */
	renaming_result = rename(path_buffer, path_buffer_locked);
	while (renaming_result == -1) {
		/* CAUTION: This could result in an infinte loop if the original file isn't there */
		nanosleep (&interval, NULL); /* Sleep for 20 nano-seconds... */
		/*... And try again */
		renaming_result = rename(path_buffer, path_buffer_locked);
	}

	/* If we are here, that means that we got the lock. Now open the file for reading and writing */
	counter_file = fopen(path_buffer_locked, "rb+");
}

/*
 * Closes the counter file and renames it back to its original name
 */
static void
close_counter()
{
	if (counter_file != NULL) {
		fclose (counter_file);
		counter_file = NULL;
	}
	rename(path_buffer_locked, path_buffer);
}

/*
 * Print errors to the stderr output stream 
 */
static void
error (char *message)
{
	char *red_color = "\x1b[31;1m";
	char *reset_color = "\x1b[0m";

	fprintf(stderr, "Ella Error: %s%s%s\n", red_color, message, reset_color);
}

/*
 * Reads the current count, increments it and returns the new id
 * @return the counter, already incremented
 */
long long unsigned int
increment_counter()
{
	long long unsigned count = -1;
	int done;
	/* Make sure the counter file is successfully open */
	if (counter_file == NULL) {
		/* Error, release the lock and return -1 */
		error("Unable to open file for reading/writing");
		close_counter();
		return -1;
	}
	
	/* Read the current count */
	done = fread(&count, sizeof(count), 1, counter_file);
	if (done < 1) {
		/* Error, close and return -1 */
		error("Read error");
		close_counter();
		return -1;
	}
	
	/* Increment the count */
	count++;
	/* Move the write cursor to the start of the file */
	fseek (counter_file, 0L, SEEK_SET);
	/* Write the current count into the file */
	done = fwrite (&count, sizeof(count), 1, counter_file);
	if (done < 1) {
		/* Write error, close file and return -1 */
		error("Write error");
		close_counter();
		return -1;
	}
	/* Now close the file and remove the lock */
	close_counter();
	/* Return the incremented count */
	return count;
}

/*
 * Creates a new counter file
 * @param id id for the new count
 * @return 0 on success, 1 on error
 */
int
add_new_count(int id)
{
	int done;
	/* Set the file name into the buffers */
	set_path_buffer(id);
	
	/* Create new file for writing */
	counter_file = fopen(path_buffer, "wb");
	if (counter_file == NULL) {
		/* Error opening the file for writing */
		error("Error opening file for writing");
		return 1;
	}
	
	/* Now "reset" the counter to zero */
	done = reset_counter();
	
	if (done == 0) {
		return 0; /* Success */
	} else {
		return 1; /* Error */
	}
}

/*
 * Resets the counter of the opened counter file.
 * @return 0 on success, 1 on error
 */
int
reset_counter()
{
	long long unsigned count = 0; /* Set the counter to zero */
	int done;
	/* Make sure the counter file is successfully open */
	if (counter_file == NULL) {
		/* Error, release the lock and return -1 */
		error("Error opening the file for reading/writing");
		close_counter();
		return 1; /* Error */
	}

	/* Write the new counter to file */
	done = fwrite (&count, sizeof(count), 1, counter_file);

	/* Now close the file and remove the lock */
	close_counter();
	if (done == 1) {
		return 0; /* Success */
	} else {
		error("Write error");
		return 1; /* Error */
	}
}

/*
 * Prints the usage help, when the command line arguments are wrong
 */
void
print_usage()
{
	char *color_cyan = "\x1b[36m";
	char *color_cyan_bold = "\x1b[36;1m";
	char *reset_color = "\x1b[0m";
	/* Print vanity header */
	printf ("%sElla%s - %sUnique, incremented id issuing utility%s\n\n", color_cyan_bold, reset_color, color_cyan, reset_color);
	/* Print command summary */
	printf ("%sAvailable commands:%s\n\tella -h\n\tella --help\tShow this help message.\n\tella create <id>\t"
	        "Creates a new counter file with the name <id>.\n\tella reset <id>\tResets the counter for counter file <id>\n"
	        "\tella <id>\tIssues a new id from counter file <id>\n",
	        color_cyan_bold, reset_color);
	/* Creating and resetting */
	printf("\n%sCreating and resetting counter files:%s\n\tA counter file is where the current count is saved on the disk. "
	       "You must create at least one counter file before you can use Ella to issue ids.\n"
		   "\n\tReseting a counter file brings the count back to zero, so the next id issues is 1 again.",
	       color_cyan_bold, reset_color);
	/* Getting the counter value */
	printf("\n%sIssuing a new id:%s\n\tCall Ella with the id of the count file you want to increment. Remember to first create at least one count file.\n\n",
	       color_cyan_bold, reset_color);
}

/*
 * Sets the path buffer with the full path to the counter file
 */
void
set_path_buffer(int counter)
{
	sprintf(path_buffer, "%s%d", PATH, counter);
	sprintf(path_buffer_locked, "%s%d.%d", PATH, counter, me); /* Me holds the current pid */
}
