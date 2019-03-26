#include "driver.h"
#include <wingdi.h>

tVolumeDeviceToDosName pVolumeDeviceToDosName=NULL;

DWORD dwVersionBuildNumber=0;
DWORD dwTimeIncrement=0;
DWORD dwMsgTickStartTime=0,dwMsgTickEndTime=0;

BOOL bDriverInit=FALSE,
bNotifyRoutineCreated=FALSE;
LONG nInHookRefCount=0;
UINT rseed=0;
PSSMSG pSSBuffer=NULL;

//System Func Offsets NtGetContextThread
typedef NTSTATUS (DDKAPI *tNtGetContextThread)(HANDLE ThreadHandle,PCONTEXT pContext);

DWORD NtGetContextThreadOffset=0;
PVOID *NtGetContextThreadAddress=NULL;
tNtGetContextThread pNtGetContextThread=NULL;
tNtGetContextThread pSecureNtGetContextThread=NULL;

//System Func Offsets NtOpenFile
typedef NTSTATUS (DDKAPI *tNtOpenFile)(PHANDLE FileHandle,
ACCESS_MASK DesiredAccess,POBJECT_ATTRIBUTES ObjectAttributes,
PIO_STATUS_BLOCK IoStatusBlock,ULONG ShareAccess,ULONG OpenOptions);

DWORD NtOpenFileOffset=0;
PVOID *NtOpenFileAddress=NULL;
tNtOpenFile pNtOpenFile=NULL;
tNtOpenFile pSecureNtOpenFile=NULL;

//System Func Offsets NtDeviceIoControlFile
typedef NTSTATUS (DDKAPI *tNtDeviceIoControlFile)(HANDLE FileHandle,HANDLE Event,
PIO_APC_ROUTINE ApcRoutine,PVOID ApcContext,PIO_STATUS_BLOCK IoStatusBlock,ULONG IoControlCode,
PVOID InputBuffer,ULONG InputBufferLength,PVOID OutputBuffer,ULONG OutputBufferLength);

DWORD NtDeviceIoControlFileOffset=0;
PVOID *NtDeviceIoControlFileAddress=NULL;
tNtDeviceIoControlFile pNtDeviceIoControlFile=NULL;
tNtDeviceIoControlFile pSecureNtDeviceIoControlFile=NULL;

//System Func Offsets NtQuerySystemInformation
typedef NTSTATUS (DDKAPI *tNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass,
PVOID SystemInformation,ULONG SystemInformationLength,PULONG ReturnLength);

DWORD NtQuerySystemInformationOffset=0;
PVOID *NtQuerySystemInformationAddress=NULL;
tNtQuerySystemInformation pNtQuerySystemInformation=NULL;
tNtQuerySystemInformation pSecureNtQuerySystemInformation=NULL;

//System Func Offsets NtQueryVirtualMemory
typedef NTSTATUS (DDKAPI *tNtQueryVirtualMemory)(HANDLE ProcessHandle,PVOID BaseAddress,MEMORY_INFORMATION_CLASS MemoryInformationClass,
  PVOID MemoryInformation,ULONG MemoryInformationLength,PULONG ReturnLength);

DWORD NtQueryVirtualMemoryOffset=0;
PVOID *NtQueryVirtualMemoryAddress=NULL;
tNtQueryVirtualMemory pNtQueryVirtualMemory=NULL;
tNtQueryVirtualMemory pSecureNtQueryVirtualMemory=NULL;

//System Func Offsets NtGdiBitBlt
typedef NTSTATUS (DDKAPI *tNtGdiBitBlt)(HDC hDCDest,INT XDest,INT YDest,INT Width,
INT Height,HDC hDCSrc,INT XSrc,INT YSrc,DWORD ROP,IN DWORD crBackColor,IN FLONG fl);

DWORD NtGdiBitBltOffset=0;
PVOID *NtGdiBitBltAddress=NULL;
tNtGdiBitBlt pNtGdiBitBlt=NULL;
tNtGdiBitBlt pSecureNtGdiBitBlt=NULL;

//System Func Offsets NtGdiSetDIBitsToDeviceInternal
typedef NTSTATUS (DDKAPI *tNtGdiSetDIBitsToDeviceInternal)(HDC hdcDest,INT XDest,INT YDest,INT Width,INT Height,
INT XSrc,INT YSrc,DWORD iStartScan,DWORD cNumScan,LPBYTE pInitBits,LPBITMAPINFO pbmi,
DWORD iUsage,UINT cjMaxBits,UINT cjMaxInfo,BOOL bTransformCoordinates,HANDLE hcmXform);

DWORD NtGdiSetDIBitsToDeviceInternalOffset=0;
PVOID *NtGdiSetDIBitsToDeviceInternalAddress=NULL;
tNtGdiSetDIBitsToDeviceInternal pNtGdiSetDIBitsToDeviceInternal=NULL;
tNtGdiSetDIBitsToDeviceInternal pSecureNtGdiSetDIBitsToDeviceInternal=NULL;

static CHAR sserial[50]={0};//short serial
static CHAR lserial[100]={0};//large serial
static char Format[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

WCHAR sCreateDeviceName[]=L"\\Device\\mydriver";
WCHAR sCreateSymbolicLinkName[]=L"\\DosDevices\\mydriver";

PVOID RevBaseAddress=NULL;
DWORD RevRegionSize=0;

////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////

int memcmp(const void *s1,const void *s2,size_t n){
  if(n!=0){
    const unsigned char *p1=(const unsigned char *)s1,*p2=(const unsigned char *)s2;
    do{
      if(*p1++!=*p2++)
        return (*--p1-*--p2);
    }while(--n!=0);
  }
  return 0;
}

VOID DDKAPI HideDriver(PDRIVER_OBJECT DriverObject){
  KIRQL OldIrql=KeRaiseIrqlToDpcLevel();
  PLDR_DATA_TABLE_ENTRY DriverEntry=DriverObject->DriverSection;
  RemoveEntryList(&DriverEntry->InLoadOrderLinks);
  InitializeListHead(&DriverEntry->InLoadOrderLinks);
  KeLowerIrql(OldIrql);
}

PSERVICE_DESCRIPTOR_TABLE DDKAPI GetServiceDescriptorShadowTableAddress(){
  PBYTE check=(PBYTE)&KeAddSystemServiceTable;
	PSERVICE_DESCRIPTOR_TABLE rc=0;UINT i;
	for(i=0;i<1024;i++){
	  rc=*(PPSERVICE_DESCRIPTOR_TABLE)check;
		if(!MmIsAddressValid(rc)||((PVOID)rc==(PVOID)&KeServiceDescriptorTable)
		||(memcmp(rc,&KeServiceDescriptorTable,sizeof(SYSTEM_SERVICE_TABLE)))){
			check++;
			rc=0;
		}
		if(rc)
			break;
	}
	return rc;
}

LPSTR DDKAPI ClearBlanks(LPSTR sFileName){
  LPSTR p;
  while(*sFileName&&*sFileName==' ')++sFileName;
  for(p=sFileName;*p;p++)
    if((unsigned)(*p-0x41)<0x1Au)*p|=0x20;
  --p;
  while(*p&&*p==' ')--p;
  if(*(++p)==' ')*p=0;
  return sFileName;
}

NTSTATUS DDKAPI CheckIfIs(LPSTR sFileName,LPCSTR sNeedFile){
  PCHAR sFile=sFileName;
  for(;*sFile;sFile++)
    if((unsigned)(*sFile-0x41)<0x1Au)*sFile|=0x20;
  if(strstr(sFileName,sNeedFile))
    return TRUE;
  return FALSE;
}

BOOL DDKAPI ImageFullPath(PEPROCESS eprocess,PCHAR fullname){
  BOOL ret=FALSE;BYTE buffer[sizeof(UNICODE_STRING)+MAX_PATH*sizeof(WCHAR)];
  HANDLE handle;DWORD returnedLength=0;ANSI_STRING DestinationString;
  if(NT_SUCCESS(ObOpenObjectByPointer(eprocess,OBJ_KERNEL_HANDLE,NULL,GENERIC_READ,0,KernelMode,&handle))){
    if(NT_SUCCESS(ZwQueryInformationProcess(handle,ProcessImageFileName,buffer,sizeof(buffer),&returnedLength))){
      RtlUnicodeStringToAnsiString(&DestinationString,(UNICODE_STRING*)buffer,TRUE);
      strncpy(fullname,DestinationString.Buffer,DestinationString.Length);ret=TRUE;
      fullname[DestinationString.Length]=0;RtlFreeAnsiString(&DestinationString);
    }
    ZwClose(handle);
  }
  return ret;
}

BOOL DDKAPI ImageFileName(PEPROCESS eprocess,PCHAR filename){
  CHAR sImageFullPath[MAX_PATH]={0};
  if(ImageFullPath(eprocess,sImageFullPath)){
    PCHAR pIFN=sImageFullPath,pIFP=sImageFullPath;
    while(*pIFP)if(*(pIFP++)=='\\')pIFN=pIFP;
    strcpy(filename,pIFN);return TRUE;
  }
  return FALSE;
}

HANDLE DDKAPI GetProcessIdByHandle(HANDLE Process){
  PROCESS_BASIC_INFORMATION ProcessBasicInfo;
  NTSTATUS status=ZwQueryInformationProcess(Process,ProcessBasicInformation,&ProcessBasicInfo,sizeof(PROCESS_BASIC_INFORMATION),NULL);
  if(NT_SUCCESS(status))
    return (HANDLE)ProcessBasicInfo.UniqueProcessId;
  return NULL;
}

HANDLE DDKAPI GetThreadIdFromHandle(HANDLE Thread){
  PETHREAD eThread;HANDLE ThreadId;
  NTSTATUS status=ObReferenceObjectByHandle(Thread,0,0,KernelMode,(PVOID)&eThread,NULL);
  if(NT_SUCCESS(status)){
    ThreadId=(HANDLE)PsGetThreadId(eThread);
    ObDereferenceObject(eThread);
    return ThreadId;
  }
  return 0;
}

NTSTATUS DDKAPI TerminateProcessById(HANDLE hProcId){
	HANDLE hProc;OBJECT_ATTRIBUTES oa;CLIENT_ID ClientId;
	InitializeObjectAttributes(&oa,NULL,0,NULL,NULL);
	ClientId.UniqueProcess=(HANDLE)hProcId;
	ClientId.UniqueThread=(HANDLE)0;
	NTSTATUS status=ZwOpenProcess(&hProc,PROCESS_ALL_ACCESS,&oa,&ClientId);
	if(status==STATUS_SUCCESS){
    status=ZwTerminateProcess(hProc,STATUS_SUCCESS);
    ZwClose(hProc);
	}
	return status;
}

ULONG DDKAPI FindProcessId(LPCSTR swName){
  PSYSTEM_PROCESSES_INFORMATION pSPInfo=NULL;
  NTSTATUS Status=STATUS_NO_MEMORY;
  ULONG SPInfoLen=1000,ulProcId=0;CHAR sFileName[MAX_PATH];
  ANSI_STRING DestinationString={MAX_PATH-sizeof(ANSI_NULL),MAX_PATH,sFileName};
  if(!swName)return 0;
  do{
    pSPInfo=ExAllocatePoolWithTag(PagedPool,SPInfoLen,0);
    if(!pSPInfo)
      break;
    Status=ZwQuerySystemInformation(SystemProcessesAndThreadsInformation,pSPInfo,SPInfoLen,&SPInfoLen);
    if(!NT_SUCCESS(Status)){
      ExFreePoolWithTag(pSPInfo,0);
      pSPInfo=NULL;
    }
  }while(Status==STATUS_INFO_LENGTH_MISMATCH);
  if(pSPInfo){
    PSYSTEM_PROCESSES_INFORMATION pSp=pSPInfo;
    do{
      if(NT_SUCCESS(RtlUnicodeStringToAnsiString(&DestinationString,&pSp->ProcessName,FALSE))){
        DestinationString.Buffer[DestinationString.Length]=0;
        if(CheckIfIs(DestinationString.Buffer,swName)){
          ulProcId=pSp->ProcessId;
          break;
        }
      }
      pSp=(PSYSTEM_PROCESSES_INFORMATION)((PBYTE)pSp+pSp->NextEntryDelta);
    }while(pSp->NextEntryDelta!=0);
    ExFreePoolWithTag(pSPInfo,0);
  }
  return ulProcId;
}

BOOL DDKAPI TerminateProcessByName(LPCSTR sName){
  HANDLE dwProcId=(HANDLE)FindProcessId(sName);
  if(dwProcId){
    NTSTATUS status=TerminateProcessById(dwProcId);
    return (status==STATUS_SUCCESS);
  }
  return TRUE;
}

NTSTATUS DDKAPI MmAllocateUserBuffer(PVOID *BaseAddress,ULONG Size){
  return ZwAllocateVirtualMemory(NtCurrentProcess(), BaseAddress, 0L, &Size, MEM_COMMIT, PAGE_READWRITE);
}

NTSTATUS DDKAPI MmFreeUserBuffer(PVOID *BaseAddress){
  ULONG RegionSize=0;
  return ZwFreeVirtualMemory(NtCurrentProcess(), BaseAddress, &RegionSize, MEM_RELEASE);
}

BOOL DDKAPI CheckExtension(PCHAR filename, PCHAR ext){
  CHAR *pFName,sFileName[MAX_PATH];
  strcpy(sFileName,filename);
  pFName=ClearBlanks(sFileName);
  while(*ext&&*pFName){
    if(*pFName==' ')
      pFName++;
    else{
      if(*ext!=*pFName)
        break;
      ext++;pFName++;
    }
  }
  if(!*ext)return TRUE;
  return FALSE;
}

VOID DDKAPI SetSeed(UINT s){
  rseed=s;
}

UINT DDKAPI Rand(){
  rseed=1103515245*rseed+12345;
  return ((rseed>>16)%0x8000);
}

VOID DDKAPI FakeSerialGenerator(VOID){
  LARGE_INTEGER sysTime;BYTE c,d;
  KeQuerySystemTime(&sysTime);
  SetSeed(sysTime.LowPart);
  for(c=0;c<5;c++)
    sserial[c]=' ';
  for(c=5;c<20;c++){
    if((c+1)%4)
      sserial[c]=(Rand()%0x1A)+0x41;
    else
      sserial[c]=(Rand()%0x0A)+0x30;
  }
  for(c=0,d=0;c<20;c++,d+=2){
    lserial[d]=Format[(sserial[c]>>4)&0xF];
    lserial[d+1]=Format[sserial[c]&0xF];
  }
}

//*********************************************************
// SYS HOOK Functions
//*********************************************************

#define CUSTOM_ROP 0xFFFFFFFF //for internal use whit driver...
NTSTATUS DDKAPI MyNtGdiBitBlt(HDC hDCDest,INT XDest,INT YDest,INT Width,
INT Height,HDC hDCSrc,INT XSrc,INT YSrc,DWORD ROP,IN DWORD crBackColor,IN FLONG fl){
  INT ret=STATUS_SUCCESS;
  InterlockedIncrement(&nInHookRefCount);
  // Anti ScreenShot
  if(pSSBuffer&&pSSBuffer->uBuffSize&&CUSTOM_ROP!=ROP){
    PSSMSG pSSBuf=NULL;
    MmAllocateUserBuffer((PVOID*)&pSSBuf,pSSBuffer->uBuffSize);
    if(pSSBuf){
      RtlCopyMemory(pSSBuf,pSSBuffer,pSSBuffer->uBuffSize);
      ret=pSecureNtGdiSetDIBitsToDeviceInternal(hDCDest,XDest,YDest,
      Width,Height,XSrc,YSrc,0,pSSBuf->bmi.bmiHeader.biHeight,pSSBuf->pBuffer,&pSSBuf->bmi,
      DIB_RGB_COLORS,pSSBuf->bmi.bmiHeader.biSizeImage,pSSBuf->bmi.bmiHeader.biSize,TRUE,NULL);
      MmFreeUserBuffer((PVOID*)&pSSBuf);
    }
  }else{
    if(CUSTOM_ROP==ROP){
      ROP=SRCCOPY;
    }
    ret=pSecureNtGdiBitBlt(hDCDest,XDest,YDest,Width,Height,hDCSrc,XSrc,YSrc,ROP,crBackColor,fl);
  }
  InterlockedDecrement(&nInHookRefCount);
  return ret;
}

NTSTATUS DDKAPI SpoofHDSerial(IN HANDLE FileHandle,IN HANDLE Event,IN PIO_APC_ROUTINE ApcRoutine,IN PVOID ApcContext,OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG IoControlCode,IN PVOID InputBuffer,IN ULONG InputBufferLength,OUT PVOID OutputBuffer,IN ULONG OutputBufferLength){
  PSTORAGE_DEVICE_DESCRIPTOR output;PSTORAGE_PROPERTY_QUERY input;PIDENTIFY_DEVICE_DATA hdid;PSERIALNUMBER shdid;
  PSENDCMDINPARAMS cmdinput;PSENDCMDOUTPARAMS cmdoutput;PSCSI_PASS_THROUGH sptin,sptout;
  // Call original...
  InterlockedIncrement(&nInHookRefCount);
  NTSTATUS result=pSecureNtDeviceIoControlFile(FileHandle,Event,ApcRoutine,ApcContext,IoStatusBlock,IoControlCode,InputBuffer,InputBufferLength,OutputBuffer,OutputBufferLength);
  if(!NT_SUCCESS(result)||(IoControlCode!=IOCTL_STORAGE_QUERY_PROPERTY&&IoControlCode!=SMART_RCV_DRIVE_DATA&&IoControlCode!=IOCTL_SCSI_PASS_THROUGH)){
    InterlockedDecrement(&nInHookRefCount);
    return result;
  }

  if(OutputBuffer&&OutputBufferLength){
    switch(IoControlCode){
      case IOCTL_STORAGE_QUERY_PROPERTY:
        input=(PSTORAGE_PROPERTY_QUERY) InputBuffer;
        output=(PSTORAGE_DEVICE_DESCRIPTOR) OutputBuffer;
        if(input->PropertyId==StorageDeviceProperty&&input->QueryType==PropertyStandardQuery&&
          output->SerialNumberOffset&&OutputBufferLength>(output->SerialNumberOffset+39)){
          PCHAR serialnum=(PCHAR)output+output->SerialNumberOffset;
          memcpy(serialnum,lserial,40);
        }
      break;
      case SMART_RCV_DRIVE_DATA:
        cmdinput=(PSENDCMDINPARAMS) InputBuffer;
        cmdoutput=(PSENDCMDOUTPARAMS) OutputBuffer;
        if(cmdoutput->bBuffer&&(!cmdinput->irDriveRegs.bCommandReg||cmdinput->irDriveRegs.bCommandReg==ATA_IDENTIFY_DEVICE)){
          hdid=(PIDENTIFY_DEVICE_DATA)cmdoutput->bBuffer;
          memcpy(hdid->SerialNumber,sserial,20);
        }
      break;
      case IOCTL_SCSI_PASS_THROUGH:
        sptin=(PSCSI_PASS_THROUGH) InputBuffer;
        sptout=(PSCSI_PASS_THROUGH) OutputBuffer;
        if(sptin&&sptout&&sptin->Cdb[0]==SCSIOP_INQUIRY&&sptin->Cdb[1]==0x01&&sptin->Cdb[2]==0x80&&sptin->Cdb[4]>15){
          shdid=(PSERIALNUMBER)((PCHAR)sptout+sptout->DataBufferOffset);
          memcpy(shdid->SerialNumber,sserial,NSM_SERIAL_NUMBER_LENGTH);
        }
      break;
      default:break;
    }
  }
  InterlockedDecrement(&nInHookRefCount);
  return result;
}

// with some AV NtDeviceIoControlFile can take so long
// so we need inline assembly
INT DDKAPI MyNtDeviceIoControlFile();
asm(
  ".globl _MyNtDeviceIoControlFile\r\n"
  "_MyNtDeviceIoControlFile:\r\n"
  " mov 0x18(%esp),%esi;\r\n"
  " cmp $0x2D1400,%esi;\r\n"
  " jz  _SpoofHDSerial;\r\n"
  " cmp $0x7C088,%esi;\r\n"
  " jz  _SpoofHDSerial;\r\n"
  " cmp $0x4D004,%eax;\r\n"
  " jz  _SpoofHDSerial;\r\n"
  " jmp *_pSecureNtDeviceIoControlFile;\r\n"
);

NTSTATUS DDKAPI MyNtQueryVirtualMemory(HANDLE ProcessHandle,PVOID BaseAddress,MEMORY_INFORMATION_CLASS MemoryInformationClass,
  PVOID MemoryInformation,ULONG MemoryInformationLength,PULONG ReturnLength){
  // Call original...
  InterlockedIncrement(&nInHookRefCount);
  NTSTATUS result=pSecureNtQueryVirtualMemory(ProcessHandle,BaseAddress,MemoryInformationClass,
    MemoryInformation,MemoryInformationLength,ReturnLength);
  if(!NT_SUCCESS(result)||!RevBaseAddress||!RevRegionSize||
     (MemoryInformationClass!=MemoryBasicInformation&&MemoryInformationClass!=MemorySectionName)){
    InterlockedDecrement(&nInHookRefCount);
    return result;
  }

  //is our memory?
  if(BaseAddress>=RevBaseAddress&&BaseAddress<(PVOID)((DWORD)RevBaseAddress+RevRegionSize)){
    if(MemoryInformationClass==MemoryBasicInformation){
      PMEMORY_BASIC_INFORMATION pMem=(PMEMORY_BASIC_INFORMATION)MemoryInformation;
      //clear info ;)
      pMem->BaseAddress=RevBaseAddress;
      pMem->AllocationBase=NULL;
      pMem->AllocationProtect=0;
      pMem->RegionSize=RevRegionSize;
      pMem->Type=0;
      pMem->State=MEM_FREE;
      pMem->Protect=PAGE_NOACCESS;
    }
    if(MemoryInformationClass==MemorySectionName){
      //nothing here ;)
      result=STATUS_SECTION_NOT_IMAGE;
    }
  }

  InterlockedDecrement(&nInHookRefCount);
  return result;
}

NTSTATUS DDKAPI MyNtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
  PVOID SystemInformation,ULONG SystemInformationLength,PULONG ReturnLength){
  PSYSTEM_PROCESSES_INFORMATION pSysProcInfo,pLastSysProcInfo;CHAR sFileName[MAX_PATH];
  ANSI_STRING DestinationString={MAX_PATH-sizeof(ANSI_NULL),MAX_PATH,sFileName};

  // Call original...
  InterlockedIncrement(&nInHookRefCount);
  NTSTATUS result=pSecureNtQuerySystemInformation(SystemInformationClass,SystemInformation,SystemInformationLength,ReturnLength);
  if(!NT_SUCCESS(result)||SystemInformationClass!=SystemProcessesAndThreadsInformation){
    InterlockedDecrement(&nInHookRefCount);
    return result;
  }

  // We Need Short Name...
  PCHAR sMeShortName=NULL,p="";
  while(*p)if(*(p++)=='\\')sMeShortName=p;
  if(!sMeShortName){
    InterlockedDecrement(&nInHookRefCount);
    return result;
  }

  // Start Find Me!
  pSysProcInfo=(PSYSTEM_PROCESSES_INFORMATION)SystemInformation;
  pLastSysProcInfo=NULL;
  while(pSysProcInfo){
    if(NT_SUCCESS(RtlUnicodeStringToAnsiString(&DestinationString,&pSysProcInfo->ProcessName,FALSE))){
      DestinationString.Buffer[DestinationString.Length]=0;
      if(!strcmp(sMeShortName,sFileName)){
        // Hide Process
        if(pLastSysProcInfo){
          if(pSysProcInfo->NextEntryDelta)
            pLastSysProcInfo->NextEntryDelta+=pSysProcInfo->NextEntryDelta;
          else
            pLastSysProcInfo->NextEntryDelta=0;
        }else{
          if(pSysProcInfo->NextEntryDelta)
            SystemInformation+=pSysProcInfo->NextEntryDelta;
          else
            SystemInformation=NULL;
        }
        // Hide Threads
        pSysProcInfo->Threads[0].State=0;
        pSysProcInfo->ThreadCount=0;
      }
    }
    // This is the last? then exit...
    if(!pSysProcInfo->NextEntryDelta)
      break;
    // Save last and update!
    pLastSysProcInfo=pSysProcInfo;
    pSysProcInfo=(PSYSTEM_PROCESSES_INFORMATION)((PBYTE)pSysProcInfo+pSysProcInfo->NextEntryDelta);
  }
  InterlockedDecrement(&nInHookRefCount);
  return result;
}

#define CONTEXT_DEBUG_REGISTERS_EX 0xFFFFFFFF //for internal use whit driver...
NTSTATUS DDKAPI MyNtGetContextThread(HANDLE ThreadHandle,PCONTEXT pContext){
  static HANDLE ThreadId=0;BOOL bNeedHide=TRUE;
  InterlockedIncrement(&nInHookRefCount);

  // Custom message!
  if(pContext&&pContext->ContextFlags==CONTEXT_DEBUG_REGISTERS_EX){
    pContext->ContextFlags=CONTEXT_DEBUG_REGISTERS;
    bNeedHide=FALSE;
    ThreadId=GetThreadIdFromHandle(ThreadHandle);
  }

  // Call original...
  NTSTATUS result=pSecureNtGetContextThread(ThreadHandle,pContext);
  if(!NT_SUCCESS(result)||!bNeedHide||!pContext||!(pContext->ContextFlags&CONTEXT_DEBUG_REGISTERS)){
    InterlockedDecrement(&nInHookRefCount);
    return result;
  }

  //it's my thread?
  if(GetThreadIdFromHandle(ThreadHandle)==ThreadId){
    if(!pContext->Eip){
      pContext->Dr0=0;
      pContext->Dr1=0;
      pContext->Dr2=0;
      pContext->Dr3=0;
      pContext->Dr7=0;
      pContext->Dr6=0;
    }
  }

  InterlockedDecrement(&nInHookRefCount);
  return result;
}

NTSTATUS DDKAPI MyNtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions){
  PFILE_OBJECT pfObject=NULL;UNICODE_STRING UnicodeString;
  STRING DestinationString,AnsiString;
  CHAR sBuffer[1024],fext[4],*pBuff;UINT len;
  InterlockedIncrement(&nInHookRefCount);
  NTSTATUS result=pSecureNtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
  if(bDriverInit){
    if(FileHandle&&*FileHandle){
      if(NT_SUCCESS(ObReferenceObjectByHandle(*FileHandle, TRUE, IoFileObjectType, KernelMode, (PVOID*)&pfObject, NULL))){
        if(NT_SUCCESS(RtlUnicodeStringToAnsiString(&DestinationString,&pfObject->FileName,TRUE))){
          if(MmIsAddressValid(DestinationString.Buffer)&&DestinationString.Buffer[0]){
            len=DestinationString.Length;
            pBuff=DestinationString.Buffer;
            while(len>0){
              if(pBuff[len]=='.'){
                ++len;
                break;
              }
              --len;
            }
            if(len>0){
              strncpy(fext,&pBuff[len],4);
              if(CheckExtension(fext,"dll")||CheckExtension(fext,"exe")){
                if(!pVolumeDeviceToDosName){
                  if(dwVersionBuildNumber<0xA28)
                    RtlInitUnicodeString(&UnicodeString, L"RtlVolumeDeviceToDosName");
                  else
                    RtlInitUnicodeString(&UnicodeString, L"IoVolumeDeviceToDosName");
                  pVolumeDeviceToDosName=(tVolumeDeviceToDosName)MmGetSystemRoutineAddress(&UnicodeString);
                }
                if(pVolumeDeviceToDosName){
                  if(!pVolumeDeviceToDosName(pfObject->DeviceObject,&UnicodeString)){
                    if(NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE))){
                      len=AnsiString.Length+DestinationString.Length;
                      if(len>=1015)len=1015;
                      strncpy(sBuffer,AnsiString.Buffer,AnsiString.Length+1);
                      strncat(sBuffer,DestinationString.Buffer,DestinationString.Length+1);
                      sBuffer[len]=0;
                      RtlFreeAnsiString(&AnsiString);
                    }
                    RtlFreeUnicodeString(&UnicodeString);
                  }
                }
              }
            }
          }
          RtlFreeAnsiString(&DestinationString);
        }
        ObfDereferenceObject(pfObject);
      }
    }
  }
  InterlockedDecrement(&nInHookRefCount);
  return result;
}

PBYTE HookSYSFunction(PBYTE dirApi,PBYTE newDirApi,UINT nBytes){
  DWORD accessmask=0;
  if(!dirApi||!newDirApi)return NULL;
  PBYTE pBuffer=(PBYTE)ExAllocatePoolWithTag(NonPagedPool,nBytes+5,0);
  if(!pBuffer)return NULL;
  memmove(pBuffer,dirApi,nBytes);
  *(pBuffer+nBytes)=0xE9;
  *(DWORD*)(pBuffer+nBytes+1)=(DWORD)(dirApi-pBuffer)-5;
  asm(
    "cli;\r\n"
    "mov %%cr0,%%eax;\r\n"
    "mov %%eax,%0;\r\n"
    "and $0xfffeffff,%%eax;\r\n"
    "mov %%eax,%%cr0":"=m"(accessmask)::"%eax"
  );
  *dirApi=0xE9;
  *(DWORD*)(dirApi+1)=(DWORD)(newDirApi-dirApi)-5;
  asm(
    "mov %0,%%eax;\r\n"
    "mov %%eax,%%cr0;\r\n"
    "sti"::"m"(accessmask):"%eax"
  );
  return pBuffer;
}

VOID UnHookSYSFunction(PBYTE dirApi,PBYTE pBuffer){
  DWORD accessmask=0;
  if(!dirApi||!pBuffer)return;
  asm(
    "cli;\r\n"
    "mov %%cr0,%%eax;\r\n"
    "mov %%eax,%0;\r\n"
    "and $0xfffeffff,%%eax;\r\n"
    "mov %%eax,%%cr0":"=m"(accessmask)::"%eax"
  );
  *dirApi=*pBuffer;
  *(DWORD*)(dirApi+1)=*(DWORD*)(pBuffer+1);
  asm(
    "mov %0,%%eax;\r\n"
    "mov %%eax,%%cr0;\r\n"
    "sti"::"m"(accessmask):"%eax"
  );
  ExFreePoolWithTag(pBuffer,0);
}

VOID DDKAPI NotifyRoutine(HANDLE ParentId,HANDLE ProcessId,BOOLEAN bCreate){
  PEPROCESS PEPObject=NULL;CHAR sProcessName[MAX_PATH]={0};
  if(NT_SUCCESS(PsLookupProcessByProcessId(ProcessId,&PEPObject))){
    if(ImageFileName(PEPObject,sProcessName)){
      if(bCreate){
      }else{
      }
    }
    ObDereferenceObject(PEPObject);
  }
}

VOID DDKAPI ForceRestoreSystem(){
  LARGE_INTEGER Interval;
  if(pNtDeviceIoControlFile){
    *NtDeviceIoControlFileAddress=(PVOID)pNtDeviceIoControlFile;
    MmUnmapIoSpace(NtDeviceIoControlFileAddress,sizeof(PVOID));//4);
    pNtDeviceIoControlFile=NULL;
  }
  if(pNtQuerySystemInformation){
    *NtQuerySystemInformationAddress=(PVOID)pNtQuerySystemInformation;
    MmUnmapIoSpace(NtQuerySystemInformationAddress,sizeof(PVOID));//4);
    pNtQuerySystemInformation=NULL;
  }
  if(pNtGetContextThread){
    *NtGetContextThreadAddress=(PVOID)pNtGetContextThread;
    MmUnmapIoSpace(NtGetContextThreadAddress,sizeof(PVOID));//4);
    pNtGetContextThread=NULL;
  }
  if(pNtQueryVirtualMemory){
    *NtQueryVirtualMemoryAddress=(PVOID)pNtQueryVirtualMemory;
    MmUnmapIoSpace(NtQueryVirtualMemoryAddress,sizeof(PVOID));//4);
    pNtQueryVirtualMemory=NULL;
  }
  if(pNtGdiBitBlt){
    *NtGdiBitBltAddress=(PVOID)pNtGdiBitBlt;
    MmUnmapIoSpace(NtGdiBitBltAddress,sizeof(PVOID));//4);
    pNtGdiBitBlt=NULL;
  }
  if(pNtGdiSetDIBitsToDeviceInternal){
    MmUnmapIoSpace(NtGdiSetDIBitsToDeviceInternalAddress,sizeof(PVOID));//4);
    pNtGdiSetDIBitsToDeviceInternal=NULL;
  }
  //wait if driver is used (NtDeviceIoControlFile)
  do{
    Interval.QuadPart=WDF_REL_TIMEOUT_IN_MS(10);
    KeDelayExecutionThread(KernelMode,FALSE,&Interval);
  }while(nInHookRefCount);
  //Free memory
  if(bDriverInit){
    bDriverInit=FALSE;
  }
}

//DRIVER IOCTL CODES!
#define IOCTL_DRIVER_INIT     CTL_CODE(0,502,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DRIVER_END      CTL_CODE(0,502,METHOD_NEITHER,FILE_ANY_ACCESS)
#define IOCTL_DRIVER_BMPBUF   CTL_CODE(0,503,METHOD_IN_DIRECT,FILE_ANY_ACCESS)

NTSTATUS DDKAPI DeviceControl(PDEVICE_OBJECT DeviceObject,PIRP Irp){
  PIO_STACK_LOCATION pIoStackIrp=NULL;
  PSSMSG pSSMsg;PDRIVERMSG pDriverMsg;
  pIoStackIrp=IoGetCurrentIrpStackLocation(Irp);
  PSERVICE_DESCRIPTOR_TABLE ServiceDescriptorShadowTable;
  NTSTATUS ntStatus=STATUS_SUCCESS;// Assume success
  if(pIoStackIrp->Parameters.DeviceIoControl.IoControlCode==IOCTL_DRIVER_INIT){
    if(!bDriverInit){
      pDriverMsg=(PDRIVERMSG)(Irp->AssociatedIrp.SystemBuffer);

      //HOOK NtDeviceIoControlFile
      NtDeviceIoControlFileOffset=pDriverMsg->NtDICFOffset;
      NtDeviceIoControlFileAddress=(PVOID*)MmMapAddress(&KeServiceDescriptorTable.ServiceTable[NtDeviceIoControlFileOffset],sizeof(PVOID));
      pSecureNtDeviceIoControlFile=(tNtDeviceIoControlFile)*NtDeviceIoControlFileAddress;
      pNtDeviceIoControlFile=(tNtDeviceIoControlFile)*NtDeviceIoControlFileAddress;
      *NtDeviceIoControlFileAddress=(PVOID)MyNtDeviceIoControlFile;

      //HOOK NtQuerySystemInformation
      NtQuerySystemInformationOffset=pDriverMsg->NtQSIOffset;
      NtQuerySystemInformationAddress=(PVOID*)MmMapAddress(&KeServiceDescriptorTable.ServiceTable[NtQuerySystemInformationOffset],sizeof(PVOID));
      pSecureNtQuerySystemInformation=(tNtQuerySystemInformation)*NtQuerySystemInformationAddress;
      pNtQuerySystemInformation=(tNtQuerySystemInformation)*NtQuerySystemInformationAddress;
      *NtQuerySystemInformationAddress=(PVOID)MyNtQuerySystemInformation;

      //HOOK NtGetContextThread
      NtGetContextThreadOffset=pDriverMsg->NtGCTOffset;
      NtGetContextThreadAddress=(PVOID*)MmMapAddress(&KeServiceDescriptorTable.ServiceTable[NtGetContextThreadOffset],sizeof(PVOID));
      pSecureNtGetContextThread=(tNtGetContextThread)*NtGetContextThreadAddress;
      pNtGetContextThread=(tNtGetContextThread)*NtGetContextThreadAddress;
      *NtGetContextThreadAddress=(PVOID)MyNtGetContextThread;

      //HOOK NtQueryVirtualMemory
      NtQueryVirtualMemoryOffset=pDriverMsg->NtQVMOffset;
      NtQueryVirtualMemoryAddress=(PVOID*)MmMapAddress(&KeServiceDescriptorTable.ServiceTable[NtQueryVirtualMemoryOffset],sizeof(PVOID));
      pSecureNtQueryVirtualMemory=(tNtQueryVirtualMemory)*NtQueryVirtualMemoryAddress;
      pNtQueryVirtualMemory=(tNtQueryVirtualMemory)*NtQueryVirtualMemoryAddress;
      *NtQueryVirtualMemoryAddress=(PVOID)MyNtQueryVirtualMemory;

      //HOOK GDI
      ServiceDescriptorShadowTable=GetServiceDescriptorShadowTableAddress();
      if(ServiceDescriptorShadowTable){

        //HOOK NtGdiBitBlt
        NtGdiBitBltOffset=pDriverMsg->NtGBBOffset-0x1000;
        NtGdiBitBltAddress=(PVOID*)MmMapAddress(&ServiceDescriptorShadowTable->win32k.ServiceTable[NtGdiBitBltOffset],sizeof(PVOID));
        pNtGdiBitBlt=(tNtGdiBitBlt)*NtGdiBitBltAddress;
        pSecureNtGdiBitBlt=(tNtGdiBitBlt)*NtGdiBitBltAddress;
        *NtGdiBitBltAddress=(PVOID)MyNtGdiBitBlt;

        //NtGdiSetDIBitsToDeviceInternal
        NtGdiSetDIBitsToDeviceInternalOffset=pDriverMsg->NtGSDIBTDIOffset-0x1000;
        NtGdiSetDIBitsToDeviceInternalAddress=(PVOID*)MmMapAddress(&ServiceDescriptorShadowTable->win32k.ServiceTable[NtGdiSetDIBitsToDeviceInternalOffset],sizeof(PVOID));
        pNtGdiSetDIBitsToDeviceInternal=(tNtGdiSetDIBitsToDeviceInternal)*NtGdiSetDIBitsToDeviceInternalAddress;
        pSecureNtGdiSetDIBitsToDeviceInternal=(tNtGdiSetDIBitsToDeviceInternal)*NtGdiSetDIBitsToDeviceInternalAddress;
      }

      bDriverInit=TRUE;
    }
  }
  if(bDriverInit){
    switch(pIoStackIrp->Parameters.DeviceIoControl.IoControlCode){
      case IOCTL_DRIVER_END:
        ForceRestoreSystem();
      break;
      case IOCTL_DRIVER_BMPBUF:
        if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength){
          pSSMsg=(PSSMSG)(MmGetSystemAddressForMdlSafe(Irp->MdlAddress,NormalPagePriority));
          if(pSSMsg){
            if(!pSSBuffer)
              pSSBuffer=(PSSMSG)ExAllocatePoolWithTag(NonPagedPool,pSSMsg->uBuffSize,0);
            else
              if(pSSBuffer->uBuffSize!=pSSMsg->uBuffSize){
                ExFreePoolWithTag(pSSBuffer,0);
                pSSBuffer=(PSSMSG)ExAllocatePoolWithTag(NonPagedPool,pSSMsg->uBuffSize,0);
              }
            if(pSSBuffer)
              RtlCopyMemory(pSSBuffer,pSSMsg,pSSMsg->uBuffSize);
            else{
              ntStatus=STATUS_INSUFFICIENT_RESOURCES;
            }
          }
          Irp->IoStatus.Information=MmGetMdlByteCount(Irp->MdlAddress);
        }
      break;
      default:break;
    }
  }
  Irp->IoStatus.Status=ntStatus;
  IofCompleteRequest(Irp,0);
  return 0;
}

VOID DDKAPI DriverUnload(PDRIVER_OBJECT pDriverObject){
  UNICODE_STRING SymbolicLinkName;
  //was loaded ok?
  if(bNotifyRoutineCreated){
    //restore system
    ForceRestoreSystem();
    //remove NotifyRoutine
    PsSetCreateProcessNotifyRoutine(NotifyRoutine,TRUE);
    //remove names
    RtlInitUnicodeString(&SymbolicLinkName,sCreateSymbolicLinkName);
    IoDeleteSymbolicLink(&SymbolicLinkName);
    IoDeleteDevice(pDriverObject->DeviceObject);
  }
}

NTSTATUS DDKAPI CreateClose(PDEVICE_OBJECT DeviceObject,PIRP Irp){
  Irp->IoStatus.Information=0;
  Irp->IoStatus.Status=0;
  IofCompleteRequest(Irp,0);
  return STATUS_SUCCESS;
}

NTSTATUS DDKAPI DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING pRegistryPath){
  NTSTATUS NtStatus=STATUS_SUCCESS;
  PDEVICE_OBJECT pDeviceObject=NULL;
  UNICODE_STRING DeviceName,SymbolicLinkName;
  dwTimeIncrement=KeQueryTimeIncrement();
  //create named driver...
  pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=DeviceControl;
  pDriverObject->MajorFunction[IRP_MJ_CREATE]=CreateClose;
  pDriverObject->MajorFunction[IRP_MJ_CLOSE]=CreateClose;
  pDriverObject->DriverUnload=DriverUnload;
  //initialize name
  RtlInitUnicodeString(&DeviceName,sCreateDeviceName);
  RtlInitUnicodeString(&SymbolicLinkName,sCreateSymbolicLinkName);
  NtStatus=IoCreateDevice(pDriverObject,0,&DeviceName,FILE_DEVICE_UNKNOWN,0,0,&pDeviceObject);
  if(NtStatus==STATUS_SUCCESS){
    NtStatus=IoCreateSymbolicLink(&SymbolicLinkName,&DeviceName);
    if(NtStatus==STATUS_SUCCESS){
      pDeviceObject->Flags&=(~DO_DEVICE_INITIALIZING);
      //create NotifyRoutine
      NtStatus=PsSetCreateProcessNotifyRoutine(NotifyRoutine,FALSE);
      if(NtStatus==STATUS_SUCCESS){
        //initalize events
        bNotifyRoutineCreated=TRUE;
        HideDriver(pDriverObject);
      }else{
        IoDeleteSymbolicLink(&SymbolicLinkName);
        IoDeleteDevice(pDeviceObject);
      }
    }else{
      IoDeleteDevice(pDeviceObject);
    }
  }
  return NtStatus;
}
