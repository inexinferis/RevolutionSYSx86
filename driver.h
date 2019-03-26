#ifndef DRIVER_H_INCLUDED
#define DRIVER_H_INCLUDED

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/winddi.h>
#include "pstypes.h"

#ifndef PACKED
#define PACKED __attribute__ ((packed))
#endif

NTKERNELAPI HANDLE NTAPI PsGetThreadId(IN PETHREAD Thread);

#pragma pack(push,1)

typedef struct PACKED _SSMSG{
  DWORD uBuffSize;
  BITMAPINFO bmi;
  BYTE  pBuffer[];
}SSMSG,*PSSMSG;

typedef struct PACKED _DRIVERMSG{
  DWORD NtDICFOffset;
  DWORD NtQSIOffset;
  DWORD NtOFOffset;
  DWORD NtGCTOffset;
  DWORD NtQITOffset;
  DWORD NtQVMOffset;
  DWORD NtGBBOffset;
  DWORD NtGSDIBTDIOffset;
}DRIVERMSG,*PDRIVERMSG;

typedef struct _NTCSStruct{
  PHANDLE SectionHandle;
  ULONG DesiredAccess;
  POBJECT_ATTRIBUTES ObjectAttributes;
  PLARGE_INTEGER MaximumSize;
  ULONG PageAttributess;
  ULONG SectionAttributes;
  HANDLE FileHandle;
}NTCSStruct,*PNTCSStruct;

typedef struct _NTCTStruct{
  PHANDLE ThreadHandle;
  ACCESS_MASK DesiredAccess;
  POBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE ProcessHandle;
  PCLIENT_ID ClientId;
  PCONTEXT ThreadContext;
  PINITIAL_TEB InitialTeb;
  BOOLEAN CreateSuspended;
}NTCTStruct,*PNTCTStruct;

#pragma pack(pop)

//===========================================================
//NtDeviceIoControlFile
//===========================================================

// Required to ensure correct PhysicalDrive IOCTL structure
#pragma pack(push,4)

typedef enum _STORAGE_QUERY_TYPE {
  PropertyStandardQuery = 0,
  PropertyExistsQuery,
  PropertyMaskQuery,
  PropertyQueryMaxDefined
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

typedef enum _STORAGE_PROPERTY_ID {
  StorageDeviceProperty = 0,
  StorageAdapterProperty
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

typedef struct _STORAGE_PROPERTY_QUERY {
  STORAGE_PROPERTY_ID PropertyId;
  STORAGE_QUERY_TYPE QueryType;
  UCHAR AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

typedef enum _STORAGE_BUS_TYPE {
  BusTypeUnknown = 0x00,
  BusTypeScsi,
  BusTypeAtapi,
  BusTypeAta,
  BusType1394,
  BusTypeSsa,
  BusTypeFibre,
  BusTypeUsb,
  BusTypeRAID,
  BusTypeMaxReserved = 0x7F
} STORAGE_BUS_TYPE, *PSTORAGE_BUS_TYPE;

typedef struct _STORAGE_DEVICE_DESCRIPTOR {
  DWORD Version;
  DWORD Size;
  BYTE DeviceType;
  BYTE DeviceTypeModifier;
  BOOLEAN RemovableMedia;
  BOOLEAN CommandQueueing;
  DWORD VendorIdOffset;
  DWORD ProductIdOffset;
  DWORD ProductRevisionOffset;
  DWORD SerialNumberOffset;
  STORAGE_BUS_TYPE BusType;
  DWORD RawPropertiesLength;
  BYTE RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

#pragma pack(pop)

// Required to ensure correct PhysicalDrive SCSI

#pragma pack(push,8)

typedef struct _SCSI_PASS_THROUGH {
  USHORT  Length;
  UCHAR  ScsiStatus;
  UCHAR  PathId;
  UCHAR  TargetId;
  UCHAR  Lun;
  UCHAR  CdbLength;
  UCHAR  SenseInfoLength;
  UCHAR  DataIn;
  ULONG  DataTransferLength;
  ULONG  TimeOutValue;
  ULONG_PTR DataBufferOffset;
  ULONG  SenseInfoOffset;
  UCHAR  Cdb[16];
}SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;

#define NSM_SERIAL_NUMBER_LENGTH        12

typedef struct _SERIALNUMBER {
  UCHAR DeviceType : 5;
  UCHAR PeripheralQualifier : 3;
  UCHAR PageCode;
  UCHAR Reserved;
  UCHAR PageLength;
  UCHAR SerialNumber[NSM_SERIAL_NUMBER_LENGTH];
} SERIALNUMBER, *PSERIALNUMBER;

#pragma pack(pop)

// Required to ensure correct PhysicalDrive SMART

#pragma pack(push,1)

typedef struct _IDEREGS {
  UCHAR bFeaturesReg;
  UCHAR bSectorCountReg;
  UCHAR bSectorNumberReg;
  UCHAR bCylLowReg;
  UCHAR bCylHighReg;
  UCHAR bDriveHeadReg;
  UCHAR bCommandReg;
  UCHAR bReserved;
} IDEREGS, *PIDEREGS, *LPIDEREGS;

typedef struct _SENDCMDINPARAMS {
  ULONG cBufferSize;
  IDEREGS irDriveRegs;
  UCHAR bDriveNumber;
  UCHAR bReserved[3];
  ULONG dwReserved[4];
  UCHAR bBuffer[1];
} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;

typedef struct _DRIVERSTATUS {
  UCHAR bDriverError;
  UCHAR bIDEError;
  UCHAR bReserved[2];
  ULONG dwReserved[2];
} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;

typedef struct _SENDCMDOUTPARAMS {
  ULONG cBufferSize;
  DRIVERSTATUS DriverStatus;
  UCHAR bBuffer[1];
} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;

typedef struct _IDENTIFY_DEVICE_DATA {
  struct {
    USHORT  Reserved1 : 1;
    USHORT  Retired3 : 1;
    USHORT  ResponseIncomplete : 1;
    USHORT  Retired2 : 3;
    USHORT  FixedDevice : 1;
    USHORT  RemovableMedia : 1;
    USHORT  Retired1 : 7;
    USHORT  DeviceType : 1;
  } GeneralConfiguration; // word 0
  USHORT  NumCylinders; // word 1
  USHORT  ReservedWord2;
  USHORT  NumHeads; // word 3
  USHORT  Retired1[2];
  USHORT  NumSectorsPerTrack; // word 6
  USHORT  VendorUnique1[3];
  UCHAR   SerialNumber[20]; // word 10-19
  USHORT  Retired2[2];
  USHORT  Obsolete1;
  UCHAR  FirmwareRevision[8]; // word 23-26
  UCHAR  ModelNumber[40]; // word 27-46
  UCHAR  MaximumBlockTransfer; // word 47
  UCHAR  VendorUnique2;
  USHORT  ReservedWord48;
  struct {
    UCHAR  ReservedByte49;
    UCHAR  DmaSupported : 1;
    UCHAR  LbaSupported : 1;
    UCHAR  IordyDisable : 1;
    UCHAR  IordySupported : 1;
    UCHAR  Reserved1 : 1;
    UCHAR  StandybyTimerSupport : 1;
    UCHAR  Reserved2 : 2;
    USHORT  ReservedWord50;
  } Capabilities; // word 49-50
  USHORT  ObsoleteWords51[2];
  USHORT  TranslationFieldsValid:3; // word 53
  USHORT  Reserved3:13;
  USHORT  NumberOfCurrentCylinders; // word 54
  USHORT  NumberOfCurrentHeads; // word 55
  USHORT  CurrentSectorsPerTrack; // word 56
  ULONG  CurrentSectorCapacity; // word 57
  UCHAR  CurrentMultiSectorSetting; // word 58
  UCHAR  MultiSectorSettingValid : 1;
  UCHAR  ReservedByte59 : 7;
  ULONG  UserAddressableSectors; // word 60-61
  USHORT  ObsoleteWord62;
  USHORT  MultiWordDMASupport : 8; // word 63
  USHORT  MultiWordDMAActive : 8;
  USHORT  AdvancedPIOModes : 8;
  USHORT  ReservedByte64 : 8;
  USHORT  MinimumMWXferCycleTime;
  USHORT  RecommendedMWXferCycleTime;
  USHORT  MinimumPIOCycleTime;
  USHORT  MinimumPIOCycleTimeIORDY;
  USHORT  ReservedWords69[6];
  USHORT  QueueDepth : 5;
  USHORT  ReservedWord75 : 11;
  USHORT  ReservedWords76[4];
  USHORT  MajorRevision;
  USHORT  MinorRevision;
  struct {
    USHORT  SmartCommands : 1;
    USHORT  SecurityMode : 1;
    USHORT  RemovableMedia : 1;
    USHORT  PowerManagement : 1;
    USHORT  Reserved1 : 1;
    USHORT  WriteCache : 1;
    USHORT  LookAhead : 1;
    USHORT  ReleaseInterrupt : 1;
    USHORT  ServiceInterrupt : 1;
    USHORT  DeviceReset : 1;
    USHORT  HostProtectedArea : 1;
    USHORT  Obsolete1 : 1;
    USHORT  WriteBuffer : 1;
    USHORT  ReadBuffer : 1;
    USHORT  Nop : 1;
    USHORT  Obsolete2 : 1;
    USHORT  DownloadMicrocode : 1;
    USHORT  DmaQueued : 1;
    USHORT  Cfa : 1;
    USHORT  AdvancedPm : 1;
    USHORT  Msn : 1;
    USHORT  PowerUpInStandby : 1;
    USHORT  ManualPowerUp : 1;
    USHORT  Reserved2 : 1;
    USHORT  SetMax : 1;
    USHORT  Acoustics : 1;
    USHORT  BigLba : 1;
    USHORT  Resrved3 : 5;
  } CommandSetSupport; // word 82-83
  USHORT  ReservedWord84;
  struct {
    USHORT  SmartCommands : 1;
    USHORT  SecurityMode : 1;
    USHORT  RemovableMedia : 1;
    USHORT  PowerManagement : 1;
    USHORT  Reserved1 : 1;
    USHORT  WriteCache : 1;
    USHORT  LookAhead : 1;
    USHORT  ReleaseInterrupt : 1;
    USHORT  ServiceInterrupt : 1;
    USHORT  DeviceReset : 1;
    USHORT  HostProtectedArea : 1;
    USHORT  Obsolete1 : 1;
    USHORT  WriteBuffer : 1;
    USHORT  ReadBuffer : 1;
    USHORT  Nop : 1;
    USHORT  Obsolete2 : 1;
    USHORT  DownloadMicrocode : 1;
    USHORT  DmaQueued : 1;
    USHORT  Cfa : 1;
    USHORT  AdvancedPm : 1;
    USHORT  Msn : 1;
    USHORT  PowerUpInStandby : 1;
    USHORT  ManualPowerUp : 1;
    USHORT  Reserved2 : 1;
    USHORT  SetMax : 1;
    USHORT  Acoustics : 1;
    USHORT  BigLba : 1;
    USHORT  Resrved3 : 5;
  } CommandSetActive; // word 85-86
  USHORT  ReservedWord87;
  USHORT  UltraDMASupport : 8; // word 88
  USHORT  UltraDMAActive  : 8;
  USHORT  ReservedWord89[4];
  USHORT  HardwareResetResult;
  USHORT  CurrentAcousticValue : 8;
  USHORT  RecommendedAcousticValue : 8;
  USHORT  ReservedWord95[5];
  ULONG  Max48BitLBA[2]; // word 100-103
  USHORT  ReservedWord104[23];
  USHORT  MsnSupport : 2;
  USHORT  ReservedWord127 : 14;
  USHORT  SecurityStatus;
  USHORT  ReservedWord129[126];
  USHORT  Signature : 8;
  USHORT  CheckSum : 8;
} IDENTIFY_DEVICE_DATA, *PIDENTIFY_DEVICE_DATA;

#pragma pack(pop)

typedef NTSTATUS (DDKAPI *tVolumeDeviceToDosName)(PVOID, PUNICODE_STRING);

//===========================================================
// LDR
//===========================================================

typedef struct _MODULE_ENTRY{
	LIST_ENTRY le_mod;
	ULONG  unknown[4];
	ULONG  base;
	ULONG  driver_start;
	ULONG  unk1;
	UNICODE_STRING driver_Path;
	UNICODE_STRING driver_Name;
} MODULE_ENTRY, *PMODULE_ENTRY;

typedef struct _IDT_DESCRIPTOR
{
  USHORT OffsetLow,OffsetHigh;
} IDT_DESCRIPTOR, *PIDT_DESCRIPTOR;

typedef struct _IDT
{
  USHORT          Limit;
  PIDT_DESCRIPTOR Descriptors;
} IDT, *PIDT;

//===========================================================
//===========================================================

//Undefined APIs

typedef NTSTATUS (NTAPI * NTPROC) ();
typedef NTPROC * PNTPROC;
typedef HANDLE HHOOK;
typedef VOID(* 	WINEVENTPROC )(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
#define NTPROC_ sizeof (NTPROC)

typedef struct tag_SYSTEM_SERVICE_TABLE {
  PNTPROC	ServiceTable; // array of entry points to the calls
	PDWORD	CounterTable; // array of usage counters
  ULONG ServiceLimit; // number of table entries
  PCHAR ArgumentTable; // array of argument counts
} SYSTEM_SERVICE_TABLE, *PSYSTEM_SERVICE_TABLE, **PPSYSTEM_SERVICE_TABLE;

typedef struct tag_SERVICE_DESCRIPTOR_TABLE {
  SYSTEM_SERVICE_TABLE ntoskrnl; // main native API table
  SYSTEM_SERVICE_TABLE win32k; // win subsystem, in shadow table
  SYSTEM_SERVICE_TABLE sst3;
  SYSTEM_SERVICE_TABLE sst4;
} SERVICE_DESCRIPTOR_TABLE, *PSERVICE_DESCRIPTOR_TABLE, **PPSERVICE_DESCRIPTOR_TABLE;

extern NTOSAPI SYSTEM_SERVICE_TABLE KeServiceDescriptorTable;

typedef struct _KSYSTEM_TIME {
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

extern NTOSAPI KSYSTEM_TIME KeTickCount;

typedef struct _SYSTEM_THREADS_INFORMATION {
    LARGE_INTEGER   KernelTime;
    LARGE_INTEGER   UserTime;
    LARGE_INTEGER   CreateTime;
    ULONG           WaitTime;
    PVOID           StartAddress;
    CLIENT_ID       ClientId;
    KPRIORITY       Priority;
    KPRIORITY       BasePriority;
    ULONG           ContextSwitchCount;
    THREAD_STATE    State;
    KWAIT_REASON    WaitReason;
} SYSTEM_THREADS_INFORMATION, *PSYSTEM_THREADS_INFORMATION;

// SystemProcessesAndThreadsInformation
typedef struct _SYSTEM_PROCESSES_INFORMATION {
    ULONG                       NextEntryDelta;
    ULONG                       ThreadCount;
    ULONG                       Reserved1[6];
    LARGE_INTEGER               CreateTime;
    LARGE_INTEGER               UserTime;
    LARGE_INTEGER               KernelTime;
    UNICODE_STRING              ProcessName;
    KPRIORITY                   BasePriority;
    ULONG                       ProcessId;
    ULONG                       InheritedFromProcessId;
    ULONG                       HandleCount;
    ULONG                       SessionId;
    ULONG                       Reserved2;
    VM_COUNTERS                 VmCounters;
    IO_COUNTERS                 IoCounters;
    SYSTEM_THREADS_INFORMATION  Threads[1];
} SYSTEM_PROCESSES_INFORMATION, *PSYSTEM_PROCESSES_INFORMATION;

//===========================================================

#ifndef OBJ_KERNEL_HANDLE
#define OBJ_KERNEL_HANDLE 0x00000200L
#endif //OBJ_KERNEL_HANDLE

#define MmMapAddress(a,s) MmMapIoSpace(MmGetPhysicalAddress((PVOID)a),s,MmNonCached)

#define ATA_IDENTIFY_DEVICE 0xEC
#define SCSI_IOCTL_DATA_IN  0x01
#define CDB6GENERIC_LENGTH  0x06
#define SCSIOP_INQUIRY      0x12

#define IOCTL_STORAGE_BASE 0x0000002d
#define IOCTL_DISK_BASE 0x00000007

#define IOCTL_STORAGE_QUERY_PROPERTY \
CTL_CODE(IOCTL_STORAGE_BASE, 0x0500,METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SMART_RCV_DRIVE_DATA \
CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_SCSI_BASE                	FILE_DEVICE_CONTROLLER
#define IOCTL_SCSI_MINIPORT             CTL_CODE(IOCTL_SCSI_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)   //0x0004D008  see NTDDSCSI.H for definition
#define IOCTL_SCSI_RESCAN_BUS           CTL_CODE(IOCTL_SCSI_BASE, 0x0407, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH         CTL_CODE(IOCTL_SCSI_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH_DIRECT  CTL_CODE(IOCTL_SCSI_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define WDF_TIMEOUT_TO_SEC              ((LONGLONG) 1 * 10 * 1000 * 1000)
#define WDF_TIMEOUT_TO_MS               ((LONGLONG) 1 * 10 * 1000)
#define WDF_TIMEOUT_TO_US               ((LONGLONG) 1 * 10)

#define WDF_REL_TIMEOUT_IN_SEC(Time)    (Time * -1 * WDF_TIMEOUT_TO_SEC)
#define WDF_ABS_TIMEOUT_IN_SEC(Time)    (Time *  1 * WDF_TIMEOUT_TO_SEC)
#define WDF_REL_TIMEOUT_IN_MS(Time)     (Time * -1 * WDF_TIMEOUT_TO_MS)
#define WDF_ABS_TIMEOUT_IN_MS(Time)     (Time *  1 * WDF_TIMEOUT_TO_MS)
#define WDF_REL_TIMEOUT_IN_US(Time)     (Time * -1 * WDF_TIMEOUT_TO_US)
#define WDF_ABS_TIMEOUT_IN_US(Time)     (Time *  1 * WDF_TIMEOUT_TO_US)

//===========================================================

#endif // DRIVER_H_INCLUDED
