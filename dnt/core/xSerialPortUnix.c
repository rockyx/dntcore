/*
 * xSerialPortUnix.c
 *
 *  Created on: 2014Äê4ÔÂ25ÈÕ
 *      Author: Rocky Tsui
 */

#include "xSerialPort_p.h"
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <android/log.h>
#include "xMessage.h"
#include "xTimer.h"

xSerialPortPlatform * x_serialport_platform_new(void)
{
	xSerialPortPlatform * ret = (xSerialPortPlatform*)malloc(sizeof(xSerialPortPlatform));
	ret->fd = -1;
	ret->read_timeout = X_SERIALPORT_INFINITE_TIMEOUT;
	ret->write_timeout = X_SERIALPORT_INFINITE_TIMEOUT;
	ret->parity = X_SERIAL_PARITY_NONE;
	ret->databits = 8;
	ret->stopbits = X_SERIAL_STOPBITS_ONE;
	ret->handshake = X_SERIAL_HANDSHAKE_NONE;

	return ret;
}

void x_serialport_platform_free(xSerialPortPlatform *port)
{
	x_return_if_fail(port);
	free(port);
}

xBoolean x_serialport_platform_is_open(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);

	return port->fd != -1;
}

static int setup_baudrate(xInt32 baudrate)
{
	switch (baudrate)
	{
	/* Some values are not defined on OSX and *BSD */
#if defined(B921600)
	case 921600: return B921600;
#endif
#if defined(B460800)
	case 460800: return B460800;
#endif
	case 230400: return B230400;
	case 115200: return B115200;
	case 57600:  return B57600;
	case 38400:  return B38400;
	case 19200:  return B19200;
	case 9600:   return B9600;
	case 4800:   return B4800;
	case 2400:   return B2400;
	case 1800:   return B1800;
	case 1200:   return B1200;
	case 600:    return B600;
	case 300:    return B300;
	case 200:    return B200;
	case 150:    return B150;
	case 134:    return B134;
	case 110:    return B110;
	case 75:     return B75;
	case 50:
	case 0:
	default:
		return -1;
	}
}

static xBoolean is_baudrate_legal(xInt32 baudrate)
{
	return setup_baudrate(baudrate) != -1 ? TRUE : FALSE;
}

static xBoolean set_attributes(int fd,
		xInt32 baudrate,
		xSerialParity parity,
		xUInt8 databits,
		xSerialStopbits stopbits,
		xSerialHandshake handshake)
{
	struct termios newtio;

	if (tcgetattr(fd, &newtio) == -1)
		return FALSE;

	newtio.c_cflag |= (CLOCAL | CREAD);
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);
	newtio.c_oflag &= ~(OPOST);
	newtio.c_iflag = IGNBRK;

	/* setup baudrate */
	baudrate = setup_baudrate(baudrate);

	/* char length */
	newtio.c_cflag &= ~CSIZE;
	switch(databits)
	{
	case 5: newtio.c_cflag |= CS5; break;
	case 6: newtio.c_cflag |= CS6; break;
	case 7: newtio.c_cflag |= CS7; break;
	case 8:
	default: newtio.c_cflag |= CS8; break;
	}

	/* stopbits */
	switch(stopbits)
	{
	case X_SERIAL_STOPBITS_NONE: /* Unhandled */ break;
	case X_SERIAL_STOPBITS_ONE: newtio.c_cflag &= ~CSTOPB; break;
	case X_SERIAL_STOPBITS_TWO: newtio.c_cflag |= CSTOPB; break;
	case X_SERIAL_STOPBITS_ONE5: /* Unhandled */ break;
	}

	/* parity */
	newtio.c_iflag &= ~(INPCK | ISTRIP);
	switch (parity)
	{
	case X_SERIAL_PARITY_NONE: newtio.c_cflag &= ~(PARENB | PARODD); break;
	case X_SERIAL_PARITY_ODD: newtio.c_cflag |= PARENB | PARODD; break;
	case X_SERIAL_PARITY_EVEN:
		newtio.c_cflag &= ~(PARODD);
		newtio.c_cflag |= (PARENB);
		break;
	case X_SERIAL_PARITY_MARK:
	case X_SERIAL_PARITY_SPACE:
		/* Unhandled */
		break;
	}

	newtio.c_iflag &= ~(IXOFF | IXON);
#ifdef CRTSCTS
	newtio.c_cflag &= ~CRTSCTS;
#endif /* CRTSCTS */
	switch (handshake)
	{
	case X_SERIAL_HANDSHAKE_NONE: /* do nothing */ break;
	case X_SERIAL_HANDSHAKE_REQUESTTOSEND:
#ifdef CRTSCTS
		newtio.c_cflag |= CRTSCTS;
#endif
		break;
	case X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF:
#ifdef CRTSCTS
		newtio.c_cflag |= CRTSCTS;
#endif
		/* fall through */
	case X_SERIAL_HANDSHAKE_XONXOFF:
		newtio.c_iflag |= IXOFF | IXON;
		break;
	}

	if (cfsetospeed(&newtio, baudrate) < 0 || cfsetispeed(&newtio, baudrate) < 0 || tcsetattr(fd, TCSANOW, &newtio) < 0)
		return FALSE;

	return TRUE;
}

static xInt32 get_signal_code(xSerialSignal signal)
{
	switch (signal)
	{
	case X_SERIAL_SIGNAL_CD: return TIOCM_CAR;
	case X_SERIAL_SIGNAL_CTS: return TIOCM_CTS;
	case X_SERIAL_SIGNAL_DSR: return TIOCM_DSR;
	case X_SERIAL_SIGNAL_DTR: return TIOCM_DTR;
	case X_SERIAL_SIGNAL_RTS: return TIOCM_RTS;
	default: return 0;
	}
}

static xBoolean set_signal(int fd, xSerialSignal signal, xBoolean value)
{
	int signals, expected;
	xBoolean activated;

	if (signal < X_SERIAL_SIGNAL_CD || signal > X_SERIAL_SIGNAL_RTS ||
			signal == X_SERIAL_SIGNAL_CD ||
			signal == X_SERIAL_SIGNAL_CTS ||
			signal == X_SERIAL_SIGNAL_DSR)
		return FALSE;

#ifdef TOMIC_DEVICE_V1
	if (signal == X_SERIAL_SIGNAL_DTR)
		system("/system/bin/pulldown");
	else
		system("/system/bin/pullup");
#endif

	expected = get_signal_code(signal);
	if (ioctl(fd, TIOCMGET, &signals) == -1)
		return FALSE;

	activated = (signals & expected) != 0 ? TRUE : FALSE;
	if (activated == value) /* Already set */
		return TRUE;

	if (value)
		signals |= expected;
	else
		signals &= ~expected;

	if (ioctl(fd, TIOCMSET, &signals) == -1)
		return FALSE;

	return TRUE;
}

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
	xChar parity_replace)
{
	x_return_val_if_fail(port, FALSE);

	port->fd = open(port_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (port->fd == -1)
		return FALSE;

	if (!is_baudrate_legal(baudrate))
		return FALSE;

	if (!set_attributes(port->fd, baudrate, parity, databits, stopbits, handshake))
		return FALSE;

	port->read_timeout = read_timeout;
	port->write_timeout = write_timeout;

	if (!set_signal(port->fd, X_SERIAL_SIGNAL_DTR, dtr_enable))
		return FALSE;

	if (handshake != X_SERIAL_HANDSHAKE_REQUESTTOSEND && handshake != X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF)
		if (!set_signal(port->fd, X_SERIAL_SIGNAL_RTS, rts_enable)) return FALSE;

	port->databits = databits;
	port->parity = parity;
	port->stopbits = stopbits;
	port->handshake = handshake;
	return TRUE;
}

/* Fill PortSettings */
xBoolean x_serialport_platform_set_baudrate(xSerialPortPlatform *port, xInt32 value)
{
	x_return_val_if_fail(port, FALSE);

	if (set_attributes(port->fd, value, port->parity, port->databits, port->stopbits, port->handshake))
	{
		port->baudrate = value;
		return TRUE;
	}
	return FALSE;
}

xBoolean x_serialport_platform_set_databits(xSerialPortPlatform *port, xUInt8 value)
{
	x_return_val_if_fail(port, FALSE);

	if (set_attributes(port->fd, port->baudrate, port->parity, value, port->stopbits, port->handshake))
	{
		port->databits = value;
		return TRUE;
	}
	return FALSE;
}

xBoolean x_serialport_platform_set_parity(xSerialPortPlatform *port, xSerialParity value)
{
	x_return_val_if_fail(port, FALSE);

	if (set_attributes(port->fd, port->baudrate, value, port->databits, port->stopbits, port->handshake))
	{
		port->parity = value;
		return TRUE;
	}
	return FALSE;
}

xBoolean x_serialport_platform_set_stopbits(xSerialPortPlatform *port, xSerialStopbits value)
{
	x_return_val_if_fail(port, FALSE);

	if (set_attributes(port->fd, port->baudrate, port->parity, port->databits, value, port->handshake))
	{
		port->stopbits = value;
		return TRUE;
	}
	return FALSE;
}

xBoolean x_serialport_platform_set_handshake(xSerialPortPlatform *port, xSerialHandshake value)
{
	x_return_val_if_fail(port, FALSE);

	if (set_attributes(port->fd, port->baudrate, port->parity, port->databits, port->stopbits, value))
	{
		port->handshake = value;
		return TRUE;
	}
	return FALSE;
}

xBoolean x_serialport_platform_set_read_timeout(xSerialPortPlatform *port, xInt32 value)
{
	x_return_val_if_fail(port, FALSE);
	if (value < 0 && value != X_SERIALPORT_INFINITE_TIMEOUT)
		return FALSE;

	port->read_timeout = value;
	return TRUE;
}

xInt32 x_serialport_platform_get_read_timeout(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, -2);

	return port->read_timeout;
}

xBoolean x_serialport_platform_set_write_timeout(xSerialPortPlatform *port, xInt32 value)
{
	x_return_val_if_fail(port, FALSE);
	if (value < 0 && value != X_SERIALPORT_INFINITE_TIMEOUT)
		return FALSE;

	port->write_timeout = value;
	return TRUE;
}

xInt32 x_serialport_platform_get_write_timeout(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, -2);
	return port->write_timeout;
}

xBoolean x_serialport_platform_set_rts_enable(xSerialPortPlatform *port, xBoolean value)
{
	x_return_val_if_fail(port, FALSE);
	return set_signal(port->fd, X_SERIAL_SIGNAL_RTS, value);
}

xBoolean x_serialport_platform_get_rts_enable(xSerialPortPlatform *port)
{
	int signals, expected;

	x_return_val_if_fail(port, FALSE);

	expected = get_signal_code(X_SERIAL_SIGNAL_RTS);

	if (ioctl(port->fd, TIOCMGET, &signals) == -1)
		return FALSE;

	return (signals & expected) != 0 ? TRUE : FALSE;
}

xBoolean x_serialport_platform_set_dtr_enable(xSerialPortPlatform *port, xBoolean value)
{
	x_return_val_if_fail(port, FALSE);

	return set_signal(port->fd, X_SERIAL_SIGNAL_DTR, value);
}

xBoolean x_serialport_platform_get_dtr_enable(xSerialPortPlatform *port)
{
	int signals, expected;

	x_return_val_if_fail(port, FALSE);

	expected = get_signal_code(X_SERIAL_SIGNAL_DTR);

	if (ioctl(port->fd, TIOCMGET, &signals) == -1)
		return FALSE;

	return (signals & expected) != 0 ? TRUE : FALSE;
}

static xInt32 get_bytes_in_buffer(int fd, xBoolean input)
{
	xInt32 retval;

	if (ioctl(fd, input ? FIONREAD : TIOCOUTQ, &retval) == -1)
		return -1;

	return retval;
}

xInt32 x_serialport_platform_bytes_to_read(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, -1);
	return get_bytes_in_buffer(port->fd, TRUE);
}

xInt32 x_serialport_platform_bytes_to_write(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, -1);
	return get_bytes_in_buffer(port->fd, FALSE);
}

static int discard_buffer(int fd, xBoolean input)
{
	return tcflush(fd, input ? TCIFLUSH : TCOFLUSH);
}

xBoolean x_serialport_platform_discard_in_buffer(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);
	return discard_buffer(port->fd, TRUE) != -1 ? TRUE : FALSE;
}

xBoolean x_serialport_platform_discard_out_buffer(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);
	return discard_buffer(port->fd, FALSE) != -1 ? TRUE : FALSE;
}

xBoolean x_serialport_platform_flush(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);
	/* Not implement yet */
	return FALSE;
}

static xBoolean poll_serial(int fd, xInt64 timeout)
{
	struct pollfd pinfo;

	pinfo.fd = fd;
	pinfo.events = POLLIN;
	pinfo.revents = 0;

	while (poll(&pinfo, 1, timeout) == -1 && errno == EINTR)
	{
		/* EINTR is an OK condition, we should not throw in the upper layer an IOException */
		if (errno != EINTR)
		{
			return FALSE;
		}
	}

	return (pinfo.revents & POLLIN) != 0 ? TRUE : FALSE;
}

xInt32 x_serialport_platform_read(xSerialPortPlatform *port, xUInt8 *buffer, xInt32 count)
{
#ifdef X_OS_ANDROID
	xTimer expire;
	xTimer current;
	xInt32 len = 0;
	xInt32 offset = 0;

	x_return_val_if_fail(port, -1);
	x_return_val_if_fail(buffer, -1);
	x_return_val_if_fail(count != 0, 0);
	x_return_val_if_fail(count > 0, -1);

	x_timer_get_current_time(&expire);
	x_timer_add_milliseconds(&expire, port->read_timeout);
	x_timer_get_current_time(&current);

	while (x_timer_compare(&current, &expire) < 0)
	{
		xInt32 size = x_serialport_platform_bytes_to_read(port);
		if (size >= count)
		{
			size = read(port->fd, buffer + offset, count);
			if (size == -1)
				return -1;
			len += size;
			return len;
		}

		if (size != 0)
		{
			size = read(port->fd, buffer + offset, size);
			if (size == -1)
				return -1;
			len += size;
			count -= size;
			offset += size;
		}

		x_timer_get_current_time(&current);
	}

	return len;
#else
	xBoolean poll_result;
	xInt32 result;

	x_return_val_if_fail(port, -1);
	x_return_val_if_fail(buffer, -1);
	x_return_val_if_fail(count != 0, 0);
	x_return_val_if_fail(count > 0, -1);

	poll_result = poll_serial(port->fd, port->read_timeout);

	if (!poll_result)
		return -1;

	result = read(port->fd, buffer, count);
	return result;
#endif
}

xInt32 x_serialport_platform_write(xSerialPortPlatform *port, const xUInt8 *buffer, xInt32 count)
{
#ifdef TOMIC_DEVICE_V1
	x_return_val_if_fail(port, -1);

	return write(port->fd, buffer, count);
#else
	struct pollfd pinfo;
	xUInt32 n;
	xUInt32 offset = 0;

	x_return_val_if_fail(port, -1);

	pinfo.fd = port->fd;
	pinfo.events = POLLOUT;
	pinfo.revents = POLLOUT;

	n = count;

	while (n > 0)
	{
		xSSize t;
		if (port->write_timeout != 0)
		{
			int c;
			while ((c == poll(&pinfo, 1, port->write_timeout)) == -1 && errno == EINTR)
				;
			if (c == -1)
			{
				return -1;
			}
		}

		do
		{
			t = write(port->fd, buffer + offset, n);
		} while (t == -1 && errno == EINTR);

		if (t < 0)
		{
			return -1;
		}

		offset += t;
		n -= t;
	}
	return 0;
#endif
}
xBoolean x_serialport_platform_set_buffer_sizes(xSerialPortPlatform *port, xInt32 read_buffer_size, xInt32 write_buffer_size)
{
	return FALSE;
}

xStringList* x_serialport_platform_get_port_names(void)
{
	return NULL;
}

