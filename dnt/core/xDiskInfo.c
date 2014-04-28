#include "xDiskInfo.h"
#include "xArray.h"
#include "xMessage.h"
#include "xString.h"

#ifdef X_OS_WIN

#pragma warning(disable:4996)
/* Required to ensure correct PhysicalDrive IOCTL structure setup */
#pragma pack(1)

/* IOCTL commands */
#define DFP_GET_VERSION			0x00074080
#define DFP_SEND_DRIVE_COMMAND	0x0007C084
#define DFP_RECEIVE_DRIVE_DATA	0x0007C088

/*#define FILE_DEVICE_SCSI	0x0000001B */
#define IOCTL_SCSI_MINIPORT_IDENTIFY	((FILE_DEVICE_SCSI << 16) + 0x0501)

/* GETVERSIONOUTPARAMS contains the data returned from the
* Get Driver Version function
*/
typedef struct _GETVERSIONOUTPARAMS
{
	BYTE bVersion; /* Binary driver version. */
	BYTE bRevision; /* Binary driver revision. */
	BYTE bReserved; /* Not used. */
	BYTE bIDEDeviceMap; /* Bit map of IDE devices. */
	DWORD fCapabilities; /* Bit mask of driver capabilities. */
	DWORD dwReserved[4]; /* For future use. */
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;

/* Bits returned in the fCapabilities member of GETVERSIONOUTPARAMS*/
#define CAP_IDE_ID_FUNCTION				1 /* ATA ID command supported */
#define CAP_IDE_ATAPI_ID				2 /* ATAPI ID command supported */
#define CAP_IDE_EXECUTE_SMART_FUNCTION	4 /* SMART commands supported */

/* Valid values for the bCommandReg member of IDEREGS. */
#define IDE_ATAPI_IDENTIFY	0xA1 /* Returns ID sector for ATAPI. */
#define IDE_ATA_IDENTIFY	0xEC /* Returns ID sector for ATA. */

/* The following struct defines the interesting part of the IDENTIFY
* buffer:
*/
typedef struct _IDSECTOR
{
	USHORT wGenConfig;
	USHORT wNumCyls;
	USHORT wReserved;
	USHORT wNumHeads;
	USHORT wBytesPerTrack;
	USHORT wBytesPerSector;
	USHORT wSectorsPerTrack;
	USHORT wVendorUnique[3];
	CHAR sSerialNumber[20];
	USHORT wBufferType;
	USHORT wBufferSize;
	USHORT wECCSize;
	CHAR sFirmwareRev[8];
	CHAR sModelNumber[40];
	USHORT wMoreVendorUnique;
	USHORT wDoubleWordIO;
	USHORT wCapabilities;
	USHORT wReserved1;
	USHORT wPIOTiming;
	USHORT wDMATiming;
	USHORT wBS;
	USHORT wNumCurrentCyls;
	USHORT wNumCurrentHeads;
	USHORT wNumCurrentSectorsPerTrack;
	ULONG ulCurrentSectorCapacity;
	USHORT wMultSectorStuff;
	ULONG ulTotalAddressableSectors;
	USHORT wSingleWordDMA;
	USHORT wMultiWordDMA;
	BYTE bReserved[128];
} IDSECTOR, *PIDSECTOR;

/*
* IDENTIFY data (from ATAPI driver source)
*/
typedef struct _IDENTIFY_DATA
{
	USHORT GeneralConfiguration; /*00 00*/
	USHORT NumberOfCylinders; /*02  1*/
	USHORT Reserved1; /*04  2*/
	USHORT NumberOfHeads; /*06  3*/
	USHORT UnformattedBytesPerTrack; /*08  4*/
	USHORT UnformattedBytesPerSector; /*0A  5*/
	USHORT SectorsPerTrack; /*0C  6*/
	USHORT VendorUnique1[3]; /*0E  7-9*/
	USHORT SerialNumber[10]; /*14  10-19*/
	USHORT BufferType; /*28  20*/
	USHORT BufferSectorSize; /*2A  21*/
	USHORT NumberOfEccBytes; /*2C  22*/
	USHORT FirmwareRevision[4]; /*2E  23-26*/
	USHORT ModelNumber[20]; /*36  27-46*/
	UCHAR MaximumBlockTransfer; /*5E  47*/
	UCHAR VendorUnique2; /*5F*/
	USHORT DoubleWordIo; /*60  48*/
	USHORT Capabilities; /*62  49*/
	USHORT Reserved2; /*64  50*/
	UCHAR VendorUnique3; /*66  51*/
	UCHAR PioCycleTimingMode; /*67*/
	UCHAR VendorUnique4; /*68  52*/
	UCHAR DmaCycleTimingMode; /*69*/
	USHORT TranslationFieldsValid : 1; /*6A  53*/
	USHORT Reserved3 : 15;
	USHORT NumberOfCurrentCylinders; /*6C  54*/
	USHORT NumberOfCurrentHeads; /*6E  55*/
	USHORT CurrentSectorsPerTrack; /*70  56*/
	ULONG CurrentSectorCapacity; /*72  57-58*/
	USHORT CurrentMultiSectorSetting; /*59*/
	ULONG UserAddressableSectors; /*60-61*/
	USHORT SingleWordDMASupport : 8; /*62*/
	USHORT SingleWordDMAActive : 8;
	USHORT MultiWordDMASupport : 8; /*63*/
	USHORT MultiWordDMAActive : 8;
	USHORT AdvancedPIOModes : 8; /*64*/
	USHORT Reserved4 : 8;
	USHORT MinimumMWXferCycleTime; /*65*/
	USHORT RecommendedMWXferCycleTime; /*66*/
	USHORT MinimumPIOCycleTime; /*67*/
	USHORT MinimumPIOCycleTimeIORDY; /*68*/
	USHORT Reserved5[2]; /*69-70*/
	USHORT ReleaseTimeOverlapped; /*71*/
	USHORT ReleaseTimeServiceCommand; /*72*/
	USHORT MajorRevision; /*73*/
	USHORT MinorRevision; /*74*/
	USHORT Reserved6[50]; /*75-126*/
	USHORT SpecialFunctionsEnabled; /*127*/
	USHORT Reserved7[128]; /*128-255*/
} IDENTIFY_DATA, *PIDENTIFY_DATA;

#pragma pack()

/* Required to ensure correct PhysicalDrive IOCTL structure setup */
#pragma pack(4)

/*
*	IOCTL_STORAGE_QUERY_PROPERTY
*
*	Input Buffer:
*		a STORAGE_PROPERTY_QUERY structure which describes what type of query
*		is being done, what property is being queried for, and any additional
*		parameters which a particular property query requires.
*
*	Output Buffer:
*		Contains a buffer to place the results of the query into.  Since all
*		property descriptors can be cast into a STORAGE_DESCRIPTOR_HEADER,
*		the IOCTL can be called once with a small buffer then again using
*		a buffer as large as the header reports is necessary.
*/

/*
*Types of queries
*/

/*
* define some initial property id's
*/

/*
* Query structure - additional parameters for specific queries can follow
* the header
*/

/*
* Device property descriptor - this is really just a rehash of the inquiry
* data retrieved from a scsi device
*
* This may only be retrieved from a target device.  Sending this to the bus
* will result in an error
*/
typedef struct _MEDIA_SERIAL_NUMBER_DATA
{
	ULONG SerialNumberLength;
	ULONG Result;
	ULONG Reserved[2];
	UCHAR SerialNumberData[];
} MEDIA_SERIAL_NUMBER_DATA, *PMEDIA_SERIAL_NUMBER_DATA;

/***************************************************************************/

/* (* Output Bbuffer for the VxD (rt_IdeDinfo record) *) */
typedef struct _rt_IdeDInfo_
{
	BYTE IDEExists[4];
	BYTE DiskExists[8];
	WORD DisksRawInfo[8 * 256];
} rt_IdeDInfo, *pt_IdeDInfo;

/* (* IdeDinfo "data fields" *) */
typedef struct _rt_DiskInfo_
{
	BOOL DiskExists;
	BOOL ATAdevice;
	BOOL RemovableDevice;
	WORD TotLogCyl;
	WORD TotLogHeads;
	WORD TotLogSPT;
	char SerialNumber[20];
	char FirmwareRevision[8];
	char ModelNumber[40];
	WORD CurLogCyl;
	WORD CurLogHeads;
	WORD CurLogSPT;
} rt_DiskInfo;

#define m_cVxDFunctionIdesDInfo 1
#define SENDIDLENGTH (sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE)
#define MAX_IDE_DRIVES 16

static xPtrArray *list = NULL;

void x_disk_info_init(void)
{
	list = x_ptr_array_new_with_free_func(free);
}

static void list_clear()
{
	x_return_if_fail(list);
	x_ptr_array_set_size(list, 0);
}

static xString* convert_to_string(size_t drive, int first_index, int last_index)
{
	int i;
	xString * str = x_string_sized_new(100);

	/* Each integer has two characters stored in it backwards */
	for (i = first_index; i <= last_index; i++)
	{
		xChar c;
		/* get high byte for list character */
		PDWORD value = (PDWORD)x_ptr_array_index(list, drive);
		c = HIBYTE(value[i]);
		if (isalnum(c))
			x_string_append_c(str, c);

		/* get low byte for 2nd character */
		c = LOBYTE(value[i]);
		if (isalnum(c))
			x_string_append_c(str, c);
	}

	return str;
}

/* do_indentify
 * FUNCTION: Send an IDENTIFY command to the drive
 * bDriveNum = 0 - 3
 * bIDCmd = IDE_ATA_IDENTIFY or IDE_ATAPI_IDENTIFY
 */
static BOOL do_identify(HANDLE hPhysicalDriveIOCTL,
	PSENDCMDINPARAMS pSCIP,
	PSENDCMDOUTPARAMS pSCOP,
	BYTE bIDCmd,
	BYTE bDriveNum,
	PDWORD lpcbBytesReturned)
{
	/* set up data structures for IDENTIFY command */
	pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;
	pSCIP->irDriveRegs.bFeaturesReg = 0;
	pSCIP->irDriveRegs.bSectorCountReg = 1;
	pSCIP->irDriveRegs.bSectorNumberReg = 1;
	pSCIP->irDriveRegs.bCylLowReg = 0;
	pSCIP->irDriveRegs.bCylHighReg = 0;

	/* Compute the drive number. */
	pSCIP->irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);

	/* The command can either be IDE identify or ATAPI identify. */
	pSCIP->irDriveRegs.bCommandReg = bIDCmd;
	pSCIP->bDriveNumber = bDriveNum;

	return DeviceIoControl(hPhysicalDriveIOCTL,
		DFP_RECEIVE_DRIVE_DATA,
		(LPVOID)pSCIP,
		sizeof (SENDCMDINPARAMS)-1,
		(LPVOID)pSCOP,
		sizeof (SENDCMDOUTPARAMS)+IDENTIFY_BUFFER_SIZE - 1,
		lpcbBytesReturned,
		NULL);
}

static BOOL add_if_new(PUSHORT pIdSector)
{
	BOOL bAdd = TRUE;
	UINT i;
	for (i = 0; i < list->len; i++)
	{
		if (memcmp(pIdSector, x_ptr_array_index(list, i), 256 * sizeof(WORD)) == 0)
		{
			bAdd = FALSE;
			break;
		}
	}

	if (bAdd)
	{
		PWORD diskdata = malloc(sizeof(WORD)* 256);
		memcpy(diskdata, pIdSector, 256 * sizeof(WORD));
		x_ptr_array_add(list, diskdata);
	}

	return bAdd;
}

/* function to decode the serial numbers of IDE hard drives
 * using the IOCTL_STORAGE_QUERY_PROPERTY command
 */
static xString* flip_and_code_bytes(const xString *str)
{
	xString *flipped = x_string_sized_new(100);
	xSize num = str->len;
	xSize i;
	xSize j;
	xSize k;

	for (i = 0; i < num; i++)
	{
		for (j = 1; j >= 0; --j)
		{
			int sum = 0;
			for (k = 0; k < 2; k++)
			{
				sum *= 16;
				switch (str->str[i + j * 2 + k])
				{
				case '0': sum += 0; break;
				case '1': sum += 1; break;
				case '2': sum += 2; break;
				case '3': sum += 3; break;
				case '4': sum += 4; break;
				case '5': sum += 5; break;
				case '6': sum += 6; break;
				case '7': sum += 7; break;
				case '8': sum += 8; break;
				case '9': sum += 9; break;
				case 'A':
				case 'a': sum += 10; break;
				case 'B':
				case 'b': sum += 11; break;
				case 'C':
				case 'c': sum += 12; break;
				case 'D':
				case 'd': sum += 13; break;
				case 'E':
				case 'e': sum += 14; break;
				case 'F':
				case 'f': sum += 15; break;
				}
			}

			if (sum > 0)
				x_string_append_c(flipped, (xChar)num);
		}
	}

	return flipped;
}

static BOOL read_physical_drive_in_nt_using_smart()
{
	BOOL done = FALSE;
	int drive = 0;
	char drive_name[128];

	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		HANDLE hPhysicalDriveIOCTL = 0;

		// Try to get a handle to PhysicalDrive IOCTL, report failure
		// and exit if can't.
		sprintf(drive_name, "\\\\.\\PhysicalDrive%d", drive);

		// Windows NT, Windows 2000, Windows Server 2003, Vista
		hPhysicalDriveIOCTL = CreateFileA(drive_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
		{
		}
		else
		{
			GETVERSIONINPARAMS GetVersionParams;
			DWORD cbBytesReturned = 0;

			// Get the version, etc of PhysicalDrive IOCTL
			memset((void*)&GetVersionParams, 0, sizeof (GetVersionParams));
			if (DeviceIoControl(hPhysicalDriveIOCTL, SMART_GET_VERSION, NULL, 0, &GetVersionParams, sizeof (GETVERSIONINPARAMS), &cbBytesReturned, NULL))
			{
				// Allocate the command buffer
				ULONG CommandSize = sizeof (SENDCMDINPARAMS)+IDENTIFY_BUFFER_SIZE;
				PSENDCMDINPARAMS Command = (PSENDCMDINPARAMS)malloc(CommandSize);
				DWORD BytesReturned = 0;
				// Retrieve the IDENTIFY data
				// Prepare the command
#define ID_CMD 0xEC // Returns ID sector for ATA
				Command->irDriveRegs.bCommandReg = ID_CMD;
				if (DeviceIoControl(hPhysicalDriveIOCTL, SMART_RCV_DRIVE_DATA, Command, sizeof (SENDCMDINPARAMS), Command, CommandSize, &BytesReturned, NULL))
				{
					// Print the IDENTIFY data
					// DWORD diskdata[256];
					USHORT *pIdSector = (USHORT *)(PIDENTIFY_DATA)((PSENDCMDOUTPARAMS)Command)->bBuffer;
					add_if_new(pIdSector);
					done = TRUE;
				}
				// Done
				CloseHandle(hPhysicalDriveIOCTL);
				free(Command);
			}
		}
	}
	return done;
}

static BOOL read_physical_drive_in_nt_with_admin_rights()
{
	BOOL done = FALSE;
	int drive = 0;
	BYTE IdOutCmd[sizeof (SENDCMDOUTPARAMS)+IDENTIFY_BUFFER_SIZE - 1];
	char drive_name[128];

	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		HANDLE hPhysicalDriveIOCTL = 0;

		// Try to get a handle to PhysicalDrive IOCTL, report failure
		// and exit if can't.
		sprintf(drive_name, "\\\\.\\PhysicalDrive%d", drive);

		// Windows NT, Windows 2000, must have admin rights
		hPhysicalDriveIOCTL = CreateFileA(drive_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			GETVERSIONOUTPARAMS VersionParams;
			DWORD cbBytesReturned = 0;

			// Get the version, etc of PhysicalDrive IOCTL
			memset((void*)&VersionParams, 0, sizeof (VersionParams));

			if (DeviceIoControl(hPhysicalDriveIOCTL, DFP_GET_VERSION, NULL, 0, &VersionParams, sizeof (VersionParams), &cbBytesReturned, NULL))
			{
				// If there is a IDE device at number "i" issue commands
				// to the device
				if (VersionParams.bIDEDeviceMap > 0)
				{
					BYTE bIDCmd = 0; // IDE or ATAPI IDENTIFY cmd
					SENDCMDINPARAMS scip;

					// Now, get the ID sector for all IDE devices in the system.
					// If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
					// otherwise use the IDE_ATA_IDENTIFY command
					bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

					memset(&scip, 0, sizeof (scip));
					memset(IdOutCmd, 0, sizeof (IdOutCmd));

					if (do_identify(hPhysicalDriveIOCTL, &scip, (PSENDCMDOUTPARAMS)& IdOutCmd, (BYTE)bIDCmd, (BYTE)drive, &cbBytesReturned))
					{
						USHORT *pIdSector = (USHORT *)((PSENDCMDOUTPARAMS)IdOutCmd)->bBuffer;
						add_if_new(pIdSector);
						done = TRUE;
					}
				}
			}
			CloseHandle(hPhysicalDriveIOCTL);
		}
	}

	return done;
}

static BOOL read_physical_drive_in_nt_with_zero_rights()
{
	BOOL done = FALSE;
	int drive = 0;
	char drive_name[128];

	for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
	{
		HANDLE hPhysicalDriveIOCTL = 0;

		// Try to get a handle to PhysicalDrive IOCTL, report failure
		// and exit if can't.
		sprintf(drive_name, "\\\\.\\PhysicalDrive%d", drive);

		// Windows NT, Windows 2000, Windows XP - admin rights not required
		hPhysicalDriveIOCTL = CreateFileA(drive_name, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			STORAGE_PROPERTY_QUERY query;
			DWORD cbBytesReturned = 0;
			char buffer[10000];

			memset((void*)&query, 0, sizeof (query));
			query.PropertyId = StorageDeviceProperty;
			query.QueryType = PropertyStandardQuery;

			memset(buffer, 0, sizeof (buffer));

			if (DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof (query), &buffer, sizeof (buffer), &cbBytesReturned, NULL))
			{
				STORAGE_DEVICE_DESCRIPTOR *descrip = (STORAGE_DEVICE_DESCRIPTOR*)& buffer;
				xString *temp = x_string_new(&buffer[descrip->SerialNumberOffset]);
				xString* serial_number = flip_and_code_bytes(temp);
				x_string_free(serial_number, TRUE);
				x_string_free(temp, TRUE);
			}
			else
			{
				DWORD err = GetLastError();
				(void)err;
			}

			memset(buffer, 0, sizeof (buffer));

			if (DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER, NULL, 0, &buffer, sizeof (buffer), &cbBytesReturned, NULL))
			{
				MEDIA_SERIAL_NUMBER_DATA * mediaSerialNumber = (MEDIA_SERIAL_NUMBER_DATA *)& buffer;
				//std::string serialNumber((char*)mediaSerialNumber->SerialNumberData);
			}
			else
			{
				DWORD err = GetLastError();
				(void)err;
			}
			CloseHandle(hPhysicalDriveIOCTL);
		}
	}
	return done;
}

static BOOL read_drive_ports_in_win9x()
{
	BOOL done = FALSE;

	HANDLE VxDHandle = 0;
	pt_IdeDInfo pOutBufVxD = 0;
	DWORD lpBytesReturned = 0;

	unsigned long int i;

	// set the thread priority high so that we get exclusive access to the disk
	BOOL status = SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	(void)status;

	/// 1. Make an output buffer for the VxD
	rt_IdeDInfo info;
	pOutBufVxD = &info;

	// *****************
	// KLUDGE WARNING!!!
	// HAVE to zero out the buffer space for the IDE information!
	// If this is NOT done then garbage could be in the memory
	// locations indicating if a disk exists or not.'
	ZeroMemory(&info, sizeof (info));

	// 1. Try to load the VxD
	//  must use the short file name path to open a VXD file
	//char StartupDirectory [2048];
	//char shortFileNamePath [2048];
	//char *p = NULL;
	//char vxd [2048];
	//  get the directory that the exe was started from
	//GetModuleFileName (hInst, (LPSTR) StartupDirectory, sizeof (StartupDirectory));
	//  cut the exe name from std::string
	//p = &(StartupDirectory [strlen (StartupDirectory) - 1]);
	//while (p >= StartupDirectory && *p && '\\' != *p) p--;
	//*p = '\0';
	//GetShortPathName (StartupDirectory, shortFileNamePath, 2048);
	//sprintf (vxd, "\\\\.\\%s\\IDE21201.VXD", shortFileNamePath);
	//VxDHandle = CreateFile (vxd, 0, 0, 0,
	//               0, FILE_FLAG_DELETE_ON_CLOSE, 0);
	VxDHandle = CreateFileA("\\\\.\\IDE21201.VXD", 0, 0, 0, 0, FILE_FLAG_DELETE_ON_CLOSE, 0);

	if (VxDHandle != INVALID_HANDLE_VALUE)
	{
		// 2. Run VxD function
		DeviceIoControl(VxDHandle, m_cVxDFunctionIdesDInfo, 0, 0, pOutBufVxD, sizeof (pt_IdeDInfo), &lpBytesReturned, 0);

		// 3. Unload VxD
		CloseHandle(VxDHandle);
	}
	else return FALSE;

	// 4. Translate and store
	for (i = 0; i < 8; i++)
	{
		if ((pOutBufVxD->DiskExists[i]) && (pOutBufVxD->IDEExists[i / 2]))
		{
			size_t j;
			BOOL bAdd;
			PWORD diskdata = malloc(sizeof(WORD)* 256);
			for (j = 0; j < 256; j++) diskdata[j] = pOutBufVxD->DisksRawInfo[i * 256 + j];

			// process the information for this buffer
			bAdd = TRUE;
			for (j = 0; j < list->len; j++)
			{
				if (memcmp(diskdata, x_ptr_array_index(list, j), 256 * sizeof (WORD)) == 0)
				{
					bAdd = FALSE;
					break;
				}
			}
			if (bAdd)
				x_ptr_array_add(list, diskdata);
			done = TRUE;
		}
	}

	//  reset the thread priority back to normal
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	return done;
}

static BOOL read_ide_drive_as_scsi_drive_in_nt()
{
	BOOL done = FALSE;
	int controller = 0;
	char drive_name[128];

	for (controller = 0; controller < 16; controller++)
	{
		HANDLE hScsiDriveIOCTL = 0;

		//  Try to get a handle to PhysicalDrive IOCTL, report failure
		//  and exit if can't.
		sprintf(drive_name, "\\\\.\\Scsi%d:", controller);

		//  Windows NT, Windows 2000, any rights should do
		hScsiDriveIOCTL = CreateFileA(drive_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
		{
			int drive = 0;
			char *buffer = (char*)malloc(sizeof (SRB_IO_CONTROL)+SENDIDLENGTH);

			for (drive = 0; drive < 2; drive++)
			{
				SRB_IO_CONTROL *p = (SRB_IO_CONTROL *)buffer;
				SENDCMDINPARAMS *pin = (SENDCMDINPARAMS *)(buffer + sizeof (SRB_IO_CONTROL));
				DWORD dummy;

				memset(buffer, 0, sizeof (buffer));
				p->HeaderLength = sizeof (SRB_IO_CONTROL);
				p->Timeout = 10000;
				p->Length = SENDIDLENGTH;
				p->ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
				strncpy((char *)p->Signature, "SCSIDISK", 8);

				pin->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
				pin->bDriveNumber = (BYTE)drive;

				if (DeviceIoControl(hScsiDriveIOCTL,
					IOCTL_SCSI_MINIPORT,
					buffer,
					sizeof (SRB_IO_CONTROL)+sizeof (SENDCMDINPARAMS)-1,
					buffer,
					sizeof (SRB_IO_CONTROL)+SENDIDLENGTH,
					&dummy,
					NULL))
				{
					SENDCMDOUTPARAMS *pOut = (SENDCMDOUTPARAMS *)(buffer + sizeof (SRB_IO_CONTROL));
					IDSECTOR *pId = (IDSECTOR *)(pOut->bBuffer);
					if (pId->sModelNumber[0])
					{
						USHORT *pIdSector = (USHORT *)pId;
						add_if_new(pIdSector);
						//BOOL bAdd = TRUE;
						//  for(UINT i =0; i< list.size();i++)
						//  {
						//   if(memcmp(pIdSector,list[i],256 * sizeof(WORD)) == 0)
						//   {
						//	   bAdd = false;
						//	   break;
						//   }
						//  }
						//  if(bAdd)
						//  {
						//    WORD* diskdata = new WORD[256];
						//	 ::memcpy(diskdata,pIdSector,256*sizeof(WORD));
						//	 list.push_back(diskdata);
						//  }

						done = TRUE;
					}
				}
			}
			CloseHandle(hScsiDriveIOCTL);
			free(buffer);
		}
	}
	return done;
}

xString* x_disk_info_get_model_number(xSize drive)
{
	return convert_to_string(drive, 27, 46);
}

xString* x_disk_info_get_serial_number(xSize drive)
{
	return convert_to_string(drive, 10, 19);
}

xString* x_disk_info_get_revision_number(xSize drive)
{
	return convert_to_string(drive, 23, 26);
}

xString* x_disk_info_get_drive_type(xSize drive)
{
	WORD type = ((PWORD)x_ptr_array_index(list, drive))[0];
	if (type & 0x0080) return x_string_new("Removable");
	else if (type & 0x0040) return x_string_new("Fixed");
	else return x_string_new("Unknown");
}

xInt64 x_disk_info_get_drive_size(xSize drive)
{
	xUInt64 bytes = 0, sectors = 0;
	PWORD p = (PWORD)x_ptr_array_index(list, drive);

	if (p[83] & 0x400)
	{
		sectors =
			p[103] * (xUInt64)65536 * (xUInt64)65536 * (xUInt64)65536 +
			p[102] * (xUInt64)65536 * (xUInt64)65536 +
			p[101] * (xUInt64)65536 +
			p[100];
	}
	else
	{
		sectors =
			p[61] * (xUInt64)65536 +
			p[60];
	}

	/* there are 512 bytes in a sector */
	bytes = sectors * 512;
	return bytes;
}

xInt32 disk_info_load()
{
	OSVERSIONINFO version;
	memset(&version, 0, sizeof (OSVERSIONINFO));
	version.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	GetVersionEx(&version);

	if (list != NULL)
		x_ptr_array_set_size(list, 0);
	else
		list = x_ptr_array_new_with_free_func(free);

	if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		//  this works under WinNT4 or Win2K if you have admin rights
		BOOL done = read_physical_drive_in_nt_with_admin_rights();

		//  this should work in WinNT or Win2K if previous did not work
		//  this is kind of a backdoor via the SCSI mini port driver into
		//     the IDE drives
		done = read_ide_drive_as_scsi_drive_in_nt();

		//this works under WinNT4 or Win2K or WinXP if you have any rights
		//done = read_physical_drive_in_nt_with_zero_rights ();
		done = read_physical_drive_in_nt_using_smart();
	}
	else
	{
		//  this works under Win9X and calls a VXD
		BOOL done = FALSE;
		//  try this up to 10 times to get a hard drive serial number
		for (int attempt = 0; attempt < 10 && !done; ++attempt) done = read_drive_ports_in_win9x();
	}
	return (xInt32)list->len;
}

void x_disk_info_unload(void)
{
	x_ptr_array_free(list, TRUE);
	list = NULL;
}

#else

xSize x_disk_info_load(void)
{
	return 0;
}

void x_disk_info_unload(void)
{
}

xInt64 x_disk_info_get_drive_size(xSize drive)
{
	return 0;
}

xString* x_disk_info_get_drive_type(xSize drive)
{
	return NULL;
}

xString* x_disk_info_get_revision_number(xSize drive)
{
	return NULL;
}

xString* x_disk_info_get_serial_number(xSize drive)
{
	return NULL;
}

xString* x_disk_info_get_model_number(xSize drive)
{
	return NULL;
}

#endif
