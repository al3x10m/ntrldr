/*
 * NTRLDR: Sets up NVRAM and chainloads firmware.nds on the 3DS.
 * Written by 2018 dr1ft/UTP
 */

#include <nds.h>
#include <nds/system.h>
#include <fat.h>
#include <stdio.h>
#include "nds_loader_arm9.h"

bool isRegularDS = true;

static unsigned char slot0[0x100];
static unsigned char slot1[0x100];

void cfile(char* filename) {
    FILE *fpt = fopen(filename,"wb");
    fclose(fpt);
}

int fileexist(char* filename) {
    FILE *fpt = fopen(filename,"rb");
	if(fpt == NULL){
		return 0;
	} else {
		fclose(fpt);
		return 1;
	}
}

int main() {
	consoleDemoInit(); // setup the display for text
	printf("NTRLDR (release 9)\nwritten by dr1ft/UTP\n\n"); // display copyright and build information

	fifoWaitValue32(FIFO_USER_06);
	u16 arm7_SNDEXCNT = fifoGetValue32(FIFO_USER_07);
	if (arm7_SNDEXCNT != 0) isRegularDS = false;	// If sound frequency setting is found, then the console is not a DS Phat/Lite
	fifoSendValue32(FIFO_USER_07, 0);

	if(fatInitDefault()) {
        if(!fileexist("/_nds/ntrldr/warning_shown") && !isRegularDS) {
            printf("\x1b[31;1mwarning:\nthis program modifies nvram\ndirectly\nusing it on dsi is not\nrecommended as it could brick\nyour device under extreme\ncircumstances\npress Y to continue...\n\x1b[39m");
            while (1) {
                swiWaitForVBlank();
                scanKeys();
                if (keysHeld() & KEY_Y) break;
            }
            cfile("/_nds/ntrldr/warning_shown");
        }

		if (!isRegularDS) {
			printf("patching nvram...");
			readFirmware(0x3FE00,slot0,0x100); // read the first slot of profile data
			readFirmware(0x3FF00,slot1,0x100); // and the second one
			slot0[0x65] = 0xFC; // overwrite the least significant byte of the flags with a known good value
			slot1[0x65] = 0xFC;
			short crc0 = swiCRC16(0xffff,slot0,0x70); // generate a crc for the profile data so that the firmware doesnt get angry with us
			short crc1 = swiCRC16(0xffff,slot1,0x70);
			slot0[0x72] = crc0 & 0xFF; // overwrite the checksums with our new ones
			slot0[0x73] = (crc0 & 0xFF00) >> 8;
			slot1[0x72] = crc1 & 0xFF;
			slot1[0x73] = (crc1 & 0xFF00) >> 8;
			writeFirmware(0x3FE00,slot0,0x100); // write our modifications back to nvram
			writeFirmware(0x3FF00,slot1,0x100);
		}
        
        printf("ok\nloading firmware.nds...");
        int error = runNdsFile("/_nds/ntrldr/firmware.nds", 0, NULL); // bootstrap firmware.nds
        printf("\x1b[31;1merror %d\nmake sure firmware.nds is in\nthe root of the filesystem!", error); // if something fucks up display an error code
	}
	else {
		printf("\x1b[31;1mfat init failure");
	}
	while(1) swiWaitForVBlank();
	return 0;
}
