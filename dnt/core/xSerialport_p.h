#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XSERIALPORT_P_H__
#define __DNT_CORE_XSERIALPORT_P_H__

#include <stdlib.h>
#include "xSerialport.h"
#if defined(X_OS_UNIX)
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#elif defined(X_OS_WIN)
#include <Windows.h>
#include <objbase.h>
#include <initguid.h>
#include <SetupAPI.h>
#include <Dbt.h>

X_CORE_BEGIN_DECLS

// ---------- default values -------------*
#define DEFAULT_DATABITS 8
#define DEFAULT_PARITY = X_SERIAL_PARITY_NONE
#define DEFAULT_STPOBITS X_SERIAL_STOPBITS_ONE
#define DEFAULT_HANDSHAKE X_SERIAL_HANDSHAKE_NONE
#define DEFAULT_BUFFER_SIZE (1024)
#define DEFAULT_BAUDRATE (9600)
#define DEFAULT_DTR_ENABLE FALSE
#define DEFAULT_RTS_ENABLE FALSE
#define DEFAULT_READ_TIMEOUT X_SERIALPORT_INFINITE_TIMEOUT
#define DEFAULT_WRITE_TIMEOUT X_SERIALPORT_INFINITE_TIMEOUT
#define DEFAULT_READ_BUFFER_SIZE (4096)
#define DEFAULT_WRITE_BUFFER_SIZE (2048)
#define MAX_DATABITS (8)
#define MIN_DATABITS (5)

struct _xSerialPortPlatform
{
	HANDLE handle;
	DCB dcb;
	COMMTIMEOUTS comm_timeouts;
	COMMPROP com_prop;
	COMSTAT com_stat;
	char parity_replace;
	xBoolean rts_enable;
	xSerialHandshake handshake;
	xString *port_name;
};

X_CORE_END_DECLS

#endif

X_CORE_BEGIN_DECLS

xSerialPortPlatform * x_serialport_platform_new(void);
void x_serialport_platform_free(xSerialPortPlatform *port);
xBoolean x_serialport_platform_is_open(xSerialPortPlatform *port);
xBoolean x_serialport_platform_open(xSerialPortPlatform *port,
	const xChar *port_name,
	xInt32 baudrate,
	xSerialParity parity,
	xUInt8 databits,
	xSerialStopbits stopbits,
	xInt32 read_timeout,
	xInt32 write_timeout,
	xSerialHandshake handshake,
	xBoolean dtr_enable,
	xBoolean rts_enable,
	xBoolean discard_null,
	xChar parity_replace);
/* Fill PortSettings */
xBoolean x_serialport_platform_set_baudrate(xSerialPortPlatform *port, xInt32 value);
xBoolean x_serialport_platform_set_databits(xSerialPortPlatform *port, xUInt8 value);
xBoolean x_serialport_platform_set_parity(xSerialPortPlatform *port, xSerialParity value);
xBoolean x_serialport_platform_set_stopbits(xSerialPortPlatform *port, xSerialStopbits value);
xBoolean x_serialport_platform_set_handshake(xSerialPortPlatform *port, xSerialHandshake value);
xBoolean x_serialport_platform_set_read_timeout(xSerialPortPlatform *port, xInt32 value);
xInt32 x_serialport_platform_get_read_timeout(xSerialPortPlatform *port);
xBoolean x_serialport_platform_set_write_timeout(xSerialPortPlatform *port, xInt32 value);
xInt32 x_serialport_platform_get_write_timeout(xSerialPortPlatform *port);
xBoolean x_serialport_platform_set_rts_enable(xSerialPortPlatform *port, xBoolean value);
xBoolean x_serialport_platform_get_rts_enable(xSerialPortPlatform *port);
xBoolean x_serialport_platform_set_dtr_enable(xSerialPortPlatform *port, xBoolean value);
xBoolean x_serialport_platform_get_dtr_enable(xSerialPortPlatform *port);
xInt32 x_serialport_platform_bytes_to_read(xSerialPortPlatform *port);
xInt32 x_serialport_platform_bytes_to_write(xSerialPortPlatform *port);
xBoolean x_serialport_platform_discard_in_buffer(xSerialPortPlatform *port);
xBoolean x_serialport_platform_discard_out_buffer(xSerialPortPlatform *port);
xBoolean x_serialport_platform_flush(xSerialPortPlatform *port);
xInt32 x_serialport_platform_read(xSerialPortPlatform *port, xUInt8 *buffer, xInt32 count);
xInt32 x_serialport_platform_write(xSerialPortPlatform *port, const xUInt8 *buffer, xInt32 count);
xBoolean x_serialport_platform_set_buffer_sizes(xSerialPortPlatform *port, xInt32 read_buffer_size, xInt32 write_buffer_size);
xStringList* x_serialport_platform_get_port_names(void);

X_CORE_END_DECLS

#endif // __DNT_CORE_SERIALPORT_P_H__
