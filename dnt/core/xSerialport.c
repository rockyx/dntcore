#include "xSerialport.h"
#include "xSerialport_p.h"
#include "xMessage.h"

#define DEFAULT_DATABITS 8
#define DEFAULT_PARITY X_SERIAL_PARITY_NONE
#define DEFAULT_STOPBITS X_SERIAL_STOPBITS_ONE
#define DEFAULT_HANDSHAKE X_SERIAL_HANDSHAKE_NONE
#define DEFAULT_BUFFER_SIZE 1024
#define DEFAULT_BAUDRATE 9600
#define DEFAULT_DTR_ENABLE FALSE
#define DEFAULT_RTS_ENABLE FALSE
#define DEFAULT_READ_TIMEOUT X_SERIALPORT_INFINITE_TIMEOUT
#define DEFAULT_WRITE_TIMEOUT X_SERIALPORT_INFINITE_TIMEOUT
#define DEFAULT_READ_BUFFER_SIZE 4096
#define DEFAULT_WRITE_BUFFER_SIZE 2048
#define MAX_DATABITS (8)
#define MIN_DATABITS (5)

xSerialPort * x_serialport_new(const xChar *port_name,
	xInt32 baudrate,
	xSerialParity parity,
	xUInt8 databits,
	xSerialStopbits stopbits)
{
	xSerialPort *port = (xSerialPort*)malloc(sizeof(xSerialPort));
	port->baudrate = baudrate;
	port->databits = databits;
	port->parity = parity;
	port->stopbits = stopbits;
	port->port_name = x_string_new(port_name);
	port->handshake = DEFAULT_HANDSHAKE;
	port->read_timeout = DEFAULT_READ_TIMEOUT;
	port->write_timeout = DEFAULT_WRITE_TIMEOUT;
	port->dtr_enable = DEFAULT_DTR_ENABLE;
	port->rts_enable = DEFAULT_RTS_ENABLE;
	port->read_buffer_size = DEFAULT_READ_BUFFER_SIZE;
	port->write_buffer_size = DEFAULT_WRITE_BUFFER_SIZE;
	port->platform = NULL;

	return port;
}

void x_serialport_free(xSerialPort *port)
{
	x_return_if_fail(port);
	x_serialport_close(port);
	x_string_free(port->port_name, TRUE);
	x_serialport_platform_free(port->platform);
	free(port);
}

xBoolean x_serialport_set_baudrate(xSerialPort *port, xInt32 value)
{
	x_return_val_if_fail(port, FALSE);

	if (value <= 0)
		return FALSE;

	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_baudrate(port->platform, value))
			return FALSE;
	}

	port->baudrate = value;
	return TRUE;
}

xInt32 x_serialport_bytes_to_write(xSerialPort *port)
{
	x_return_val_if_fail(port, -1);

	if (!x_serialport_is_open(port))
		return -1;
	return x_serialport_platform_bytes_to_write(port->platform);
}

xInt32 x_serialport_bytes_to_read(xSerialPort *port)
{
	x_return_val_if_fail(port, -1);

	if (!x_serialport_is_open(port))
		return -1;
	return x_serialport_platform_bytes_to_read(port->platform);
}

xBoolean x_serialport_set_databits(xSerialPort *port, xUInt8 value)
{
	x_return_val_if_fail(port, FALSE);

	if (value < MIN_DATABITS || value > MAX_DATABITS)
		return FALSE;
	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_databits(port->platform, value))
			return FALSE;
	}
	port->databits = value;
	return TRUE;
}

xBoolean x_serialport_set_dtr_enable(xSerialPort *port, xBoolean value)
{
	x_return_val_if_fail(port, FALSE);
	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_dtr_enable(port->platform, value))
			return FALSE;
	}

	port->dtr_enable = value;
	return TRUE;
}

xBoolean x_serialport_set_handshake(xSerialPort *port, xSerialHandshake value)
{
	x_return_val_if_fail(port, FALSE);

	if (value < X_SERIAL_HANDSHAKE_NONE || value > X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF)
		return FALSE;

	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_handshake(port->platform, value))
			return FALSE;
	}

	port->handshake = value;
	return TRUE;
}

xBoolean x_serialport_is_open(xSerialPort *port)
{
	x_return_val_if_fail(port, FALSE);
	return (port->platform != NULL) && x_serialport_platform_is_open(port->platform);
}

xBoolean x_serialport_set_parity(xSerialPort *port, xSerialParity value)
{
	x_return_val_if_fail(port, FALSE);

	if (value < X_SERIAL_PARITY_NONE || value > X_SERIAL_PARITY_SPACE)
		return FALSE;

	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_parity(port->platform, value))
			return FALSE;
	}

	port->parity = value;
	return TRUE;
}

xBoolean x_serialport_set_port_name(xSerialPort *port, const xChar *value)
{
	x_return_val_if_fail(port, FALSE);
	x_return_val_if_fail(value, FALSE);

	if (x_serialport_is_open(port))
		return FALSE;

	port->port_name = x_string_assign(port->port_name, value);
	return TRUE;
}

xBoolean x_serialport_set_read_buffer_size(xSerialPort *port, xInt32 value)
{
	x_return_val_if_fail(port, FALSE);

	if (x_serialport_is_open(port))
		return FALSE;

	port->read_buffer_size = value;
	return TRUE;
}

xBoolean x_serialport_set_read_timeout(xSerialPort *port, xInt32 value)
{
	x_return_val_if_fail(port, FALSE);
	if (value < 0 && value != X_SERIALPORT_INFINITE_TIMEOUT)
		return FALSE;

	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_read_timeout(port->platform, value))
			return FALSE;
	}

	port->read_timeout = value;
	return TRUE;
}

xBoolean x_serialport_set_rts_enable(xSerialPort *port, xBoolean value)
{
	x_return_val_if_fail(port, FALSE);

	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_rts_enable(port->platform, value))
			return FALSE;
	}

	port->rts_enable = value;
	return TRUE;
}

xBoolean x_serialport_set_stopbits(xSerialPort *port, xSerialStopbits value)
{
	/* this range check looks wrong, but it really is correct. One = 1, Two = 2, and OnePointFive = 3 */
	if (value < X_SERIAL_STOPBITS_ONE || value > X_SERIAL_STOPBITS_ONE5)
		return FALSE;

	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_stopbits(port->platform, value))
			return FALSE;
	}

	port->stopbits = value;
	return TRUE;
}

xBoolean x_serialport_set_write_buffer_size(xSerialPort *port, xInt32 value)
{
	x_return_val_if_fail(port, FALSE);
	x_return_val_if_fail(value > 0, FALSE);

	if (x_serialport_is_open(port))
		return FALSE;

	port->write_buffer_size = value;
	return TRUE;
}

xBoolean x_serialport_set_write_timeout(xSerialPort *port, xInt32 value)
{
	x_return_val_if_fail(port, FALSE);
	
	if (value <= 0 && value != X_SERIALPORT_INFINITE_TIMEOUT)
		return FALSE;

	if (x_serialport_is_open(port))
	{
		if (!x_serialport_platform_set_write_timeout(port->platform, value))
			return FALSE;
	}

	port->write_timeout = value;
	return TRUE;
}

xBoolean x_serialport_close(xSerialPort *port)
{
	x_return_val_if_fail(port, FALSE);

	if (x_serialport_is_open(port))
	{
		x_serialport_flush(port);
		x_serialport_platform_free(port->platform);
		port->platform = NULL;
	}

	return TRUE;
}

xBoolean x_serialport_discard_in_buffer(xSerialPort *port)
{
	x_return_val_if_fail(port, FALSE);

	if (!x_serialport_is_open(port))
		return FALSE;

	return x_serialport_platform_discard_in_buffer(port->platform);
}

xBoolean x_serialport_discard_out_buffer(xSerialPort *port)
{
	x_return_val_if_fail(port, FALSE);

	if (!x_serialport_is_open(port))
		return FALSE;

	return x_serialport_platform_discard_out_buffer(port->platform);
}

xStringList * x_serialport_get_port_names(void)
{
	return x_serialport_platform_get_port_names();
}

xBoolean x_serialport_flush(xSerialPort *port)
{
	x_return_val_if_fail(port, FALSE);
	
	if (!x_serialport_is_open(port))
		return FALSE;
	return x_serialport_platform_flush(port->platform);
}

xBoolean x_serialport_open(xSerialPort *port)
{
	x_return_val_if_fail(port, FALSE);

	if (x_serialport_is_open(port))
		return FALSE;

	if (port->platform != NULL)
		x_serialport_platform_free(port->platform);

	port->platform = x_serialport_platform_new();

	if (!x_serialport_platform_open(port->platform,
		port->port_name->str,
		port->baudrate,
		port->parity,
		port->databits,
		port->stopbits,
		port->read_timeout,
		port->write_timeout,
		port->handshake,
		port->dtr_enable,
		port->rts_enable,
		FALSE,
		'\0'))
		return FALSE;

	x_serialport_platform_set_buffer_sizes(port->platform, 
		port->read_buffer_size, 
		port->write_buffer_size);
	return TRUE;
}

xInt32 x_serialport_read(xSerialPort *port, xUInt8 *buffer, xInt32 count)
{
	x_return_val_if_fail(port, -1);
	x_return_val_if_fail(buffer, -1);
	x_return_val_if_fail(count > 0, -1);

	if (!x_serialport_is_open(port))
		return -1;
	return x_serialport_platform_read(port->platform, buffer, count);
}

xInt32 x_serialport_write(xSerialPort *port, const xUInt8 *buffer, xInt32 count)
{
	x_return_val_if_fail(port, -1);
	x_return_val_if_fail(buffer, -1);
	x_return_val_if_fail(count > 0, -1);

	if (!x_serialport_is_open(port))
		return -1;

	return x_serialport_platform_write(port->platform, buffer, count);
}
