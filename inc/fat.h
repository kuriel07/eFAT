/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : fat.h                                             */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#include "config.h"
#include "ioman.h"
#ifndef __EFAT_FAT_H

#define FAT_MBR_PARTITION_SIZE	16
#define FAT_MBR_OFFSET_P1		446
#define FAT_MBR_OFFSET_P2		(FAT_MBR_OFFSET_P1 + FAT_MBR_PARTITION_SIZE)
#define FAT_MBR_OFFSET_P3		(FAT_MBR_OFFSET_P2 + FAT_MBR_PARTITION_SIZE)
#define FAT_MBR_OFFSET_P4		(FAT_MBR_OFFSET_P3 + FAT_MBR_PARTITION_SIZE)

#define FAT_BR32_OFFSET_BPS		0x0B
#define FAT_BR32_OFFSET_SPC		0x0D
#define FAT_BR32_OFFSET_NRS		0x0E
#define FAT_BR32_OFFSET_NOF		0x10
#define FAT_BR32_OFFSET_TS16	0x13
#define FAT_BR32_OFFSET_TS32	0x20
#define FAT_BR32_OFFSET_SPF		0x24
#define FAT_BR32_OFFSET_ROOT	0x2C
#define FAT_BR32_OFFSET_SGN		0x1FE

#define DIR_NAME				0x00	
#define DIR_NAME_SIZE			11 		//bytes
#define DIR_ATTR				0x0B	//8 Bits
#define DIR_FSTCLUSHI			0x14	//16 Bits
#define DIR_FSTCLUSLO			0x1A	//16 Bits
#define DIR_FILESIZE			0x1C	//32 Bits

#define FAT_SECTOR_SIZE 		512
#define FAT_EMPTY_CLUSTER		0
#define FAT32_EMPTY_CHAIN		0xfffffff
#define FAT16_EMPTY_CHAIN		0xffff
#define FAT_SIGNATURE			0xaa55

#define FAT_ATTR_DIR			(1<<4)
#define FAT_ATTR_READONLY		(1<<0)
#define FAT_ATTR_HIDDEN			(1<<1)
#define FAT_ATTR_SYSTEM			(1<<2)
#define FAT_ATTR_VOLID			(1<<3)
#define FAT_ATTR_ARCHIVE		(1<<5)
#define FAT_ATTR_FILE			(0x00)

#define TYPE_FAT32				32
#define TYPE_FAT16				16
#define TYPE_FAT12				12

#define SIZE_MAX_LFN			256		//maximum fat long filename
#define SIZE_MAX_8_3			11		//maximum 8.3 filename
#define SIZE_OF_DIR_ENTRY		32		//entry size


//16 byte
typedef struct fat_Partition {
	uchar bootflag;
	uchar start_head;
	uint16 start_sector;	
	uchar type;
	uchar end_head;
	uint16 end_sector;
	uint32 lba;
	uint32 numofsec;
} fat_Partition;

typedef struct fat_MasterBootRecord {
	uchar code[FAT_SECTOR_SIZE];
} fat_MasterBootRecord;

typedef struct fat_BootRecord {
	uint16 byte_per_sec;
	uchar sec_per_clus;
	uint16 num_of_rsv_sect;
	uchar num_of_fats;
	uint16 total_sector_16;
	uint32 total_sector_32;
	uint32 sec_per_fat;
	uint32 root;
	uint16 signature;	//0xaa55 always
} fat_BootRecord;

typedef struct fat_Entry {
	uint16 media_type;
	uint16 partition_state;
} fat_Entry;

typedef struct fat_FileSystem {
	uchar type;			//determine which fat type
	uint32 fat_begin_lba;
	uint32 cluster_begin_lba;
	uchar sectors_per_cluster;
	uint32 root_dir_first_cluster;
	HW_Interface * hw_interface;
} fat_FileSystem;

typedef struct fat_DirEntry {
	uchar lfn[SIZE_MAX_LFN];
	uchar attrib;
	uchar is_lfn;
	uint16 offset;		//offset of current entry in a sector
	uint32 sector;		//sector of current entry
	uint32 cluster;
	uint32 size;
} fat_DirEntry;

uchar fat_GetDirEntry(uint16 no, uint32 current_sector, fat_DirEntry * direc, fat_FileSystem * fs);
void fat_UpdateDirEntry(fat_DirEntry * dr, fat_FileSystem * fs);
uint32 fat_AllocNewCluster(fat_FileSystem * fs);
uint32 fat_UnlinkCluster(uint32 root_sector, fat_FileSystem * fs);
uint32 fat_GetNextCluster(uint32 root_sector, fat_FileSystem * fs);
uint32 fat_GetDataCluster(uint32 fst_cluster, fat_FileSystem * fs);
uint32 fat_GetChainOffset(uint32 root_sector, fat_FileSystem * fs);
uint32 fat_AllocClusterLink(uint32 root_sector, fat_FileSystem * fs);
void fat_RemoveDirEntry(fat_DirEntry * dr, fat_FileSystem * fs);
void fat_CreateNewEntry(uchar attrib, uint32 root_sector, fat_DirEntry * direc, uchar * name, fat_FileSystem * fs);
void fat_ClearCluster(uint32 cluster_no, fat_FileSystem * fs);
void fat_CreateDirCluster(uint32 cluster_no, uint32 parent_cluster, fat_FileSystem * fs);
uint16 fat_ReadCluster(uint32 root_sector, uint16 offset, uint16 size, uchar * buf, fat_FileSystem * fs);
uint16 fat_WriteCluster(uint32 root_sector, uint16 offset, uint16 size, uchar * buf, fat_FileSystem * fs);
void fat_InitPartition32(fat_FileSystem * fs, fat_Partition * p, fat_BootRecord * br, HW_Interface * iface);
void fat_InitPartition16(fat_FileSystem * fs, fat_Partition * p, fat_BootRecord * br, HW_Interface * iface);

fat_Partition * fat_ReadPartition(uchar partition_id, HW_Interface * interface);
fat_BootRecord * fat_GetBootRecord(fat_Partition * p, HW_Interface * interface);
fat_FileSystem * fat_Init(uchar partition_id, HW_Interface * interface);

#define __EFAT_FAT_H
#endif

