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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

#include "wincompact.h"

int my_open(const char* pathname, int flags, ...)
{
	int mode = 0;
	//fprintf(stdout, "open: %s\n", pathname);
	if (flags & O_CREAT)
	{
		va_list args;
		va_start(args, flags);
		va_arg(args, int); /* Consume the mode argument from the stack */
		va_end(args);

		/* Hardcode the Windows permission to read-write */
		mode = _S_IREAD | _S_IWRITE;
	}

	/*
	 * Call the underlying Windows _open function.
	 * Always add O_BINARY to ensure binary mode.
	 * The win_pmode will be correctly set if O_CREAT was present,
	 * or it will be 0 and ignored otherwise.
	 */
	return _open(pathname, flags | O_BINARY, mode);
}
