#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XSERIALPORT_H__
#define __DNT_CORE_XSERIALPORT_H__

#include <dnt/core/xGlobal.h>
#include <dnt/core/xStringList.h>

X_CORE_BEGIN_DECLS

typedef enum _xSerialHandshake
{
	X_SERIAL_HANDSHAKE_NONE = 0,
	X_SERIAL_HANDSHAKE_XONXOFF = 1,
	X_SERIAL_HANDSHAKE_REQUESTTOSEND = 2,
	X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF = 3
} xSerialHandshake;

typedef enum _xSerialParity
{
	X_SERIAL_PARITY_NONE = 0,
	X_SERIAL_PARITY_ODD = 1,
	X_SERIAL_PARITY_EVEN = 2,
	X_SERIAL_PARITY_MARK = 3,
	X_SERIAL_PARITY_SPACE = 4
} xSerialParity;

typedef enum _xSerialStopbits
{
	X_SERIAL_STOPBITS_NONE = 0,
	X_SERIAL_STOPBITS_ONE = 1,
	X_SERIAL_STOPBITS_TWO = 2,
	X_SERIAL_STOPBITS_ONE5 = 3
} xSerialStopbits;

typedef enum _xSerialSignal
{
	X_SERIAL_SIGNAL_NONE = 0,
	X_SERIAL_SIGNAL_CD = 1, // Carrier detect
	X_SERIAL_SIGNAL_CTS = 2, // Clear to send
	X_SERIAL_SIGNAL_DSR = 4, // Data set ready
	X_SERIAL_SIGNAL_DTR = 8, // Data terminal ready
	X_SERIAL_SIGNAL_RTS = 16 // Request to send
} xSerialSignal;

typedef struct _xSerialPort xSerialPort;
typedef struct _xSerialPortPlatform xSerialPortPlatform;

#define X_SERIALPORT_INFINITE_TIMEOUT (-1)

struct _xSerialPort
{
	xUInt8 databits;
	xInt32 baudrate;
	xSerialParity parity;
	xSerialStopbits stopbits;
	xInt32 read_timeout;
	xInt32 write_timeout;
	xBoolean dtr_enable;
	xBoolean rts_enable;
	xSerialHandshake handshake;
	xInt32 read_buffer_size;
	xInt32 write_buffer_size;
	xString *port_name;

	xSerialPortPlatform *platform;
};

xSerialPort * x_serialport_new(const xChar *port_name, 
	xInt32 baudrate, 
	xSerialParity parity, 
	xUInt8 databits, 
	xSerialStopbits stopbits);
void x_serialport_free(xSerialPort *port);
xBoolean x_serialport_set_baudrate(xSerialPort *port, xInt32 value);
xInt32 x_serialport_bytes_to_write(xSerialPort *port);
xInt32 x_serialport_bytes_to_read(xSerialPort *port);
xBoolean x_serialport_set_databits(xSerialPort *port, xUInt8 value);
xBoolean x_serialport_set_dtr_enable(xSerialPort *port, xBoolean value);
xBoolean x_serialport_set_handshake(xSerialPort *port, xSerialHandshake value);
xBoolean x_serialport_is_open(xSerialPort *port);
xBoolean x_serialport_set_parity(xSerialPort *port, xSerialParity value);
xBoolean x_serialport_set_port_name(xSerialPort *port, const xChar *value);
xBoolean x_serialport_set_read_buffer_size(xSerialPort *port, xInt32 value);
xBoolean x_serialport_set_read_timeout(xSerialPort *port, xInt32 value);
xBoolean x_serialport_set_rts_enable(xSerialPort *port, xBoolean value);
xBoolean x_serialport_set_stopbits(xSerialPort *port, xSerialStopbits stopbits);
xBoolean x_serialport_set_write_buffer_size(xSerialPort *port, xInt32 value);
xBoolean x_serialport_set_write_timeout(xSerialPort *port, xInt32 value);
xBoolean x_serialport_close(xSerialPort *port);
xBoolean x_serialport_discard_in_buffer(xSerialPort *port);
xBoolean x_serialport_discard_out_buffer(xSerialPort *port);
xBoolean x_serialport_open(xSerialPort *port);
xBoolean x_serialport_flush(xSerialPort *port);
xInt32 x_serialport_read(xSerialPort *port, xUInt8 *buffer, xInt32 count);
xInt32 x_serialport_write(xSerialPort *port, const xUInt8 *buffer, xInt32 count);
xStringList * x_serialport_get_port_names(void);

X_CORE_END_DECLS

#endif // __DNT_CORE_SERIALPORT_H__

