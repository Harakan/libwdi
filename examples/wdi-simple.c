/*
* wdi-simple.c: Console Driver Installer for a single USB device
* Copyright (c) 2010-2016 Pete Batard <pete@akeo.ie>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#ifdef _MSC_VER
#include "getopt/getopt.h"
#else
#include <getopt.h>
#endif

#include "wdi-simple.h"
#include "libwdi.h"
#include "zadig.h"


#if defined(_PREFAST_)
/* Disable "Banned API Usage:" errors when using WDK's OACR/Prefast */
#pragma warning(disable:28719)
/* Disable "Consider using 'GetTickCount64' instead of 'GetTickCount'" when using WDK's OACR/Prefast */
#pragma warning(disable:28159)
#endif

#define oprintf(...) do {if (!opt_silent) printf(__VA_ARGS__);} while(0)

/*
 * Change these values according to your device if
 * you don't want to provide parameters
 */
#define BOOTLOADER_DESC        "STM32 BOOTLOADER"
#define BOOTLOADER_VID         0x0483
#define BOOTLOADER_PID         0xDF11
#define PEACHY_DESC        "Peachy Printer"
#define PEACHY_VID         0x16D0
#define PEACHY_PID         0x0AF3
#define INF_NAME    "stm32_bootloader.inf"
#define DEFAULT_DIR "usb_driver"

void usage(void)
{
	printf("\n");
	printf("-v, --verbose       print a bunch of debug stuff during operation\n");
	printf("-b, --bootloader    switch bootloader driver to WINUSB\n");
	printf("-l, --list          list all peachy's states\n");
	printf("-h, --help          display usage\n");
	printf("-p, --pause         Pause at end of script to see output\n");
	printf("\n");
	printf("Press Enter to continue\n");
	getchar(); //hacky pause (press enter)
}
int set_bootloader_to_winusb(int verbose) {
	int opt_silent = verbose;
	struct wdi_device_info *device, list;
	char* inf_name = NULL;
	int r;
	struct wdi_options_prepare_driver driver_opts = {0};

	driver_opts.driver_type = WDI_WINUSB;
	driver_opts.vendor_name = NULL;
	driver_opts.device_guid = NULL;
	driver_opts.disable_cat = FALSE;
	driver_opts.disable_signing = FALSE;
	driver_opts.use_wcid_driver = FALSE;	


	list = listDevices(verbose);
	if (&list != NULL) {
		oprintf("set_bootloader_to_winusb - valid list\n");
		for (device = &list; device != NULL; device = device->next) {
			if ((device->vid == BOOTLOADER_VID) && (device->pid == BOOTLOADER_PID)) {
				//inf_name = INF_NAME;
				oprintf("Installing using inf name: %s\n", INF_NAME);
				if (wdi_prepare_driver(device, DEFAULT_DIR, INF_NAME, &driver_opts) == WDI_SUCCESS) {
					oprintf("Successful driver prepare!\n");
					r=wdi_install_driver(device, DEFAULT_DIR, INF_NAME, NULL);
					oprintf("got return code: %d=%s \n", r,wdi_strerror(r));
				}
			}
		}
	}
	wdi_destroy_list(&list);
	return r;
}

struct wdi_device_info listDevices(int verbose){

	int opt_silent = verbose;
	struct wdi_device_info *device, *list;
	static struct wdi_options_create_list ocl = { 0 };

	ocl.list_all = TRUE;
	ocl.list_hubs = TRUE;
	ocl.trim_whitespaces = TRUE;

	if (wdi_create_list(&list, &ocl) == WDI_SUCCESS) {
		for (device = list; device != NULL; device = device->next) {
			oprintf("Found: %s @ (%04X:%04X)\n", device->desc, device->vid, device->pid);
			if ((device->vid == BOOTLOADER_VID) && (device->pid == BOOTLOADER_PID)) {
				printf("BOOTLOADER,DRIVER:%s,VERSION:%I64d\n",device->driver,device->driver_version);
			}
			else if ((device->vid == PEACHY_VID) && (device->pid == PEACHY_PID)) {
				printf("PEACHY,DRIVER:%s,VERSION:%I64d\n", device->driver, device->driver_version);
			}
		}
	}
	return *list;

}
// from http://support.microsoft.com/kb/124103/
HWND GetConsoleHwnd(void)
{
	HWND hwndFound;
	char pszNewWindowTitle[128];
	char pszOldWindowTitle[128];

	GetConsoleTitleA(pszOldWindowTitle, 128);
	wsprintfA(pszNewWindowTitle,"%d/%d", GetTickCount(), GetCurrentProcessId());
	SetConsoleTitleA(pszNewWindowTitle);
	Sleep(40);
	hwndFound = FindWindowA(NULL, pszNewWindowTitle);
	SetConsoleTitleA(pszOldWindowTitle);
	return hwndFound;
}

int __cdecl main(int argc, char** argv)
{
	static int opt_silent = 1, log_level = WDI_LOG_LEVEL_WARNING;
	struct wdi_device_info *list;
	int c,r;
	BOOL list_usbs=FALSE, bootloaderWinusbInstall=FALSE;
	BOOL pause=FALSE;
	char *inf_name = INF_NAME;
	char *ext_dir = DEFAULT_DIR;
	char *cert_name = NULL;

	static struct option long_options[] = {
		{"verbose", no_argument, 0, 'v'},
		{"list", no_argument, 0, 'l'},
		{"help", no_argument, 0, 'h'},
		{"pause", no_argument, 0, 'p'},
		{"bootloader", no_argument, 0, 'b'},
		{0, 0, 0, 0}
	};

	while(1)
	{
		c = getopt_long(argc, argv, "blhvp", long_options, NULL);
		oprintf("got argument: %d \n", c);
		if (c == -1)
			break;
		switch(c) {
		case 'v':
			opt_silent = 0;
			break;
		case 'h':
			usage();
			break;
		case 'l':
			list_usbs=TRUE;
			break;
		case 'p':
			pause=TRUE;
			break;
		case 'b':
			bootloaderWinusbInstall = TRUE;
			break;
		default:
			usage();
			exit(0);
		}
	}

	wdi_set_log_level(log_level);

	if (is_x64()) {
		oprintf("I see you are on a 64 bit system, nice\n");
	}
	else {
		oprintf("What a lovely 32 bit system you have\n");
	}

	if (list_usbs) {
		listDevices(opt_silent);
	}

	if (bootloaderWinusbInstall) {
		r=set_bootloader_to_winusb(opt_silent);
		printf("RETURN:%d\n", r); //just print the return code for now
	}

	if (pause){
		printf("Press Enter to continue\n");
		getchar(); //hacky pause (press enter)
	}
	
	return 0;
}
