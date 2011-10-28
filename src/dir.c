/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : dir.c                                             */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#include "config.h"
#include "fat.h"
#include "util.h"
#include "dir.h"

void dir_Create(uint32 root_sector, uchar * name, fat_DirEntry * entry, fat_FileSystem * fs) {
	uint32 cluster_no;
	fat_CreateNewEntry(FAT_ATTR_DIR, root_sector, entry, name, fs);
	cluster_no = fat_GetDataCluster(entry->cluster, fs);
	fat_ClearCluster(cluster_no, fs);
	fat_CreateDirCluster(entry->cluster, fat_GetChainOffset(root_sector, fs), fs);
}

dir_DirList * dir_CreateEntryList(uchar * path, fat_FileSystem * fs) {
	uchar a = 0x00;
	uchar entry_found = 0;
	dir_DirList * list;
	uint16 i, length, j = 0;
	uchar skip;
	uchar dir_name[256];
	list = (dir_DirList *)efat_malloc(sizeof(dir_DirList));
	list->entry = (fat_DirEntry *)efat_malloc(sizeof(fat_DirEntry));
	list->fs = fs;
	list->next = 0;
	list->entry->attrib |= FAT_ATTR_DIR;
	list->cluster = fs->cluster_begin_lba;
	i = 0;
	while(entry_found==0) {
		length = util_DirNameCopy(dir_name, path + i);
		if(length == 0) {
			if(list->entry->attrib & FAT_ATTR_DIR) 
				return list;		//is directory
			dir_DeleteList(list);
			return 0;		//not directory
		}
		i += length;
		if(dir_name[0] != 0) {		//eat empty space ex: \\libraries\\.. --> libraries\\..
			while((skip = fat_GetDirEntry(j, list->cluster, list->entry, list->fs)) !=0 ) {
				j += skip;
				if(util_DirNameCompare(dir_name, list->entry->lfn)) {
						list->cluster = fat_GetDataCluster(list->entry->cluster, fs); 
					j = 0;
					break;
				}
			}
		}
	}
	dir_DeleteList(list);
	return 0;	//file not found
}

uchar dir_GetEntryList(dir_DirList * list) {
	uchar skip;
	skip = fat_GetDirEntry(list->next, list->cluster, list->entry, list->fs);
	list->next += skip;
	return skip;
}

void dir_DeleteList(dir_DirList * list) {
	list->next = 0;
	list->cluster = 0;
	efat_free(list->entry);
	efat_free(list);
}
