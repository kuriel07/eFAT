/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : dir.h                                             */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#include "fat.h"
#ifndef __EFAT_DIR_H

typedef struct dir_DirList {
	uint16 next;
	uint32 cluster;
	fat_FileSystem * fs;
	fat_DirEntry * entry;
} dir_DirList;

void dir_Create(uint32 root_sector, uchar * name, fat_DirEntry * entry, fat_FileSystem * fs);
dir_DirList * dir_CreateEntryList(uchar * path, fat_FileSystem * fs);
uchar dir_GetEntryList(dir_DirList * list);
void dir_DeleteList(dir_DirList * list);

#define __EFAT_DIR_H
#endif
