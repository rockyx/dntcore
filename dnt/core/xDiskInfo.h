#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XDISKINFO_H__
#define __DNT_CORE_XDISKINFO_H__

#include <dnt/core/xGlobal.h>
#include <dnt/core/xString.h>

#ifdef X_OS_WIN
#include <Windows.h>
#include <winioctl.h>
#include <ntddscsi.h>
#endif

X_CORE_BEGIN_DECLS

xSize x_disk_info_load(void);
void x_disk_info_unload(void);
xInt64 x_disk_info_get_drive_size(xSize drive);
xString* x_disk_info_get_drive_type(xSize drive);
xString* x_disk_info_get_revision_number(xSize drive);
xString* x_disk_info_get_serial_number(xSize drive);
xString* x_disk_info_get_model_number(xSize drive);

X_CORE_END_DECLS

#endif // __DNT_CORE_DISK_INFO_H__
