/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : util.h                                            */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "defs.h"
#ifndef __EFAT_UTIL_H

uint16 memcopy(void *pdest, void *psrc, uint16 offset, uint16 size);
uchar memcompare(char *pdest, char *psrc, uint16 offset, uint16 size);
void util_memset(uchar * dst, uchar value, uint16 size);
uint16 util_DirNameCopy(uchar * dst, uchar * src);
uchar util_DirNameCompare(uchar * dst, uchar * src);

#define __EFAT_UTIL_H
#endif
