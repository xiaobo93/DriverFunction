#include "AttachToFDO.h"

NTSTATUS GagQueryRoutine( PWSTR ValueName,ULONG ValueType,PVOID ValueData,ULONG ValueLength,PVOID Context,PVOID EntryContext );

// 功能模块使用的变量。
typedef int	BOOL;
#define MAX_PATH	260

// 注册表回掉函数的结构体
typedef struct _CLASS_REG
{
	BOOL bIsDelete;
	WCHAR serviceName[MAX_PATH];
	WCHAR calssReg[MAX_PATH];

}ClassReg, *PClassReg;

/************************************************************************/
/* 函数名： 
/* 函数参数：
/* bIsDelete ：是否卸载（TRUE: 卸载服务；FALSE:安装服务）
/* pServiceName ：服务名
/* pGuid : GUID
/************************************************************************/
VOID GagOperClassUsbReg(int bIsDelete,PWCHAR pServiceName,PWCHAR pGuid)
{

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	RTL_QUERY_REGISTRY_TABLE paramTable[2];
	WCHAR Log[100] = { 0 };
	ClassReg MyClassReg = { 0 };
	MyClassReg.bIsDelete = bIsDelete;

	RtlZeroMemory(paramTable, sizeof(paramTable));
	paramTable[0].QueryRoutine = GagQueryRoutine;
	paramTable[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
	paramTable[0].Name = L"UpperFilters";
	paramTable[0].EntryContext = NULL;
	paramTable[0].DefaultType = REG_MULTI_SZ;
	paramTable[0].DefaultData = L"GAGNOTFOUND";
	paramTable[0].DefaultLength = sizeof(L"GAGNOTFOUND");

	
	//RtlStringCchCopyW(MyClassReg.calssReg, MAX_PATH, L"Class\\{36fc9e60-c465-11cf-8056-444553540000}");
	RtlStringCchCopyW(MyClassReg.serviceName,MAX_PATH,pServiceName);
	RtlStringCchPrintfW(MyClassReg.calssReg,MAX_PATH,L"Class\\%ws",pGuid);
	KdPrint(("MyClassReg.calssReg . . . %ws\n",MyClassReg.calssReg));
	//RtlStringCchCopyW(MyClassReg.calssReg, MAX_PATH, L"Class\\{4d36e96b-e325-11ce-bfc1-08002be10318}");
	status = RtlQueryRegistryValues(
		RTL_REGISTRY_CONTROL, 
		MyClassReg.calssReg, 
		paramTable, 
		(PVOID)&MyClassReg, 
		NULL
		);
	if (!NT_SUCCESS(status))
	{
		RtlStringCchPrintfW(Log, 100, L"<GagOperClassUsbReg> Call RtlQueryRegistryValues Error : 0x%x", status);
		KdPrint(("Log\n"));
		goto exitlab;
	}
exitlab:
	return;
}
NTSTATUS GagQueryRoutine(
						 PWSTR ValueName,
						 ULONG ValueType,
						 PVOID ValueData,
						 ULONG ValueLength,
						 PVOID Context,
						 PVOID EntryContext
						 )
{
	ClassReg MyClassReg = *(ClassReg*)Context;
	BOOLEAN bIsFound = FALSE;
	WCHAR *pPtr = NULL;
	WCHAR regData[MAX_PATH] = {0};
	PVOID pRegData = (PVOID)regData;
	PVOID pPtrbk = ValueData;
	ULONG length = 0;
	WCHAR Log[100] = { 0 };
	

	RtlStringCchCopyW(regData,MAX_PATH,ValueData);

	//_asm int 3;

	__try
	{
		// 创建
		if (!MyClassReg.bIsDelete)
		{
			int t;
			if (wcscmp(ValueData, L"GAGNOTFOUND") == 0 || ValueLength <= 4)
			{
				RtlWriteRegistryValue(
					RTL_REGISTRY_CONTROL,
					MyClassReg.calssReg,
					L"UpperFilters",
					REG_MULTI_SZ,
					MyClassReg.serviceName,
					wcslen(MyClassReg.serviceName)*2+2
					);

				goto exitflag;
			}

			do
			{
				pPtr = (PWCHAR)pRegData;
				length += wcslen(pPtr)*2+2;

				if (wcscmp(pPtr, MyClassReg.serviceName) == 0)
				{
					goto exitflag;
				}

				(PCHAR)pRegData += wcslen(pPtr)*2+2;
			} while (wcslen(pRegData) > 0);
			
			RtlStringCchCopyW(pRegData,MAX_PATH-length,MyClassReg.serviceName);
			RtlWriteRegistryValue(
				RTL_REGISTRY_CONTROL,
				MyClassReg.calssReg,
				L"UpperFilters",
				REG_MULTI_SZ,
				regData,
				length+wcslen(MyClassReg.serviceName)*2+2
				);
		}
		else
		{
			// 删除
			if (wcscmp(ValueData, L"GAGNOTFOUND") == 0 || ValueLength <= 4)
			{
				goto exitflag;
			}

			do
			{
				pPtr = (PWCHAR)ValueData;
				length += wcslen(pPtr)*2+2;

				if (wcscmp(pPtr, MyClassReg.serviceName) == 0)
				{
					DbgPrint("[GagSecurity.sys] <%s> cmp\n", __FUNCTION__);
					bIsFound = TRUE;
					RtlMoveMemory(ValueData,(PVOID)((PCHAR)ValueData+wcslen(MyClassReg.serviceName)*2+2), ValueLength-length);
					break;
				}

				(PCHAR)ValueData += wcslen(pPtr)*2+2;
			} while (wcslen(ValueData) > 0);

			if (bIsFound)
			{
				RtlWriteRegistryValue(
					RTL_REGISTRY_CONTROL,
					MyClassReg.calssReg,
					L"UpperFilters",
					REG_MULTI_SZ,
					pPtrbk,
					ValueLength - (wcslen(MyClassReg.serviceName)+2) - 2
					);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		//RtlStringCchCopyW(Log, 100, L"<GagQueryRoutine> EXCEPTION_EXECUTE_HANDLER");
		//KernelWriteLog(L"PortDriver", Log);
		DbgPrint("[PortDriver.sys] <%s> EXCEPTION_EXECUTE_HANDLER\n", __FUNCTION__);
	}

exitflag:
	return STATUS_SUCCESS;
}