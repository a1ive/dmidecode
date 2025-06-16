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
#include <stdlib.h>

#include "windmi.h"

static UINT
get_fw_table(DWORD fw_table_sig, DWORD fw_table_id,
	PVOID buf, DWORD size)
{
	UINT(WINAPI * pfn_get_fw_table)
		(DWORD fw_table_sig, DWORD fw_table_id, PVOID buf, DWORD size) = NULL;
	HMODULE h_mod = GetModuleHandleW(L"kernel32");

	if (h_mod)
		*(FARPROC*)&pfn_get_fw_table = GetProcAddress(h_mod, "GetSystemFirmwareTable");

	if (pfn_get_fw_table)
		return pfn_get_fw_table(fw_table_sig, fw_table_id, buf, size);

	// TODO: Support NT5
#if 0
	if (fw_table_sig == 'RSMB')
		return NT5GetSmbios(buf, size);
#endif

	return 0;
}

struct raw_smbios_data* dmi_get_smbios(void)
{
	struct raw_smbios_data* smbios_data = NULL;
	DWORD smbios_size = 0;
	smbios_size = get_fw_table('RSMB', 0, NULL, 0);
	if (smbios_size == 0)
		return NULL;
	smbios_data = (struct raw_smbios_data*)malloc(smbios_size);
	if (!smbios_data)
		return NULL;
	smbios_size = get_fw_table('RSMB', 0, smbios_data, smbios_size);
	if (smbios_size == 0)
	{
		free(smbios_data);
		return NULL;
	}
	return smbios_data;
}

static inline uint8_t
eps_checksum(const void* data, size_t length)
{
	const uint8_t* ptr = (const uint8_t*)data;
	uint8_t sum = 0;

	for (size_t i = 0; i < length; ++i)
	{
		sum += ptr[i];
	}

	return (uint8_t)(0 - sum);
}

static void parse_smbios_table_metrics(const uint8_t* table_data, size_t table_size,
	uint16_t* out_max_struct_size, uint16_t* out_num_structs)
{
	// Initialize output values
	*out_max_struct_size = 0;
	*out_num_structs = 0;

	const uint8_t* p = table_data;
	const uint8_t* end = table_data + table_size;

	while (p + 4 <= end) // Ensure there's at least room for a header
	{
		// Read the standard header of the current structure
		uint8_t type = p[0];
		uint8_t formatted_length = p[1];

		// Find the end of the string section (double null-terminator)
		const uint8_t* q = p + formatted_length;
		while (q + 1 < end && (*q != 0 || *(q + 1) != 0))
		{
			q++;
		}

		// If we found the end-of-structure marker before the end of the table
		if (q + 1 < end)
		{
			size_t current_struct_size = (q + 2) - p;

			// Update metrics
			if (current_struct_size > *out_max_struct_size) {
				*out_max_struct_size = (uint16_t)current_struct_size;
			}
			(*out_num_structs)++;

			// Move pointer to the next structure
			p = q + 2;
		}
		else
		{
			// Malformed table, stop parsing
			break;
		}
	}
}

void* dmi_construct_entry(off_t base, size_t* max_len)
{
	size_t entry_point_size = 0;

	struct raw_smbios_data* smbios_data = dmi_get_smbios();
	if (!smbios_data)
		return NULL;

	uint16_t max_struct_size = 0;
	uint16_t num_structs = 0;
	parse_smbios_table_metrics(smbios_data->data, smbios_data->length,
		&max_struct_size, &num_structs);

	union
	{
		struct smbios_ieps v1;
		struct smbios_eps v2;
		struct smbios_eps3 v3;
	} eps;

	memset(&eps, 0, sizeof(eps));
	if (smbios_data->major_ver >= 3)
	{
		memcpy(eps.v3.anchor, "_SM3_", 5);
		eps.v3.version_major = smbios_data->major_ver;
		eps.v3.version_minor = smbios_data->minor_ver;
		eps.v3.length = sizeof(struct smbios_eps3);
		eps.v3.docrev = 0; // FIXME
		eps.v3.revision = 1; // FIXME
		eps.v3.maximum_table_length = smbios_data->length;
		eps.v3.table_address = 0; // FIXME
		eps.v3.checksum = 0;
		eps.v3.checksum = eps_checksum(&eps.v3, sizeof(struct smbios_eps3));

		entry_point_size = sizeof(struct smbios_eps3);
	}
	else if (smbios_data->major_ver == 2)
	{
		memcpy(eps.v2.anchor, "_SM_", 4);
		eps.v2.version_major = smbios_data->major_ver;
		eps.v2.version_minor = smbios_data->minor_ver;
		eps.v2.revision = smbios_data->dmi_rev;
		eps.v2.length = sizeof(struct smbios_eps);
		eps.v2.maximum_structure_size = max_struct_size;
		
		memcpy(eps.v2.intermediate.anchor, "_DMI_", 5);
		eps.v2.intermediate.table_length = smbios_data->length;
		eps.v2.intermediate.table_address = 0; // FIXME
		eps.v2.intermediate.structures = num_structs;
		eps.v2.intermediate.revision = smbios_data->dmi_rev;
		eps.v2.intermediate.checksum = 0;
		eps.v2.intermediate.checksum = eps_checksum(&eps.v2.intermediate, sizeof(struct smbios_ieps));

		eps.v2.checksum = 0;
		eps.v2.checksum = eps_checksum(&eps.v2, sizeof(struct smbios_eps));

		entry_point_size = sizeof(struct smbios_eps);
	}
	else
	{
		memcpy(eps.v1.anchor, "_DMI_", 5);
		eps.v1.table_length = smbios_data->length;
		eps.v1.table_address = 0; // FIXME
		eps.v1.structures = num_structs;
		eps.v1.revision = smbios_data->dmi_rev;

		eps.v1.checksum = 0;
		eps.v1.checksum = eps_checksum(&eps.v1, sizeof(struct smbios_ieps));

		entry_point_size = sizeof(struct smbios_ieps);
	}

	free(smbios_data);
	if (base >= (off_t)entry_point_size)
	{
		*max_len = 0;
		return NULL;
	}

	size_t bytes_to_copy = entry_point_size - (size_t)base;
	bytes_to_copy = min(bytes_to_copy, *max_len);
	void* final_buffer = malloc(bytes_to_copy);
	if (!final_buffer)
	{
		perror("malloc");
		return NULL;
	}

	memcpy(final_buffer, (uint8_t*)&eps + base, bytes_to_copy);
	*max_len = bytes_to_copy;

	return final_buffer;
}

void* dmi_construct_table(off_t base, size_t* max_len)
{
	struct raw_smbios_data* smbios_data = dmi_get_smbios();
	if (!smbios_data)
		return NULL;

	uint8_t* table_data = smbios_data->data;
	size_t table_size = smbios_data->length;

	if (base >= (off_t)table_size)
	{
		free(smbios_data);
		*max_len = 0;
		return NULL;
	}

	size_t bytes_to_copy = table_size - (size_t)base;
	bytes_to_copy = min(bytes_to_copy, *max_len);

	void* final_buffer = malloc(bytes_to_copy);
	if (!final_buffer)
	{
		free(smbios_data);
		perror("malloc");
		return NULL;
	}

	memcpy(final_buffer, table_data + base, bytes_to_copy);
	*max_len = bytes_to_copy;

	free(smbios_data);
	return final_buffer;
}
