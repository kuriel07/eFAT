/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : fat.h                                             */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#include "config.h"
#include "fat.h"
#include "ioman.h"
#include "util.h"

//calculate cluster no of 1st data cluster on file entry/directory entry
uint32 fat_GetDataCluster(uint32 fst_cluster, fat_FileSystem * fs) {
	return (((fst_cluster - 2) * fs->sectors_per_cluster) + fs->cluster_begin_lba);
}

uint32 fat_GetChainOffset(uint32 root_sector, fat_FileSystem * fs) {
	if(fs->type == TYPE_FAT32) {
		return (((root_sector - fs->cluster_begin_lba) / fs->sectors_per_cluster) + 2) * sizeof(uint32);
	}
	return 0;
}

//suppose there's no cluster larger than 64k
uint16 fat_ReadCluster(uint32 root_sector, uint16 offset, uint16 size, uchar * buf, fat_FileSystem * fs) {
	uint16 a_offset = offset;
	uint16 byte_read = 0;
	uint32 current_sector = root_sector + (a_offset / FAT_SECTOR_SIZE);
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	offset = (a_offset % FAT_SECTOR_SIZE);
	ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	next_sector:
	//prevent redundant access to the same sector
	if(current_sector != (root_sector + (a_offset / FAT_SECTOR_SIZE))) {
		current_sector = (root_sector + (a_offset / FAT_SECTOR_SIZE));
		ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	}
	if(size < FAT_SECTOR_SIZE) {
		if(offset != 0) {
			efat_memcopy((uchar *)buf, (uchar *)((uint32)block + offset), 0, size);
			offset = 0;
		} else {
			efat_memcopy((uchar *)buf, block, 0, size);
		}
		//size -= size;
		byte_read += size;
		efat_free(block);
		return byte_read;
	}
	if(offset != 0) {
		efat_memcopy((uchar *)buf, block + offset, 0, size);
		
	} else {
		efat_memcopy((uchar *)buf, block, 0, size);
	}
	size -= (FAT_SECTOR_SIZE - offset);
	byte_read += (FAT_SECTOR_SIZE - offset);
	buf += (FAT_SECTOR_SIZE - offset);
	offset = 0;
	a_offset += byte_read;
	goto next_sector;
}

//suppose there's no cluster larger than 64k
uint16 fat_WriteCluster(uint32 root_sector, uint16 offset, uint16 size, uchar * buf, fat_FileSystem * fs) {
	uint16 a_offset = offset;
	uint16 byte_write = 0;
	uint32 current_sector = root_sector + (a_offset / FAT_SECTOR_SIZE);
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	offset = (a_offset % FAT_SECTOR_SIZE);
	ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	next_sector:
	//prevent redundant access to the same sector
	if(current_sector != (root_sector + (a_offset / FAT_SECTOR_SIZE))) {
		current_sector = (root_sector + (a_offset / FAT_SECTOR_SIZE));
		ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	}
	if(size < FAT_SECTOR_SIZE) {
		if(offset != 0) {
			efat_memcopy(block + offset, (uchar *)buf, 0, size);
			offset = 0;
		} else {
			efat_memcopy(block, (uchar *)buf, 0, size);
		}
		ioman_writeblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
		//size -= size;
		byte_write += size;
		efat_free(block);
		return byte_write;
	}
	if(offset != 0) {
		efat_memcopy(block + offset, (uchar *)buf, 0, size);
	} else {
		efat_memcopy(block, (uchar *)buf, 0, size);
	}
	ioman_writeblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	size -= (FAT_SECTOR_SIZE - offset);
	byte_write += (FAT_SECTOR_SIZE - offset);
	buf += (FAT_SECTOR_SIZE - offset);
	offset = 0;
	a_offset += byte_write;
	goto next_sector;
}

//get directory entry parameter no of entry, root cluster no, directory entry, file system
uchar fat_GetDirEntry(uint16 no, uint32 root_sector, fat_DirEntry * direc, fat_FileSystem * fs) {
	uint32 a_offset = (no * SIZE_OF_DIR_ENTRY);		//posisi offset thdp cluster
	uint32 offset = (no * SIZE_OF_DIR_ENTRY);			//posisi offset thdp sector
	uint16 cluster_high;
	uint16 cluster_low;
	uint16 lfn_offset = 0;
	uint32 current_sector = 0;
	uchar name_stack[SIZE_MAX_LFN];
	uchar dirname[SIZE_MAX_8_3];
	uchar c, i, j;
	uchar _stack_index = 0;
	uchar skip = 0;
	uchar * block;
	//Uart_Printf("allocating\n");
	block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	direc->is_lfn = 0;
	//Uart_Printf("allocated\n");
	next_dir_entry:
	if((a_offset / FAT_SECTOR_SIZE) >= (fs->sectors_per_cluster)) {
		//get next cluster;
		root_sector = fat_GetNextCluster(root_sector, fs);
		if(root_sector == 0) {
			//have no next cluster
			efat_free(block);
			return 0;
		}
		a_offset -= (fs->sectors_per_cluster * FAT_SECTOR_SIZE);
		goto next_dir_entry;
	}
	//prevent redundant access to the same sector
	if(current_sector != (root_sector + (a_offset / FAT_SECTOR_SIZE))) {
		current_sector = root_sector + (a_offset / FAT_SECTOR_SIZE);
		ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	}
	offset = (a_offset % FAT_SECTOR_SIZE);
	efat_memcopy(&direc->attrib, (void *)((uint32)block + offset + DIR_ATTR), 0, sizeof(uchar));
	if((direc->attrib & 0x0f) == 0x0f && *(uchar *)((uint32)block + offset) != 0xe5) {
		//instead of unicode use ascii
		name_stack[_stack_index++] = block[offset + 30];
		name_stack[_stack_index++] = block[offset + 28];
		name_stack[_stack_index++] = block[offset + 26];
		name_stack[_stack_index++] = block[offset + 24];
		name_stack[_stack_index++] = block[offset + 22];
		name_stack[_stack_index++] = block[offset + 20];
		name_stack[_stack_index++] = block[offset + 18];
		name_stack[_stack_index++] = block[offset + 16];
		name_stack[_stack_index++] = block[offset + 14];
		name_stack[_stack_index++] = block[offset + 9];
		name_stack[_stack_index++] = block[offset + 7];
		name_stack[_stack_index++] = block[offset + 5];
		name_stack[_stack_index++] = block[offset + 3];
		name_stack[_stack_index++] = block[offset + 1];	
		a_offset += 32;
		skip++;
		direc->is_lfn = 1;
		goto next_dir_entry;
	} else {
		efat_memcopy(dirname, (void *)((uint32)block + offset + DIR_NAME), 0, DIR_NAME_SIZE);
		if(dirname[0] == 0xe5) {	//check if directory entry is deleted
			a_offset += 32;	
			skip++;
			_stack_index = 0;
			direc->is_lfn = 0;
			goto next_dir_entry;
		}
		if(dirname[0] == 0) {
			efat_free(block);
			return 0;
		}
		if(_stack_index != 0) {
			do {
				
				c = name_stack[--_stack_index];
				if(c == 0xff) break;
				if(c != 0) {
					direc->lfn[lfn_offset++] = c;
				}
			} while(_stack_index != 0);
			direc->lfn[lfn_offset] = 0;
		}
		if(!direc->is_lfn) {
			i=0;
			while(i<8) {
				c = dirname[i];
				if(c > 0x40 && c < 0x5b) {
					c = c + 0x20;
				} else if(c==0x20) {
					break;
				}
				direc->lfn[i++] = c;
				j = i; 
			}
			if(direc->attrib & (1<<5) || direc->attrib==0x00) {
				i = 8;
				direc->lfn[j++] = '.';
				while(i<11) {
					c = dirname[i];
					if(c > 0x40 && c < 0x5b) {
						c = c + 0x20;
					} else if(c==0x20) {
						break;
					}
					direc->lfn[j++] = c;
					i++; 
				}
				direc->lfn[j] = 0;
			} else {
				direc->lfn[j] = 0;
			}
		}
		direc->sector = current_sector;			//set directory entry root sector for deletion
		direc->offset = offset;
		efat_memcopy(&cluster_high, (void *)((uint32)block + offset + DIR_FSTCLUSHI), 0, sizeof(uint16));
		efat_memcopy(&cluster_low, (void *)((uint32)block + offset + DIR_FSTCLUSLO), 0, sizeof(uint16));
		direc->cluster =  ((uint32)cluster_high << 16) | (uint32)cluster_low;
		efat_memcopy(&direc->size, (void *)((uint32)block + offset + DIR_FILESIZE), 0, sizeof(uint32));
		//call directory entry hook
		//fat_GetDirEntryHook(direc, fs);
		skip++;
	}
	efat_free(block);
	return skip;
}

void fat_UpdateDirEntry(fat_DirEntry * dr, fat_FileSystem * fs) {
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	uint16 cluster_high = (uint16)(dr->cluster >> 16);
	uint16 cluster_low = (uint16)(dr->cluster & 0xffff);
	ioman_readblock(fs->hw_interface, block, dr->sector, FAT_SECTOR_SIZE);
	efat_memcopy((void *)((uint32)block + dr->offset + DIR_ATTR), &dr->attrib, 0, sizeof(uchar));
	efat_memcopy((void *)((uint32)block + dr->offset + DIR_FSTCLUSHI), &cluster_high, 0, sizeof(uint16));
	efat_memcopy((void *)((uint32)block + dr->offset + DIR_FSTCLUSLO), &cluster_low, 0, sizeof(uint16));
	efat_memcopy((void *)((uint32)block + dr->offset + DIR_FILESIZE), &dr->size, 0, sizeof(uint32));
	ioman_writeblock(fs->hw_interface, block, dr->sector, FAT_SECTOR_SIZE);
	efat_free(block);
}

void fat_RemoveDirEntry(fat_DirEntry * dr, fat_FileSystem * fs) {
	uchar attrib = 0x0f;
	uint16 cluster_high = (uint16)(dr->cluster >> 16);
	uint16 cluster_low = (uint16)(dr->cluster & 0xffff);
	uint32 cluster = dr->cluster;
	uchar * block;
	block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	ioman_readblock(fs->hw_interface, block, dr->sector, FAT_SECTOR_SIZE);
	*(uchar *)((uint32)block + dr->offset + DIR_NAME) = 0xe5;
	ioman_writeblock(fs->hw_interface, block, dr->sector, FAT_SECTOR_SIZE);
	/*while(attrib == 0x0f) {		//delete long filename if exist
		if(dr->offset == 0) {
			dr->sector--;
			dr->offset = FAT_SECTOR_SIZE;
		}
		dr->offset -= SIZE_OF_DIR_ENTRY;
		ioman_readblock(fs->hw_interface, block, dr->sector, FAT_SECTOR_SIZE);
		attrib = *(uchar *)((uint32)block + dr->offset + DIR_ATTR);
		if(attrib == 0x0f) {
			*(uchar *)((uint32)block + dr->offset + DIR_NAME) = 0xe5;
			ioman_writeblock(fs->hw_interface, block, dr->sector, FAT_SECTOR_SIZE);
		}
	}*/
	while(cluster != 0 && cluster != (fs->cluster_begin_lba - (fs->sectors_per_cluster * 2))) {
		cluster = fat_UnlinkCluster(cluster, fs);		//remove all cluster
	}
	efat_free(block);
}

uchar fat_DosCheckSum(uchar * dosname) {
	uchar chksum = 0;
	uint16 i;
	for(i=0;i<10;i++) {
		if(dosname[i] & 0x80) {
			chksum = (((dosname[i] + chksum) << 1) | 0x01);
		} else {
			chksum = ((dosname[i] + chksum)<< 1);
		}
	}
	return (chksum + dosname[i]);
}

//create new directory entry, return new entry
void fat_CreateNewEntry(uchar attrib, uint32 root_sector, fat_DirEntry * direc, uchar * name, fat_FileSystem * fs) {
	uint32 a_offset = 0;		//posisi offset thdp cluster
	uint32 offset = 0;			//posisi offset thdp sector
	uint16 cluster_high;
	uint16 cluster_low;
	uint16 lfn_offset = 0;
	uint32 no_of_sectors;
	uint32 current_sector = 0;
	uint32 next_root_sector;
	uint16 lfn_length = efat_strlen(name);
	uint16 allocated_name_size = (lfn_length + (13 - (lfn_length % 13)));
	uint16 allocated_entry_size = (allocated_name_size / 13) + 1;
	uint16 current_entry_size = 0;
	uchar ordinal = 0x40 + (allocated_name_size / 13);
	uchar dos_chksum;	
	uchar name_stack[SIZE_MAX_LFN];
	uchar dirname[SIZE_MAX_8_3];
	uchar c, i, j;
	uchar _stack_index = 0;
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	efat_strcpy(direc->lfn, name);
	direc->attrib = attrib;
	direc->size = 0;
	for(i=(lfn_length + 1); i<allocated_name_size;i++) {
		direc->lfn[i] = 0xff;
	}
	//set dos filename
	if(direc->attrib & FAT_ATTR_DIR) {
		for(i=0;i<9;i++) {
			if(direc->lfn[i] > 0x60 && direc->lfn[i] < 0x7A) {
				dirname[i] = direc->lfn[i] - 0x20;
			} else {
				dirname[i] = direc->lfn[i];
			}
		}
		dirname[i++] = '~';
		dirname[i++] = '1';
	} else {
		if(lfn_length <= 8) {
			for(i=0;i<lfn_length;i++) {
				if(direc->lfn[i] > 0x60 && direc->lfn[i] < 0x7A) {
					dirname[i] = direc->lfn[i] - 0x20;
				} else {
					dirname[i] = direc->lfn[i];
				}
			}
			for(i=lfn_length;i<8;i++) {
				dirname[i] = ' ';
			}
		} else {
			for(i=0;i<6;i++) {
				if(direc->lfn[i] > 0x60 && direc->lfn[i] < 0x7A) {
					dirname[i] = direc->lfn[i] - 0x20;
				} else {
					dirname[i] = direc->lfn[i];
				}
			}
			dirname[i++] = '~';
			dirname[i++] = '1';
		}
		dirname[8] = direc->lfn[lfn_length - 3];	//extension
		dirname[9] = direc->lfn[lfn_length - 2];
		dirname[10] = direc->lfn[lfn_length - 1];
	}
	dos_chksum = fat_DosCheckSum(dirname);		//calculate checksum
	next_dir_entry:
	if((a_offset / FAT_SECTOR_SIZE)  >= (fs->sectors_per_cluster)) {
		//get next cluster;
		next_root_sector = fat_GetNextCluster(root_sector, fs);
		if(next_root_sector == 0) {		//create new cluster link
			next_root_sector = fat_AllocClusterLink(root_sector, fs);
			root_sector = next_root_sector;
			//to do : clear current cluster with 0xe5
			for(i=0;i<fs->sectors_per_cluster;i++) {
				ioman_readblock(fs->hw_interface, block, (root_sector + i), FAT_SECTOR_SIZE);
				efat_memset(block, 0, FAT_SECTOR_SIZE);
				ioman_writeblock(fs->hw_interface, block, (root_sector + i), FAT_SECTOR_SIZE);
			}
			a_offset = 0;
			goto entry_creation;
		}
		current_entry_size = 0;		//setiap pindah cluster current_entry_size = 0;
		root_sector = next_root_sector;
		a_offset = 0;
		goto next_dir_entry;
	}
	//prevent redundant access to the same sector
	if(current_sector != (root_sector + (a_offset / FAT_SECTOR_SIZE))) {
		current_sector = (root_sector + (a_offset / FAT_SECTOR_SIZE));
		ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	}
	offset = a_offset % FAT_SECTOR_SIZE;
	efat_memcopy(&direc->attrib, (void *)((uint32)block + offset + DIR_ATTR), 0, sizeof(uchar));
	if(allocated_entry_size == current_entry_size) {		//current_entry_size cukup untuk allocated_entry_size
		a_offset -= current_entry_size * SIZE_OF_DIR_ENTRY;
		goto entry_creation;
	}
	if(*(uchar *)((uint32)block + offset) == 0xe5) {	//cek if entry is empty
		a_offset += 32;
		current_entry_size++;
		goto next_dir_entry;
	} 
	if(*(uchar *)((uint32)block + offset) == 0x00) {
		*(uchar *)((uint32)block + offset) = 0xe5;		//this not and empty entry
		ioman_writeblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
		a_offset += 32;
		current_entry_size++;
		goto next_dir_entry;
	} else {
		a_offset += 32;
		current_entry_size = 0;		//jika tidak ketemu 0xe5 brarti alokasi tidak mencukupi, set current_entry_size = 0;
		goto next_dir_entry;
	}
	entry_creation:
	//prevent redundant access to the same sector
	if(current_sector != root_sector + (a_offset / FAT_SECTOR_SIZE)) {
		current_sector = root_sector + (a_offset / FAT_SECTOR_SIZE);
		ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);			//read new sector
	}
	offset = a_offset % FAT_SECTOR_SIZE;		//first entry
	allocated_name_size -= 13;
	block[offset+0] = ordinal;
	block[offset+1] = direc->lfn[allocated_name_size+0];	//coro bodo ???
	if(block[offset+1] == 0xff) { block[offset+2] = 0xff; } else { block[offset+2] = 0x00; }
	block[offset+3] = direc->lfn[allocated_name_size+1];
	if(block[offset+3] == 0xff) { block[offset+4] = 0xff; } else { block[offset+4] = 0x00; }
	block[offset+5] = direc->lfn[allocated_name_size+2];
	if(block[offset+5] == 0xff) { block[offset+6] = 0xff; } else { block[offset+6] = 0x00; }
	block[offset+7] = direc->lfn[allocated_name_size+3];
	if(block[offset+7] == 0xff) { block[offset+8] = 0xff; } else { block[offset+8] = 0x00; }
	block[offset+9] = direc->lfn[allocated_name_size+4];
	if(block[offset+9] == 0xff) { block[offset+10] = 0xff; } else { block[offset+10] = 0x00; }
	block[offset+11] = 0x0f;
	block[offset+12] = 0;
	block[offset+13] = dos_chksum;
	block[offset+14] = direc->lfn[allocated_name_size+5];
	if(block[offset+14] == 0xff) { block[offset+15] = 0xff; } else { block[offset+15] = 0x00; }
	block[offset+16] = direc->lfn[allocated_name_size+6];
	if(block[offset+16] == 0xff) { block[offset+17] = 0xff; } else { block[offset+17] = 0x00; }
	block[offset+18] = direc->lfn[allocated_name_size+7];
	if(block[offset+18] == 0xff) { block[offset+19] = 0xff; } else { block[offset+19] = 0x00; }
	block[offset+20] = direc->lfn[allocated_name_size+8];
	if(block[offset+20] == 0xff) { block[offset+21] = 0xff; } else { block[offset+21] = 0x00; }
	block[offset+22] = direc->lfn[allocated_name_size+9];
	if(block[offset+22] == 0xff) { block[offset+23] = 0xff; } else { block[offset+23] = 0x00; }
	block[offset+24] = direc->lfn[allocated_name_size+10];
	if(block[offset+24] == 0xff) { block[offset+25] = 0xff; } else { block[offset+25] = 0x00; }
	block[offset+26] = 0;		//nullified char
	block[offset+27] = 0;		//nullified char
	block[offset+28] = direc->lfn[allocated_name_size+11];
	if(block[offset+28] == 0xff) { block[offset+29] = 0xff; } else { block[offset+29] = 0x00; }
	block[offset+30] = direc->lfn[allocated_name_size+12];
	if(block[offset+30] == 0xff) { block[offset+31] = 0xff; } else { block[offset+31] = 0x00; }
	ioman_writeblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);			//write previous sector
	a_offset += 32;
	for(i=1;i<(current_entry_size - 1);i++, a_offset += 32) {		//if there's secondary entry
		if(current_sector != root_sector + (a_offset / FAT_SECTOR_SIZE)) {
			current_sector = root_sector + (a_offset / FAT_SECTOR_SIZE);
			ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);		//read new sector
		}
		offset = a_offset % FAT_SECTOR_SIZE;
		allocated_name_size -= 13;	
		block[offset+0] = i;
		block[offset+1] = direc->lfn[allocated_name_size+0];
		block[offset+2] = 0;
		block[offset+3] = direc->lfn[allocated_name_size+1];
		block[offset+4] = 0;
		block[offset+5] = direc->lfn[allocated_name_size+2];
		block[offset+6] = 0;
		block[offset+7] = direc->lfn[allocated_name_size+3];
		block[offset+8] = 0;
		block[offset+9] = direc->lfn[allocated_name_size+4];
		block[offset+10] = 0;
		block[offset+11] = 0x0f;
		block[offset+12] = 0;
		block[offset+13] = dos_chksum;
		block[offset+14] = direc->lfn[allocated_name_size+5];
		block[offset+15] = 0;
		block[offset+16] = direc->lfn[allocated_name_size+6];
		block[offset+17] = 0;
		block[offset+18] = direc->lfn[allocated_name_size+7];
		block[offset+19] = 0;
		block[offset+20] = direc->lfn[allocated_name_size+8];
		block[offset+21] = 0;
		block[offset+22] = direc->lfn[allocated_name_size+9];
		block[offset+23] = 0;
		block[offset+24] = direc->lfn[allocated_name_size+10];
		block[offset+25] = 0;		//nullified char
		block[offset+26] = 0;		//nullified char
		block[offset+27] = 0;
		block[offset+28] = direc->lfn[allocated_name_size+11];
		block[offset+29] = 0;
		block[offset+30] = direc->lfn[allocated_name_size+12];
		block[offset+31] = 0;
		ioman_writeblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);			//write previous sector
	}
	if(current_sector != root_sector + (a_offset / FAT_SECTOR_SIZE)) {
		current_sector = root_sector + (a_offset / FAT_SECTOR_SIZE);
		ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);		//read new sector
	}
	offset = a_offset % FAT_SECTOR_SIZE;
	direc->offset = offset;
	direc->sector = current_sector;
	direc->cluster = fat_AllocNewCluster(fs);
	direc->attrib = attrib;
	cluster_high = (uint16)(direc->cluster >> 16);
	cluster_low = (uint16)(direc->cluster & 0xffff);
	ioman_readblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	efat_memcopy((void *)((uint32)block + direc->offset), dirname, 0, SIZE_MAX_8_3); 
	*(uchar *)((uint32)block + direc->offset + DIR_ATTR) = direc->attrib;
	efat_memcopy((void *)((uint32)block + direc->offset + DIR_FSTCLUSHI), &cluster_high, 0, sizeof(uint16));
	efat_memcopy((void *)((uint32)block + direc->offset + DIR_FSTCLUSLO), &cluster_low, 0, sizeof(uint16));
	efat_memcopy((void *)((uint32)block + direc->offset + DIR_FILESIZE), &direc->size, 0, sizeof(uint32));
	ioman_writeblock(fs->hw_interface, block, current_sector, FAT_SECTOR_SIZE);
	efat_free(block);
	//printf("entry created\n");
	//return direc;
}

void fat_ClearCluster(uint32 cluster_no, fat_FileSystem * fs) {
	uint16 c;
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	for(c=0;c<fs->sectors_per_cluster;c++) {						//clear directory cluster
		ioman_readblock(fs->hw_interface, block, (cluster_no + c), FAT_SECTOR_SIZE);
		efat_memset(block, 0, FAT_SECTOR_SIZE);
		ioman_writeblock(fs->hw_interface, block, (cluster_no + c), FAT_SECTOR_SIZE);
	}
	efat_free(block);
}

//create "." and ".." entry at the first directory cluster
void fat_CreateDirCluster(uint32 cluster_no, uint32 parent_cluster, fat_FileSystem * fs) {
	uint16 cluster_high;
	uint16 cluster_low;
	uchar dirname[SIZE_MAX_8_3];
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	ioman_readblock(fs->hw_interface, block, fat_GetDataCluster(cluster_no, fs), FAT_SECTOR_SIZE);
	cluster_high = (uint16)(cluster_no >> 16);
	cluster_low = (uint16)(cluster_no & 0xffff);
	strcpy(dirname, ".          ");
	efat_memcopy((void *)((uint32)block + 0), dirname, 0, SIZE_MAX_8_3); 
	*(uchar *)((uint32)block + 0 + DIR_ATTR) = FAT_ATTR_DIR;
	efat_memcopy((void *)((uint32)block + 0 + DIR_FSTCLUSHI), &cluster_high, 0, sizeof(uint16));
	efat_memcopy((void *)((uint32)block + 0 + DIR_FSTCLUSLO), &cluster_low, 0, sizeof(uint16));
	*(uint32 *)((uint32)block + 0 + DIR_FILESIZE) = 0;
	cluster_high = (uint16)(parent_cluster >> 16);
	cluster_low = (uint16)(parent_cluster & 0xffff);
	strcpy(dirname, "..         ");
	efat_memcopy((void *)((uint32)block + SIZE_OF_DIR_ENTRY), dirname, 0, SIZE_MAX_8_3); 
	*(uchar *)((uint32)block + SIZE_OF_DIR_ENTRY + DIR_ATTR) = FAT_ATTR_DIR;
	efat_memcopy((void *)((uint32)block + SIZE_OF_DIR_ENTRY + DIR_FSTCLUSHI), &cluster_high, 0, sizeof(uint16));
	efat_memcopy((void *)((uint32)block + SIZE_OF_DIR_ENTRY + DIR_FSTCLUSLO), &cluster_low, 0, sizeof(uint16));
	*(uint32 *)((uint32)block + SIZE_OF_DIR_ENTRY + DIR_FILESIZE) = 0;
	ioman_writeblock(fs->hw_interface, block, fat_GetDataCluster(cluster_no, fs), FAT_SECTOR_SIZE);
	efat_free(block);
}


void fat_InitPartition32(fat_FileSystem * fs, fat_Partition * p, fat_BootRecord * br, HW_Interface * interface) {
	uint32 data_sectors;
	fs->fat_begin_lba = (uint32)p->lba + (uint32)br->num_of_rsv_sect;
	fs->cluster_begin_lba = (uint32)p->lba + (uint32)br->num_of_rsv_sect + ((uint32)br->num_of_fats * (uint32)br->sec_per_fat);
	fs->sectors_per_cluster = br->sec_per_clus;
	fs->root_dir_first_cluster = br->root;
	data_sectors = (uint32)br->num_of_rsv_sect + ((uint32)br->num_of_fats * (uint32)br->sec_per_fat);
	fs->type = TYPE_FAT32;
	fs->hw_interface = interface;
}

void fat_InitPartition16(fat_FileSystem * fs, fat_Partition * p, fat_BootRecord * br, HW_Interface * iface) {
	fs->fat_begin_lba = (uint32)p->lba + (uint32)br->num_of_rsv_sect;
	fs->cluster_begin_lba = (uint32)p->lba + (uint32)br->num_of_rsv_sect + ((uint32)br->num_of_fats * (uint32)br->sec_per_fat);
	fs->sectors_per_cluster = br->sec_per_clus;
	fs->root_dir_first_cluster = br->root;
	fs->type = TYPE_FAT16;
	fs->hw_interface = iface;
}

//unused function, for efat testing purpose only
uchar DirList(uint16 depth, uint32 i, uint32 cluster_no, fat_FileSystem * fs) {
	uchar filename[25];
	uchar skip;
	uint16 d;
	fat_DirEntry * dr = (fat_DirEntry *)efat_malloc(sizeof(fat_DirEntry));
	efat_memset(filename, 0, 25);
	while((skip = fat_GetDirEntry(i, cluster_no, dr, fs)) !=0 ) {
		i += skip;
		for(d = 0; d<depth ;d++) {
			Uart_Printf("    ");
		}
		Uart_Printf("+ %s\n", dr->lfn); 
		if((dr->attrib & FAT_ATTR_DIR) && i != 0) {
			DirList(depth +1, 2, fat_GetDataCluster(dr->cluster, fs), fs);
		}
	}
	efat_free(dr);
	return skip;
}

//unused function, for efat testing purpose only
uchar DirDebug(uint32 cluster_no, fat_FileSystem * fs) {
	uchar filename[25];
	uint32 i = 0;
	uchar skip;
	fat_DirEntry * dr = (fat_DirEntry *)efat_malloc(sizeof(fat_DirEntry));
	efat_memset(filename, 0, 25);
	while((skip = fat_GetDirEntry(i, cluster_no, dr, fs)) !=0 ) {
		i += skip;
		Uart_Printf("+ %s\n", dr->lfn); 
		if((dr->attrib & FAT_ATTR_DIR) && i != 0) {
			//getch();
			DirList(1, 2, fat_GetDataCluster(dr->cluster, fs), fs);
		}
	}
	efat_free(dr);
	return skip;
}

uint32 fat_GetNextCluster(uint32 root_sector, fat_FileSystem * fs) {
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	uint32 fat_lba_sector;
	uint32 next_root_sector;
	uint32 fat_lba_offset; 
	if(fs->type == TYPE_FAT32) {
		fat_lba_offset = (((root_sector - fs->cluster_begin_lba) / fs->sectors_per_cluster) + 2) * sizeof(uint32);
		fat_lba_sector = (fat_lba_offset / FAT_SECTOR_SIZE) + fs->fat_begin_lba;
		fat_lba_offset = (fat_lba_offset % FAT_SECTOR_SIZE);
		ioman_readblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);
		next_root_sector = *(uint32 *)(block + fat_lba_offset);
		if(next_root_sector == FAT32_EMPTY_CHAIN) {		//have no next cluster
			efat_free(block);
			return 0;
		}
		next_root_sector = next_root_sector - 2;
		next_root_sector = next_root_sector * (fs->sectors_per_cluster);
		next_root_sector = next_root_sector + (fs->cluster_begin_lba);
		efat_free(block);
		return next_root_sector;
	} else if(fs->type == TYPE_FAT16) {
		fat_lba_offset = (((root_sector - fs->cluster_begin_lba) / fs->sectors_per_cluster) + 2) * sizeof(uint16);
		fat_lba_sector = (fat_lba_offset / FAT_SECTOR_SIZE) + fs->fat_begin_lba;
		fat_lba_offset = (fat_lba_offset % FAT_SECTOR_SIZE);
		ioman_readblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);
		next_root_sector = *(uint32 *)(block + fat_lba_offset);
		if(next_root_sector == FAT32_EMPTY_CHAIN) {		//have no next cluster
			efat_free(block);
			return 0;
		}		
		next_root_sector = next_root_sector - 2;
		next_root_sector = next_root_sector * (fs->sectors_per_cluster);
		next_root_sector = next_root_sector + (fs->cluster_begin_lba);
		efat_free(block);
		return next_root_sector;
	}
	return 0;
}

uint32 fat_ReadFATChain(uint32 sector, fat_FileSystem * fs) {
	uint32 fat_lba_sector;
	uint32 fat_lba_offset;
	uint32 next_root_sector;
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	fat_lba_offset = (((sector - fs->cluster_begin_lba) / fs->sectors_per_cluster) + 2) * sizeof(uint32);
	fat_lba_sector = (fat_lba_offset / FAT_SECTOR_SIZE) + fs->fat_begin_lba;
	fat_lba_offset = (fat_lba_offset % FAT_SECTOR_SIZE);
	ioman_readblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);
	next_root_sector = *(uint32 *)(block + fat_lba_offset);
	efat_free(block);
	return next_root_sector;
}

void fat_WriteFATChain(uint32 sector, uint32 value, fat_FileSystem * fs) {
	uint32 fat_lba_sector;
	uint32 fat_lba_offset;
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	fat_lba_offset = (((sector - fs->cluster_begin_lba) / fs->sectors_per_cluster) + 2) * sizeof(uint32);
	fat_lba_sector = (fat_lba_offset / FAT_SECTOR_SIZE) + fs->fat_begin_lba;
	fat_lba_offset = (fat_lba_offset % FAT_SECTOR_SIZE);
	ioman_readblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);
	*(uint32 *)(block + fat_lba_offset) = value;
	ioman_writeblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);
	efat_free(block);
}

//allocate new cluster and link to previous cluster
uint32 fat_AllocClusterLink(uint32 root_sector, fat_FileSystem * fs) {
	uint16 i = 0;
	uint32 next_root_sector;
	uint32 fat_lba_offset;
	uint32 fat_lba_sector;
	uint32 sector_offset;
	uint32 chain;
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	if(fs->type == TYPE_FAT32) {
		fat_lba_offset = (((root_sector - fs->cluster_begin_lba) / fs->sectors_per_cluster) + 2) * sizeof(uint32);
		fat_lba_sector = (fat_lba_offset / FAT_SECTOR_SIZE) + fs->fat_begin_lba;
		sector_offset = (fat_lba_offset % FAT_SECTOR_SIZE);
		ioman_readblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);
		chain = *(uint32 *)(block + sector_offset);
		if(chain != FAT32_EMPTY_CHAIN) {		//have next cluster?
			//root not empty
			efat_free(block);
			return 0;
		}
		fat_lba_sector = fs->fat_begin_lba;		//start search for empty cluster from fat_begin_lba
		while(fat_lba_sector < fs->cluster_begin_lba) {	
			//printf("fat_lba_offset : %d\n", fat_lba_sector);
			ioman_readblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);	
			for(i=sector_offset; i<FAT_SECTOR_SIZE; i+=sizeof(uint32)) {
				chain = *(uint32 *)(block + i);
				if(chain == FAT_EMPTY_CLUSTER) {	//check if chain == 0x00000000(empty)
					next_root_sector = (fat_lba_sector + (i/sizeof(uint32))) - fs->fat_begin_lba;	//calculate offset from fat_begin_lba
					fat_WriteFATChain(root_sector, next_root_sector, fs);
					*(uint32 *)(block + i) = FAT32_EMPTY_CHAIN;		//set to 0x0fffffff
					ioman_writeblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);		//update current sector
					next_root_sector = next_root_sector - 2;
					next_root_sector = next_root_sector * (fs->sectors_per_cluster);
					next_root_sector = next_root_sector + (fs->cluster_begin_lba);		//translate to absolute address
					efat_free(block);
					return next_root_sector;
				}
			}
			sector_offset = 0;		//clear offset
			fat_lba_sector++;
		}	
	} else if(fs->type == TYPE_FAT16) {

	}
	efat_free(block);
	return 0;
}

//allocate new cluster, return chain of first cluster
uint32 fat_AllocNewCluster(fat_FileSystem * fs) {
	uint16 i = 0;
	uint32 next_root_sector;
	uint32 fat_lba_offset;
	uint32 fat_lba_sector;
	uint32 chain;
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	if(fs->type == TYPE_FAT32) {
		fat_lba_sector = fs->fat_begin_lba;		//start search for empty cluster from fat_begin_lba
		while(fat_lba_sector < fs->cluster_begin_lba) {		
			ioman_readblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);	
			for(i = 0; i < FAT_SECTOR_SIZE;i += sizeof(uint32)) {
				chain = *(uint32 *)(block + i);
				if(chain == FAT_EMPTY_CLUSTER) {	//check if chain == 0x00000000(empty)
					next_root_sector = (fat_lba_sector + (i/sizeof(uint32))) - fs->fat_begin_lba;	//calculate offset from fat_begin_lba
					*(uint32 *)(block + i) = FAT32_EMPTY_CHAIN;		//set to 0x0fffffff
					ioman_writeblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);		//update current sector
					//next_root_sector = next_root_sector - 2;
					//next_root_sector = next_root_sector * (fs->sectors_per_cluster);
					//next_root_sector = next_root_sector + (fs->cluster_begin_lba);		//translate to absolute address
					efat_free(block);
					return next_root_sector;
				}
			}
			fat_lba_sector++;
		}	
	} else if(fs->type == TYPE_FAT16) {

	}
	efat_free(block);
	return 0;
}

//delete chain and return next cluster
uint32 fat_UnlinkCluster(uint32 root_sector, fat_FileSystem * fs) {
	uint16 i = 0;
	uint32 next_root_sector;
	uint32 fat_lba_offset;
	uint32 fat_lba_sector;
	uint32 sector_offset;
	uint32 chain;
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	if(fs->type == TYPE_FAT32) {
		fat_lba_offset = (((root_sector - fs->cluster_begin_lba) / fs->sectors_per_cluster) + 2) * sizeof(uint32);
		fat_lba_sector = (fat_lba_offset / FAT_SECTOR_SIZE) + fs->fat_begin_lba;
		fat_lba_offset = (fat_lba_offset % FAT_SECTOR_SIZE);
		ioman_readblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);
		next_root_sector = *(uint32 *)(block + fat_lba_offset);
		//unlink here
		if(next_root_sector == FAT_EMPTY_CLUSTER) {
			efat_free(block);
			return 0;
		}
		*(uint32 *)(block + fat_lba_offset) = 0x0000;	//set to zero zero
		ioman_writeblock(fs->hw_interface, block, fat_lba_sector, FAT_SECTOR_SIZE);
		if(next_root_sector == FAT32_EMPTY_CHAIN) {		//have no next cluster
			efat_free(block);
			return 0;
		}
		next_root_sector = next_root_sector - 2;
		next_root_sector = next_root_sector * (fs->sectors_per_cluster);
		next_root_sector = next_root_sector + (fs->cluster_begin_lba);
		efat_free(block);
		return next_root_sector;	
	} else if(fs->type == TYPE_FAT16) {

	}
	efat_free(block);
	return 0;
}

//partition initialization
fat_Partition * fat_ReadPartition(uchar partition_id, HW_Interface * interface) {
	uint16 i;
	struct fat_Partition * p = (fat_Partition *)efat_malloc(sizeof(struct fat_Partition));
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	ioman_readblock(interface, (uchar *)block, 0, FAT_SECTOR_SIZE);
	efat_memcopy((void *)p, (void *)((uint32)block + FAT_MBR_OFFSET_P1 + (partition_id * FAT_MBR_PARTITION_SIZE)), 0, sizeof(fat_Partition));
	efat_free(block);
	return p;
}

//boot record initialization
fat_BootRecord * fat_GetBootRecord(fat_Partition * p, HW_Interface * interface) {
	struct fat_BootRecord * br = (fat_BootRecord *)efat_malloc(sizeof(struct fat_BootRecord));
	uchar * block = (uchar *)efat_malloc(FAT_SECTOR_SIZE);
	br->signature = 0x0000;		//initialized signature with non fat signature
	if(p->lba != 0) { 
		ioman_readblock(interface, block, p->lba, FAT_SECTOR_SIZE);
		//terpaksa pake metode ini untuk menghindari memory alignment dari kompiler
		efat_memcopy(&br->byte_per_sec, (void *)((uint32)block + FAT_BR32_OFFSET_BPS), 0, sizeof(uint16));
		efat_memcopy(&br->sec_per_clus, (void *)((uint32)block + FAT_BR32_OFFSET_SPC), 0, sizeof(uchar));
		efat_memcopy(&br->num_of_rsv_sect, (void *)((uint32)block + FAT_BR32_OFFSET_NRS), 0, sizeof(uint16));
		efat_memcopy(&br->num_of_fats, (void *)((uint32)block + FAT_BR32_OFFSET_NOF), 0, sizeof(uchar));
		efat_memcopy(&br->total_sector_16, (void *)((uint32)block + FAT_BR32_OFFSET_TS16), 0, sizeof(uint16));
		efat_memcopy(&br->total_sector_32, (void *)((uint32)block + FAT_BR32_OFFSET_TS32), 0, sizeof(uint32));
		efat_memcopy(&br->sec_per_fat, (void *)((uint32)block + FAT_BR32_OFFSET_SPF), 0, sizeof(uint32));
		efat_memcopy(&br->root, (void *)((uint32)block + FAT_BR32_OFFSET_ROOT), 0, sizeof(uint32));
		efat_memcopy(&br->signature, (void *)((uint32)block + FAT_BR32_OFFSET_SGN), 0, sizeof(uint16));
		efat_free(block);
	}
	if(br->signature == FAT_SIGNATURE) return br;
	efat_free(br);
	return NULL;
}

//file system initialization
fat_FileSystem * fat_Init(uchar partition_id, HW_Interface * interface) {
	struct fat_FileSystem * fs = (fat_FileSystem *)efat_malloc(sizeof(struct fat_FileSystem));
	struct fat_Partition * p;
	struct fat_BootRecord * br;
	if(ioman_init(interface)) {
		p = fat_ReadPartition(partition_id, interface);
		//TO DO : cek jenis partisi
		br = fat_GetBootRecord(p, interface);
		if(br != NULL) { 
			fat_InitPartition32(fs, p, br, interface);
			//DirDebug(fs->cluster_begin_lba, fs);
			//printf("fs->fat_begin_lba : %x\n", fs->fat_begin_lba);
			//printf("fs->cluster_begin_lba : %x\n", fs->cluster_begin_lba);
			//freed all allocated variable except fs
			efat_free(br);
			efat_free(p);
			return fs;
		} else {
			//this is not fat file system, freed all allocated variable
			efat_free(br);
			efat_free(p);
			efat_free(fs);
			return 0;
		}
		efat_free(br);
		efat_free(p);
	}
	efat_free(fs);
	return 0;		//interface not initialized
}
