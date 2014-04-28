#include "xSerialport_p.h"
#include <assert.h>
#include "xMessage.h"

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
#define INFINITE_TIMEOUT -2
#define DEFAULT_XONCHAR 17
#define DEFAULT_XOFFCHAR 19
#define EOFCHAR 26
#define DEFAULT_PORT_NAME "COM1"

static xBoolean initialize_dcb(xSerialPortPlatform *port,
	xInt32 baudrate,
	xSerialParity parity,
	xUInt8 databits,
	xSerialStopbits stopbits,
	xBoolean discard_null)
{
	/* first get the current dcb structure setup */
	if (!GetCommState(port->handle, &(port->dcb)))
		return FALSE;

	port->dcb.DCBlength = sizeof(port->dcb);

	/* set parameterized properties */
	port->dcb.BaudRate = (DWORD)baudrate;
	port->dcb.ByteSize = (BYTE)databits;

	switch (stopbits)
	{
	case X_SERIAL_STOPBITS_ONE:
		port->dcb.StopBits = ONESTOPBIT;
		break;
	case X_SERIAL_STOPBITS_TWO:
		port->dcb.StopBits = TWOSTOPBITS;
		break;
	case X_SERIAL_STOPBITS_ONE5:
		port->dcb.StopBits = ONE5STOPBITS;
		break;
	default:
		assert(FALSE);
		break;
	}

	port->dcb.Parity = (BYTE)parity;
	port->dcb.fParity = (parity == X_SERIAL_PARITY_NONE) ? FALSE : TRUE;
	port->dcb.fBinary = TRUE; /* always true for communications resources */

	port->dcb.fOutxCtsFlow = (port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSEND ||
		port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF) ? TRUE : FALSE;

	port->dcb.fOutxDsrFlow = FALSE; /* dsrTimeout is always set to 0. */
	port->dcb.fDtrControl = DTR_CONTROL_DISABLE;
	port->dcb.fDsrSensitivity = FALSE; /* this should remain off */
	port->dcb.fInX = (port->handshake == X_SERIAL_HANDSHAKE_XONXOFF) || (port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF) ? TRUE : FALSE;
	port->dcb.fOutX = (port->handshake == X_SERIAL_HANDSHAKE_XONXOFF) || (port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF) ? TRUE : FALSE;

	/* if no parity, we have no error character (i.e. ErrorChar = '\0' or null character) */
	if (parity != X_SERIAL_PARITY_NONE)
	{
		port->dcb.fErrorChar = (port->parity_replace != '\0') ? TRUE : FALSE;
		port->dcb.ErrorChar = port->parity_replace;
	}
	else
	{
		port->dcb.fErrorChar = FALSE;
		port->dcb.ErrorChar = '\0';
	}

	port->dcb.fNull = discard_null ? TRUE : FALSE;

	/* Setting RTS control, which is RTS_CONTROL_HANDSHAKE if RTS / RTS-XOnXOff handshaking
	 * used, RTS_ENABLE (RTS pin used during operation) if rtsEnable true but XOnXOff / No handshaking
	 * used, and disable otherwise.
	 */
	if ((port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSEND) ||
		(port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF))
	{
		port->dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	}
	else if (port->dcb.fRtsControl == RTS_CONTROL_HANDSHAKE)
	{
		port->dcb.fRtsControl = RTS_CONTROL_DISABLE;
	}

	port->dcb.XonChar = DEFAULT_XONCHAR; /* may be exposed later but for now, constant */
	port->dcb.XoffChar = DEFAULT_XOFFCHAR;

	/* minimum number of bytes allowed in each buffer before flow control activated
	 * heuristically, this has been set at 1/4 of the buffer size
	 */
	port->dcb.XonLim = port->dcb.XoffLim = (WORD)(port->com_prop.dwCurrentRxQueue / 4);
	port->dcb.EofChar = EOFCHAR;

	port->dcb.EvtChar = EOFCHAR;

	if (!SetCommState(port->handle, &(port->dcb)))
		return FALSE;
	return TRUE;
}

xSerialPortPlatform * x_serialport_platform_new(void)
{
	xSerialPortPlatform *port = (xSerialPortPlatform*)malloc(sizeof(xSerialPortPlatform));
	port->handle = INVALID_HANDLE_VALUE;
	port->parity_replace = '?';
	port->rts_enable = FALSE;
	port->handshake = X_SERIAL_HANDSHAKE_NONE;
	port->port_name = x_string_new(DEFAULT_PORT_NAME);
	return port;
}

void x_serialport_platform_free(xSerialPortPlatform *port)
{
	xBoolean skip_sp_access;

	x_return_if_fail(port);

	skip_sp_access = FALSE;
	if (port->handle != INVALID_HANDLE_VALUE)
	{
		if (!EscapeCommFunction(port->handle, CLRDTR))
		{
			DWORD hr = GetLastError();

			if (hr == ERROR_ACCESS_DENIED || hr == ERROR_BAD_COMMAND)
			{
				skip_sp_access = TRUE;
			}
			else
			{
				/* Should not happen */
				assert(FALSE);
				CloseHandle(port->handle);
				port->handle = INVALID_HANDLE_VALUE;
				x_string_free(port->port_name, TRUE);
				free(port);
				return;
			}
		}

		if (!skip_sp_access && (port->handle != INVALID_HANDLE_VALUE))
		{
			x_serialport_platform_flush(port);
		}

		if (!skip_sp_access)
		{
			x_serialport_platform_discard_in_buffer(port);
			x_serialport_platform_discard_out_buffer(port);
		}

		CloseHandle(port->handle);
		port->handle = INVALID_HANDLE_VALUE;
	}
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
	DWORD flags = 0;
	xChar real_name[128];
	DWORD file_type;
	DWORD pin_status;

	x_return_val_if_fail(port, FALSE);

	if (port_name == NULL ||
		strlen(port_name) < 4 ||
		port_name[0] != 'C' ||
		port_name[1] != 'O' ||
		port_name[2] != 'M')
		return FALSE;

	/* Error checking done in serial port*/
	sprintf(real_name, "\\\\.\\%s", port_name);
	port->handle = CreateFileA(real_name,
		GENERIC_READ | GENERIC_WRITE,
		0, /* comm devices must be opened w/exclusive-access */
		NULL, /* no security attributes */
		OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
		flags,
		NULL /* hTemplate must be NULL for comm devices */);

	if (port->handle == INVALID_HANDLE_VALUE)
		return FALSE;

	file_type = GetFileType(port->handle);

	/* Allowing FILE_TYPE_UNKNOWN for legitimate serial device such as USB to serial adapter device */
	if ((file_type != FILE_TYPE_CHAR) && (file_type != FILE_TYPE_UNKNOWN))
		return FALSE;

	/* set properties of the stream that exist as members in xSerialPortPlatform */
	port->port_name = x_string_assign(port->port_name, port_name);
	port->handshake = handshake;
	port->parity_replace = parity_replace;

	/* Fill COMMPROPERTIES struct, which has our maximum allowed baud rate.
	 * Call a serial specific API such as GetCommModemStatus which would fail
	 * in case the device is not a legitimate serial device. For instance,
	 * some illegal FILE_TYPE_UNKNOWN device (or) "LPT1" on Win9x
	 * trying to pass for serial will be caught here. GetCommProperties works
	 * fine for "LPT1" on Win9x, so that alone can't be relied here to
	 * detect non serial devices.
	 */
	pin_status = 0;
	if (!GetCommProperties(port->handle, &(port->com_prop)) ||
		!GetCommModemStatus(port->handle, &pin_status))
	{
		/* If the portName they have passed in is a FILE_TYPE_CHAR but not a serial port,
		 * for example "LPT1", this API will fail. For this reason we handle the error message specially.
		 */
		DWORD error_code = GetLastError();
		if ((error_code == ERROR_INVALID_PARAMETER) || (error_code == ERROR_INVALID_HANDLE))
			// port name
			;
		else
			;
		return FALSE;
	}

	if (port->com_prop.dwMaxBaud != 0 && (DWORD)baudrate > port->com_prop.dwMaxBaud)
		return FALSE;

	/* create internal DCB structure, initialize according to Platform SDK
	 * standard: ms-help://MS.MSNDNQTR.2002APR.1003/hardware/commun_965u.htm
	 * set constant properties of the DCB
	 */
	if (!initialize_dcb(port, baudrate, parity, databits, stopbits, discard_null))
		return FALSE;

	if (!x_serialport_platform_set_dtr_enable(port, dtr_enable))
		return FALSE;

	/* query and cache the initial RtsEnable value
	 * so that setRtsEnable can do the (value != rtsEnable) optimization
	 */
	port->rts_enable = port->dcb.fRtsControl == RTS_CONTROL_ENABLE;

	/* now set _rtsEnable to the specified value.
	 * Handshake takes precedence, this will be a nop if
	 * handshake is either RequestToSend or RequestToSendXOnXOff
	 */
	if ((handshake != X_SERIAL_HANDSHAKE_REQUESTTOSEND && handshake != X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF))
		x_serialport_platform_set_rts_enable(port, rts_enable);

	/* NOTE: this logic should match what is in the ReadTimeout property */
	if (read_timeout == 0)
	{
		port->comm_timeouts.ReadTotalTimeoutConstant = 0;
		port->comm_timeouts.ReadTotalTimeoutMultiplier = 0;
		port->comm_timeouts.ReadIntervalTimeout = MAXDWORD;
	}
	else if (read_timeout == X_SERIALPORT_INFINITE_TIMEOUT)
	{
		/* SetCommTimeouts doesn't like a value of -1 for some reason, so
		 * we'll use -1(INFINITE_TIMEOUT) to represent infinite.
		 */
		port->comm_timeouts.ReadTotalTimeoutConstant = INFINITE_TIMEOUT;
		port->comm_timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
		port->comm_timeouts.ReadIntervalTimeout = MAXDWORD;
	}
	else
	{
		port->comm_timeouts.ReadTotalTimeoutMultiplier = (DWORD)read_timeout;
		port->comm_timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
		port->comm_timeouts.ReadIntervalTimeout = MAXDWORD;
	}

	port->comm_timeouts.WriteTotalTimeoutMultiplier = 0;
	port->comm_timeouts.WriteTotalTimeoutConstant = (DWORD)((write_timeout == X_SERIALPORT_INFINITE_TIMEOUT) ? 0 : write_timeout);

	if (!SetCommTimeouts(port->handle, &(port->comm_timeouts)))
		return FALSE;

	return TRUE;
}

static xChar* full_path_name(const xChar *name)
{
	xString *str = x_string_new(name);
	if (name[0] == 'C' && name[1] == 'O' && name[2] == 'M')
		x_string_prepend(str, "\\\\.\\");
	return x_string_free(str, FALSE);
}

/* see http://msdn.microsoft.com/en-us/library/windows/hardware/ff553426(v=vs.85).aspx
 * for list of GUID classes
 */
#ifndef GUID_DEVCLASS_PORTS
DEFINE_GUID(GUID_DEVCLASS_PORTS, 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);
#endif

static xString* get_reg_key_value(HKEY key, const xChar *property)
{
	DWORD size = 0;
	DWORD type;
	BYTE *buff;
	xString *result = x_string_new("");
	RegQueryValueExA(key, property, NULL, NULL, NULL, &size);
	buff = (BYTE*)malloc(sizeof(BYTE)* size);
	if (RegQueryValueExA(key, property, NULL, &type, buff, &size) == ERROR_SUCCESS)
		result = x_string_append_len(result, (xChar*)buff, size);
	RegCloseKey(key);
	free(buff);
	return result;

}

static xString* get_device_property(HDEVINFO dev_info, PSP_DEVINFO_DATA dev_data, DWORD property)
{
	DWORD buff_size = 0;
	BYTE *buff;
	xString *str = x_string_new("");
	SetupDiGetDeviceRegistryPropertyA(dev_info, dev_data, property, NULL, NULL, 0, &buff_size);
	buff = (BYTE*)malloc(sizeof(BYTE)* buff_size);
	SetupDiGetDeviceRegistryPropertyA(dev_info, dev_data, property, NULL, buff, 0, &buff_size);
	x_string_append_len(str, (xChar*)buff, buff_size);
	return str;
}

static xString* get_device_details(HDEVINFO dev_info, PSP_DEVINFO_DATA dev_data)
{
	HKEY dev_key = SetupDiOpenDevRegKey(dev_info, dev_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
	return get_reg_key_value(dev_key, "PortName");
}

static void enumerate_device(const GUID *guid, xStringList *info_list)
{
	HDEVINFO dev_info;
	if ((dev_info = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_PRESENT)) != INVALID_HANDLE_VALUE)
	{
		SP_DEVINFO_DATA dev_info_data;
		dev_info_data.cbSize = sizeof (SP_DEVINFO_DATA);
		for (int i = 0; SetupDiEnumDeviceInfo(dev_info, i, &dev_info_data); ++i)
		{
			xString *str = get_device_details(dev_info, &dev_info_data);
			x_string_list_append_str(info_list, str);
			x_string_free(str, TRUE);
		}

		SetupDiDestroyDeviceInfoList(dev_info);
	}
}

xStringList * x_serialport_platform_get_port_names(void)
{
	xStringList *result = x_string_list_new();
	enumerate_device(&GUID_DEVCLASS_PORTS, result);
	return result;
}

xBoolean x_serialport_platform_is_open(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);

	return port->handle != INVALID_HANDLE_VALUE;
}

xBoolean x_serialport_platform_set_baudrate(xSerialPortPlatform *port, xInt32 value)
{	
	x_return_val_if_fail(port, FALSE);

	if (value <= 0 || (value > (xInt32)(port->com_prop.dwMaxBaud) && port->com_prop.dwMaxBaud > 0))
	{
		/* if no upper bound on baud rate imposed by serial driver, note that argument must be positive */
		if (port->com_prop.dwMaxBaud == 0)
		{

		}
		else
		{

		}
		return FALSE;
	}

	// Set only if it's different. Rollback to previous values if setting fails.
	// This pattern occurs through most of the other properties in this class.
	if (value != port->dcb.BaudRate)
	{
		DWORD baudrate_old = port->dcb.BaudRate;
		port->dcb.BaudRate = (DWORD)value;
		if (!SetCommState(port->handle, &(port->dcb)))
		{
			port->dcb.BaudRate = baudrate_old;
			return FALSE;
		}
	}

	return TRUE;
}

xBoolean x_serialport_platform_set_databits(xSerialPortPlatform *port, xUInt8 value)
{
	x_return_val_if_fail(port, FALSE);

	assert(!(value < MIN_DATABITS || value > MAX_DATABITS));
	if (value != port->dcb.ByteSize)
	{
		BYTE byte_size_old = port->dcb.ByteSize;
		port->dcb.ByteSize = value;

		if (!SetCommState(port->handle, &(port->dcb)))
		{
			port->dcb.ByteSize = byte_size_old;
			return FALSE;
		}
	}
	return TRUE;
}

xBoolean x_serialport_platform_set_parity(xSerialPortPlatform *port, xSerialParity value)
{
	BYTE parity;

	x_return_val_if_fail(port, FALSE);

	assert(!(value < X_SERIAL_PARITY_NONE || value > X_SERIAL_PARITY_SPACE));

	
	switch (value)
	{
	case X_SERIAL_PARITY_NONE:
		parity = NOPARITY;
		break;
	case X_SERIAL_PARITY_ODD:
		parity = ODDPARITY;
		break;
	case X_SERIAL_PARITY_EVEN:
		parity = EVENPARITY;
		break;
	case X_SERIAL_PARITY_MARK:
		parity = MARKPARITY;
		break;
	case X_SERIAL_PARITY_SPACE:
		parity = SPACEPARITY;
		break;
	}

	if (parity != port->dcb.Parity)
	{
		BYTE parity_old = port->dcb.Parity;

		/* in the DCB structure, the parity setting also potentially effects:
		 * fParity, fErrorChar, ErrorChar
		 * so these must be saved as well.
		 */
		DWORD f_parity_old = port->dcb.fParity;
		char error_char_old = port->dcb.ErrorChar;
		DWORD f_error_char_old = port->dcb.fErrorChar;

		port->dcb.Parity = parity;
		port->dcb.fParity = parity == NOPARITY ? 0 : 1;
		if (port->dcb.fParity)
		{
			port->dcb.fErrorChar = port->parity_replace != '\0' ? TRUE : FALSE;
			port->dcb.ErrorChar = port->parity_replace;
		}
		else
		{
			port->dcb.fErrorChar = FALSE;
			port->dcb.ErrorChar = '\0';
		}

		if (!SetCommState(port->handle, &(port->dcb)))
		{
			port->dcb.Parity = parity_old;
			port->dcb.fParity = f_parity_old;
			port->dcb.ErrorChar = error_char_old;
			port->dcb.fErrorChar = error_char_old;
			return FALSE;
		}
	}

	return TRUE;
}

xBoolean x_serialport_platform_set_read_timeout(xSerialPortPlatform *port, xInt32 value)
{
	DWORD read_constant_old;
	DWORD read_interval_old;
	DWORD read_multiplier_old;

	x_return_val_if_fail(port, FALSE);

	if (value < 0 && value != X_SERIALPORT_INFINITE_TIMEOUT)
		return FALSE;

	if (port->handle == INVALID_HANDLE_VALUE)
		return FALSE;

	read_constant_old = port->comm_timeouts.ReadTotalTimeoutConstant;
	read_interval_old = port->comm_timeouts.ReadIntervalTimeout;
	read_multiplier_old = port->comm_timeouts.ReadTotalTimeoutMultiplier;

	/* NOTE: this logic should match what is in the constructor */
	if (value == 0)
	{
		port->comm_timeouts.ReadTotalTimeoutConstant = 0;
		port->comm_timeouts.ReadTotalTimeoutMultiplier = 0;
		port->comm_timeouts.ReadIntervalTimeout = MAXDWORD;
	}
	else if (value == X_SERIALPORT_INFINITE_TIMEOUT)
	{
		/* SetCommTimeouts doesn't like a value of -1 for some reason, so
		 * we'll use -2(InfiniteTimeoutConst) to represent infinite.
		 */
		port->comm_timeouts.ReadTotalTimeoutConstant = INFINITE_TIMEOUT;
		port->comm_timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
		port->comm_timeouts.ReadIntervalTimeout = MAXDWORD;
	}
	else
	{
		port->comm_timeouts.ReadTotalTimeoutConstant = value;
		port->comm_timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
		port->comm_timeouts.ReadIntervalTimeout = MAXDWORD;
	}

	if (!SetCommTimeouts(port->handle, &(port->comm_timeouts)))
	{
		port->comm_timeouts.ReadTotalTimeoutConstant = read_constant_old;
		port->comm_timeouts.ReadTotalTimeoutMultiplier = read_multiplier_old;
		port->comm_timeouts.ReadIntervalTimeout = read_interval_old;
		return FALSE;
	}

	return TRUE;
}

xInt32 x_serialport_platform_get_read_timeout(xSerialPortPlatform *port)
{
	DWORD constant;
	x_return_val_if_fail(port, -1);

	constant = port->comm_timeouts.ReadTotalTimeoutConstant;
	if (constant == INFINITE_TIMEOUT)
		return X_SERIALPORT_INFINITE_TIMEOUT;
	else
		return (xInt32)constant;
}

xBoolean x_serialport_platform_set_rts_enable(xSerialPortPlatform *port, xBoolean value)
{
	x_return_val_if_fail(port, FALSE);

	if ((port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSEND ||
		port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF))
	{
		return FALSE;
	}

	if (value != port->rts_enable)
	{
		DWORD f_rts_control_old = port->dcb.fRtsControl;

		port->rts_enable = value;
		if (value)
			port->dcb.fRtsControl = RTS_CONTROL_ENABLE;
		else
			port->dcb.fRtsControl = RTS_CONTROL_DISABLE;

		if (!SetCommState(port->handle, &(port->dcb)))
		{
			port->dcb.fRtsControl = f_rts_control_old;
			// set it back to the old value on a failure
			port->rts_enable = !port->rts_enable;
			return FALSE;
		}

		if (!EscapeCommFunction(port->handle, value ? SETRTS : CLRRTS))
			return FALSE;
	}

	return TRUE;
}

xBoolean x_serialport_platform_set_stopbits(xSerialPortPlatform *port, xSerialStopbits value)
{
	BYTE stopbits;

	x_return_val_if_fail(port, FALSE);

	assert(!(value < X_SERIAL_STOPBITS_ONE || value > X_SERIAL_STOPBITS_ONE5));

	if (value == X_SERIAL_STOPBITS_ONE)
		stopbits = ONESTOPBIT;
	else if (value == X_SERIAL_STOPBITS_ONE5)
		stopbits = ONE5STOPBITS;
	else
		stopbits = TWOSTOPBITS;

	if (stopbits != port->dcb.StopBits)
	{
		BYTE stopbits_old = port->dcb.StopBits;
		port->dcb.StopBits = stopbits;

		if (!SetCommState(port->handle, &(port->dcb)))
		{
			port->dcb.StopBits = stopbits_old;
			return FALSE;
		}
	}

	return TRUE;
}

xBoolean x_serialport_platform_set_write_timeout(xSerialPortPlatform *port, xInt32 value)
{
	DWORD write_constant_old;

	x_return_val_if_fail(port, FALSE);

	if (value <= 0 && value != X_SERIALPORT_INFINITE_TIMEOUT)
		return FALSE;
	if (port->handle == INVALID_HANDLE_VALUE)
		return FALSE;
	write_constant_old = port->comm_timeouts.WriteTotalTimeoutConstant;
	port->comm_timeouts.WriteTotalTimeoutConstant = ((value == X_SERIALPORT_INFINITE_TIMEOUT) ? 0 : (DWORD)value);

	if (!SetCommTimeouts(port->handle, &(port->comm_timeouts)))
	{
		port->comm_timeouts.WriteTotalTimeoutConstant = write_constant_old;
		return FALSE;
	}

	return TRUE;
}

xInt32 x_serialport_platform_get_write_timeout(xSerialPortPlatform *port)
{
	DWORD timeout;

	x_return_val_if_fail(port, -1);

	timeout = port->comm_timeouts.WriteTotalTimeoutConstant;
	return (timeout == 0) ? X_SERIALPORT_INFINITE_TIMEOUT : (xInt32)timeout;
}

xInt32 x_serialport_platform_bytes_to_read(xSerialPortPlatform *port)
{
	DWORD error_code;

	x_return_val_if_fail(port, -1);

	if (!ClearCommError(port->handle, &error_code, &(port->com_stat)))
		return -1;
	return (xInt32)port->com_stat.cbInQue;
}

xInt32 x_serialport_platform_bytes_to_write(xSerialPortPlatform *port)
{
	DWORD error_code;

	x_return_val_if_fail(port, -1);

	if (!ClearCommError(port->handle, &error_code, &(port->com_stat)))
		return -1;
	return (xInt32)port->com_stat.cbOutQue;
}

xBoolean x_serialport_platform_get_dtr_enable(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);

	return port->dcb.fDtrControl == DTR_CONTROL_ENABLE;
}

xBoolean x_serialport_platform_set_dtr_enable(xSerialPortPlatform *port, xBoolean value)
{
	DWORD f_dtr_control_old;

	x_return_val_if_fail(port, FALSE);
	/* first set the fDtrControl field in the DCB struct */
	f_dtr_control_old = port->dcb.fDtrControl;

	port->dcb.fDtrControl = value ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
	if (!SetCommState(port->handle, &(port->dcb)))
	{
		port->dcb.fDtrControl = f_dtr_control_old;
		return FALSE;
	}

	// then set the actual pin
	if (!EscapeCommFunction(port->handle, value ? SETDTR : CLRDTR))
		return FALSE;
	return TRUE;
}

xBoolean x_serialport_platform_set_handshake(xSerialPortPlatform *port, xSerialHandshake value)
{
	x_return_val_if_fail(port, FALSE);

	assert(!(value < X_SERIAL_HANDSHAKE_NONE || value > X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF));

	if (value != port->handshake)
	{
		/* in the DCB, handshake affects the fRtsControl, fOutxCtsFlow, and fInX, fOutX fields,
		 * so we must save everything in that closure before making any changes.
		 */
		xSerialHandshake handshake_old = port->handshake;
		DWORD f_in_out_x_old = port->dcb.fInX;
		DWORD f_out_x_cts_flow_old = port->dcb.fOutxCtsFlow;
		DWORD f_rts_control_old = port->dcb.fRtsControl;
		xBoolean f_in_x_out_x_flag;

		port->handshake = value;
		f_in_x_out_x_flag = (port->handshake == X_SERIAL_HANDSHAKE_XONXOFF || port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF) ? TRUE : FALSE;
		port->dcb.fInX = f_in_x_out_x_flag;
		port->dcb.fOutX = f_in_x_out_x_flag;
		port->dcb.fOutxCtsFlow = (port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSEND || port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF) ? TRUE : FALSE;

		if ((port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSEND || port->handshake == X_SERIAL_HANDSHAKE_REQUESTTOSENDXONXOFF))
		{
			port->dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
		}
		else if (port->rts_enable)
		{
			port->dcb.fRtsControl = RTS_CONTROL_ENABLE;
		}
		else
		{
			port->dcb.fRtsControl = RTS_CONTROL_DISABLE;
		}

		if (!SetCommState(port->handle, &(port->dcb)))
		{
			port->handshake = handshake_old;
			port->dcb.fInX = f_in_out_x_old;
			port->dcb.fOutX = f_in_out_x_old;
			port->dcb.fOutxCtsFlow = f_out_x_cts_flow_old;
			port->dcb.fRtsControl = f_rts_control_old;
			return FALSE;
		}
	}

	return TRUE;
}

xBoolean x_serialport_platform_get_rts_enable(xSerialPortPlatform *port)
{
	DWORD f_rts_control;

	x_return_val_if_fail(port, FALSE);

	f_rts_control = port->dcb.fRtsControl;
	if (f_rts_control == RTS_CONTROL_HANDSHAKE)
	{
		return FALSE;
	}
	return (f_rts_control == RTS_CONTROL_ENABLE);
}

xBoolean x_serialport_platform_discard_in_buffer(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);

	if (!PurgeComm(port->handle, PURGE_RXCLEAR | PURGE_RXABORT))
		return FALSE;
	return TRUE;
}

xBoolean x_serialport_platform_discard_out_buffer(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);

	if (!PurgeComm(port->handle, PURGE_TXCLEAR | PURGE_TXABORT))
		return FALSE;
	return TRUE;
}

xBoolean x_serialport_platform_flush(xSerialPortPlatform *port)
{
	x_return_val_if_fail(port, FALSE);

	if (port->handle != INVALID_HANDLE_VALUE)
		return FALSE;
	if (FlushFileBuffers(port->handle))
		return TRUE;
	return FALSE;
}

static xInt32 read(xSerialPortPlatform *port, xUInt8 *array, xInt32 count, xInt32 timeout)
{
	DWORD num_bytes_read;

	x_return_val_if_fail(port, -1);
	x_return_val_if_fail(array, -1);
	x_return_val_if_fail(count != 0, 0);
	x_return_val_if_fail(count > 0, -1);
	if (array == NULL) return -1; /* return immediately if no bytes requested; no need for overhead. */

	assert(timeout == X_SERIALPORT_INFINITE_TIMEOUT || timeout >= 0);

	// Check to see we have no handle-related error, since the port's always supposed to be open.
	if (port->handle == INVALID_HANDLE_VALUE)
		return -1;

	num_bytes_read = 0;

	if (!ReadFile(port->handle, (void*)array, (DWORD)count, &num_bytes_read, NULL))
	{
		DWORD hr = GetLastError();
		return -1;
	}

	return (xInt32)num_bytes_read;
}

xInt32 x_serialport_platform_read(xSerialPortPlatform *port, xUInt8 *array, xInt32 count)
{
	return read(port, array, count, x_serialport_platform_get_read_timeout(port));
}

static xInt32 write(xSerialPortPlatform *port, const xUInt8 *array, xInt32 count, xInt32 timeout)
{
	DWORD bytes_written;

	x_return_val_if_fail(port, -1);
	x_return_val_if_fail(array, -1);
	x_return_val_if_fail(count != 0, 0); /* no need to expend overhead in creating asyncResult, etc.*/
	x_return_val_if_fail(count > 0, -1);

	assert(timeout == X_SERIALPORT_INFINITE_TIMEOUT || timeout >= 0);

	/* check for open handle, though the port is always supposed to be open */
	if (port->handle = INVALID_HANDLE_VALUE)
		return -1;
	
	bytes_written = 0;
	if (!WriteFile(port->handle, (const void*)array, (DWORD)count, &bytes_written, NULL))
	{
		DWORD hr = GetLastError();
		return -1;
	}

	return (xInt32)bytes_written;
}

xInt32 x_serialport_platform_write(xSerialPortPlatform *port, const xUInt8 *array, xInt32 count)
{
	return write(port, array, count, x_serialport_platform_get_write_timeout(port));
}

xBoolean x_serialport_platform_set_buffer_sizes(xSerialPortPlatform *port, xInt32 read_buffer_size, xInt32 write_buffer_size)
{
	x_return_val_if_fail(port, FALSE);

	if (port->handle == INVALID_HANDLE_VALUE)
		return FALSE;

	if (!SetupComm(port->handle, (DWORD)read_buffer_size, (DWORD)write_buffer_size))
		return FALSE;
	return TRUE;

}
