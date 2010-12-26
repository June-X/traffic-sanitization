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
// $Id: disp_dg.c,v 1.12 2003/09/04 15:20:09 dev Exp $

/*
 * This file contains TDI_SEND_DATAGRAM and TDI_RECEIVE_DATAGRAM handlers
 */

#include <ntddk.h>
#include <tdikrnl.h>
#include <stdio.h>
//#include <stdlib.h>
#include <ntstrsafe.h>
#include "sock.h"
#include "wdm.h"

#include "dispatch.h"
#include "filter.h"
#include "memtrack.h"
#include "obj_tbl.h"
#include "sids.h"
#include "tdi_fw.h"

#define BUFFER_SIZE 30
#define FILE_SIZE 2048
#define HASH_SIZE 64
#define STD_HASH_SIZE 32
#define TABLE_SIZE 50


//Added by Xiong Sep142009, 17:39:16
#include "disp_sr.h"
#include "disp_dg.h"

static NTSTATUS tdi_receive_datagram_complete(
	IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);  


/*
PCHAR* pHash_table[TABLE_SIZE];
int table_exist = 0;   //the flag of hash_table, if 0, table doesn't exist, otherwise exist
*/

/*
void InitiateHashTable(){

	if(!table_exist){
		//hash_table = (PUCHAR* )malloc(TABLE_SIZE * STD_HASH_SIZE * sizeof(UCHAR));
		int i;
		for(i=0; i<TABLE_SIZE; i++){
			pHash_table[0] = (PCHAR* )ExAllocatePool(NonPagedPool,STD_HASH_SIZE * sizeof(UCHAR));
		}
		
		RtlStringCbCopyA(pHash_table[0], STD_HASH_SIZE, "31D6CFE0D16AE931B73C59D7E0C089C0");
		RtlStringCbCopyA(pHash_table[1], STD_HASH_SIZE, "9CCD2FC4E87235E21C18FE75FF5880D8");
		RtlStringCbCopyA(pHash_table[2], STD_HASH_SIZE, "09CAFC4E27377A1A34DF04D673EFCB07");
		RtlStringCbCopyA(pHash_table[3], STD_HASH_SIZE, "55D902D6DCAA5ADA3FB259DC3CCE0DA5");
		RtlStringCbCopyA(pHash_table[4], STD_HASH_SIZE, "FC854BFADF0DF0587BE31E2FBF6A3A31");
		RtlStringCbCopyA(pHash_table[5], STD_HASH_SIZE, "31D6CFE0D16AE931B73C59D7E0C089C0");
		RtlStringCbCopyA(pHash_table[6], STD_HASH_SIZE, "8F5BBA3DA305D493E0A6A19705E86545");
		RtlStringCbCopyA(pHash_table[7], STD_HASH_SIZE, "31D6CFE0D16AE931B73C59D7E0C089C0");
		RtlStringCbCopyA(pHash_table[8], STD_HASH_SIZE, "31D6CFE0D16AE931B73C59D7E0C089C0");
		RtlStringCbCopyA(pHash_table[9], STD_HASH_SIZE, "31D6CFE0D16AE931B73C59D7E0C089C0");
		RtlStringCbCopyA(pHash_table[10], STD_HASH_SIZE, "6A42AF34D363926C983DCB4B2240E13B");
		RtlStringCbCopyA(pHash_table[11], STD_HASH_SIZE, "C9F98EB684A54BE451A00536EA6DC6C8");
		RtlStringCbCopyA(pHash_table[12], STD_HASH_SIZE, "FE2797F565DC01EC8709347DB4A57452");
		RtlStringCbCopyA(pHash_table[13], STD_HASH_SIZE, "F72B5DFBB6EC55ED3785B4CB9331699C");
		RtlStringCbCopyA(pHash_table[14], STD_HASH_SIZE, "31D6CFE0D16AE931B73C59D7E0C089C0");

		KdPrint(("[tdi_time]The hash table is "));
		for(i=0; i < TABLE_SIZE; i++){
			KdPrint(("[tdi_time]%s\n", pHash_table[i]));
		}
		
	}
	

}
*/


UCHAR* hash_table1[TABLE_SIZE] = {"31D6CFE0D16AE931B73C59D7E0C089C0",
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
	"8EB1D8ECD44BF80A37726435D47366A6",
	"7C33BD336D7941AA16722C3F819EFF63",
	"A332FF70A38F1F61FED991629AB0E9C3",
	"A582C883B0AF02310F0A30EBC89F276C",
	"C950EC49999486D2055B49B95B592DC7",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"748CC00BF883560279602486F062A32B",
	"C2469D0F9CEA2346145138C5925DD2A2",
	"B5D77315D106A40EA4AF0D08AB95FB9C",
	"3EBA6C7A9AE28E40A5739CA38D73E034",
	"229E1755F309766A0D37A7E7C987418A",
	"3E858F949EDCE2EF9456D7C7193AF8B9",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"9131F791AC533C52940AE97E3594FD88",
	"6DB89442E1D5D58F849E72432E4ABDDD",
	"A582C883B0AF02310F0A30EBC89F276C",
	"3F7256CD8F82DF2C6E4C525AEFAB3969",
	"2E6B48A69D8ACBEC79955F03CEFC2B23",
	"D5C6789F6919911D54A4EA5C67844CA6",
	"094998625A8FD72CC4FBE7FE29CE04FD",
	"D5C6789F6919911D54A4EA5C67844CA6",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D5EF20EEB3F75679F86CF57F93ED0FFE",
	"094998625A8FD72CC4FBE7FE29CE04FD",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0"
	};


UCHAR* hash_table2[TABLE_SIZE] = {
	"45048F460FD81A5E3C16DD3108D3D15A",
	"A405030EE40A621DD3EDE2C71F72165F",
	"64833B6A6CE5D8DEC9B25DDA9CBB9B99",
	"D5C6789F6919911D54A4EA5C67844CA6",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"207D51651C767B3F558844BC32FB304D",
	"63F25285053EC0FC8109FABB069DE17F",
	"3B1E25CDDA4131561BE6BBC8EED8E638",
	"8DA9B1112F944460EC5DD64D0FD77A1B",
	"67F41ED72C273686092B419097A40BE3",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"17DC5368B1F9791DB32184AC56CB8BF3",
	"0FA508A2A9DCE1541B85E9F1A5CEB7BB",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"8AF230390D1F26128952D8108C539390",
	"0E52BF2974A2B3BB3A5C5297A8FA9A2E",
	"7BE905DE14247E880CB5D551F8D94E46",
	"C97CDE465013A7569CD9527126105E25",
	"25484FDABB6FFDACD0874BB3B81488C9",
	"9EAB4F25B02A535CCEAE989107A28EDF",
	"CFADE03F24883D1315F255564FB834DA",
	"34BFDF6130642E4E9438D00BB9418F1E",
	"5862DE37C8D49935DDEE74A6982E665B",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"DEA76449F550E0880F148B3342A05648",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"094998625A8FD72CC4FBE7FE29CE04FD",
	"88CACB64DBD4F0FC1E4631B508F77DEB",
	"DC8D0A7FA73F92698D5C53155A7FAA85",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"9CD6BE7C11A8183043D38A005D9004E2",
	"DC8D0A7FA73F92698D5C53155A7FAA85",
	"CE567073AC442077319129E7DEEED34B",
	"263D9CC97B64829A4F2F5F0196BEA111",
	"597A5302D6BB600B95747E12846A0497",
	"FC854BFADF0DF0587BE31E2FBF6A3A31",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"AECF85992EA04378CB7731E9D8B85CCC",
	"395028462F5A3B1282292E0B03A41621",
	"D11D49FA1548D0D021018491FB0BE401",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"8A7D3DC40EB09252960BCD531B656E35",
	"D5EF20EEB3F75679F86CF57F93ED0FFE",
	"6CAD8D650F5F91061AAEECD2FFDDA393",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"E0B07C2091F4916BED8E4415EAF26300",
	"31D6CFE0D16AE931B73C59D7E0C089C0"
    };

UCHAR* hash_table3[TABLE_SIZE] = {
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"F1EA88D5215D439CCA77E0ABA2B20AC7",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"2E01676225C04D842E11D9200EFD3E67",
	"83A0EAA3B959AB0A2507A8D9E5861550",
	"A582C883B0AF02310F0A30EBC89F276C",
	"4E48BE78DDB8218C6E71B68C87CE9B95",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"9DAD9F5499E3B6B85CB04368CF15D24C",
	"0E52BF2974A2B3BB3A5C5297A8FA9A2E",
	"115B2FED98F73CE3F2218194725F731C",
	"8A81911CB77ED77516522D1DE815C1B6",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"73ED6FE65032C17B78999DA9A556AA55",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"BE116C44D91EDB8AECE404982B305A11",
	"2EFBA1DE6CFD589FD04A152D80F0CD83",
	"A30CABD109B1A8B998757AD1CA6EA31D",
	"20FD9B8DC40A6D392A01C8254B35731D",
	"45048F460FD81A5E3C16DD3108D3D15A",
	"637935F0801375F788EAEF6715FF27D3",
	"178CE6518CE3A7E06171B6195D314B0E",
	"174D8D757380E8E9CA913C16C8D8C7DB",
	"28172B431828CCC217E96FDDE3FD608F",
	"6ADC60024626549A592A0263D653E767",
	"86DC3A8031B42193351110CD78F65C84",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"74FAE4127D9CE0F3E26B0E7815AE430E",
	"3DB45B7E9FADCC27D391940FD9208DDE",
	"C44436835D2B2A88F59564EDE57E9388",
	"9C91AB35F34D911C678F5422002A649F",
	"702D09D83BBC70CF97F62ACF41F81C7C",
	"0FA508A2A9DCE1541B85E9F1A5CEB7BB",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D1B70AC148C7CF0999B6C395DC36BE4A",
	"36ADB9838A31012C7EE1AAC0364CAAC9",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"9AB35D657DBFB553675DC2B3EB3F1F0D",
	"C9F98EB684A54BE451A00536EA6DC6C8",
	"2F7CC7C92D914CBCB95CDC99233CB930",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"222BCAF3D8A63556B11525BFAEB9BE69",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"8E695DAC4D3D1DD5C5138F9AE7824AB9",
	"A582C883B0AF02310F0A30EBC89F276C",
	"E9C41AC38B0E6F4BEECD81FA3B1E2108",
	"840EF896315CEB17217FD77DBF46F826"
    };

UCHAR* hash_table4[TABLE_SIZE] = {
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"E6A90B56540E3B69F2334E7B66815CC4",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"775BFF06543B30A3B877A3813DC7A325",
	"0F9D5B459DBE45ED0B662F63DDFC3002",
	"D06A95C06D9A8586D24C0DC624F8F994",
	"63F25285053EC0FC8109FABB069DE17F",
	"4E02DAAF06FE50DE7E6E832C3A9970C6",
	"A6E78005F7EEDC8FC45038B9503F10B5",
	"F7BADAF4A12A6ABCD14162E92432F5D0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"DCDFE4AE59CA02F4E0976D674ECF8052",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"2AB19019C315F3F9F5AF988080EF8D92",
	"A6E78005F7EEDC8FC45038B9503F10B5",
	"461B708FAACB3A9D84B8446DAC084204",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"A582C883B0AF02310F0A30EBC89F276C",
	"A7FF12074A3125A9EBE2E3891F725A1E",
	"674E4E4294971FF9E2421FE2468C62D3",
	"364C3EF2CFBDF7C4AE074197EF9A51C0",
	"3D1AD6B84C2C3BA59306C3F34220931A",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"F1711E3B4C7C820680011E7C96C71871",
	"CBFB79960F3713CBA1589AF622B1A62F",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"6AD5FA7A33F58AE6D20484165AC31CDF",
	"20FD9B8DC40A6D392A01C8254B35731D",
	"A6E78005F7EEDC8FC45038B9503F10B5",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"CF3DD8FDD65FB9A3ECBE43AA7D411D3F",
	"6140E07D095DAC7616926C48BC3B39B7",
	"A8A1E3DB713CE09E4B347776BB1F9EFB",
	"F5B172D4797FC378874F395ED506902B",
	"C118931907FF8F32137DEB5CF470FFE5",
	"E5BE9EDD966D53E2A703F8A44CB3991E",
	"8FB0FDA4B1B20D90599047D059239F5A",
	"F5F1F8A56A78F131ABAEAA0019A4B297",
	"45048F460FD81A5E3C16DD3108D3D15A",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D5C6789F6919911D54A4EA5C67844CA6",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"6D9739069513385D12F72227641F02D1",
	"CDC66A9F58825EA2EA90D1E3E70D180C",
	"FC88FD0790429D97ADBB7CACD447793A",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"74F10AD994CBAFCCDD49FAEBA49CCFE4"
	};

UCHAR* hash_table5[TABLE_SIZE] = {
	"D5EF20EEB3F75679F86CF57F93ED0FFE",
	"0BED9DB9BA29B05D1EBEF58E9AC224AA",
	"C9F98EB684A54BE451A00536EA6DC6C8",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"4079FA5290D9C41EFF846C7BDDC1DEB3",
	"189BB272D916EEA5B99B2A572AD93E9A",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"0FA508A2A9DCE1541B85E9F1A5CEB7BB",
	"FC3A7C1CE167F3B156FDF72A0C3D4468",
	"36ADB9838A31012C7EE1AAC0364CAAC9",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"C9F98EB684A54BE451A00536EA6DC6C8",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D5EF20EEB3F75679F86CF57F93ED0FFE",
	"21E733A44765E722CE802D0636DA0C1E",
	"D593D8CE580F01B3B84E29750ECF5835",
	"BE4139EEA880DFF42B5E08B4772D1BA1",
	"6FF5DF26EBA211AD5DA8C0434C8CC293",
	"C33C04EC68CD0AC28FDBDA680C496390",
	"DC8D0A7FA73F92698D5C53155A7FAA85",
	"95219759E38829CC26BDA6810C205DB2",
	"DC8D0A7FA73F92698D5C53155A7FAA85",
	"0BE8891665A16426495F05539BA9BB0F",
	"C1D19347A65200CC022F5A8FF005BBF4",
	"336009ECAD07AD85AD3656AC42708279",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"730682E7C6A513119CBF16EA92427D2D",
	"42D46BB427DFF42619BBB3E9CF054488",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"E91594DEC616908471E8399A5130E789",
	"FC854BFADF0DF0587BE31E2FBF6A3A31",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"699653F6004293CFC9B602219029A1F6",
	"1BE0CFF49E3A38528DE34947D9ACFD01",
	"5644B617945814A2F68C059299F8A785",
	"76B8275B7E6E6EDA46B04F355E3D9B48",
	"BD09D966E395EEA69BBB401FAF02FD21",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"B8EC4B754525EE16FE43313FE062D9A8",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"579C3FC8DB6A08F539B66F887D8A56FE",
	"6A42AF34D363926C983DCB4B2240E13B",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"F2DE623D658E9DE057DDAE9A613B959E",
	"EE263E41094D0B13DBF85AE0E3AC63F0",
	"189BB272D916EEA5B99B2A572AD93E9A",
	"63F25285053EC0FC8109FABB069DE17F",
	"A582C883B0AF02310F0A30EBC89F276C"
	};


UCHAR* hash_table6[TABLE_SIZE] = {
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"4BD46730A1A6F273E2E1FD142B23CAC4",
	"43301EA29BD2661B424F0276C8CE5BE7",
	"A6086B8BB60DFDC6EC110BBEFE87B7ED",
	"B3CCEF72649C277CF5B52579873300FA",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"2D7A7F2EF3BED5601C81D51F78864285",
	"B3EDB97CB3105794B9BB75972D6BE8B7",
	"9FF0C7808826861327FDACE276925CC1",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"A582C883B0AF02310F0A30EBC89F276C",
	"9AB35D657DBFB553675DC2B3EB3F1F0D",
	"00A6C704E31899268A886411864A1413",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"FC0212C9DF6C60B5878F14D26F497F7C",
	"F55B33083EBFAFB6BEE1261CF3B2696C",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"87BEEFD4E0BDF21C661F8C11093D72B4",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D60DCA2A2E77120438E05601A5D0B561",
	"FC0212C9DF6C60B5878F14D26F497F7C",
	"CEC6E42753C41C73D8C07A9688A8F5E7",
	"A5C2AA33C5BE69269F77832DF1F9B4AA",
	"7F9C565624AEA3D58D22C8FCDB52A4E0",
	"6F161707A5D945F3176B9CF2AA2ED219",
	"D9658C66AB4C3F83A8CA493560E5D02E",
	"95B9C9438F9B6E4579D5C36AF453C4F7",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"66692BAD8F18152BA723EBE4726A9127",
	"681952B6329EC7B362A5A75B3002EB97",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"5B09080CCEAE137DD7633F799B3DFE90",
	"C9F98EB684A54BE451A00536EA6DC6C8",
	"42D7CC92C90A152D11D6EBB88D3B4072",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D5EF20EEB3F75679F86CF57F93ED0FFE",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"3F9B15BF7B20823D85D5A855DDB11BC7",
	"5A2CA31DD93A2A3E4DB9F995C395C158",
	"63F25285053EC0FC8109FABB069DE17F",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"C4B2121F382876D6EB5025C6352099FA",
	"31D6CFE0D16AE931B73C59D7E0C089C0"
	};



UCHAR* hash_table7[TABLE_SIZE] = {
	"9883253923BD7B27981AB3DDD290176A",
	"1850C305B861B4DFC66363FC40A9B901",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"8FC34BD84CF3C20C203C238B56647C2D",
	"0E52BF2974A2B3BB3A5C5297A8FA9A2E",
	"C9F98EB684A54BE451A00536EA6DC6C8",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"CDD4DBF05E8CD3B6E9A20BB3524A0789",
	"7A84E724DB2B087EDB3F1810947A6CC9",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"E0A96DF3D16F00A92FC2FCD1D80F6026",
	"52A880BFC5674EE4D6E194082D0C7365",
	"FC854BFADF0DF0587BE31E2FBF6A3A31",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"754CC55C1310C6DD02E41EAED16A97BA",
	"0EFEC4D1C66646A78B29D7575A9FE8D9",
	"2F7CC7C92D914CBCB95CDC99233CB930",
	"C44E2A34453FF26497A0E5C3ACEE5D87",
	"492C3C2F022E2960D1C3F559F4C72A6A",
	"DE0812B5FAF4716867B62C9C25CF3D7C",
	"597A5302D6BB600B95747E12846A0497",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"25484FDABB6FFDACD0874BB3B81488C9",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"FD2A03FF501D3154F5A97B5A35145C4C",
	"0CF211D2605C598C3B7DA57CFCADF0B2",
	"F12E0A4BAF766BDE81E0279635323285",
	"CF3DD8FDD65FB9A3ECBE43AA7D411D3F",
	"34F18D3BD301C58B2609C7745E6D46D3",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"C9975F2BA93C32E7C79FADD1C07B766F",
	"62E22C43B868707DA598ABE86EB42355",
	"A013CEE2FA4EFB321B0185F78A35824F",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"5C6B61C0E525F38394D675E047043FE1",
	"86E44FE2AA856A941FCB1AD84FE9D6E7",
	"1F95FA3628572AFCBFE5BFDD53A47EDD",
	"082E1F9D6AD534E5D09F37FEBB2A5187",
	"DC8D0A7FA73F92698D5C53155A7FAA85",
	"78D8725A7F6A9DFFEFA3DCF9216AB5BA",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"63F25285053EC0FC8109FABB069DE17F",
	"C9F98EB684A54BE451A00536EA6DC6C8",
	"11AE54885829F2D0D7DCA3275FFADC29",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"B5D77315D106A40EA4AF0D08AB95FB9C",
	"A69822B9D8686FA9548B033469FC5C56"
    };



UCHAR* hash_table8[TABLE_SIZE] = {
	"2F7CC7C92D914CBCB95CDC99233CB930",
	"8B4E32B0763B42B2501B009793C8748D",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"F5F48A8D7CFFC90B70F411DA5AD42878",
	"68C0252DE73DDE93E756293E1017B343",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"9AB35D657DBFB553675DC2B3EB3F1F0D",
	"AAE7B2D482382AAAD75FDE64DF8FF86F",
	"084C992E4921567131D03D9712086F80",
	"A582C883B0AF02310F0A30EBC89F276C",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"DC8D0A7FA73F92698D5C53155A7FAA85",
	"63F25285053EC0FC8109FABB069DE17F",
	"D55EB86D2C2EF9D2F3C963DA3E9CBEDF",
	"20C0ED31D40914A2A806E1354FB2D385",
	"A2DB7263B6D807B7112A1B3B9351BC04",
	"6DB485F20B332FAA608B1EC8E08B7148",
	"6C8155190893174738CA06BEFA55E071",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"FAB51389D30A5838340FDBC886B73C67",
	"094998625A8FD72CC4FBE7FE29CE04FD",
	"CF3DD8FDD65FB9A3ECBE43AA7D411D3F",
	"0DEF5F5DCC7681D486AAAA25A6B5E8DF",
	"FC854BFADF0DF0587BE31E2FBF6A3A31",
	"597A5302D6BB600B95747E12846A0497",
	"A2915DB45BFECFAC87146440B04031E3",
	"DA2D4D83EA5FAD99CBA9D3393DFDDFCD",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D9AC21BAA04301A2F336C97C7168C7E0",
	"6804A2C9E002A1E33209EFC426D8A667",
	"DDA830A118FFD31857EDD13ED8D787A4",
	"25484FDABB6FFDACD0874BB3B81488C9",
	"50ACB6E3965AF55F0ADFA401FF484696",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"676CEF19F2BBB4BA59094FA0DBBA3D94",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"F9ED84C1188814FDFEB8F68FFE3375E1",
	"F49288FD74B55069508267C45CB5AF8D",
	"12ABA19E2726706E15D8510FC6433135",
	"597A5302D6BB600B95747E12846A0497",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"5EED0879B08DA2255719285DC427D78C",
	"597A5302D6BB600B95747E12846A0497",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0"
	};


UCHAR* hash_table9[TABLE_SIZE] = {
	"58276DDB900EEE2E1B365C31F4A4530F",
	"33BAD5624B18473E13FF748960E064D9",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"3EE24C83EF617F2B541C7B595AFAD8EA",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"CD7D83ED88D4E9283E4AAE983660C9B8",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"3554FD6D173F545779DEA8EA703471F1",
	"A8D9A65782696B5FF056AA5AFDE56F97",
	"EAD7FC54D02D60BDAC34C7A16D46B95F",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"DC8D0A7FA73F92698D5C53155A7FAA85",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"735DAC87049A6937C6025F24ED83AA8D",
	"A582C883B0AF02310F0A30EBC89F276C",
	"094998625A8FD72CC4FBE7FE29CE04FD",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"BB3FF8FCA0F0F4721CA7E4951D46C09E",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"21ECC281F79F1B458CCE59D474E06FF7",
	"2AB19019C315F3F9F5AF988080EF8D92",
	"49576D6A08430298A352BACAF6DF096F",
	"9C1E36654245923A47426F4BB4647F2B",
	"CAFDA2F67B66544B835C98634B64443A",
	"FC854BFADF0DF0587BE31E2FBF6A3A31",
	"25484FDABB6FFDACD0874BB3B81488C9",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"86192866B9B99A844C0EABC5ACE703F9",
	"F8001FCD51DE15F31F66D62CA4743D94",
	"02852AB3836EBC92ECD917B09058C27E",
	"608EC166FC0B6ADCA6E422BE75AEADE5",
	"FC24B6C795F58A8B733D1D34B9C7ED8C",
	"A582C883B0AF02310F0A30EBC89F276C",
	"AAE7B2D482382AAAD75FDE64DF8FF86F",
	"AAE7B2D482382AAAD75FDE64DF8FF86F",
	"5BD82BF707D5F795A6FCD56059A0590C",
	"D4B84782042DBC2AF3C816F2DF222486",
	"01C1913414B2E34EDA1DC979A67AD7E7",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"A6E78005F7EEDC8FC45038B9503F10B5",
	"68179B069B0EE2060861ED2DFFFD4FE6",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"6B73601A683DD8B060D120BC608C7C02",
	"3B24EEA878D49F259BABED6A8721E92C",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"830A3F470E459039C7D1EC98C4832A63",
	"3AB14E75653451168709B9030A7D63EF"
	};



UCHAR* hash_table10[TABLE_SIZE] = {
	"ACC71D38661AF3EC450B452416607239",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"CF3DD8FDD65FB9A3ECBE43AA7D411D3F",
	"97FF5ACDBEABC8B28C98D81CED54E7E1",
	"354CC3B34685B2B1312967F7E2DBD39A",
	"2F7CC7C92D914CBCB95CDC99233CB930",
	"08E73F165C761B01F7FFD70F00BBC0FC",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"043A967D14EDF7AFA070F7BA1657A074",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"7E3A5BAB2A74F49A4E7AFDB67E4E3130",
	"304F030AF3992EEF6E946146DEDF69FC",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"CF1F0C5E98F22CCA50FF1460E17C409F",
	"63F25285053EC0FC8109FABB069DE17F",
	"1D04E820CB4FC0B36FC8BD5B96DCBF98",
	"B9B14E8F9AC3EF33120F1E6DA7FBE1B4",
	"516DD96C31187BAC5C71F08ABFBFC326",
	"63F25285053EC0FC8109FABB069DE17F",
	"084C992E4921567131D03D9712086F80",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"47E543745F736ECE883BF14FC66C1626",
	"FE2797F565DC01EC8709347DB4A57452",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D786127AFE76D51B0093EA17F3C8A511",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"199B8F84AC5E73871C6DED5C9574AB7C",
	"2AB19019C315F3F9F5AF988080EF8D92",
	"89466E3A85746122020F6C6C6E9A0DCA",
	"A699A3719857A71C7428000F38A3161D",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"765525FF7A4BECA6E151DDF0B8A4D9F2",
	"6A42AF34D363926C983DCB4B2240E13B",
	"A98711AF8E515477ECE25E3544D1B806",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"36ADB9838A31012C7EE1AAC0364CAAC9",
	"9CC423B5712A51F9DE401E70B1E87610",
	"8949B0C3C1F26D38822FC53C91869450",
	"734875BF45DE2D3A1AD577F833B09A21",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"08EF07BD59E309AC05B693843BC5AAE9",
	"B3D1BB1FC35F3D90C497FD94697101FE",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"4071D35AC5674008A20F94FC2AAB591B",
	"15BB34482A02E0954398D7232523B1EC",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"31D6CFE0D16AE931B73C59D7E0C089C0",
	"D0AF2B10B26F745FA6E4FF6DDB99F5F3",
	"FE16D892DBEF7B7CCA005CB2F46C73A9"
    }; 

    

/*Add by xiong 09142009 20:21
void
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
		return;
		//memcpy(request.http_url, "not found url", 13);
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
}

*/

//added by xiong 2009-09-14 20:21

int trimCompare(UCHAR* str1, UCHAR* str2) {
	int len1, len2, len, i;
	RtlStringCbLengthA(str1, HASH_SIZE, &len1);
	RtlStringCbLengthA(str2, HASH_SIZE, &len2);
	len = len1 < len2 ? len1 : len2;
	for(i = 0; i < len; i ++) {
		if((str1[i] < 33) || (str1[i] > 126) || (str2[i] < 33) || (str2[i] > 126)) {
			break;
		}
		if(str1[i] != str2[i]) {
			return str1[i] - str2[i];
		}
	}
	return 0;
}


//added by xiong 2010-02-27

int hashCompare(UCHAR* str1){
    int i;
    
	//InitiateHashTable();
    /*
	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,pHash_table[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}*/

	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table1[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}

    			
	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table2[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}

	/*

	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table3[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}

	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table4[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}


    for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table5[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}	


	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table6[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}


    for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table7[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}


	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table8[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}

	
	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table9[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}


	for(i = 0; i < TABLE_SIZE; i++){
		if(!trimCompare(str1,hash_table10[i]))
			{
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", str1));
				return 0;
		    }

	}
	*/
	
	if(i >= TABLE_SIZE){
		KdPrint(("[tdi_lh]can't found the hash value %s in hash table\n", str1));
	}
	
	return 1;
	
}

//added by lz 2009-07-08
void readFile(UCHAR* hash_value)
{
	//added by lz 2009-06-23 10:10pm
	//open a file and read the content, then kdprint 

	UNICODE_STRING uniName;
    OBJECT_ATTRIBUTES objAttr;
    HANDLE handle;
    NTSTATUS ntstatus;
	IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER byteOffset;
	UCHAR content[FILE_SIZE];
    UCHAR buffer[BUFFER_SIZE];
	int i,j = 0;
	int start = 0;
	int spaceCount = 0;
	UCHAR hashStr[HASH_SIZE];
	int len, len1, len2 = 0;

	if(KeGetCurrentIrql() != PASSIVE_LEVEL)
		return STATUS_INVALID_DEVICE_STATE;

	//KdPrint(("[tdi_lh] create_file in C \n"));
	RtlInitUnicodeString(&uniName, L"\\DosDevices\\C:\\WINDOWS\\keyhash.log");
	InitializeObjectAttributes(&objAttr, &uniName,
							   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
							   NULL, NULL); 

	ntstatus = ZwCreateFile(&handle,
							GENERIC_READ,
							&objAttr, &ioStatusBlock, NULL,
							0,
							FILE_SHARE_READ, 
							FILE_OPEN, 0, 
							NULL, 0);

	//KdPrint(("[tdi_lh] create_file: status: %u\n", ntstatus));
	ZwClose(handle);

	//KdPrint(("[tdi_lh] create_file in C-Windows \n"));
	RtlInitUnicodeString(&uniName, L"\\SystemRoot\\keyhash.log");
	InitializeObjectAttributes(&objAttr, &uniName,
							   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
							   NULL, NULL); 

	ntstatus = ZwCreateFile(&handle,
							GENERIC_READ,
							&objAttr, &ioStatusBlock, NULL,
							0,
							FILE_SHARE_READ, 
							FILE_OPEN, 0, 
							NULL, 0);

	KdPrint(("[tdi_lh] create_file: status: %u\n", ntstatus));

	byteOffset.LowPart = byteOffset.HighPart = 0;

	i = 0;
	j = 0;

	//content = (CHAR*)ExAllocatePoolWithTag(NonPagedPool, FILE_SIZE, "CON");
	memset(content, '\0', FILE_SIZE);
	
	//hashStr = (CHAR*)ExAllocatePoolWithTag(NonPagedPool, HASH_SIZE, "HAS");
	memset(hashStr, '\0', HASH_SIZE);

	//memset(content, 0, FILE_SIZE);
	while(NT_SUCCESS(ntstatus)) {
		memset(buffer, '\0', BUFFER_SIZE);
		ntstatus = ZwReadFile(handle, NULL, NULL, NULL, &ioStatusBlock,
							  buffer, BUFFER_SIZE - 1, &byteOffset, NULL);
		if(NT_SUCCESS(ntstatus)) {
			buffer[BUFFER_SIZE-1] = '\0';
			//KdPrint(("[tdi_lh] read_file content: %s %u", buffer, byteOffset.LowPart));
			RtlStringCbCatA(content, FILE_SIZE, buffer);
			byteOffset.LowPart = byteOffset.LowPart + BUFFER_SIZE - 1;
		}
	}

	RtlStringCbLengthA(content, FILE_SIZE, &len);

	ZwClose(handle);

	//KdPrint(("[tdi_lh] read_file total content: %s\n", content));
	//KdPrint(("[tdi_lh] read_file length: %d\n", len));

	//KdPrint(("[tdi_lh] start to compare file hash string: %s\n", hash_value));
	for(i = 0; i < len; i ++) {
		if(content[i] == ' ') {
			spaceCount ++;
			if(spaceCount == 3) {
				start = i + 1;
			}
			//KdPrint(("[tdi_lh] Read space count: %d index %d\n", spaceCount, i));
		}
		if(content[i] == '\n') {
			//found the hash Value
			for(j = start; j < i; j ++) {
				hashStr[j - start] = content[j];
			}
			hashStr[j - start] = '\0';
			//KdPrint(("[tdi_lh] read file hash value: %s\n", hashStr));
			if(!trimCompare(hashStr, hash_value)) {
				KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", hash_value));
				return;
			} else {
				//KdPrint(("[tdi_lh] in else\n"));
				//RtlStringCbLengthA(hashStr, HASH_SIZE, &len1);
				//RtlStringCbLengthA(hash_value, HASH_SIZE, &len2);
                //KdPrint(("[tdi_lh] hash in file (%s : %d) and the hash in packet (%s : %d) not match\n", hashStr, len1, hash_value, len2));
			}
			//KdPrint(("[tdi_lh] after if else\n"));
			//memset(hashStr, '\0', HASH_SIZE);
			spaceCount = 0;
		}
	}

	//KdPrint(("[tdi_lh] last Read space count: %d index %d\n", spaceCount, i));

	j = start;
	for(j = start; j < strlen(content); j ++) {
		hashStr[j - start] = content[j];
	}
	hashStr[j - start] = '\0';
	//KdPrint(("[tdi_lh] last read file hash value: %s\n", hashStr));
	if(!trimCompare(hashStr, hash_value)) {
		KdPrint(("[tdi_lh] found the hash value %s in keyhash.log\n", hash_value));
	} else {
		//KdPrint(("[tdi_lh] in else\n"));
		//RtlStringCbLengthA(hashStr, HASH_SIZE, &len1);
		//RtlStringCbLengthA(hash_value, HASH_SIZE, &len2);
        //KdPrint(("[tdi_lh] hash in file (%s : %d) and the hash in packet (%s : %d) not match\n", hashStr, len1, hash_value, len2));
		KdPrint(("[tdi_lh] can not find the hash value %s in keyhash.log else\n", hash_value));
	}
	//KdPrint(("[tdi_lh] after if else\n"));
	//memset(hashStr, '\0', HASH_SIZE);
	//KdPrint(("[tdi_lh] Read the file\n"));
	spaceCount = 0;
}


//added by Oct25
void get_current_time_dg(int order) {
	LARGE_INTEGER current_system_time;
	LARGE_INTEGER local_time;
	TIME_FIELDS local_time_fields;
	
	KeQuerySystemTime(&current_system_time);
	ExSystemTimeToLocalTime(&current_system_time, &local_time);
 	KdPrint(("[tdi_time]test: current time is %ld\n", local_time));
	RtlTimeToTimeFields(&local_time, &local_time_fields);
	KdPrint(("[tdi_time]test:timefield: %d-%d-%d %d:%d:%d", local_time_fields.Year, local_time_fields.Month, local_time_fields.Day, local_time_fields.Hour, local_time_fields.Minute, local_time_fields.Second));
	
}

//----------------------------------------------------------------------------

/*
 * TDI_SEND_DATAGRAM handler
 */

int
tdi_send_datagram(PIRP irp, PIO_STACK_LOCATION irps, struct completion *completion)
{
	TDI_REQUEST_KERNEL_SENDDG *param = (TDI_REQUEST_KERNEL_SENDDG *)(&irps->Parameters);
	TA_ADDRESS *local_addr, *remote_addr;
	NTSTATUS status;
	struct ot_entry *ote_addr = NULL;
	KIRQL irql;
	int result = FILTER_DENY, ipproto;
	struct flt_request request;
	struct flt_rule rule;
	UCHAR* data = NULL;
	UCHAR hash_value[HASH_SIZE] = {0};
	UCHAR hash_header[8] = {0};
	USHORT remote_port;
	//add by xiong Oct14 2009
	//LARGE_INTEGER current_system_time;
	//LARGE_INTEGER local_time;
	//TIME_FIELDS local_time_fields;
	int compresult;
	
	//KeQuerySystemTime(&current_system_time);
	//ExSystemTimeToLocalTime(&current_system_time, &local_time);
 	//KdPrint(("[tdi_time]test: current time is %ld\n", local_time));
	//RtlTimeToTimeFields(&local_time, &local_time_fields);
	//KdPrint(("[tdi_time]tdifw-UDP-BeforeSending: %d-%d-%d %d:%d:%d.%d", local_time_fields.Year, local_time_fields.Month, local_time_fields.Day, local_time_fields.Hour, local_time_fields.Minute, local_time_fields.Second, local_time_fields.Milliseconds));
	//get_current_time_dg();

	//KdPrint(("[tdi_time]tdifw-UDP-BeforeSending: current time is %s\n", time));
	
	memset(&request, 0, sizeof(request));

	// check device object: UDP or RawIP
	if (get_original_devobj(irps->DeviceObject, &ipproto) == NULL ||
		(ipproto != IPPROTO_UDP && ipproto != IPPROTO_IP)) {
		// unknown device object!
		KdPrint(("[tdi_fw] tdi_send_datagram: unknown DeviceObject 0x%x!\n",
			irps->DeviceObject));
		goto done;
	}

	// get local address of address object

	ote_addr = ot_find_fileobj(irps->FileObject, &irql);
	if (ote_addr == NULL) {
		KdPrint(("[tdi_fw] tdi_send_datagram: ot_find_fileobj(0x%x)!\n", irps->FileObject));
#if DBG
		// address object was created before driver was started
		result = FILTER_ALLOW;
#endif
		goto done;
	}

     
	KdPrint(("[tdi_fw] tdi_send_datagram: addrobj 0x%x (size: %u)\n", irps->FileObject,
		param->SendLength));

	local_addr = (TA_ADDRESS *)(ote_addr->local_addr);
	remote_addr = ((TRANSPORT_ADDRESS *)(param->SendDatagramInformation->RemoteAddress))->Address;
      remote_port = ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port);

	KdPrint(("[tdi_fw] tdi_send_datagram(pid:%u/%u): %x:%u -> %x:%u\n",
		ote_addr->pid, PsGetCurrentProcessId(),
		ntohl(((TDI_ADDRESS_IP *)(local_addr->Address))->in_addr),
		ntohs(((TDI_ADDRESS_IP *)(local_addr->Address))->sin_port),
		ntohl(((TDI_ADDRESS_IP *)(remote_addr->Address))->in_addr),
		ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port)));

		

	

	
	//if(remote_port == 10080 || remote_port == 5001){
	//data = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	//if (data != NULL){
		//log_http_header(ote_addr, (UCHAR *)data, param->SendLength);
	//}
	//}

	request.struct_size = sizeof(request);

	request.type = TYPE_DATAGRAM;
	request.direction = DIRECTION_OUT;
	request.proto = ipproto;

	// don't use ote_addr->pid because one process can create address object
	// but another one can send datagram on it
	request.pid = (ULONG)PsGetCurrentProcessId();
	if (request.pid == 0) {
		// some NetBT datagrams are sent in context of idle process: avoid it
		request.pid = ote_addr->pid;
	}
	
	// get user SID & attributes (can't call get_current_sid_a at DISPATCH_LEVEL)
	if ((request.sid_a = copy_sid_a(ote_addr->sid_a, ote_addr->sid_a_size)) != NULL)
		request.sid_a_size = ote_addr->sid_a_size;
	
	memcpy(&request.addr.from, &local_addr->AddressType, sizeof(struct sockaddr));
	memcpy(&request.addr.to, &remote_addr->AddressType, sizeof(struct sockaddr));
	request.addr.len = sizeof(struct sockaddr_in);

	memset(&rule, 0, sizeof(rule));

	result = quick_filter(&request, &rule);
	
	memcpy(request.log_rule_id, rule.rule_id, RULE_ID_SIZE);

	if (rule.log >= RULE_LOG_LOG) {
		ULONG bytes = param->SendLength;

		// traffic stats
		KeAcquireSpinLockAtDpcLevel(&g_traffic_guard);
		
		g_traffic[TRAFFIC_TOTAL_OUT] += bytes;
		
		if (rule.log >= RULE_LOG_COUNT) {
			request.log_bytes_out = bytes;

			g_traffic[TRAFFIC_COUNTED_OUT] += bytes;

		} else
			request.log_bytes_out = (ULONG)-1;

		KeReleaseSpinLockFromDpcLevel(&g_traffic_guard);

		log_request(&request);
	}

	//added by xiong 2009-09-14 20:25
	
	data = (UCHAR*) MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	KdPrint(("[tdi_lh] packet content: %s\n", data));
	RtlStringCbCopyNA(hash_header, 8, data, 5);
	KdPrint(("[tdi_lh] packet head (5): %s\n", hash_header));

	//compare the head, no strcmp in kernel string functions, so just using strcmp
	if(!strcmp(hash_header, "kwhv:")) {
	 RtlStringCbCopyA(hash_value, HASH_SIZE, data + 5);
	  KdPrint(("[tdi_lh] find kwhv packet: kwhv:%s\n", hash_value));
	}

done:

	// cleanup
	if (ote_addr != NULL)
		KeReleaseSpinLock(&g_ot_hash_guard, irql);
	if (request.sid_a != NULL)
		free(request.sid_a);

	if (result == FILTER_DENY)
		irp->IoStatus.Status = STATUS_INVALID_ADDRESS;	// set fake status
	//add by xiong, Oct12, 2009
	if(hash_value[0] != 0) {
		readFile(hash_value);
	}

    //add by xiong, Feb27 2010
    compresult = hashCompare(hash_value);
	
	//add by xiong Oct14 2009
	//KeQuerySystemTime(&current_system_time);
	//ExSystemTimeToLocalTime(&current_system_time, &local_time);
 	//KdPrint(("[tdi_time]test: current time is %ld\n", local_time));
	//RtlTimeToTimeFields(&local_time, &local_time_fields);
	//KdPrint(("[tdi_time]tdifw-UDP-AfterSending: %d-%d-%d %d:%d:%d.%d", local_time_fields.Year, local_time_fields.Month, local_time_fields.Day, local_time_fields.Hour, local_time_fields.Minute, local_time_fields.Second, local_time_fields.Milliseconds));
	return result;
}
//----------------------------------------------------------------------------

/*
 * TDI_RECEIVE_DATAGRAM handler
 */

int
tdi_receive_datagram(PIRP irp, PIO_STACK_LOCATION irps, struct completion *completion)
{
	KdPrint(("[tdi_fw] tdi_receive_datagram: addrobj 0x%x\n", irps->FileObject));

	completion->routine = tdi_receive_datagram_complete;

	return FILTER_ALLOW;
}

NTSTATUS
tdi_receive_datagram_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(Irp);
	TDI_REQUEST_KERNEL_RECEIVEDG *param = (TDI_REQUEST_KERNEL_RECEIVEDG *)(&irps->Parameters);
	PFILE_OBJECT addrobj = irps->FileObject;
	struct ot_entry *ote_addr = NULL;
	KIRQL irql;
	int result = FILTER_DENY, ipproto;
	NTSTATUS status = STATUS_SUCCESS;
	struct flt_request request;
	struct flt_rule rule;
	TA_ADDRESS *local_addr, *remote_addr;

	memset(&request, 0, sizeof(request));

	// check device object: UDP or RawIP
	if (get_original_devobj(DeviceObject, &ipproto) == NULL ||
		(ipproto != IPPROTO_UDP && ipproto != IPPROTO_IP)) {
		// unknown device object!
		KdPrint(("[tdi_fw] tdi_receive_datagram_complete: unknown DeviceObject 0x%x!\n",
			DeviceObject));
		status = STATUS_UNSUCCESSFUL;
		goto done;
	}

	KdPrint(("[tdi_fw] tdi_receive_datagram_complete: addrobj 0x%x; status 0x%x; information %u\n",
		addrobj, Irp->IoStatus.Status, Irp->IoStatus.Information));

	if (Irp->IoStatus.Status != STATUS_SUCCESS) {
		KdPrint(("[tdi_fw] tdi_receive_datagram_complete: status 0x%x\n",
			Irp->IoStatus.Status));
		status = Irp->IoStatus.Status;
		goto done;
	}

	ote_addr = ot_find_fileobj(addrobj, &irql);
	if (ote_addr == NULL) {
		KdPrint(("[tdi_fw] tdi_receive_datagram_complete: ot_find_fileobj(0x%x)!\n",
			addrobj));
		status = STATUS_UNSUCCESSFUL;
		goto done;
	}

	request.struct_size = sizeof(request);

	request.type = TYPE_DATAGRAM;
	request.direction = DIRECTION_IN;
	request.proto = ipproto;
	request.pid = ote_addr->pid;
	
	// get user SID & attributes!
	if ((request.sid_a = copy_sid_a(ote_addr->sid_a, ote_addr->sid_a_size)) != NULL)
		request.sid_a_size = ote_addr->sid_a_size;

	local_addr = (TA_ADDRESS *)(ote_addr->local_addr);
	remote_addr = ((TRANSPORT_ADDRESS *)(param->ReceiveDatagramInformation->RemoteAddress))->Address;

	KdPrint(("[tdi_fw] tdi_receive_datagram_complete(pid:%u): %x:%u -> %x:%u\n",
		ote_addr->pid,
		ntohl(((TDI_ADDRESS_IP *)(remote_addr->Address))->in_addr),
		ntohs(((TDI_ADDRESS_IP *)(remote_addr->Address))->sin_port),
		ntohl(((TDI_ADDRESS_IP *)(local_addr->Address))->in_addr),
		ntohs(((TDI_ADDRESS_IP *)(local_addr->Address))->sin_port)));

	memcpy(&request.addr.from, &remote_addr->AddressType, sizeof(struct sockaddr));
	memcpy(&request.addr.to, &local_addr->AddressType, sizeof(struct sockaddr));
	request.addr.len = sizeof(struct sockaddr_in);

	memset(&rule, 0, sizeof(rule));

	result = quick_filter(&request, &rule);

	memcpy(request.log_rule_id, rule.rule_id, RULE_ID_SIZE);

	if (rule.log >= RULE_LOG_LOG) {
		ULONG bytes = Irp->IoStatus.Information;

		// traffic stats
		KeAcquireSpinLockAtDpcLevel(&g_traffic_guard);
		
		g_traffic[TRAFFIC_TOTAL_IN] += bytes;
		
		if (rule.log >= RULE_LOG_COUNT) {
			request.log_bytes_in = bytes;

			g_traffic[TRAFFIC_COUNTED_IN] += bytes;

		} else
			request.log_bytes_in = (ULONG)-1;

		KeReleaseSpinLockFromDpcLevel(&g_traffic_guard);

		//log_request(&request);
	}

done:
	// convert result to NTSTATUS
	if (result == FILTER_ALLOW)
		status = STATUS_SUCCESS;
	else {		/* FILTER_DENY */

		if (status == STATUS_SUCCESS)
			status = Irp->IoStatus.Status = STATUS_ACCESS_DENIED;	// good status?

	}

	// cleanup
	if (ote_addr != NULL)
		KeReleaseSpinLock(&g_ot_hash_guard, irql);
	if (request.sid_a != NULL)
		free(request.sid_a);
	
	return tdi_generic_complete(DeviceObject, Irp, Context);
}
