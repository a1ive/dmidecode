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

#ifndef WINDMI_H
#define WINDMI_H

#include <stdint.h>
#include <sys/types.h>

#pragma pack(push, 1)

struct smbios_ieps
{
	uint8_t anchor[5]; /* "_DMI_" */
	uint8_t checksum;
	uint16_t table_length;
	uint32_t table_address;
	uint16_t structures;
	uint8_t revision;
};

struct smbios_eps
{
	uint8_t anchor[4]; /* "_SM_" */
	uint8_t checksum;
	uint8_t length; /* 0x1f */
	uint8_t version_major;
	uint8_t version_minor;
	uint16_t maximum_structure_size;
	uint8_t revision;
	uint8_t formatted[5];
	struct smbios_ieps intermediate;
};

struct smbios_eps3
{
	uint8_t anchor[5]; /* "_SM3_" */
	uint8_t checksum;
	uint8_t length; /* 0x18 */
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t docrev;
	uint8_t revision;
	uint8_t reserved;
	uint32_t maximum_table_length;
	uint64_t table_address;
};

#pragma pack(pop)

struct raw_smbios_data
{
	uint8_t used_20_calling_method;
	uint8_t major_ver;
	uint8_t minor_ver;
	uint8_t dmi_rev;
	uint32_t length;
	uint8_t data[];
};

struct raw_smbios_data* dmi_get_smbios(void);

void* dmi_construct_entry(off_t base, size_t* max_len);

void* dmi_construct_table(off_t base, size_t* max_len);

#endif // WINDMI_H
