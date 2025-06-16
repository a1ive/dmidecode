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

#include <ctype.h>
#include <stdint.h>

int
strcasecmp(const char* s1, const char* s2)
{
	while (*s1 && *s2)
	{
		if (tolower((uint8_t)*s1) != tolower((uint8_t)*s2))
			break;
		s1++;
		s2++;
	}
	return (int)tolower((uint8_t)*s1) - (int)tolower((uint8_t)*s2);
}

int
strncasecmp(const char* s1, const char* s2, size_t n)
{
	if (n == 0)
		return 0;

	while (*s1 && *s2 && --n)
	{
		if (tolower((uint8_t)*s1) != tolower((uint8_t)*s2))
			break;
		s1++;
		s2++;
	}
	return (int)tolower((uint8_t)*s1) - (int)tolower((uint8_t)*s2);
}
