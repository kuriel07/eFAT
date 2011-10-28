/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : file.h                                            */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#include "fat.h"
#ifndef __EFAT_FILE_H

#define FILE_OPENED			0x80
#define FILE_OPEN_CREATE	0x08
#define FILE_MODE_APPEND	0x04
#define FILE_OPEN_WRITE		0x02
#define FILE_OPEN_READ		0x01

typedef struct file_File {
	uchar status;			//status of current file
	fat_DirEntry * entry;	//entry of current file (everyfile must have 1 entry)
	uint32 data_cluster;	//1st cluster of current file
	uint32 current_cluster;	//current cluster for read/write operation
	uint32 current_offset;	//current cluster offset for read/write operation
	uint32 file_ptr;		//data offset of current file
	fat_FileSystem * fs;	//file system of current file
} file_File;

//available modes
//r = read access
//w = write access
//c = allow creation
//+ = append (move pointer to the last offset)
uchar file_GetStatus(uchar * attrib);
file_File * file_Open(uchar * path, fat_FileSystem * fs, uchar * attrib);
file_File * file_OpenStatus(uchar * path, fat_FileSystem * fs, uchar status);
uint32 file_Read(uint32 size, file_File * file, uchar * buf);
uint32 file_Write(uint32 size, file_File * file, uchar * buf);
uint32 file_FileRead(uint32 offset, uint32 size, file_File * file, uchar * buf);
uint32 file_FileWrite(uint32 offset, uint32 size, file_File * file, uchar * buf);
uchar file_Seek(file_File * file, uint32 offset);
void file_Delete(file_File * file);
uchar file_Close(file_File * file);

#define __EFAT_FILE_H
#endif
