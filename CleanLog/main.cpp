#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

void main( void )
{
	time_t tv;
	struct tm *tm;

	time(&tv);
	tm = localtime(&tv);
	if (tm == NULL) {
		return;
	}
	char http_fname[1024];
	if (_snprintf(http_fname, sizeof(http_fname),
		"%s\\system32\\LogFiles\\tdifw\\%04d%02d%02d.http.log",
			getenv("SystemRoot"),
				tm->tm_year + 1900,
				tm->tm_mon + 1,
				tm->tm_mday) == -1) {
				return;
	}
	char http_bak_fname[1028];
	if (_snprintf(http_bak_fname, sizeof(http_bak_fname), "%s.bak", http_fname) == -1) {
		return;
	}
	char key_fname[1024];
	if (_snprintf(key_fname, sizeof(key_fname),
		"%s\\system32\\LogFiles\\tdifw\\%04d%02d%02d.key.log",
			getenv("SystemRoot"),
				tm->tm_year + 1900,
				tm->tm_mon + 1,
				tm->tm_mday) == -1) {
				return;
	}
	char key_bak_fname[1028];
	if (_snprintf(key_bak_fname, sizeof(key_bak_fname), "%s.bak", key_fname) == -1) {
		return;
	}
	char mouse_fname[1024];
	if (_snprintf(mouse_fname, sizeof(mouse_fname),
		"%s\\system32\\LogFiles\\tdifw\\%04d%02d%02d.mouse.log",
			getenv("SystemRoot"),
				tm->tm_year + 1900,
				tm->tm_mon + 1,
				tm->tm_mday) == -1) {
				return;
	}
	char mouse_bak_fname[1028];
	if (_snprintf(mouse_bak_fname, sizeof(mouse_bak_fname), "%s.bak", mouse_fname) == -1) {
		return;
	}
	rename(http_fname, http_bak_fname);
	rename(key_fname, key_bak_fname);
	rename(mouse_fname, mouse_bak_fname);
	char rm_cmd[1024];
	if (_snprintf(rm_cmd, sizeof(rm_cmd), "del %s\\system32\\LogFiles\\tdifw\\*.log", getenv("SystemRoot")) == -1) {
		return;
	}
	system(rm_cmd);
	rename(http_bak_fname, http_fname);
	rename(key_bak_fname, key_fname);
	rename(mouse_bak_fname, mouse_fname);
}
