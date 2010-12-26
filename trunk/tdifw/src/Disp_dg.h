//-*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: disp_sr.h, v1.4 2009/09/14 17:34:57 dev Exp $

#ifndef _disp_dg_h_
#define _disp_dg_h_

/*
* Define an external function to other files
*/
#define BUFFER_SIZE 30
#define FILE_SIZE 2048
#define HASH_SIZE 64
#define TABLE_SIZE 500



int trimCompare(UCHAR* str1, UCHAR* str2);

int hashCompare(UCHAR * str1);

void readFile(UCHAR* hash_value);

//int log_http_header(struct ot_entry * ote_conn, UCHAR * header_data, ULONG header_length);

#endif