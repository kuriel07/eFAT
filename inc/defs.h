/***************************************************************/
/*eFAT : elvish FAT Library                                    */
/*Filename : defs.h                                            */
/*Copyright (C) 2011 Agus Purwanto                             */
/*http://www.my.opera.com/kuriel                               */
/***************************************************************/
#ifndef _DEFS_DEFINED
#define _DMA_DEBUG 		0
#if _DMA_DEBUG
#define	HEAP_CALC	 	1	 /* Should be made 0 to turn OFF debugging */
#endif

#ifndef NULL	//cek apakah NULL sudah pernah didefine sebelummnya
#define NULL						0
#endif

#define TRUE 	1   
#define FALSE 	0
#define OK		1
#define FAIL	0

#define ESC_KEY	('q')	// 0x1b
//primitive data type definition
typedef int int32;
typedef long eint32;
typedef unsigned int uint32;
typedef unsigned long euint32;
typedef unsigned long ulong;
typedef short int16;
typedef unsigned short uint16;
typedef unsigned short euint16;
typedef unsigned short uint;
typedef unsigned char uchar;
typedef char int8;
typedef unsigned char uint8;
typedef const unsigned char uint8_t;	//taruh di code program
typedef const unsigned short uint16_t;	//taruh di code program
typedef const unsigned int uint32_t;
typedef char eint8;
typedef signed char esint8;
typedef unsigned char euint8;
typedef short eint16;
typedef signed short esint16;
typedef unsigned short euint16; 
typedef long eint32; 
typedef signed long esint32;
typedef unsigned long euint32; 
typedef float          	fp32;                    // 单精度浮点数(32位长度)
typedef double         	fp64;                    // 双精度浮点数(64位长度)
typedef int 			mode_t;


//custom data type definition
struct Rect {
	int16 x;
	int16 y;
	uint16 width;
	uint16 height;
};
typedef struct Rect Rect;

struct Point {
	int16 x;
	int16 y;
};
typedef struct Point Point;

struct Size {
	uint16 width;
	uint16 height;
};
typedef struct Size Size;

#define _LARGEFILE64_SOURCE 0
#define _FILE_OFFSET_BITS	0
#define __GNUC__			0
#define __GNUC_MINOR__		0

//Use Midgard MM for memory allocation to prevent collision
//#define malloc				m_alloc
//#define free				m_free
#define _DEFS_DEFINED
#endif
