/*
 * Common "util" functions
 * This file is part of the dmidecode project.
 *
 *   Copyright (C) 2002-2023 Jean Delvare <jdelvare@suse.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *   For the avoidance of doubt the "preferred form" of this code is one which
 *   is in an open unpatent encumbered format. Where cryptographic key signing
 *   forms part of the process of creating an executable the information
 *   including keys needed to generate an equivalently functional executable
 *   are deemed to be part of the source code.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "../dmidecode/types.h"
#include "../dmidecode/util.h"

#include "windmi.h"

static int myread(FILE* fp, u8 *buf, size_t count, const char *prefix)
{
	ssize_t r = 1;
	size_t r2 = 0;

	while (r2 != count && r != 0)
	{
		r = fread(buf + r2, 1, count - r2, fp);
		if (r == 0)
		{
			if (ferror(fp))
			{
				perror(prefix);
				return -1;
			}
			break;  // EOF reached
		}
		else
			r2 += r;
	}

	if (r2 != count)
	{
		fprintf(stderr, "%s: Unexpected end of file\n", prefix);
		return -1;
	}

	return 0;
}

int checksum(const u8 *buf, size_t len)
{
	u8 sum = 0;
	size_t a;

	for (a = 0; a < len; a++)
		sum += buf[a];
	return (sum == 0);
}

/*
 * Reads all of file from given offset, up to max_len bytes.
 * A buffer of at most max_len bytes is allocated by this function, and
 * needs to be freed by the caller.
 * This provides a similar usage model to mem_chunk()
 *
 * Returns a pointer to the allocated buffer, or NULL on error, and
 * sets max_len to the length actually read.
 */
void *read_file(off_t base, size_t *max_len, const char *filename)
{
	struct _stat64 statbuf;
	FILE* fp;
	u8 *p = NULL;

	//fprintf(stdout, "read_file: %llx %zu %s\n", (unsigned long long)base, *max_len, filename);
	if (strcmp(filename, "/sys/firmware/dmi/tables/smbios_entry_point") == 0)
		return dmi_construct_entry(base, max_len);
	if (strcmp(filename, "/sys/firmware/dmi/tables/DMI") == 0)
		return dmi_construct_table(base, max_len);

	/*
	 * Don't print error message on missing file, as we will try to read
	 * files that may or may not be present.
	 */
	if ((fp = fopen(filename, "rb")) == NULL)
	{
		if (errno != ENOENT)
			perror(filename);
		return NULL;
	}

	/*
	 * Check file size, don't allocate more than can be read.
	 */
	if (_fstat64(_fileno(fp), &statbuf) == 0)
	{
		if (base >= statbuf.st_size)
		{
			fprintf(stderr, "%s: Can't read data beyond EOF\n",
				filename);
			p = NULL;
			goto out;
		}
		if (*max_len > (size_t)statbuf.st_size - base)
			*max_len = statbuf.st_size - base;
	}

	if ((p = malloc(*max_len)) == NULL)
	{
		perror("malloc");
		goto out;
	}

	if (_fseeki64(fp, base, SEEK_SET) == -1)
	{
		fprintf(stderr, "%s: ", filename);
		perror("_fseeki64");
		goto err_free;
	}

	if (myread(fp, p, *max_len, filename) == 0)
		goto out;

err_free:
	free(p);
	p = NULL;

out:
	if (fclose(fp) != 0)
		perror(filename);

	return p;
}

/*
 * Copy a physical memory chunk into a memory buffer.
 * This function allocates memory.
 */
void *mem_chunk(off_t base, size_t len, const char *devmem)
{
	// TODO: Read physical memory.
	//fprintf(stdout, "mem_chunk: %llx %zu %s\n", (unsigned long long)base, len, devmem);
	return NULL;
}
