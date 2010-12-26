/* Copyright (c) 2002-2005 Vladislav Goncharov.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */
 
// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: disp_sr.c,v 1.2 2003/09/04 15:20:09 dev Exp $

/*
 * This file contains TDI_SEND and TDI_RECEIVE handlers
 */

#include <ntddk.h>
#include <tdikrnl.h>
#include "sock.h"
#include "filter.h"
#include <ntstrsafe.h>
#include "wdm.h"

#include "dispatch.h"
#include "memtrack.h"
#include "obj_tbl.h"
#include "tdi_fw.h"
#include "umac.h"

//Added by Xiong Sep142009, 17:39:16
#include "disp_sr.h"
#include "disp_dg.h"

#define HASH_SIZE 64


UCHAR* hash_table[TABLE_SIZE] = {"31D6CFE0D16AE931B73C59D7E0C089C0",
							"9CCD2FC4E87235E21C18FE75FF5880D8",
							"09CAFC4E27377A1A34DF04D673EFCB07",
							"55D902D6DCAA5ADA3FB259DC3CCE0DA5",
							"FC854BFADF0DF0587BE31E2FBF6A3A31",
							"31D6CFE0D16AE931B73C59D7E0C089C0",
							"8F5BBA3DA305D493E0A6A19705E86545",
							"31D6CFE0D16AE931B73C59D7E0C089C0",
							"31D6CFE0D16AE931B73C59D7E0C089C0",
							"31D6CFE0D16AE931B73C59D7E0C089C0",
							"6A42AF34D363926C983DCB4B2240E13B",
							"C9F98EB684A54BE451A00536EA6DC6C8",
							"FE2797F565DC01EC8709347DB4A57452",
							"F72B5DFBB6EC55ED3785B4CB9331699C",
							"31D6CFE0D16AE931B73C59D7E0C089C0",
							"31D6CFE0D16AE931B73C59D7E0C089C0",
							"36ADB9838A31012C7EE1AAC0364CAAC9",
							"DEB4CCC9912548C39583D417AFAD765F",
							"0166266EB8A982BED9E9AB56FDE8F86B",
							"9AB35D657DBFB553675DC2B3EB3F1F0D",
							"8EB1D8ECD44BF80A37726435D47366A6"
							}; 

static NTSTATUS tdi_receive_complete(
	IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);

//----------------------------------------------------------------------------

/*
 * TDI_SEND handler
 */

int
log_http_header(struct ot_entry *ote_conn, UCHAR* header_data, ULONG header_length)
{
	struct flt_request request;
	
	
	TA_ADDRESS* local_addr = (TA_ADDRESS *)(ote_conn->local_addr);
	TA_ADDRESS* remote_addr = (TA_ADDRESS *)(ote_conn->remote_addr);

	UCHAR* p;
	UCHAR* p2;

	PIRP pIrp;
	PUCHAR pMac = NULL;
	IO_STATUS_BLOCK isb;
	UCHAR nonce[8] = {0};

	LARGE_INTEGER tick;
	
	KdPrint(("[tdi_flt] log_http_header: %x:%u -> %x:%u\n",
		ntohl(((TDI_ADDRESS_IP *)(local_addr->Address))->in_addr),
		ntohs(((TDI_ADDRESS_IP *)(local_addr->Address))->sin_port),
		ntohl(((TDI_ADDRESS_IP *)(remote_addr->Address))->in_addr),
		ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port)));

	memset(&request, 0, sizeof(request));

	request.struct_size = sizeof(request);
	request.type = 0;
	request.result = FILTER_HTTP;
	request.proto = ote_conn->ipproto;
	request.direction = DIRECTION_OUT;

	request.pid = ote_conn->pid;		

	// get user SID & attributes!
	if ((request.sid_a = copy_sid_a(ote_conn->sid_a, ote_conn->sid_a_size)) != NULL)
		request.sid_a_size = ote_conn->sid_a_size;

	memcpy(&request.addr.from, &local_addr->AddressType, sizeof(struct sockaddr));
	memcpy(&request.addr.to, &remote_addr->AddressType, sizeof(struct sockaddr));
	request.addr.len = sizeof(struct sockaddr_in);

	request.log_bytes_in = ote_conn->bytes_in;
	request.log_bytes_out = header_length;

	memset(request.http_url, 0, 128);
	p = strstr(header_data, " HTTP/1.1");
	if (p == NULL) {
		p = strstr(header_data, " HTTP/1.0");
	}
	if (p == NULL) {
		//KdPrint(("[tdi_test]:it is in log_http_header and we are trying to find the header of http, but cann't find it"));
		return 0;
		//Add by Xiong Aug12, 2010
		//memcpy(request.http_url, header_data, 127 < p - header_data ? 127 : p - header_data);
	} else {
		memcpy(request.http_url, header_data, 127 < p - header_data ? 127 : p - header_data);
	}
	memset(request.http_host, 0, 64);
	p = strstr(header_data, "Host: ");
	if (p == NULL) {
		p = strstr(header_data, "HOST: ");
	}
	if (p == NULL) {
		memcpy(request.http_host, "not found host", 14);
	} else {
		p += strlen("Host: ");
		p2 = strstr(p, "\r\n");
		if (p2 == NULL) {
			memcpy(request.http_host, "not found host", 14);
		} else {
			memcpy(request.http_host, p, 63 < p2 - p ? 63 : p2 - p);
		}
	}

	log_request(&request);

	pMac = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, UMAC_OUTPUT_LEN + 1 + sizeof(tick) + 1, "cam");
	KeQueryTickCount(&tick);
	if (pMac) {
		KdPrint(("[tdi_test]:it is in log_http_header and we can have pMac"));
		memset(pMac, 0, UMAC_OUTPUT_LEN + 1 + sizeof(tick) + 1);
		memcpy(pMac + UMAC_OUTPUT_LEN + 1, &tick, sizeof(tick));
		umac_reset(UMAC);
		umac(UMAC, header_data, (header_length < 128) ? header_length : 128, pMac, nonce);
		pIrp = IoBuildDeviceIoControlRequest(IOCTL_CMD_PASSTHRU_INTERNAL, g_passthru_devobj,
								pMac, UMAC_OUTPUT_LEN + 1 + sizeof(tick) + 1, NULL, 0, TRUE, NULL, &isb);
		if (!pIrp) {
			KdPrint(("[tdi_flt] log_http_header: IoBuildDeviceIoControlReques Error\n"));
			ExFreePoolWithTag(pMac, "cam");
		}
		KdPrint(("[tdi_flt] log_http_header: IoBuildDeviceIoControlRequest: Done!\n"));
		IoCallDriver(g_passthru_devobj, pIrp);
	}
	return 1;
}

void
log_udp(struct ot_entry *ote_conn, UCHAR* header_data, ULONG header_length)
{
	struct flt_request request;
	
	TA_ADDRESS* local_addr = (TA_ADDRESS *)(ote_conn->local_addr);
	TA_ADDRESS* remote_addr = (TA_ADDRESS *)(ote_conn->remote_addr);

	UCHAR* p;
	UCHAR* p2;

	PIRP pIrp;
	PUCHAR pMac = NULL;
	IO_STATUS_BLOCK isb;
	UCHAR nonce[8] = {0};

	LARGE_INTEGER tick;
	
	KdPrint(("[tdi_flt] log_udp: %x:%u -> %x:%u\n",
		ntohl(((TDI_ADDRESS_IP *)(local_addr->Address))->in_addr),
		ntohs(((TDI_ADDRESS_IP *)(local_addr->Address))->sin_port),
		ntohl(((TDI_ADDRESS_IP *)(remote_addr->Address))->in_addr),
		ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port)));

	memset(&request, 0, sizeof(request));

	request.struct_size = sizeof(request);
	request.type = 0;
	request.result = FILTER_HTTP;
	request.proto = ote_conn->ipproto;
	request.direction = DIRECTION_OUT;

	request.pid = ote_conn->pid;		

	// get user SID & attributes!
	if ((request.sid_a = copy_sid_a(ote_conn->sid_a, ote_conn->sid_a_size)) != NULL)
		request.sid_a_size = ote_conn->sid_a_size;

	memcpy(&request.addr.from, &local_addr->AddressType, sizeof(struct sockaddr));
	memcpy(&request.addr.to, &remote_addr->AddressType, sizeof(struct sockaddr));
	request.addr.len = sizeof(struct sockaddr_in);

	request.log_bytes_in = ote_conn->bytes_in;
	request.log_bytes_out = header_length;

	log_request(&request);

	pMac = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, UMAC_OUTPUT_LEN + 1 + sizeof(tick) + 1, "cam");
	
	if (pMac) {
		KdPrint(("[tdi_test]it is log_udp and we have pMac"));
		tick = KeQueryPerformanceCounter(NULL);
		memset(pMac, 0, UMAC_OUTPUT_LEN + 1 + sizeof(tick) + 1);
		memcpy(pMac + UMAC_OUTPUT_LEN + 1, &tick, sizeof(tick));

		
		umac_reset(UMAC);
		umac(UMAC, header_data, (header_length < 256) ? header_length : 256, pMac, nonce);
		//umac(UMAC, header_data, header_length, pMac, nonce);
		
		//memcpy(pMac, header_data, (header_length < UMAC_OUTPUT_LEN) ? header_length : UMAC_OUTPUT_LEN);
		pIrp = IoBuildDeviceIoControlRequest(IOCTL_CMD_PASSTHRU_INTERNAL, g_passthru_devobj,
								pMac, UMAC_OUTPUT_LEN + 1 + sizeof(tick) + 1, NULL, 0, TRUE, NULL, &isb);
		if (!pIrp) {
			KdPrint(("[tdi_flt] log_udp: IoBuildDeviceIoControlReques Error\n"));
			ExFreePoolWithTag(pMac, "cam");
		}
		KdPrint(("[tdi_flt] log_udp: IoBuildDeviceIoControlRequest: Done!\n"));
		IoCallDriver(g_passthru_devobj, pIrp);
	}

}


//added by Oct25
/*
void get_current_time_sr(UCHAR* time) {
	LARGE_INTEGER current_system_time;
	LARGE_INTEGER local_time;
	TIME_FIELDS local_time_fields;
	
	KeQuerySystemTime(&current_system_time);
	ExSystemTimeToLocalTime(&current_system_time, &local_time);
	RtlTimeToTimeFields(&local_time, &local_time_fields);
	RtlStringCbPrintfA(time, 64 * sizeof(UCHAR), L"%d-%d-%d %d:%d:%d", local_time_fields.Year, 
		local_time_fields.Month, local_time_fields.Day, local_time_fields.Hour, 
		local_time_fields.Minute, local_time_fields.Second);
}
*/


int
tdi_send(PIRP irp, PIO_STACK_LOCATION irps, struct completion *completion)
{
	TDI_REQUEST_KERNEL_SEND *param = (TDI_REQUEST_KERNEL_SEND *)(&irps->Parameters);
	struct ot_entry *ote_conn;
	KIRQL irql;
	PVOID *data;
	//add by xiong Oct12, 2009
	UCHAR hash_value[HASH_SIZE] = {0};
	UCHAR hash_header[8] = {0};
	//add by xiong Oct14 2009
	//LARGE_INTEGER current_system_time;
	//LARGE_INTEGER local_time;
	//TIME_FIELDS local_time_fields;
	int compresult;
	int httpheader = 0;
	
	//added by xiong for P2P traffic sanitization
	//KeQuerySystemTime(&current_system_time);
	//ExSystemTimeToLocalTime(&current_system_time, &local_time);
 	//KdPrint(("[tdi_time]test: current time is %ld\n", local_time));
	//RtlTimeToTimeFields(&local_time, &local_time_fields);
	//KdPrint(("[tdi_time]tdifw-TCP-BeforeSending: %d-%d-%d %d:%d:%d.%d", local_time_fields.Year, local_time_fields.Month, local_time_fields.Day, local_time_fields.Hour, local_time_fields.Minute, local_time_fields.Second, local_time_fields.Milliseconds));
	//UCHAR time[256];
	//get_current_time_sr(time);

	//KdPrint(("[tdi_time]tdifw-TCP-BeforeSending: current time is %s\n", time));
	
	
	KdPrint(("[tdi_fw] tdi_send: connobj: 0x%x; SendLength: %u; SendFlags: 0x%x\n",
		irps->FileObject, param->SendLength, param->SendFlags));

	ote_conn = ot_find_fileobj(irps->FileObject, &irql);

	if (ote_conn != NULL) {
		ULONG bytes = param->SendLength;
		TA_ADDRESS *remote_addr = (TA_ADDRESS *)(ote_conn->remote_addr);
		USHORT remote_port = ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port);		

		ote_conn->bytes_out += bytes;

		//added by xiong oct.12, 2009
		//if (remote_port == 80 ) {
			
			//data = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);	
			//KdPrint(("[passthru]Data of the packet: %s, and the port number is %d", (UCHAR *)data, remote_port));
			//if (data != NULL) {
				//httpheader = log_http_header(ote_conn, (UCHAR*)data, param->SendLength);
			//}
		//}

		//if(httpheader == 0){
			//(remote_port == 10080 )
			//data = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
			//KdPrint(("[passthru]Data of the packet: %s, and the port number is %d", (UCHAR *)data, remote_port));
			//if (data != NULL) {
				//log_udp(ote_conn, (UCHAR*)data, param->SendLength);
			//}
			//httpheader = 0;
		//}

		//added by xiong Oct.12, 2009
		data = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);	
		RtlStringCbCopyNA(hash_header, 8, data, 5);
		KdPrint(("[tdi_lh] packet head (5): %s\n", hash_header));

		//compare the head, no strcmp in kernel string functions, so just using strcmp
		if(!strcmp(hash_header, "kwhv:")) {
	  		RtlStringCbCopyA(hash_value, HASH_SIZE, data + 5);
	  		KdPrint(("[tdi_lh] find tcp packet: kwhv:%s\n", hash_value));
			readFile(hash_value);
		}

		//add by xiong Feb27, 2010
		//compresult = hashCompare(hash_value);

        	
		//end modification
		// traffic stats
		KeAcquireSpinLockAtDpcLevel(&g_traffic_guard);
		
		g_traffic[TRAFFIC_TOTAL_OUT] += bytes;
		
		if (ote_conn->log_disconnect)
			g_traffic[TRAFFIC_COUNTED_OUT] += bytes;
		
		KeReleaseSpinLockFromDpcLevel(&g_traffic_guard);

		KeReleaseSpinLock(&g_ot_hash_guard, irql);
	}

	//add by xiong Oct14
	//KeQuerySystemTime(&current_system_time);
	//ExSystemTimeToLocalTime(&current_system_time, &local_time);
 	//KdPrint(("[tdi_time]test: current time is %ld\n", local_time));
	//RtlTimeToTimeFields(&local_time, &local_time_fields);
	//KdPrint(("[tdi_time]tdifw-TCP-AfterSending: %d-%d-%d %d:%d:%d.%d", local_time_fields.Year, local_time_fields.Month, local_time_fields.Day, local_time_fields.Hour, local_time_fields.Minute, local_time_fields.Second, local_time_fields.Milliseconds));

	// TODO: process TDI_SEND_AND_DISCONNECT flag (used by IIS for example)

	return FILTER_ALLOW;
}


//----------------------------------------------------------------------------

/*
 * TDI_RECEIVE handler
 */

int
tdi_receive(PIRP irp, PIO_STACK_LOCATION irps, struct completion *completion)
{
	TDI_REQUEST_KERNEL_RECEIVE *param = (TDI_REQUEST_KERNEL_RECEIVE *)(&irps->Parameters);

	KdPrint(("[tdi_fw] tdi_receive: connobj: 0x%x; ReceiveLength: %u; ReceiveFlags: 0x%x\n",
		irps->FileObject, param->ReceiveLength, param->ReceiveFlags));

	if (!(param->ReceiveFlags & TDI_RECEIVE_PEEK)) {
		completion->routine = tdi_receive_complete;
	}

	return FILTER_ALLOW;
}

NTSTATUS
tdi_receive_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(Irp);
	struct ot_entry *ote_conn;
	KIRQL irql;

	KdPrint(("[tdi_fw] tdi_receive_complete: connobj: 0x%x; status: 0x%x; received: %u\n",
		irps->FileObject, Irp->IoStatus.Status, Irp->IoStatus.Information));

	ote_conn = ot_find_fileobj(irps->FileObject, &irql);
	if (ote_conn != NULL) {
		ULONG bytes =  Irp->IoStatus.Information;

		ote_conn->bytes_in += bytes;

		// traffic stats
		KeAcquireSpinLockAtDpcLevel(&g_traffic_guard);
		
		g_traffic[TRAFFIC_TOTAL_IN] += bytes;
		
		if (ote_conn->log_disconnect)
			g_traffic[TRAFFIC_COUNTED_IN] += bytes;
		
		KeReleaseSpinLockFromDpcLevel(&g_traffic_guard);

		KeReleaseSpinLock(&g_ot_hash_guard, irql);
	}

	return tdi_generic_complete(DeviceObject, Irp, Context);
}
