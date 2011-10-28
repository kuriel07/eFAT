/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : efat.h                                            */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#include "config.h"
#include "fat.h"
#include "file.h"
#include "dir.h"
#ifndef __EFAT_H
//x86 calculation
//total heap used 0 file + 1 fs max = approx 0.35kb (356 byte) + compiler alignment + allocation headers, no operation
//total heap used 1 file + 1 fs max = approx 1.2kb (1184 byte) + compiler alignment + allocation headers, read/write operation
//total heap used 2 file + 1 fs max = approx 2kb (2012 byte) + compiler alignment + allocation headers, read/write operation
//total heap used 2 file + 2 fs max = approx 2.4kb (2368 byte) + compiler alignment + allocation headers, read/write operation
//stack used max = approx 300byte (297 byte) + (stack needed for registers saving context * 4) + compiler alignment, read/write operation

//example calculation
//suppose we have an atmega32 2kb of ram, and we only use 1 file for read and write operation then we can calculate free memory area
//by substracting total memory of device with the amount of heap and stack used for efat
//2kb - (1.2kb + 300b + (32 * 4)) = 0.37kb (this is the amount left for heap, stack and global/static variable)
//with atmega128 (4kb of ram) the amount of free memory would be 2.37kb, this is enough to open 2 file + 1 fs at the same time

#define EFAT_DIRECTORY	FAT_ATTR_DIR

#define EFAT_OPEN		FILE_OPENED
#define EFAT_APPEND		FILE_MODE_APPEND
#define EFAT_WRITE		FILE_OPEN_WRITE
#define EFAT_READ		FILE_OPEN_READ

//datatype definition
#define efat_Entry		fat_DirEntry
#define efat_FileSystem fat_FileSystem
#define efat_File 		file_File
#define efat_DirList 	dir_DirList

//efat_Init(partition_no, hw_interface)
//parameter 1 : number of partition (each fat drive have at least 1 partition and max up to 4 partition)
//parameter 2 : pointer to the hw_interface, used to determine which card selected
//return : pointer to the current initialized file system
//ex : efat_FileSystem * fs = efat_Init(0, 0);
#define efat_Init fat_Init

//efat_Open(path, file_system, attrib)
//parameter 1 : path to current file
//parameter 2 : pointer to the file system
//parameter 3 : attribute of the opened file read(r), write(w), append(+)
//return : pointer to the opened file
//ex: efat_File * myFile = efat_Open("log.txt", fs, "rw+");
#define efat_Open file_Open

//efat_Seek(file, offset)
//parameter 1 : pointer to current file
//parameter 2 : offset of the current file
//return : void
//ex : efat_Seek(myFile, 0);
#define efat_Seek file_Seek

//efat_Read(size, file, buffer)
//parameter 1 : size to read in bytes
//parameter 2 : pointer file to read
//parameter 3 : pointer to the read buffer
//return : size of bytes read
//ex : uint32 read_size = efat_Read(512, myFile, (uchar *)buf);
#define efat_Read file_Read

//efat_Write(size, file, buffer)
//parameter 1 : size to write in bytes
//parameter 2 : pointer file to write
//parameter 3 : pointer to the write buffer
//return : size of bytes wrote
//ex : uint32 write_size = efat_Write(512, myFile, buf);
#define efat_Write file_Write

//efat_Close(file)
//parameter 1 : pointer to the current file
//return : void
//ex : efat_Close(myFile);
#define efat_Close file_Close

//efat_CreateDirList(path, file_system)
//parameter 1 : directory to list
//parameter 2 : pointer to the file system
//return : pointer to DirList for directory listing
//ex : efat_DirList * dlist = efat_CreateDirList("\\temporary", fs);
#define efat_CreateDirList dir_CreateEntryList

//efat_DirListNext(dirlist)
//parameter 1 : pointer to DirList
//return : non zero value if next directory exist, zero if there's no directory anymore
//ex : while(efat_DirListNext(dlist) != 0) { printf("%s\n", dlist->entry->lfn); }
#define efat_DirListNext dir_GetEntryList

//dir_DeleteList(dirlist)
//parameter 1 : pointer to DirList
//return : void
//ex : dir_DeleteList(dlist);
#define efat_DestroyDirList dir_DeleteList

#define __EFAT_H
#endif
