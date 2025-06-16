/*
 * This file is part of the dmidecode-win32 project.
 *
 *   Copyright (C) 2025 Gen Li <ligenlive@gmail.com>
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
 */

#include "sys/socket.h"

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "wincompact.h"

const char*
my_inet_ntop(int af, const void* restrict a0, char* restrict s, socklen_t l)
{
	const unsigned char* a = a0;
	int i, j, max, best;
	char buf[100];

	switch (af)
	{
	case AF_INET:
		if (snprintf(s, l, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]) < l)
			return s;
		break;
	case AF_INET6:
		if (memcmp(a, "\0\0\0\0\0\0\0\0\0\0\377\377", 12))
			snprintf(buf, sizeof buf,
				"%x:%x:%x:%x:%x:%x:%x:%x",
				256 * a[0] + a[1], 256 * a[2] + a[3],
				256 * a[4] + a[5], 256 * a[6] + a[7],
				256 * a[8] + a[9], 256 * a[10] + a[11],
				256 * a[12] + a[13], 256 * a[14] + a[15]);
		else
			snprintf(buf, sizeof buf,
				"%x:%x:%x:%x:%x:%x:%d.%d.%d.%d",
				256 * a[0] + a[1], 256 * a[2] + a[3],
				256 * a[4] + a[5], 256 * a[6] + a[7],
				256 * a[8] + a[9], 256 * a[10] + a[11],
				a[12], a[13], a[14], a[15]);
		/* Replace longest /(^0|:)[:0]{2,}/ with "::" */
		for (i = best = 0, max = 2; buf[i]; i++)
		{
			if (i && buf[i] != ':')
				continue;
			j = (int)strspn(buf + i, ":0");
			/* The leading sequence of zeros (best==0) is
			 * disadvantaged compared to sequences elsewhere
			 * as it doesn't have a leading colon. One extra
			 * character is required for another sequence to
			 * beat it fairly. */
			if (j > max + (best == 0))
				best = i, max = j;
		}
		if (max > 3)
		{
			buf[best] = buf[best + 1] = ':';
			memmove(buf + best + 2, buf + best + max, i - best - max + 1);
		}
		if (strlen(buf) < (size_t)l)
		{
			strcpy(s, buf);
			return s;
		}
		break;
	default:
		errno = EAFNOSUPPORT;
		return 0;
	}
	errno = ENOSPC;
	return 0;
}
