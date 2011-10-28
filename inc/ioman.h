/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : ioman.h                                           */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
//for testing purpose use offline file, include stdio
#include <stdio.h>
#ifndef __EFAT_IOMAN_H

#define CMD_DATBYTE		0
#define CMD_DATHW		1
#define CMD_DATWORD		2
#define CMD_LONGRES		4		//long response
#define CMD_NORES		8		//no response
#define CMD_DATA		16		//with data
#define CMD_DATBLOCK	32		//transmit block
#define CMD_DATWIDE		64		//wide bus
#define CMD_DATXMIT		128		//transmit
#define CMD_SHORTRES	0

#define GO_IDLE_STATE		0	//CMD0
#define SEND_OP_COND		1	//CMD1
#define ALL_SEND_CID		2	//CMD2
#define SEND_RELATIVE_ADDR	3	//CMD3
#define SET_DSR				4	//CMD4
#define IO_SEND_OP_COND		5	//CMD5
#define SELECT_CARD			7	//CMD7
#define SD_READ_CSD			9	//CMD9
#define SD_READ_CID			10	//CMD10
#define STOP_TRANSMISSION	12	//CMD12
#define SD_READ_STATUS		13	//CMD13
#define GO_INACTIVE_STATE	15	//CMD15
#define SET_BLOCKLEN		16	//CMD16
#define READ_SINGLE_BLOCK	17	//CMD17
#define WRITE_SINGLE_BLOCK	24	//CMD24
#define APP_CMD				55	//CMD55
#define SD_STATUS			13	//ACMD13
#define SD_APP_OP_COND		41	//ACMD41
#define SD_READ_OCR			58	//OCR opearating condition register

#define SCMD_TIMEOUT		64
#define	SCMD_NODEV			44
#define SCMD_SUCCESS		12
#define SCMD_FAIL			13

#define SDICCON_START		(1<<8)
#define SDICCON_WAITRES		(1<<9)
#define SDICCON_LONGRES		(1<<10)
#define SDICCON_WITHDAT		(1<<11)

#define SDIDCON_DATASIZE	22
#define SDIDCON_TARSP		(1<<20)
#define SDIDCON_RACMD		(1<<19)
#define SDIDCON_BACMD		(1<<18)
#define SDIDCON_DATMODE		12
#define SDIDCON_WIDEBUS		(1<<16)
#define SDIDCON_BLOCKMODE	(1<<17)
#define SDIDCON_START		(1<<14)

#define SDICSTA_CRCFAIL		(1<<12)
#define SDICSTA_CMDSENT		(1<<11)
#define SDICSTA_TIMEOUT		(1<<10)
#define SDICSTA_RESPFIN		(1<<9)

#define SDIFSTA_FRST		(1<<16)
#define SDIFSTA_TFDET		(1<<13)
#define SDIFSTA_RFDET		(1<<12)

#define SDICON_TYPEB 		(1<<4)
#define SDICON_CLKEN		(1)
#define SDICON_MMCCLK		(1<<5)
#define SDICON_RESET		(1<<8)

typedef struct HW_Interface {
	uchar id;
	uint16 rca;
}  HW_Interface;

typedef struct SD_Command {
	uchar res;
	uchar cmd;
	uint32 arg;
	uint32 response[4];
} SD_Command;

uchar SDSendCommand(struct SD_Command * scmd, void * buffer, uint32 size, HW_Interface * interface);
esint8 sd_Init(HW_Interface * interface);
uchar ioman_init(HW_Interface * interface);
void ioman_readblock(HW_Interface * interface, uchar * buffer, uint32 address, uint32 size);
void ioman_writeblock(HW_Interface * interface, uchar * buffer, uint32 address, uint32 size);

#define __EFAT_IOMAN_H
#endif
