/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : file.c                                            */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#include "config.h"
#include "fat.h"
#include "util.h"
#include "file.h"

uchar file_GetStatus(uchar * attrib) {
	uchar status = 0x00;
	uchar c;
	while(1) {
		c = *(attrib++);
		switch(c) {
			case 0:
				return status;
			case 'c':
				status |= FILE_OPEN_CREATE;
				break;
			case 'w':
				status |= FILE_OPEN_WRITE;
				break;
			case 'r':
				status |= FILE_OPEN_READ;
				break;
			case '+':
				status |= FILE_MODE_APPEND;
				break;
			default:
				break;
		}
	}
}

void file_Create(uint32 root_sector, uchar * name, file_File * file, fat_FileSystem * fs) {
	fat_CreateNewEntry(FAT_ATTR_FILE, root_sector, file->entry, name, fs);
}

file_File * file_Open(uchar * path, fat_FileSystem * fs, uchar * attrib) {
	return file_OpenStatus(path, fs, file_GetStatus(attrib));
}

file_File * file_OpenStatus(uchar * path, fat_FileSystem * fs, uchar status) {
	uchar a = 0x00;
	uchar entry_found = 0;
	file_File * file;
	uint16 i, length, j = 0;
	uint16 c;
	uchar skip;
	uchar file_name[SIZE_MAX_LFN];
	uint32 cluster_no;
	cluster_no = fs->cluster_begin_lba;
	file = (file_File *)efat_malloc(sizeof(file_File));
	file->entry = (fat_DirEntry *)efat_malloc(sizeof(fat_DirEntry));
	file->status = status;
	file->entry->attrib |= FAT_ATTR_DIR;		//set current file to root directory
	i = 0;
	while(entry_found==0) {
		fopen_get_next_entry:
		length = util_DirNameCopy(file_name, path + i);
		if(length == 0) {
			if(file->entry->attrib & FAT_ATTR_DIR) {	//check if directory
				file_Close(file);
				return 0;						//return null if directory
			}
			return file;
		}
		i += length;
		if(file_name[0] != 0) {		//eat empty space ex: \\libraries\\.. --> libraries\\..
			while((skip = fat_GetDirEntry(j, cluster_no, file->entry, fs)) !=0 ) {
				j += skip;
				if(util_DirNameCompare(file_name, file->entry->lfn)) {
					if(file->entry->attrib & FAT_ATTR_DIR) {
						cluster_no = fat_GetDataCluster(file->entry->cluster, fs); 
					} else {
						file->data_cluster = fat_GetDataCluster(file->entry->cluster, fs);
						if(a & FILE_MODE_APPEND) {
							file->file_ptr = file->entry->size - 1;
						} else {
							file->file_ptr = 0;
						}
						(fat_FileSystem *)file->fs = fs;
						file->status |= FILE_OPENED;		//set status
						file->current_cluster = file->data_cluster;		//set current cluster to 1st cluster
						file->current_offset = 0;
						return file;						//asumsi tidak ada file didalam file ??
					}
					j = 0;
					goto fopen_get_next_entry;
				}
			}
			if(status & FILE_OPEN_CREATE) {		//check if creation allowed
				if(path[i - 1] == '\\') {		//is directory?
					//use dir create
					dir_Create(cluster_no, file_name, file->entry, fs);				//create directory
					cluster_no = fat_GetDataCluster(file->entry->cluster, fs);
					j = 0;
					goto fopen_get_next_entry;
				} else {						//then file
					file_Create(cluster_no, file_name, file, fs);
					file->data_cluster = fat_GetDataCluster(file->entry->cluster, fs);
					if(a & FILE_MODE_APPEND) {
						file->file_ptr = file->entry->size - 1;
					} else {
						file->file_ptr = 0;
					}
					(fat_FileSystem *)file->fs = fs;
					file->status |= FILE_OPENED;		//set status
					file->current_cluster = file->data_cluster;		//set current cluster to 1st cluster
					file->current_offset = 0;
					return file;
				}
			}
			//file/directory tidak ditemukan
			file_Close(file);
			return 0;
		}
	}
	file_Close(file);
	return 0;	//file not found
}

uint32 file_Read(uint32 size, file_File * file, uchar * buf) {
	uint32 bytes;
	bytes = file_FileRead(file->file_ptr, size, file, buf);
	file->file_ptr += bytes;
	return bytes;
}

uint32 file_Write(uint32 size, file_File * file, uchar * buf) {
	uint32 bytes;
	bytes = file_FileWrite(file->file_ptr, size, file, buf);
	file->file_ptr += bytes;
	return bytes;
}

uint32 file_FileRead(uint32 offset, uint32 size, file_File * file, uchar * buf) {
	uint32 byte_read = 0;
	uint32 cluster_size;
	uint32 a_offset;
	uint32 d_offset;
	cluster_size = (file->fs->sectors_per_cluster * FAT_SECTOR_SIZE);
	//status check
	if((file->status & FILE_OPEN_READ) == 0x00) {
		return byte_read;
	}
	if(offset > file->entry->size) {
		return byte_read;
	}
	if((offset + size) >= file->entry->size) {
		size = file->entry->size - offset;		//change size
	}
	if(offset >= file->current_offset) {		//start from current cluster
		offset -= file->current_offset;
	} else { 									//start from 1st cluster
		file->current_cluster = file->data_cluster;
		file->current_offset = 0;
	}
	//set current cluster to the write offset
	while(offset > cluster_size) {
		file->current_cluster = fat_GetNextCluster(file->current_cluster, file->fs);
		file->current_offset += cluster_size;
		offset -= cluster_size;
	}
	d_offset = (offset + size);
	//begin read data from cluster
	while(d_offset > cluster_size) {
		size -= (cluster_size - offset);
		byte_read += fat_ReadCluster(file->current_cluster, offset, (cluster_size - offset), (uchar *)((uint32)buf + byte_read), file->fs);
		offset = 0;
		d_offset -= cluster_size;
		//set current cluster to next cluster
		file->current_cluster = fat_GetNextCluster(file->current_cluster, file->fs);
		file->current_offset += cluster_size;
	}
	byte_read += fat_ReadCluster(file->current_cluster, offset, size, (uchar *)((uint32)buf + byte_read), file->fs);
	return byte_read;
}

uint32 file_FileWrite(uint32 offset, uint32 size, file_File * file, uchar * buf) {
	uint32 byte_write = 0;
	uint32 cluster_size;
	uint32 added_size = 0;
	uint32 alloc_size = 0;
	uint32 c_cluster_no;
	uint32 d_offset;
	cluster_size = (file->fs->sectors_per_cluster * FAT_SECTOR_SIZE);
	//status check
	if((file->status & FILE_OPEN_WRITE) == 0x00) {
		return byte_write;
	}
	if(offset > file->entry->size) {
		return byte_write;
	}
	if((offset + size) >= file->entry->size) {
		added_size = (offset + size) - file->entry->size;		//change size
		alloc_size = (added_size + offset) - file->current_offset;
		//alloc_size -= (alloc_size % cluster_size);
	}
	if(offset >= file->current_offset) {		//start from current cluster
		offset -= file->current_offset;
	} else { 									//start from 1st cluster
		file->current_cluster = file->data_cluster;
		file->current_offset = 0;
	}
	//set current cluster to the write offset
	while(offset > cluster_size) {
		file->current_cluster = fat_GetNextCluster(file->current_cluster, file->fs);
		file->current_offset += cluster_size;
		offset -= cluster_size;
	}
	//allocate new cluster chains
	c_cluster_no = file->current_cluster;
	while(alloc_size > cluster_size) {
		c_cluster_no = fat_AllocClusterLink(c_cluster_no, file->fs);
		alloc_size -= cluster_size;
	}
	d_offset = (offset + size);
	//begin write data to cluster
	while(d_offset > cluster_size) {
		size -= (cluster_size - offset);
		byte_write += fat_WriteCluster(file->current_cluster, offset, (cluster_size - offset), (uchar *)((uint32)buf + byte_write), file->fs);
		offset = 0;
		d_offset -= cluster_size;
		//set current cluster to next cluster
		file->current_cluster = fat_GetNextCluster(file->current_cluster, file->fs);
		file->current_offset += cluster_size;
	}
	byte_write += fat_WriteCluster(file->current_cluster, offset, size, (uchar *)((uint32)buf + byte_write), file->fs);

	//set file entry fields
	//printf("added size : %d\n", added_size);
	file->entry->size += added_size;

	//update entry
	fat_UpdateDirEntry(file->entry, file->fs);
	return byte_write;
}

uchar file_Seek(file_File * file, uint32 offset) {
	if(offset < file->entry->size) {
		file->file_ptr = offset;
		return 0;
	}
	return -1;
}

void file_Delete(file_File * file) {
	if(file != NULL) {
		fat_RemoveDirEntry(file->entry, file->fs);
		file->status &= ~FILE_OPENED;
		efat_free(file->entry);
		efat_free(file);
	}
}

uchar file_Close(file_File * file) {
	if(file != NULL && file->status & FILE_OPENED) {
		file->status &= ~FILE_OPENED;
		efat_free(file->entry);
		efat_free(file);
		return 0;
	}
	return -1;
}
