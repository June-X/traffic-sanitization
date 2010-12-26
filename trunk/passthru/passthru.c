/*++

Copyright (c) 1992-2000  Microsoft Corporation
 
Module Name:
 
    passthru.c

Abstract:

    Ndis Intermediate Miniport driver sample. This is a passthru driver.

Author:

Environment:


Revision History:


--*/


#include "precomp.h"
#include "umac.h"
#pragma hdrstop

#pragma NDIS_INIT_FUNCTION(DriverEntry)

NDIS_HANDLE         ProtHandle = NULL;
NDIS_HANDLE         DriverHandle = NULL;
NDIS_MEDIUM         MediumArray[4] =
                    {
                        NdisMedium802_3,    // Ethernet
                        NdisMedium802_5,    // Token-ring
                        NdisMediumFddi,     // Fddi
                        NdisMediumWan       // NDISWAN
                    };

NDIS_SPIN_LOCK     GlobalLock;
NDIS_SPIN_LOCK	   MacListLock;

PADAPT             pAdaptList = NULL;
LONG               MiniportCount = 0;

NDIS_HANDLE        NdisWrapperHandle;

//
// To support ioctls from user-mode:
//

#define LINKNAME_STRING     L"\\DosDevices\\Passthru"
#define NTDEVICE_STRING     L"\\Device\\Passthru"

NDIS_HANDLE     NdisDeviceHandle = NULL;
PDEVICE_OBJECT  ControlDeviceObject = NULL;

PMacNode MacListHead = NULL;

umac_ctx_t UMAC = NULL;
ULONG time_per_tick = 0;
PDRIVER_OBJECT PassthruDriver;
HANDLE LogFileHandle;


enum _DEVICE_STATE
{
    PS_DEVICE_STATE_READY = 0,    // ready for create/delete
    PS_DEVICE_STATE_CREATING,    // create operation in progress
    PS_DEVICE_STATE_DELETING    // delete operation in progress
} ControlDeviceState = PS_DEVICE_STATE_READY;



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT        DriverObject,
    IN PUNICODE_STRING       RegistryPath
    )
/*++

Routine Description:

    First entry point to be called, when this driver is loaded.
    Register with NDIS as an intermediate driver.

Arguments:

    DriverObject - pointer to the system's driver object structure
        for this driver
    
    RegistryPath - system's registry path for this driver
    
Return Value:

    STATUS_SUCCESS if all initialization is successful, STATUS_XXX
    error code if not.

--*/
{
    NDIS_STATUS                        Status;
    NDIS_PROTOCOL_CHARACTERISTICS      PChars;
    NDIS_MINIPORT_CHARACTERISTICS      MChars;
    NDIS_STRING                        Name;
    UNICODE_STRING	LogFilename;
		OBJECT_ATTRIBUTES LogFileObject;
	IO_STATUS_BLOCK isb;
    Status = NDIS_STATUS_SUCCESS;

    UMAC = umac_new("abcdefghijklmnopqrstuvwxyz");

    NdisAllocateSpinLock(&GlobalLock);
    NdisAllocateSpinLock(&MacListLock);

/*
  RtlInitUnicodeString(&LogFilename, L"\DosDevices\C:\WINDOWS\system32\LogFiles\performance.log");
  InitializeObjectAttributes(&LogFileObject, &LogFilename, OBJ_KERNEL_HANDLE, NULL, NULL);
  Status = ZwCreateFile(&LogFileHandle, FILE_APPEND_DATA, &LogFileObject, 
  	&isb, NULL, FILE_ATTRIBUTE_NORMAL, 
  	FILE_SHARE_READ, FILE_OPEN_IF, FILE_SEQUENTIAL_ONLY, NULL, 0);
  if (Status != STATUS_SUCCESS) {
  	KdPrint(("[passthru] ZwCreateFile Failed\n"));
	return Status;
  }
  */
   time_per_tick = KeQueryTimeIncrement();
   PassthruDriver = DriverObject;
    NdisMInitializeWrapper(&NdisWrapperHandle, DriverObject, RegistryPath, NULL);

    do
    {
        //
        // Register the miniport with NDIS. Note that it is the miniport
        // which was started as a driver and not the protocol. Also the miniport
        // must be registered prior to the protocol since the protocol's BindAdapter
        // handler can be initiated anytime and when it is, it must be ready to
        // start driver instances.
        //

        NdisZeroMemory(&MChars, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

        MChars.MajorNdisVersion = PASSTHRU_MAJOR_NDIS_VERSION;
        MChars.MinorNdisVersion = PASSTHRU_MINOR_NDIS_VERSION;

        MChars.InitializeHandler = MPInitialize;
        MChars.QueryInformationHandler = MPQueryInformation;
        MChars.SetInformationHandler = MPSetInformation;
        MChars.ResetHandler = NULL;
        MChars.TransferDataHandler = MPTransferData;
        MChars.HaltHandler = MPHalt;
#ifdef NDIS51_MINIPORT
        MChars.CancelSendPacketsHandler = MPCancelSendPackets;
        MChars.PnPEventNotifyHandler = MPDevicePnPEvent;
        MChars.AdapterShutdownHandler = MPAdapterShutdown;
#endif // NDIS51_MINIPORT

        //
        // We will disable the check for hang timeout so we do not
        // need a check for hang handler!
        //
        MChars.CheckForHangHandler = NULL;
        MChars.ReturnPacketHandler = MPReturnPacket;

        //
        // Either the Send or the SendPackets handler should be specified.
        // If SendPackets handler is specified, SendHandler is ignored
        //
        MChars.SendHandler = NULL;    // MPSend;
        MChars.SendPacketsHandler = MPSendPackets;

        Status = NdisIMRegisterLayeredMiniport(NdisWrapperHandle,
                                                  &MChars,
                                                  sizeof(MChars),
                                                  &DriverHandle);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

#ifndef WIN9X
        NdisMRegisterUnloadHandler(NdisWrapperHandle, PtUnload);
#endif

        //
        // Now register the protocol.
        //
        NdisZeroMemory(&PChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
        PChars.MajorNdisVersion = PASSTHRU_PROT_MAJOR_NDIS_VERSION;
        PChars.MinorNdisVersion = PASSTHRU_PROT_MINOR_NDIS_VERSION;

        //
        // Make sure the protocol-name matches the service-name
        // (from the INF) under which this protocol is installed.
        // This is needed to ensure that NDIS can correctly determine
        // the binding and call us to bind to miniports below.
        //
        NdisInitUnicodeString(&Name, L"Passthru");    // Protocol name
        PChars.Name = Name;
        PChars.OpenAdapterCompleteHandler = PtOpenAdapterComplete;
        PChars.CloseAdapterCompleteHandler = PtCloseAdapterComplete;
        PChars.SendCompleteHandler = PtSendComplete;
        PChars.TransferDataCompleteHandler = PtTransferDataComplete;
    
        PChars.ResetCompleteHandler = PtResetComplete;
        PChars.RequestCompleteHandler = PtRequestComplete;
        PChars.ReceiveHandler = PtReceive;
        PChars.ReceiveCompleteHandler = PtReceiveComplete;
        PChars.StatusHandler = PtStatus;
        PChars.StatusCompleteHandler = PtStatusComplete;
        PChars.BindAdapterHandler = PtBindAdapter;
        PChars.UnbindAdapterHandler = PtUnbindAdapter;
        PChars.UnloadHandler = PtUnloadProtocol;

        PChars.ReceivePacketHandler = PtReceivePacket;
        PChars.PnPEventHandler= PtPNPHandler;

        NdisRegisterProtocol(&Status,
                             &ProtHandle,
                             &PChars,
                             sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

        if (Status != NDIS_STATUS_SUCCESS)
        {
            NdisIMDeregisterLayeredMiniport(DriverHandle);
            break;
        }

        NdisIMAssociateMiniport(DriverHandle, ProtHandle);
    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(NdisWrapperHandle, NULL);
    }

    return(Status);
}


NDIS_STATUS
PtRegisterDevice(
    VOID
    )
/*++

Routine Description:

    Register an ioctl interface - a device object to be used for this
    purpose is created by NDIS when we call NdisMRegisterDevice.

    This routine is called whenever a new miniport instance is
    initialized. However, we only create one global device object,
    when the first miniport instance is initialized. This routine
    handles potential race conditions with PtDeregisterDevice via
    the ControlDeviceState and MiniportCount variables.

    NOTE: do not call this from DriverEntry; it will prevent the driver
    from being unloaded (e.g. on uninstall).

Arguments:

    None

Return Value:

    NDIS_STATUS_SUCCESS if we successfully register a device object.

--*/
{
    NDIS_STATUS            Status = NDIS_STATUS_SUCCESS;
    UNICODE_STRING         DeviceName;
    UNICODE_STRING         DeviceLinkUnicodeString;
    PDRIVER_DISPATCH       DispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1];

    DBGPRINT(("==>PtRegisterDevice\n"));

    NdisAcquireSpinLock(&GlobalLock);

    ++MiniportCount;
    
    if (1 == MiniportCount)
    {
        ASSERT(ControlDeviceState != PS_DEVICE_STATE_CREATING);

        //
        // Another thread could be running PtDeregisterDevice on
        // behalf of another miniport instance. If so, wait for
        // it to exit.
        //
        while (ControlDeviceState != PS_DEVICE_STATE_READY)
        {
            NdisReleaseSpinLock(&GlobalLock);
            NdisMSleep(1);
            NdisAcquireSpinLock(&GlobalLock);
        }

        ControlDeviceState = PS_DEVICE_STATE_CREATING;

        NdisReleaseSpinLock(&GlobalLock);

    
        NdisZeroMemory(DispatchTable, (IRP_MJ_MAXIMUM_FUNCTION+1) * sizeof(PDRIVER_DISPATCH));

        DispatchTable[IRP_MJ_CREATE] = PtDispatch;
        DispatchTable[IRP_MJ_CLEANUP] = PtDispatch;
        DispatchTable[IRP_MJ_CLOSE] = PtDispatch;
        DispatchTable[IRP_MJ_DEVICE_CONTROL] = PtDispatch;
	 DispatchTable[IRP_MJ_INTERNAL_DEVICE_CONTROL] = PtDispatch;
        

        NdisInitUnicodeString(&DeviceName, NTDEVICE_STRING);
        NdisInitUnicodeString(&DeviceLinkUnicodeString, LINKNAME_STRING);

        //
        // Create a device object and register our dispatch handlers
        //
        
        Status = NdisMRegisterDevice(
                    NdisWrapperHandle, 
                    &DeviceName,
                    &DeviceLinkUnicodeString,
                    &DispatchTable[0],
                    &ControlDeviceObject,
                    &NdisDeviceHandle
                    );

        NdisAcquireSpinLock(&GlobalLock);

        ControlDeviceState = PS_DEVICE_STATE_READY;
    }

    NdisReleaseSpinLock(&GlobalLock);

    DBGPRINT(("<==PtRegisterDevice: %x\n", Status));

    return (Status);
}

static NTSTATUS
GetMAC(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIrpSp = NULL;
	PMacNode node = NULL;
	PMacNode p = NULL;
	ULONG count = 0;
	
	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	node = (PMacNode)ExAllocatePoolWithTag(NonPagedPool, sizeof(MacNode), MACNODE_POOL_TAG);
	if (node) {
		node->Buffer = pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
		node->Length = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
		NdisAcquireSpinLock(&MacListLock);
		if (!MacListHead) {
			MacListHead = node;
			MacListHead->Next = NULL;
		} else {
			node->Next = MacListHead;
			MacListHead = node;
		}
		NdisReleaseSpinLock(&MacListLock);
	} else {
		//KdPrint(("[passthru] GetMac::ExAllocatePoolWithTag Error\n"));
		//ntStatus = STATUS_WARNING;
	}
	NdisAcquireSpinLock(&MacListLock);
	p = MacListHead;
	while (p) {
		p = p->Next;
		count++;
	}
	NdisReleaseSpinLock(&MacListLock);
	//KdPrint(("[passthru] GetMac::count %lu\n", count));
	return ntStatus;	
}


NTSTATUS
PtDispatch(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++
Routine Description:

    Process IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object
    Irp      - pointer to an I/O Request Packet

Return Value:

    NTSTATUS - STATUS_SUCCESS always - change this when adding
    real code to handle ioctls.

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(DeviceObject);
    
    //DBGPRINT(("==>Pt Dispatch\n"));
    irpStack = IoGetCurrentIrpStackLocation(Irp);
      

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            break;
            
        case IRP_MJ_CLEANUP:
            break;
            
        case IRP_MJ_CLOSE:
            break;        
            
        case IRP_MJ_DEVICE_CONTROL:
            //
            // Add code here to handle ioctl commands sent to passthru.
            //
            break;
	  case IRP_MJ_INTERNAL_DEVICE_CONTROL:
	  	if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CMD_PASSTHRU_INTERNAL) {
			status = GetMAC(DeviceObject, Irp);
		}
		break;			
        default:
            break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //DBGPRINT(("<== Pt Dispatch\n"));

    return status;

} 

NDIS_STATUS
PtDeregisterDevice(
    VOID
    )
/*++

Routine Description:

    Deregister the ioctl interface. This is called whenever a miniport
    instance is halted. When the last miniport instance is halted, we
    request NDIS to delete the device object

Arguments:

    NdisDeviceHandle - Handle returned by NdisMRegisterDevice

Return Value:

    NDIS_STATUS_SUCCESS if everything worked ok

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DBGPRINT(("==>PassthruDeregisterDevice\n"));

    NdisAcquireSpinLock(&GlobalLock);

    ASSERT(MiniportCount > 0);

    --MiniportCount;
    
    if (0 == MiniportCount)
    {
        //
        // All miniport instances have been halted. Deregister
        // the control device.
        //

        ASSERT(ControlDeviceState == PS_DEVICE_STATE_READY);

        //
        // Block PtRegisterDevice() while we release the control
        // device lock and deregister the device.
        // 
        ControlDeviceState = PS_DEVICE_STATE_DELETING;

        NdisReleaseSpinLock(&GlobalLock);

        if (NdisDeviceHandle != NULL)
        {
            Status = NdisMDeregisterDevice(NdisDeviceHandle);
            NdisDeviceHandle = NULL;
        }

        NdisAcquireSpinLock(&GlobalLock);
        ControlDeviceState = PS_DEVICE_STATE_READY;
    }

    NdisReleaseSpinLock(&GlobalLock);

    DBGPRINT(("<== PassthruDeregisterDevice: %x\n", Status));
    return Status;
    
}

VOID
PtUnload(
    IN PDRIVER_OBJECT        DriverObject
    )
//
// PassThru driver unload function
//
{
    UNREFERENCED_PARAMETER(DriverObject);
    
    DBGPRINT(("PtUnload: entered\n"));
    
    PtUnloadProtocol();
    
    NdisIMDeregisterLayeredMiniport(DriverHandle);
    
    NdisFreeSpinLock(&GlobalLock);

    DBGPRINT(("PtUnload: done!\n"));
}

