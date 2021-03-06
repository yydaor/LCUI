﻿/* ***************************************************************************
 * LCUI.h -- Records with common data type definitions, macro definitions and
 * function declarations
 *
 * Copyright (C) 2012-2013 by
 * Liu Chao
 *
 * This file is part of the LCUI project, and may only be used, modified, and
 * distributed under the terms of the GPLv2.
 *
 * (GPLv2 is abbreviation of GNU General Public License Version 2)
 *
 * By continuing to use, modify, or distribute this file you indicate that you
 * have read the license and understand and accept it fully.
 *
 * The LCUI project is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GPL v2 for more details.
 *
 * You should have received a copy of the GPLv2 along with this file. It is
 * usually in the LICENSE.TXT file, If not, see <http://www.gnu.org/licenses/>.
 * ****************************************************************************/

/* ****************************************************************************
 * LCUI.h -- 记录着常用的数据类型定义，宏定义，以及函数声明
 *
 * 版权所有 (C) 2012-2013 归属于
 * 刘超
 *
 * 这个文件是LCUI项目的一部分，并且只可以根据GPLv2许可协议来使用、更改和发布。
 *
 * (GPLv2 是 GNU通用公共许可证第二版 的英文缩写)
 *
 * 继续使用、修改或发布本文件，表明您已经阅读并完全理解和接受这个许可协议。
 *
 * LCUI 项目是基于使用目的而加以散布的，但不负任何担保责任，甚至没有适销性或特
 * 定用途的隐含担保，详情请参照GPLv2许可协议。
 *
 * 您应已收到附随于本文件的GPLv2许可协议的副本，它通常在LICENSE.TXT文件中，如果
 * 没有，请查看：<http://www.gnu.org/licenses/>.
 * ****************************************************************************/

/* 为了解决结构体被重复定义，用宏进行头文件保护(其它地方出现类似的内容，将不再注释) */
#ifndef __LCUI_H__  /* 如果没有定义 __LCUI_H__ 宏 */
#define __LCUI_H__  /* 定义 __LCUI_H__ 宏 */

#define LCUI_VERSION "0.13.0"

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* 打开文件时的错误 */
#define FILE_ERROR_OPEN_ERROR		-1
#define FILE_ERROR_SHORT_FILE		-2
#define FILE_ERROR_BIG_FILE		-3
#define FILE_ERROR_UNKNOWN_FORMAT	-4

/* 图像类型 */
#define TYPE_PNG	1
#define TYPE_JPG	2
#define TYPE_BMP	3

/* 状态 */
#define ACTIVE	1
#define KILLED	-1
#define REMOVE	-1
#define INSIDE	1

/***************** 一些输入输出设备 *********************/
#define FB_DEV	"/dev/fb0"		/* 图形输出设备 */
#define TS_DEV	"/dev/jz_ts"		/* 触屏输入设备 */
#define MS_DEV	"/dev/input/mice"	/* 鼠标设备 */
/*****************************************************/

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* 触屏校准后的文件 */
#define LCUI_CALIBFILE "/mnt/Data/LC-SOFT/pointercal"

/* 鼠标事件 */
#define MOUSE_EVENT_MOVE	1
#define MOUSE_EVENT_CLICK	2

#define LABEL_TEXT_MAX_SIZE	2048

/* 定义图形的混合方式 */
#define GRAPH_MIX_FLAG_OVERLAY	 1<<7
#define GRAPH_MIX_FLAG_REPLACE	 1<<8

/************ 任务的添加模式 ***************/
#define ADD_MODE_ADD_NEW	0 /* 新增 */
#define ADD_MODE_NOT_REPEAT	1 /* 不能重复 */
#define ADD_MODE_REPLACE	2 /* 覆盖 */

#define AND_ARG_F	1<<3	/* 第一个参数 */
#define AND_ARG_S 	1<<4	/* 第二个参数 */
/*****************************************/

/****************** 图像的处理方式 *****************/
#define FILL_MODE_ZOOM		LAYOUT_ZOOM	/* 缩放 */
#define FILL_MODE_STRETCH  	LAYOUT_STRETCH	/* 拉伸 */
#define FILL_MODE_CENTER	LAYOUT_CENTER	/* 居中 */
#define FILL_MODE_TILE		LAYOUT_TILE	/* 平铺 */
#define FILL_MODE_NONE		LAYOUT_NONE	/* 无 */
#define FILL_MODE_NORMAL	LAYOUT_NORMAL
/*************************************************/

/****************** 图像的处理方式 *****************/
/* 缩放，缩放比例随着PictureBox部件的尺寸的改变而改变 */
#define SIZE_MODE_ZOOM		LAYOUT_ZOOM

/* 固定缩放，用于照片查看器，PictureBox部件的尺寸的改变不影响缩放比列 */
#define SIZE_MODE_BLOCK_ZOOM	5

#define SIZE_MODE_STRETCH	LAYOUT_STRETCH	/* 拉伸 */
#define SIZE_MODE_CENTER	LAYOUT_CENTER	/* 居中 */
#define SIZE_MODE_TILE		LAYOUT_TILE	/* 平铺 */
#define SIZE_MODE_NONE		LAYOUT_NONE	/* 无 */
#define SIZE_MODE_NORMAL	LAYOUT_NORMAL
/*************************************************/


/*********** 图像与文本的几种相对位置 *******************/
#define Overlay		10   /* 文字覆盖在图片上面 */
#define ImageAboveText	11   /* 图片在上，文字在下 */
#define TextAboveImage	12   /* 图片在下，文字在上 */
#define ImageBeforeText	13   /* 图片在左，文字在右 */
#define TextBeforeImage	14   /* 图片在右，文字在左 */
/***************************************************/

LCUI_BEGIN_HEADER

typedef unsigned char LCUI_BOOL;

/****************** 图像的布局 *****************/
typedef enum LAYOUT_TYPE_
{
	LAYOUT_NONE	= 0,	  /* 无 */
	LAYOUT_NORMAL	= 0,
	LAYOUT_ZOOM	= 1,	  /* 缩放 */
	LAYOUT_STRETCH	= 1<<1,	  /* 拉伸 */
	LAYOUT_CENTER	= 1<<2,	  /* 居中 */
	LAYOUT_TILE	= 1<<3	  /* 平铺 */
}LAYOUT_TYPE;
/**********************************************/

typedef unsigned long int LCUI_ID;

typedef void (*CallBackFunc)(void*,void*);

/* 先使用typedef为结构体创建同义字，之后再定义结构体 */
typedef struct	LCUI_Widget_		LCUI_Widget;
typedef struct	LCUI_Key_		LCUI_Key;

typedef unsigned char uchar_t;
typedef unsigned int uint_t;

/********** 按键信息 ************/
struct LCUI_Key_
{
	int code;
	int state;
};
/******************************/

/*--------- RGB配色数据 ---------*/
typedef struct LCUI_RGB_ {
	uchar_t red;
	uchar_t green;
	uchar_t blue;
} LCUI_RGB;
/*----------- END -------------*/

/*--------- RGBA配色数据 --------*/
typedef struct LCUI_RGBA_ {
	uchar_t red;
	uchar_t green;
	uchar_t blue;
	uchar_t alpha;
} LCUI_RGBA;
/*----------- END -------------*/

/*------- 二维坐标 --------*/
typedef struct LCUI_Pos_ {
	int x, y;
} LCUI_Pos;
/*--------- END ----------*/

/*------- 尺寸 --------*/
typedef struct LCUI_Size_ {
	int w, h;
} LCUI_Size;
/*------- END --------*/

/*------------- 像素点信息 ------------*/
typedef struct Pixel_ {
	LCUI_Pos pos;	/* 位置 */
	LCUI_RGB rgb;	/* RGBA值 */
} Pixel;
/*--------------- END ---------------*/

/*---------------- 字符串 ----------------*/
typedef struct LCUI_String_ {
	char   *string; /* 字符串内容 */
	uint_t length;	/* 长度 */
} LCUI_String;
/*----------------- END -----------------*/

/*------------------- 区域数据 ---------------------*/
typedef struct LCUI_Rect_ {
	int x,y;
	int width,height;
	double center_x,center_y; /* 中心点的坐标 */
} LCUI_Rect;
/*--------------------- END ----------------------*/

/*---------------- 字体位图数据 ------------------*/
typedef struct LCUI_FontBMP_ {
	int top;		/* 与顶边框的距离 */
	int left;		/* 与左边框的距离 */
	int width;		/* 位图宽度 */
	int rows;		/* 位图行数 */
	int pitch;
	uchar_t *buffer;	/* 字体位图数据 */
	short num_grays;
	char pixel_mode;
	LCUI_Pos advance;	/* XY轴的跨距 */
} LCUI_FontBMP;
/*------------------- END ---------------------*/

/*------------------------ 宽字符位图及相关数据 -----------------------*/
typedef struct LCUI_WChar_ {
	wchar_t		char_code;	/* 字符码 */
	LCUI_FontBMP	*bitmap;	/* 字符的位图数据 */
	LCUI_RGB	color;		/* 该文字的配色 */
	LCUI_BOOL	update;		/* 标明这个字符是否需要刷新 */
	int		color_type;	/* 颜色类型(DEFAULT / CUSTOM) */
} LCUI_WChar;
/*-----------------------------END ----------------------------------*/

/*---------- 宽字符串 ----------*/
typedef struct LCUI_WString_ {
	wchar_t *string;
	uint_t length;
} LCUI_WString;
/*------------ END ------------*/

LCUI_END_HEADER

#include LC_THREAD_H
#include LC_QUEUE_H

LCUI_BEGIN_HEADER

/*---------------------------- 图形数据 -------------------------------*/
typedef struct LCUI_Graph_ LCUI_Graph;
struct LCUI_Graph_ {
	int	type;		/* 图片类型 */
	int	bit_depth;	/* 位深 */

	LCUI_Mutex mutex;	/* 锁，用于数据保护 */

	LCUI_BOOL quote;	/* 指示是否引用其它图层中的图形 */
	LCUI_Graph *src;	/* 所引用的对象 */
	LCUI_Pos pos;		/* 在引用另一个图层中的图形时，会保存区域的起点位置 */
	int width, height;	/* 尺寸 */

	uchar_t	alpha;	/* 全局透明度，表示整张图片的透明度，默认为255 */
	uchar_t** rgba;	/* 像素数据缓冲区 */
	size_t mem_size; /* 像素数据缓冲区大小 */

	LCUI_BOOL have_alpha;	/* 标志，指定是否需要透明度，分配内存时会根据它分配 */
	LCUI_BOOL is_opaque;	/* 标志，指定该图形是否为不透明 */
	LCUI_BOOL not_visible;	/* 标志，指定该图形是否不可见，也就是全透明 */
};
/*------------------------------ END ---------------------------------*/

/*---------------- 用于表示像素或百分比 -----------------*/
typedef struct PX_P_t_ {
	LCUI_BOOL which_one;	/* 指定用哪个类型的变量 */
	int px;			/* 数值，单位为像素 */
	double scale;		/* 比例 */
} IntOrFloat_t;
/*---------------------- END -------------------------*/

/*----------------- 用于表示字体大小 --------------------*/
typedef struct PX_PT_t_ {
	LCUI_BOOL which_one;	/* 指定用哪个类型的变量 */
	int px;			/* pixel, 字体大小（像素） */
	int pt;			/* point，字体大小（点数） */
} PX_PT_t;
/*---------------------- END -------------------------*/

/*------------------- 内边距和外边距 --------------------*/
typedef struct common_box_ {
	int top, bottom, left, right;
} LCUI_Margin, LCUI_Padding;
/*---------------------- END -------------------------*/

/*----------------- 自动尺寸调整模式 --------------------*/
typedef enum AUTOSIZE_MODE_ {
	AUTOSIZE_MODE_GROW_AND_SHRINK,	/* 增大和缩小 */
	AUTOSIZE_MODE_GROW_ONLY		/* 只增大 */
}
AUTOSIZE_MODE;
/*---------------------- END --------------------------*/

/*---------------- 鼠标游标数据 -------------------*/
typedef struct LCUI_Cursor_ {
	LCUI_Pos	current_pos;	/* 当前的坐标 */
	LCUI_Pos	new_pos;	/* 将要更新的坐标 */
	int		visible;	/* 是否可见 */
	LCUI_Graph	graph;		/* 游标的图形 */
} LCUI_Cursor;
/*------------------- END -----------------------*/

/*----------------- 对齐方式 -------------------*/
typedef enum ALIGN_TYPE_ {
	ALIGN_NONE,		/* 无 */
	ALIGN_TOP_LEFT,	  	/* 向左上角对齐 */
	ALIGN_TOP_CENTER,	/* 向上中间对齐 */
	ALIGN_TOP_RIGHT,	/* 向右上角对齐 */
	ALIGN_MIDDLE_LEFT,	/* 向中央偏左对齐 */
	ALIGN_MIDDLE_CENTER,	/* 向正中央对齐 */
	ALIGN_MIDDLE_RIGHT,	/* 向中央偏由对齐 */
	ALIGN_BOTTOM_LEFT,	/* 向底部偏左对齐 */
	ALIGN_BOTTOM_CENTER,	/* 向底部居中对齐 */
	ALIGN_BOTTOM_RIGHT	/* 向底部偏右对齐 */
} ALIGN_TYPE;
/*---------------------------------------------*/

LCUI_END_HEADER

#define MAX_APP_IDLE_TIME	50
#define MAX_LCUI_IDLE_TIME	50

#define nobuff_printf(format, ...) \
	{ \
		printf(format, ##__VA_ARGS__); \
		fflush(stdout); \
	}

#include LC_MISC_H
#include LC_GRAPHLAYER_H
#include LC_KERNEL_MAIN_H
#include LC_KERNEL_DEV_H
#include LC_KERNEL_TIMER_H
#include LC_KERNEL_TASK_H
#include LC_KERNEL_EVENT_H

/* 如果是使用C++语言，那么就包含LCUI的C++版头文件 */
#ifdef __cplusplus
#include LC_LCUI_HPP
#endif

#endif /* __LCUI_H__ */


