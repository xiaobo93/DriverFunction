#pragma  once 
#include<wdm.h>
#include"AttachToFDO.h"

#define KBDCLASS_DRIVER_NAME	L"\\driver\\kbdclass"

typedef struct _Device_Extension_{
	PDEVICE_OBJECT LowerDeviceObject;
}*PDEVICE_EXTENSION,DEVICE_EXTENSION;


VOID keyUnload(IN PDRIVER_OBJECT DriverObject)
{
	PDEVICE_EXTENSION pDev = NULL;
	PDEVICE_OBJECT pDeviceObject = NULL;
	//GagOperClassUsbReg(TRUE,L"myScanDriver\0",L"{4d36e96b-e325-11ce-bfc1-08002be10318}");
	//卸载绑定的设备对象
	pDeviceObject = DriverObject->DeviceObject;
	while (pDeviceObject)
	{
		PDEVICE_OBJECT pTempDeviceObject = pDeviceObject->NextDevice;
		pDev = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
		IoDetachDevice(pDev->LowerDeviceObject);
		pDev->LowerDeviceObject = NULL;
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = pTempDeviceObject;
		KdPrint(("keyUnload \n"));
	}
	//KdPrint(("keyUnload\n"));
}
NTSTATUS keyDispathGeneral(IN PDEVICE_OBJECT DeviceObject,IN PIRP irp)
{
	NTSTATUS status = STATUS_SUCCESS;

	return status;
}
NTSTATUS AttachToKeyBoard(PDRIVER_OBJECT pDriverObject);


NTSYSAPI
NTSTATUS
NTAPI ObReferenceObjectByName( IN PUNICODE_STRING ObjectName,
							  IN ULONG Attributes,
							  IN PACCESS_STATE AccessState OPTIONAL,
							  IN ACCESS_MASK DesiredAccess OPTIONAL,
							  IN POBJECT_TYPE ObjectType,
							  IN KPROCESSOR_MODE AccessMode,
							  IN OUT PVOID ParseContext OPTIONAL,
							  OUT PVOID* Object );

extern POBJECT_TYPE* IoDriverObjectType;


NTSTATUS AttachToKeyBoard(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDRIVER_OBJECT kbdclassDrvObj;
	UNICODE_STRING kdbclassDrvName;
	RtlInitUnicodeString(&kdbclassDrvName,KBDCLASS_DRIVER_NAME);
	status = ObReferenceObjectByName(&kdbclassDrvName,OBJ_CASE_INSENSITIVE,NULL,0,*IoDriverObjectType,KernelMode,NULL,(PVOID*)&kbdclassDrvObj);
	if(NT_SUCCESS(status))
	{
		ObDereferenceObject(kbdclassDrvObj);
	}else{
		KdPrint(("ObReferenceObjectByName error \n"));
		goto EndFun;
	}

	{//遍历绑定设备
		PDEVICE_OBJECT pTargetDeviceObject = kbdclassDrvObj->DeviceObject;
		PDEVICE_OBJECT pFilterDeviceObject = NULL;
		PDEVICE_EXTENSION devExt = NULL;
		while(NULL != pTargetDeviceObject)
		{
			status = IoCreateDevice(
				pDriverObject,
				sizeof(DEVICE_EXTENSION),
				NULL,
				pTargetDeviceObject->DeviceType,
				pTargetDeviceObject->Characteristics,
				FALSE,
				&pFilterDeviceObject
				);
			if(!NT_SUCCESS(status))
			{
				goto EndFun;
			}
			devExt = (PDEVICE_EXTENSION)pFilterDeviceObject->DeviceExtension;
			devExt->LowerDeviceObject = IoAttachDeviceToDeviceStack(pFilterDeviceObject,pTargetDeviceObject);
			if(!(devExt->LowerDeviceObject))
			{
				goto EndFun;
			}

			pFilterDeviceObject->DeviceType=devExt->LowerDeviceObject->DeviceType; 
			pFilterDeviceObject->Characteristics=devExt->LowerDeviceObject->Characteristics; 
			pFilterDeviceObject->StackSize=devExt->LowerDeviceObject->StackSize+1; 
			pFilterDeviceObject->Flags |= devExt->LowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE) ; 

			pFilterDeviceObject->Flags=pFilterDeviceObject->Flags & ~DO_DEVICE_INITIALIZING;

			pTargetDeviceObject = pTargetDeviceObject->NextDevice;
		}
	}

EndFun:
	return status;

}
NTSTATUS keyDispatchPnp(IN PDEVICE_OBJECT pDeviceObject,IN PIRP Irp)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION pdevExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	switch(irpStack->MinorFunction)
	{
	case  IRP_MN_REMOVE_DEVICE:
		IoSkipCurrentIrpStackLocation(Irp);
		IoCallDriver(pdevExt->LowerDeviceObject,Irp);
		IoDetachDevice(pdevExt->LowerDeviceObject);
		pdevExt->LowerDeviceObject = NULL;
		IoDeleteDevice(pDeviceObject);
		status = STATUS_SUCCESS;
		break;
	default:
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(pdevExt->LowerDeviceObject,Irp);
	}
	return status;

}
NTSTATUS keyDispatchRead( 
						 IN PDEVICE_OBJECT DeviceObject, 
						 IN PIRP Irp ) 
{ 
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION dev = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IoSkipCurrentIrpStackLocation(Irp);
	KdPrint(("----------------------read\n"));
	status = IoCallDriver(dev->LowerDeviceObject,Irp);
	KdPrint(("keyDispatchRead\n"));

	return status;	
}
NTSTATUS keyDispatchGeneral(IN PDEVICE_OBJECT DeviceObject,IN PIRP	Irp	)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION dev = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(dev->LowerDeviceObject,Irp);
	return status;
}
NTSTATUS keyAddDevice (
				   __in struct _DRIVER_OBJECT *DriverObject,
				   __in struct _DEVICE_OBJECT *PhysicalDeviceObject
				   )
{
	NTSTATUS status = STATUS_SUCCESS;
	_asm int 3;
	if(NULL != DriverObject && NULL != PhysicalDeviceObject)
	{
		PDEVICE_OBJECT pFilterDeviceObject = NULL;
		PDEVICE_EXTENSION devExt = NULL;
		KdPrint(("begin attach DeviceObject\n"));
		status = IoCreateDevice(DriverObject,
			sizeof(DEVICE_EXTENSION),
			NULL,
			PhysicalDeviceObject->DeviceType,
			PhysicalDeviceObject->Characteristics,
			FALSE,
			&pFilterDeviceObject
			);
		if(!NT_SUCCESS(status))
		{
			KdPrint((" keyAddDevice IoCreateDevice failed\n"));
			goto endFun;
		}
		devExt = (PDEVICE_EXTENSION)pFilterDeviceObject->DeviceExtension;
		devExt->LowerDeviceObject = IoAttachDeviceToDeviceStack(pFilterDeviceObject,PhysicalDeviceObject);
		if(!(devExt->LowerDeviceObject ))
		{
			KdPrint(("keyAddDevice IoAttachDeviceToDeviceStack failed\n"));
			goto endFun;
		}
		pFilterDeviceObject->DeviceType=devExt->LowerDeviceObject->DeviceType; 
		pFilterDeviceObject->Characteristics=devExt->LowerDeviceObject->Characteristics; 
		pFilterDeviceObject->StackSize=devExt->LowerDeviceObject->StackSize+1; 
		pFilterDeviceObject->Flags |= devExt->LowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE) ; 

		pFilterDeviceObject->Flags=pFilterDeviceObject->Flags & ~DO_DEVICE_INITIALIZING;
	}
endFun:
	KdPrint(("keyAddDevice \n"));
	return status;
}
NTSTATUS DriverEntry( 
					 IN PDRIVER_OBJECT DriverObject, 
					 IN PUNICODE_STRING RegistryPath 
					 ) 
{ 
	ULONG i; 
	NTSTATUS status = STATUS_SUCCESS; 
	KdPrint (("c2p.SYS: entering DriverEntry\n")); 
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) 
	{ 
		DriverObject->MajorFunction[i] = keyDispatchGeneral; 
	} 
	DriverObject->MajorFunction[IRP_MJ_READ] = keyDispatchRead; 
	//DriverObject->MajorFunction [IRP_MJ_POWER] = c2pPower; 
	DriverObject->MajorFunction [IRP_MJ_PNP] = keyDispatchPnp; 
	DriverObject->DriverUnload = keyUnload; 
	DriverObject->DriverExtension->AddDevice = keyAddDevice;
	//gDriverObject = DriverObject;
	status =AttachToKeyBoard(DriverObject);
	GagOperClassUsbReg(FALSE,L"keyboardFilter\0",L"{4d36e96b-e325-11ce-bfc1-08002be10318}");
	return status; 
}