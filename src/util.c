/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : util.c                                            */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#include "util.h"
#include "defs.h"

//standard memcopy for efat
uint16 memcopy(void *pdest, void *psrc, uint16 offset, uint16 size)
{
	uint16 wrote = 0;
	while(size>offset){
		*((eint8*)pdest+size-1)=*((eint8*)psrc+size-1);
		size--;
		wrote++;
	}
	return wrote;
}

uchar memcompare(char *pdest, char *psrc, uint16 offset, uint16 size)
{
	while(size>offset){
		if(*((eint8*)pdest+size-1)!=*((eint8*)psrc+size-1)) return FALSE;
		size--;
	}
	return TRUE;
}

void util_memset(uchar * dst, uchar value, uint16 size) 
{
	uint16 wrote = 0;
	while(size > 0){
		*((eint8*)dst+size-1)=value;
		size--;
		wrote++;
	}
	//return wrote;
}

//copy string from src to dst until "slash" or "NULL" char found
uint16 util_DirNameCopy(uchar * dst, uchar * src) {
	uchar c = 0x01;
	uint16 i = 0;
	c = *src;
	if(c == 0) {
		return 0;
	}
	i++;
	while(c != '\\' && c != '\x0') {
		c = *(src++);
		if(c != '\\' && c != '\x0') {
			*(dst++) = c;
			i++;
		}
	}
	*dst++ = 0;
	return i;
}

//compare string between src and dst with insensitive case
uchar util_DirNameCompare(uchar * dst, uchar * src) {
	uchar c1 = 0x01;
	uchar c2 = 0x01;
	while(c1 != '\x0') {
		c1 = *(src++);
		if(c1 > 0x40 && c1 <= 0x5A) {		//disable case sensitive, convert to lower case
			c1 += 0x20;
		}
		c2 = *(dst++);
		if(c2 > 0x40 && c2 <= 0x5A) {		//disable case sensitive, convert to lower case
			c2 += 0x20;
		}
		if(c1 != c2) return 0;
	}
	return 1;
}
