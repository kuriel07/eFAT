/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : config.h                                          */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#include "util.h"
#include "string.h"

#ifndef __EFAT_CONFIG_H
//change these defined name into target compiler specific functions name for better optimization
#define efat_malloc		malloc		//void * efat_malloc(size_t size);
#define efat_free		free		//void efat_free(void * pointer);
#define efat_memcopy	memcopy		//void efat_memcopy(void *pdest, void *psrc, uint32 offset, size_t size);
#define efat_memset		util_memset		//void efat_memset(uchar * dst, uchar value, size_t size);
#define efat_strlen 	strlen
#define efat_strcpy		strcpy

#define __EFAT_CONFIG_H
#endif
