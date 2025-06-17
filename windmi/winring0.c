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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include <winerror.h>
#include <pathcch.h>
#include "winring0.h"
#include "winring0_def.h"

#pragma comment(lib,"pathcch.lib")

struct wr0_drv_t
{
	LPCWSTR driver_name;
	LPCWSTR driver_id;
	LPCWSTR driver_obj;
	WCHAR driver_path[MAX_PATH];
	SC_HANDLE scManager;
	SC_HANDLE scDriver;
	HANDLE hhDriver;
	int errorcode;
};

static BOOL load_driver(struct wr0_drv_t* drv)
{
	BOOL Ret = FALSE;
	DWORD Status = 0;
	BOOL Retry = TRUE;

	drv->scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (drv->scManager == NULL)
		return FALSE;
retry:
	drv->scDriver = CreateServiceW(drv->scManager, drv->driver_id, drv->driver_id,
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
		drv->driver_path, NULL, NULL, NULL, NULL, NULL);
	if (drv->scDriver == NULL)
	{
		drv->scDriver = OpenServiceW(drv->scManager, drv->driver_id, SERVICE_ALL_ACCESS);
		if (drv->scDriver == NULL)
		{
			CloseServiceHandle(drv->scManager);
			return FALSE;
		}
	}

	Ret = StartServiceW(drv->scDriver, 0, NULL);
	if (Ret == FALSE)
	{
		Status = GetLastError();
		if (Status == ERROR_SERVICE_ALREADY_RUNNING)
			Ret = TRUE;
		else if (Retry == TRUE)
		{
			Retry = FALSE;
			DeleteService(drv->scDriver);
			CloseServiceHandle(drv->scDriver);
			goto retry;
		}
		else
			Ret = FALSE;
	}

	CloseServiceHandle(drv->scDriver);
	CloseServiceHandle(drv->scManager);

	return Ret;
}

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
static BOOL is_x64(void)
{
#ifdef _WIN64
	return TRUE;
#else
	BOOL bIsWow64 = FALSE;
	HMODULE hMod = GetModuleHandleW(L"kernel32");
	LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
	if (hMod)
		fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(hMod, "IsWow64Process");
	if (fnIsWow64Process)
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	return bIsWow64;
#endif
}

static BOOL find_driver(struct wr0_drv_t* driver, LPCWSTR name, LPCWSTR id, LPCWSTR obj)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;

	GetModuleFileNameW(NULL, driver->driver_path, MAX_PATH);

	PathCchRemoveFileSpec(driver->driver_path, MAX_PATH);
	PathCchAppend(driver->driver_path, MAX_PATH, name);
	hFile = CreateFileW(driver->driver_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		driver->driver_id = id;
		driver->driver_name = name;
		driver->driver_obj = obj;
		CloseHandle(hFile);
		return TRUE;
	}

	ZeroMemory(driver->driver_path, sizeof(driver->driver_path));
	return FALSE;
}

struct wr0_drv_t* wr0_driver_open_real(LPCWSTR name, LPCWSTR id, LPCWSTR obj)
{
	struct wr0_drv_t* drv;
	BOOL status = FALSE;

	drv = (struct wr0_drv_t*)malloc(sizeof(struct wr0_drv_t));
	if (!drv)
		return NULL;
	ZeroMemory(drv, sizeof(struct wr0_drv_t));

	if (!find_driver(drv, name, id, obj))
		goto fail;
	status = load_driver(drv);
	if (status)
	{
		drv->hhDriver = CreateFileW(drv->driver_obj,
			GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		if (drv->hhDriver == INVALID_HANDLE_VALUE)
			status = FALSE;
	}

	if (!status)
		goto fail;
	return drv;
fail:
	free(drv);
	return NULL;
}

struct wr0_drv_t* wr0_driver_open(void)
{
	struct wr0_drv_t* drv = NULL;
	if (is_x64())
	{
		drv = wr0_driver_open_real(OLS_DRIVER_NAME_X64, OLS_DRIVER_ID, OLS_DRIVER_OBJ);
		if (drv)
			return drv;
		drv = wr0_driver_open_real(OLS_DRIVER_NAME_WIN7_X64, OLS_DRIVER_ID, OLS_DRIVER_OBJ);
		if (drv)
			return drv;
		drv = wr0_driver_open_real(OLS_ALT_DRIVER_NAME_X64, OLS_ALT_DRIVER_ID, OLS_ALT_DRIVER_OBJ);
	}
	else
	{
		drv = wr0_driver_open_real(OLS_DRIVER_NAME, OLS_DRIVER_ID, OLS_DRIVER_OBJ);
		if (drv)
			return drv;
		drv = wr0_driver_open_real(OLS_ALT_DRIVER_NAME, OLS_ALT_DRIVER_ID, OLS_ALT_DRIVER_OBJ);
	}
	return drv;
}

unsigned long
wr0_read_phys_mem(struct wr0_drv_t* drv,
	unsigned long long phys, void* buffer, unsigned long len)
{
	if (!drv || !drv->hhDriver || drv->hhDriver == INVALID_HANDLE_VALUE || !buffer)
		return 0;

	DWORD	returnedLength = 0;
	BOOL	result = FALSE;
	DWORD	size = 0;
	OLS_READ_MEMORY_INPUT inBuf;
	DWORD_PTR address = (DWORD_PTR)phys;

	if (sizeof(DWORD_PTR) == 4)
	{
		inBuf.Address.HighPart = 0;
		inBuf.Address.LowPart = (DWORD)address;
	}
	else
	{
		inBuf.Address.QuadPart = address;
	}

	inBuf.UnitSize = 1;
	inBuf.Count = (ULONG)len;
	size = inBuf.UnitSize * inBuf.Count;

	result = DeviceIoControl(drv->hhDriver, IOCTL_OLS_READ_MEMORY,
		&inBuf, sizeof(OLS_READ_MEMORY_INPUT), buffer, size, &returnedLength, NULL);

	if (result && returnedLength == size)
		return len;
	return 0;
}

int wr0_driver_close(struct wr0_drv_t* drv)
{
	SERVICE_STATUS srvStatus = { 0 };
	if (drv == NULL)
		return 0;
	if (drv->hhDriver && drv->hhDriver != INVALID_HANDLE_VALUE)
	{
		CloseHandle(drv->hhDriver);
		drv->hhDriver = NULL;
		drv->scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (drv->scManager)
			drv->scDriver = OpenServiceW(drv->scManager, drv->driver_id, SERVICE_ALL_ACCESS);
	}
	if (drv->scDriver)
	{
		ControlService(drv->scDriver, SERVICE_CONTROL_STOP, &srvStatus);
		DeleteService(drv->scDriver);
		CloseServiceHandle(drv->scDriver);
		drv->scDriver = NULL;
	}
	if (drv->scManager)
	{
		CloseServiceHandle(drv->scManager);
		drv->scManager = NULL;
	}
	free(drv);
	return 0;
}
