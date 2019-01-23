#pragma  once
#include<wdm.h>
#include<ntstrsafe.h>


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