/* ***************************************************************************
 * textbox.c -- LCUI's TextBox widget
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
 * textbox.c -- LCUI 的文本框部件
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

//#define DEBUG

#include <LCUI_Build.h>
#include LC_LCUI_H
#include LC_DRAW_H
#include LC_WIDGET_H
#include LC_GRAPH_H
#include LC_FONT_H
#include LC_TEXTBOX_H
#include LC_LABEL_H
#include LC_SCROLLBAR_H
#include LC_INPUT_H
#include LC_INPUT_METHOD_H
#include LC_ERROR_H 

typedef struct _LCUI_TextBox
{
	LCUI_Widget *text;		/* 文本显示层 */
	LCUI_Widget *cursor;		/* 光标 */
	LCUI_Widget *scrollbar[2];	/* 两个滚动条 */
	int block_size;			/* 块大小 */
	LCUI_Queue text_block_buff;	/* 文本块缓冲区 */
	LCUI_BOOL show_placeholder;	/* 表示占位符是否已经显示 */
	LCUI_WString placeholder;	/* 文本框的占位符 */
	LCUI_WString allow_input_char;	/* 允许输入的字符 */
	wchar_t password_char_bak;	/* 屏蔽符的副本 */
	LCUI_TextStyle placeholder_style;	/* 占位符的文本样式 */
	LCUI_TextStyle textstyle_bak;		/* 文本框内文本样式的副本 */
}
LCUI_TextBox;

typedef struct _LCUI_TextBlock
{
	int pos_type;	/* 指定该文本块要添加至哪个位置 */
	wchar_t *text;	/* 指向文本所在内存空间的指针 */
}
LCUI_TextBlock;

/*----------------------------- Private ------------------------------*/
static LCUI_Widget *active_textbox = NULL; 
static int __timer_id = -1;

static void
_putin_textbox_cursor( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	active_textbox = widget;
	/* 设定输入法的目标 */
	LCUIIME_SetTarget( widget );
	Widget_Draw( widget );
}

static void
_putout_textbox_cursor( LCUI_Widget *widget, LCUI_WidgetEvent *unused )
{
	LCUI_TextBox *tb;
	
	if( widget == active_textbox ) {
		active_textbox = NULL;
	}
	tb = Widget_GetPrivData( widget );
	Widget_Hide( tb->cursor );
	Widget_Draw( widget );
}

static void 
hide_textbox_cursor( LCUI_Widget *widget  )
{
	LCUI_TextBox *tb;
	if( widget == NULL ) {
		return;
	}
	tb = (LCUI_TextBox*)Widget_GetPrivData( widget );
	if( tb == NULL ) {
		active_textbox = NULL;
		return;
	}
	Widget_Hide( tb->cursor );
}

static void 
show_textbox_cursor( LCUI_Widget *widget )
{
	LCUI_TextBox *tb;
	if( widget == NULL ) {
		return;
	}
	tb = (LCUI_TextBox*)Widget_GetPrivData( widget );
	if( tb == NULL ) {
		active_textbox = NULL;
		return;
	}
	Widget_Show( tb->cursor );
}

static LCUI_TextLayer *
TextBox_GetTextLayer( LCUI_Widget *widget )
/* 获取文本框部件内的文本图层指针 */
{
	LCUI_Widget *label;
	label = TextBox_GetLabel( widget );
	return Label_GetTextLayer( label );
}

static void 
blink_cursor( void )
/* 闪烁文本框中的光标 */
{
	static int cur_state = 0;
	if(cur_state == 0) {
		show_textbox_cursor( active_textbox );
		cur_state = 1;
	} else {
		hide_textbox_cursor( active_textbox );
		cur_state = 0;
	}
}

static void 
TextBox_TextLayer_Click( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	static LCUI_Pos pos;
	static LCUI_TextBox *tb;
	static LCUI_TextLayer *layer;
	
	/* 保存当前已获得焦点的部件 */
	active_textbox = widget;
	layer = TextBox_GetTextLayer( widget );
	tb = Widget_GetPrivData( active_textbox );
	if( tb->show_placeholder ) {
		pos.x = pos.y = 0;
	} else {
		/* 全局坐标转换成相对坐标 */
		pos = Widget_ToRelPos( tb->text, event->drag.cursor_pos );
	}
	//printf("pos: %d,%d\n", pos.x, pos.y);
	/* 根据像素坐标，设定文本光标的位置 */
	TextLayer_Cursor_SetPixelPos( layer, pos );
	/* 获取光标的当前位置 */
	pos = TextLayer_Cursor_GetPos( layer );
	/* 主要是调用该函数更新当前文本浏览区域，以使光标处于显示区域内 */
	TextBox_Cursor_Move( widget, pos );
}

/* 处理按键事件 */
static void
TextBox_ProcessKey( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	static int cols, rows;
	static LCUI_Pos cur_pos;
	static LCUI_TextLayer *layer;
	static LCUI_TextBox *textbox;
	
	//_DEBUG_MSG("you input: %d\n", event->key.key_code);
	layer = TextBox_GetTextLayer( widget );
	textbox = Widget_GetPrivData( widget );
	cur_pos = TextLayer_Cursor_GetPos( layer );
	cols = TextLayer_GetRowLen( layer, cur_pos.y );
	rows = TextLayer_GetRows( layer ); 
	switch( event->key.key_code ) {
	    case LCUIKEY_HOMEPAGE: //home键移动光标至行首
		cur_pos.x = 0;
		goto mv_cur_pos;
		
	    case LCUIKEY_END: //end键移动光标至行尾
		cur_pos.x = cols;
		goto mv_cur_pos;
		
	    case LCUIKEY_LEFT:
		if( cur_pos.x > 0 ) {
			cur_pos.x--;
		} else if( cur_pos.y > 0 ) {
			cur_pos.y--;
			cur_pos.x = TextLayer_GetRowLen( layer, cur_pos.y );
		}
		goto mv_cur_pos;
		
	    case LCUIKEY_RIGHT:
		if( cur_pos.x < cols ) {
			cur_pos.x++;
		} else if( cur_pos.y < rows-1 ) {
			cur_pos.y++;
			cur_pos.x = 0;
		}
		goto mv_cur_pos;
		
	    case LCUIKEY_UP:
		if( cur_pos.y > 0 ) {
			cur_pos.y--;
		}
		goto mv_cur_pos;
		
	    case LCUIKEY_DOWN:
		if( cur_pos.y < rows-1 ) {
			cur_pos.y++;
		}
mv_cur_pos:;
		/* 移动光标位置 */
		if( textbox->show_placeholder ) {
			cur_pos.x = cur_pos.y = 0;
		}
		TextBox_Cursor_Move( widget,cur_pos );
		break;
		
	    case LCUIKEY_BACKSPACE: //删除光标左边的字符
		TextBox_Text_Backspace( widget, 1 );
		break;
		
	    case LCUIKEY_DELETE:
		//删除光标右边的字符
		
		break;
		
	    default:break;
	}
}

/* 处理输入法对文本框输入的内容 */
static void
TextBox_Input( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	unsigned int i;
	wchar_t *ptr, *tmp_ptr, *ptr_last;
	LCUI_TextBox *textbox;

	ptr = event->input.text;
	ptr_last = ptr + MAX_INPUT_TEXT_LEN;
	textbox = (LCUI_TextBox*)Widget_GetPrivData( widget );
	if( textbox->allow_input_char.length > 0 ) {
		for( ; ptr<ptr_last && *ptr!='\0'; ++ptr ) {
			/* 判断当前字符是否为限制范围内的字符 */
			for( i=0; i<textbox->allow_input_char.length; ++i ) {
				if( textbox->allow_input_char.string[i] == *ptr ) {
					break;
				}
			}
			/* 如果已提前结束循环，则表明当前字符是允许的 */
			if( i < textbox->allow_input_char.length ) {
				continue;
			}
			/* 否则不是允许的字符，需移除该字符 */
			for( tmp_ptr=ptr; tmp_ptr<ptr_last
			 && *tmp_ptr!='\0'; ++tmp_ptr ) {
				*tmp_ptr = *(tmp_ptr+1);
			}
		}
	}
	TextBox_Text_AddW( widget, event->input.text );
}

static void 
TextBox_HoriScroll_TextLayer( ScrollBar_Data data, void *arg )
/* 水平滚动文本框内的文本图层 */
{
	LCUI_Pos pos;
	LCUI_Widget *widget;
	
	widget = (LCUI_Widget *)arg;
	//_DEBUG_MSG("data: size: %d / %d, num: %d / %d\n", 
	//data.current_size, data.max_size, data.current_num, data.max_num);
	
	pos = TextBox_ViewArea_GetPos( arg );
	pos.x = 0 - data.current_num;
	pos.y = 0 - pos.y;
	//_DEBUG_MSG("offset.x: %d\n", pos.x);
	TextBox_TextLayer_SetOffset( widget, pos );
}

static void 
TextBox_TextLayer_VertScroll( ScrollBar_Data data, void *arg )
/* 垂直滚动文本框内的文本图层 */
{
	LCUI_Pos pos;
	LCUI_Widget *widget;
	
	widget = (LCUI_Widget *)arg;
	//_DEBUG_MSG("data: size: %d / %d, num: %d / %d\n", 
	//data.current_size, data.max_size, data.current_num, data.max_num);
	pos = TextBox_ViewArea_GetPos( arg );
	pos.y = 0 - data.current_num;
	pos.x = 0 - pos.x;
	//_DEBUG_MSG("offset.y: %d\n", pos.y);
	TextBox_TextLayer_SetOffset( widget, pos );
}

static void
destroy_textblock( void *arg )
{
	LCUI_TextBlock *ptr;
	ptr = (LCUI_TextBlock *)arg;
	free( ptr->text );
}

static void 
TextBox_Init( LCUI_Widget *widget )
/* 初始化文本框相关数据 */
{
	LCUI_TextBox *textbox;
	
	widget->valid_state = WIDGET_STATE_ACTIVE | WIDGET_STATE_OVERLAY;
	widget->valid_state |= (WIDGET_STATE_NORMAL | WIDGET_STATE_DISABLE);
	textbox = WidgetPrivData_New(widget, sizeof(LCUI_TextBox));
	textbox->text = Widget_New( "label" );
	textbox->cursor = Widget_New( NULL );
	textbox->scrollbar[0] = Widget_New( "scrollbar" );
	textbox->scrollbar[1] = Widget_New( "scrollbar" );
	/* 不可获得焦点 */
	textbox->text->focus = FALSE;
	textbox->cursor->focus = FALSE;
	textbox->scrollbar[0]->focus = FALSE;
	textbox->scrollbar[1]->focus = FALSE;
	textbox->block_size = 256;
	textbox->show_placeholder = FALSE;
	LCUIWString_Init( &textbox->placeholder );
	LCUIWString_Init( &textbox->allow_input_char );
	TextStyle_Init( &textbox->placeholder_style );
	TextStyle_FontColor( &textbox->placeholder_style, RGB(100,100,100) );
	Label_AutoSize( textbox->text, FALSE, 0 );
	Widget_SetSize( textbox->text, "100%", "100%" );
	
	/* 添加至相应的容器 */
	Widget_Container_Add( textbox->text, textbox->cursor ); 
	Widget_Container_Add( widget, textbox->text ); 
	Widget_Container_Add( widget, textbox->scrollbar[0] );
	Widget_Container_Add( widget, textbox->scrollbar[1] );
	/* 设置滚动条的尺寸 */
	Widget_SetSize( textbox->scrollbar[0], "10px", NULL );
	Widget_SetSize( textbox->scrollbar[1], NULL, "10px" );
	Widget_SetAlign( textbox->scrollbar[0], ALIGN_TOP_RIGHT, Pos(0,0) );
	Widget_SetAlign( textbox->scrollbar[1], ALIGN_BOTTOM_LEFT, Pos(0,0) );
	/* 滚动条设为横向 */
	ScrollBar_SetDirection( textbox->scrollbar[1], 1 );
	/* 将回调函数与滚动条连接 */
	ScrollBar_Connect( textbox->scrollbar[0], TextBox_TextLayer_VertScroll, widget );
	ScrollBar_Connect( textbox->scrollbar[1], TextBox_HoriScroll_TextLayer, widget );
	Widget_Show( textbox->text );
	
	Queue_Init( &textbox->text_block_buff, sizeof(LCUI_TextBlock), destroy_textblock );
	
	TextLayer_UsingStyleTags( Label_GetTextLayer(textbox->text), FALSE );
	Widget_SetPadding( widget, Padding(2,2,2,2) );
	Widget_SetBackgroundColor( textbox->cursor, RGB(0,0,0) );
	Widget_SetBackgroundTransparent( textbox->cursor, FALSE );
	Widget_SetBackgroundTransparent( widget, FALSE );
	
	Widget_Resize( textbox->cursor, Size(1, 14) );
	/* 设置可点击区域的alpha值要满足的条件 */
	Set_Widget_ClickableAlpha( textbox->cursor, 0, 1 );
	Set_Widget_ClickableAlpha( textbox->text, 0, 1 );
	
	/* 设定定时器，每1秒闪烁一次 */
	if( __timer_id == -1 ) {
		__timer_id = LCUITimer_Set( 500, blink_cursor, TRUE );
	}
	Widget_Event_Connect( widget, EVENT_DRAG, TextBox_TextLayer_Click );
	/* 关联 FOCUS_OUT 和 FOCUS_IN 事件 */
	Widget_Event_Connect( widget, EVENT_FOCUSOUT, _putout_textbox_cursor );
	Widget_Event_Connect( widget, EVENT_FOCUSIN, _putin_textbox_cursor );
	/* 关联按键输入事件 */
	Widget_Event_Connect( widget, EVENT_KEYBOARD, TextBox_ProcessKey );
	Widget_Event_Connect( widget, EVENT_INPUT, TextBox_Input );
	/* 默认不启用多行文本模式 */
	TextBox_Multiline( widget, FALSE );
}

static void 
Destroy_TextBox( LCUI_Widget *widget )
/* 销毁文本框占用的资源 */
{
	
}

static LCUI_Widget *
TextBox_GetScrollbar( LCUI_Widget *widget, int which )
{
	LCUI_TextBox *tb;
	tb = Widget_GetPrivData( widget );
	if( which == 0 ) {
		return tb->scrollbar[0];
	}
	return tb->scrollbar[1];
}

static void 
TextBox_ScrollBar_UpdateSize( LCUI_Widget *widget )
/* 更新滚动条的长度 */
{
	int tmp;
	char size_str[15];
	LCUI_Size area_size, layer_size;
	LCUI_Widget *scrollbar[2], *label;
	LCUI_TextLayer *layer;
	
	label = TextBox_GetLabel( widget );
	/* 获取文本图层 */
	layer = TextBox_GetTextLayer( widget );
	
	/* 获取文本图层和文本框区域的尺寸 */
	layer_size = TextLayer_GetSize( layer );
	area_size = Widget_GetContainerSize( widget );
	/* 获取纵向和横向滚动条 */
	scrollbar[0] = TextBox_GetScrollbar( widget, 0 );
	scrollbar[1] = TextBox_GetScrollbar( widget, 1 );
	
	/* 如果文本图层高度超过显示区域 */
	if( layer->enable_multiline && area_size.h > 0 
	  && layer_size.h > area_size.h ) {
		tmp = area_size.w - Widget_GetWidth( scrollbar[0] );
#ifdef LCUI_BUILD_IN_LINUX
		snprintf( size_str, sizeof(size_str)-1, "%dpx", tmp );
#else
		sprintf_s( size_str, sizeof(size_str)-1, "%dpx", tmp );
#endif
		Widget_SetSize( label, size_str, NULL );
		
		/* 修改滚动条中记录的最大值和当前值，让滚动条在更新后有相应的长度 */
		ScrollBar_SetMaxSize( scrollbar[0], layer_size.h );
		ScrollBar_SetCurrentSize( scrollbar[0], area_size.h );
		Widget_Show( scrollbar[0] );
		/* 如果横向滚动条可见 */
		if( scrollbar[1]->visible ) {
			tmp = area_size.h - Widget_GetHeight( scrollbar[1] );
#ifdef LCUI_BUILD_IN_LINUX
			snprintf( size_str, sizeof(size_str)-1, "%dpx", tmp );
#else
			sprintf_s( size_str, sizeof(size_str)-1, "%dpx", tmp );
#endif
			Widget_SetSize( scrollbar[0], NULL, size_str );
		} else {
			Widget_SetSize( scrollbar[0], NULL, "100%" );
		}
	} else {
		/* 不需要显示滚动条 */
		Widget_Hide( scrollbar[0] );
		Widget_SetSize( label, "100%", NULL );
	}
	/* 和上面的处理基本一样，这个是处理横向滚动条 */
	if( layer->enable_multiline &&
	 area_size.w > 0 && layer_size.w > area_size.w ) {
		tmp = area_size.h - Widget_GetHeight( scrollbar[1] );
#ifdef LCUI_BUILD_IN_LINUX
		snprintf( size_str, sizeof(size_str)-1, "%dpx", tmp );
#else
		sprintf_s( size_str, sizeof(size_str)-1, "%dpx", tmp );
#endif
		Widget_SetSize( label, NULL, size_str );
		
		ScrollBar_SetMaxSize( scrollbar[1], layer_size.w );
		ScrollBar_SetCurrentSize( scrollbar[1], area_size.w );
		Widget_Show( scrollbar[1] );
		
		if( scrollbar[0]->visible ) {
			tmp = area_size.w - Widget_GetWidth( scrollbar[0] );
#ifdef LCUI_BUILD_IN_LINUX
			snprintf( size_str, sizeof(size_str)-1, "%dpx", tmp );
#else
			sprintf_s( size_str, sizeof(size_str)-1, "%dpx", tmp );
#endif
			Widget_SetSize( scrollbar[1], size_str, NULL );
		} else {
			Widget_SetSize( scrollbar[1], "100%", NULL );
		}
	} else {
		Widget_SetSize( label, NULL, "100%" );
		Widget_Hide( scrollbar[1] );
	}
}

static int
TextBox_ScrollBar_UpdatePos( LCUI_Widget *widget )
/* 更新滚动条的位置 */
{
	static ScrollBar_Data scrollbar_data;
	static LCUI_Pos area_pos;
	static LCUI_Size layer_size, area_size;
	static LCUI_Widget *scrollbar[2];
	static LCUI_TextLayer *layer;
	
	area_size = Widget_GetContainerSize( widget ); 
	//_DEBUG_MSG("area_size: (%d,%d)\n", area_size.w, area_size.h);
	scrollbar[0] = TextBox_GetScrollbar( widget, 0 );
	scrollbar[1] = TextBox_GetScrollbar( widget, 1 ); 
	if( scrollbar[0]->visible ) {
		area_size.w -= Widget_GetWidth( scrollbar[0] );
	}
	if( scrollbar[1]->visible ) {
		area_size.h -= Widget_GetHeight( scrollbar[1] );
	}
	
	if( area_size.h <= 0 || area_size.w <= 0 ) {
		return -1;
	}
	
	layer = TextBox_GetTextLayer( widget );
	if( !layer->enable_multiline ) {
		return 1;
	}
	layer_size = TextLayer_GetSize( layer );
	//_DEBUG_MSG("layer_size: (%d,%d)\n", layer_size.w, layer_size.h);
	if( layer_size.h <= 0 || layer_size.w <= 0 ) {
		return -2;
	}
	area_pos = TextBox_ViewArea_GetPos( widget );
	//_DEBUG_MSG("area_pos: (%d,%d)\n", area_pos.x, area_pos.y);
	ScrollBar_SetMaxNum( scrollbar[0], layer_size.h - area_size.h );
	
	if( area_pos.y + area_size.h > layer_size.h ) {
		area_pos.y = layer_size.h - area_size.h;
	}
	if( area_pos.x + area_size.w > layer_size.w ) {
		area_pos.x = layer_size.w - area_size.w;
	}
	if( area_pos.y < 0 ) {
		area_pos.y = 0;
	}
	if( area_pos.x < 0 ) {
		area_pos.x = 0;
	}
	ScrollBar_SetCurrentValue( scrollbar[0], area_pos.y ); 
	scrollbar_data = ScrollBar_GetData( scrollbar[0] );
	TextBox_TextLayer_VertScroll( scrollbar_data, widget );
	
	ScrollBar_SetMaxNum( scrollbar[1], layer_size.w - area_size.w );
	ScrollBar_SetCurrentValue( scrollbar[1], area_pos.x );
	scrollbar_data = ScrollBar_GetData( scrollbar[1] );
	TextBox_HoriScroll_TextLayer( scrollbar_data, widget );
	return 0;
}

/* 在文本末尾追加文本 */
static void 
__TextBox_Text_AppendW( LCUI_Widget *widget, wchar_t *new_text )
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	TextLayer_Text_AppendW( layer, new_text );
}

/* 在光标处添加文本 */
static void 
__TextBox_Text_Add( LCUI_Widget *widget, wchar_t *new_text )
{
	LCUI_Pos cur_pos;
	LCUI_TextLayer *layer;
	
	layer = TextBox_GetTextLayer( widget );
	TextLayer_Text_AddW( layer, new_text );
	cur_pos = TextLayer_Cursor_GetPos( layer );
	TextBox_Cursor_Move( widget, cur_pos );
}

/* 更新当前文本框的样式 */
static void
TextBox_ExecUpdateStyle( LCUI_Widget *widget )
{
	LCUI_Border border;
	
	Widget_SetBackgroundColor( widget, RGB(255,255,255) );
	Widget_SetBackgroundTransparent( widget, FALSE );
	/* 如果该部件已经获得全局焦点 */
	if( Widget_GetGlobalFocus( widget ) ) {
		border = Border( 1, BORDER_STYLE_SOLID, RGB(55,123,203) );
		Widget_SetBorder( widget, border );
		return;
	}
	Border_Radius( &border, 0 );
	/* 根据不同的状态，设定不同的边框样式 */
	switch( widget->state ) {
	case WIDGET_STATE_NORMAL:
		border.left_width = 1;
		border.left_style = BORDER_STYLE_SOLID;
		border.left_color = RGB(204,204,204);
		border.top_width = 1;
		border.top_style = BORDER_STYLE_SOLID;
		border.top_color = RGB(204,204,204);
		border.right_width = 1;
		border.right_style = BORDER_STYLE_SOLID;
		border.right_color = RGB(221,221,221);
		border.bottom_width = 1;
		border.bottom_style = BORDER_STYLE_SOLID;
		border.bottom_color = RGB(221,221,221);
		break;
	case WIDGET_STATE_OVERLAY :
		border.left_width = 1;
		border.left_style = BORDER_STYLE_SOLID;
		border.left_color = RGB(171,171,171);
		border.top_width = 1;
		border.top_style = BORDER_STYLE_SOLID;
		border.top_color = RGB(171,171,171);
		border.right_width = 1;
		border.right_style = BORDER_STYLE_SOLID;
		border.right_color = RGB(204,204,204);
		border.bottom_width = 1;
		border.bottom_style = BORDER_STYLE_SOLID;
		border.bottom_color = RGB(204,204,204);
		break;
	case WIDGET_STATE_ACTIVE :
		border = Border( 1, BORDER_STYLE_SOLID, RGB(55,123,203) );
		break;
	case WIDGET_STATE_DISABLE :
		break;
		default : break;
	}
	Widget_SetBorder( widget, border );
}

/* 更新文本框的样式以及文本图层相关的数据 */
static void
TextBox_ExecUpdate( LCUI_Widget *widget )
{
	LCUI_TextLayer *layer;
	LCUI_TextBlock *text_ptr;
	LCUI_TextBox *textbox;
	
	textbox = Widget_GetPrivData( widget );
	/* 如果缓冲区内有文本块 */
	if( Queue_GetTotal( &textbox->text_block_buff ) > 0 ) {
		/* 获取文本块 */
		text_ptr = Queue_Get( &textbox->text_block_buff, 0 );
		if( text_ptr ) {
			//_DEBUG_MSG("text block: %p, text: %p\n", 
			//	text_ptr, text_ptr->text);
			switch( text_ptr->pos_type ) {
			    case AT_TEXT_LAST:
				/* 将此文本块追加至文本末尾 */
				__TextBox_Text_AppendW( widget, text_ptr->text );
				break;
			    case AT_CURSOR_POS:
				/* 将此文本块插入至光标当前处 */
				__TextBox_Text_Add( widget, text_ptr->text );
				break;
			    default: break;
			}
		}
		/* 删除该文本块 */
		Queue_Delete( &textbox->text_block_buff, 0 );
		/* 更新滚动条的位置 */
		TextBox_ScrollBar_UpdatePos( widget );
		/* 标记下次继续更新 */
		__Widget_Update( widget );
	}
	/* 如果文本框内没有文本，且还未显示占位符，则设置占位符并显示 */
	if( TextBox_Text_GetTotalLength( widget ) == 0
	 && !textbox->show_placeholder ) {
		layer = TextBox_GetTextLayer(widget);
		/* 备份文本框内的文本样式以及屏蔽符 */
		textbox->textstyle_bak = layer->default_data;
		textbox->password_char_bak = layer->password_char.char_code;
		/* 占位符不能被屏蔽，所以设置屏蔽符为0 */
		TextLayer_Text_SetPasswordChar( layer, 0 );
		/* 文本框内显示占位符 */
		TextLayer_TextW( layer, textbox->placeholder.string );
		/* 占位符的字体大小取缺省大小 */
		textbox->placeholder_style.pixel_size = layer->default_data.pixel_size;
		/* 设置占位符的样式 */
		TextLayer_Text_SetDefaultStyle( layer, textbox->placeholder_style );
		textbox->show_placeholder = TRUE;
	}
	TextBox_ExecUpdateStyle( widget ); /* 更新文本框的样式 */
	Widget_ExecUpdate( TextBox_GetLabel( widget ) ); /* 更新文本图层的内容 */
	TextBox_ScrollBar_UpdateSize( widget ); /* 更新滚动条的长度 */
	TextBox_Cursor_Update( widget ); /* 更新文本框内的光标 */
}

LCUI_API void
Process_TextBox_Drag(LCUI_Widget *widget, LCUI_WidgetDragEvent *event)
/* 处理鼠标对文本框的拖动事件 */
{
	
}

LCUI_API void 
Process_TextBox_Clicked(LCUI_Widget *widget, LCUI_EventSlot *event)
/* 处理鼠标对文本框的点击事件 */
{
	
}
/*--------------------------- End Private ----------------------------*/


/*----------------------------- Public -------------------------------*/
/* 剪切板 */
//static LCUI_String clip_board;
LCUI_API void
Register_TextBox(void)
/* 注册文本框部件 */
{
	WidgetType_Add ( "text_box" );
	WidgetFunc_Add ( "text_box", TextBox_Init, FUNC_TYPE_INIT );
	WidgetFunc_Add ( "text_box", TextBox_ExecUpdate, FUNC_TYPE_UPDATE );
	WidgetFunc_Add ( "text_box", Destroy_TextBox, FUNC_TYPE_DESTROY );
}

LCUI_API LCUI_Pos
TextBox_ViewArea_GetPos( LCUI_Widget *widget )
/* 获取文本显示区域的位置 */
{
	LCUI_Pos pos;
	LCUI_TextLayer *layer;
	
	layer = TextBox_GetTextLayer( widget );
	pos = layer->offset_pos;
	pos.x = -pos.x;
	pos.y = -pos.y;
	return pos;
}

LCUI_API int
TextBox_ViewArea_Update( LCUI_Widget *widget )
/* 更新文本框的文本显示区域 */
{
	static int cursor_h;
	static ScrollBar_Data scrollbar_data;
	static LCUI_Pos cursor_pos, area_pos;
	static LCUI_Size layer_size, area_size;
	static LCUI_Widget *scrollbar[2];
	static LCUI_TextLayer *layer;
	//_DEBUG_MSG("enter\n");
	/* 获取显示区域的尺寸 */
	area_size = Widget_GetContainerSize( widget );
	/* 获取滚动条 */
	scrollbar[0] = TextBox_GetScrollbar( widget, 0 );
	scrollbar[1] = TextBox_GetScrollbar( widget, 1 );
	/* 如果滚动条可见，那么区域尺寸就需要减去滚动条占用的尺寸 */
	if( scrollbar[0]->visible ) {
		area_size.w -= Widget_GetWidth( scrollbar[0] );
	}
	if( scrollbar[1]->visible ) {
		area_size.h -= Widget_GetHeight( scrollbar[1] );
	}
	
	if( area_size.h <= 0 || area_size.w <= 0 ) {
		return -1;
	}
	
	layer = TextBox_GetTextLayer( widget );
	layer_size = TextLayer_GetSize( layer );
	if( layer_size.h <= 0 || layer_size.w <= 0 ) {
		return -2;
	}
	/* 获取显示区域的位置 */
	area_pos = TextBox_ViewArea_GetPos( widget );
	/* 获取光标的坐标 */
	cursor_pos = TextLayer_Cursor_GetFixedPixelPos( layer );
	/* 获取当前行的最大高度作为光标的高度 */
	cursor_h = TextLayer_CurRow_GetMaxHeight( layer );
	
	/* 设定滚动条的数据中的最大值 */
	ScrollBar_SetMaxNum( scrollbar[0], layer_size.h - area_size.h );
	
	//_DEBUG_MSG("cursor_pos: %d,%d\n", cursor_pos.x, cursor_pos.y);
	//_DEBUG_MSG("area_rect: %d,%d,%d,%d\n", 
	//	area_pos.x, area_pos.y,area_size.w, area_size.h );
	//_DEBUG_MSG("layer_size: %d,%d\n", layer_size.w, layer_size.h);
	
	/* 检测光标Y轴坐标是否超出显示区域的范围 */
	if( cursor_pos.y < area_pos.y ) {
		area_pos.y = cursor_pos.y;
	}
	if(cursor_pos.y + cursor_h > area_pos.y + area_size.h ) {
		area_pos.y = cursor_pos.y + cursor_h - area_size.h;
	}
	/* 如果显示区域在Y轴上超过文本图层的范围 */
	if( area_pos.y + area_size.h > layer_size.h ) {
		area_pos.y = layer_size.h - area_size.h;
	}
	if( area_pos.y < 0 ) {
		area_pos.y = 0;
	}
	/* 设定滚动条的数据中的当前值 */
	ScrollBar_SetCurrentValue( scrollbar[0], area_pos.y );
	/* 获取滚动条的数据，供滚动文本层利用 */
	scrollbar_data = ScrollBar_GetData( scrollbar[0] );
	
	//_DEBUG_MSG("scrollbar_data: size: %d / %d, num: %d / %d\n", 
	//scrollbar_data.current_size, scrollbar_data.max_size, 
	//scrollbar_data.current_num, scrollbar_data.max_num);
	/* 根据数据，滚动文本图层至响应的位置，也就是移动文本显示区域 */
	TextBox_TextLayer_VertScroll( scrollbar_data, widget );
	
	/* 设定滚动条的数据中的最大值 */
	ScrollBar_SetMaxNum( scrollbar[1], layer_size.w - area_size.w );
	
	if( cursor_pos.x < area_pos.x ) {
		area_pos.x = cursor_pos.x;
	}
	if(cursor_pos.x +5 > area_pos.x + area_size.w ) {
		area_pos.x = cursor_pos.x+5 - area_size.w;
	}
	if( area_pos.x + area_size.w > layer_size.w ) {
		area_pos.x = layer_size.w - area_size.w;
	}
	if( area_pos.x < 0 ) {
		area_pos.x = 0;
	}
	/* 设定滚动条的数据中的当前值 */
	ScrollBar_SetCurrentValue( scrollbar[1], area_pos.x );
	/* 获取滚动条的数据，供滚动文本层利用 */
	scrollbar_data = ScrollBar_GetData( scrollbar[1] );
	
	//_DEBUG_MSG("scrollbar_data: size: %d / %d, num: %d / %d\n", 
	//scrollbar_data.current_size, scrollbar_data.max_size, 
	//scrollbar_data.current_num, scrollbar_data.max_num);
	TextBox_HoriScroll_TextLayer( scrollbar_data, widget );
	//_DEBUG_MSG("quit\n");
	return 0;
}

/* 获取文本框部件内的label部件指针 */
LCUI_API LCUI_Widget*
TextBox_GetLabel( LCUI_Widget *widget )
{
	LCUI_TextBox *textbox;
	textbox = Widget_GetPrivData( widget );
	return textbox->text;
}

/* 获取文本框部件内的光标 */
LCUI_API LCUI_Widget*
TextBox_GetCursor( LCUI_Widget *widget )
{
	LCUI_TextBox *tb;
	tb = Widget_GetPrivData( widget );
	return tb->cursor;
}

/* 获取文本框内文本 */
LCUI_API size_t
TextBox_GetText( LCUI_Widget *widget, wchar_t *buff, size_t max_len )
{
	LCUI_TextLayer *layer;
	if( widget == NULL ) {
		return 0;
	}
	layer = TextBox_GetTextLayer( widget );
	if( layer == NULL ) {
		return 0;
	}
	return TextLayer_GetText( layer, buff, max_len );
}

/* 获取文本框内的文本总长度 */
LCUI_API int
TextBox_Text_GetTotalLength( LCUI_Widget *widget )
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	return TextLayer_Text_GetTotalLength( layer );
}

/* 将文本添加至缓冲区内 */
static int
TextBox_TextBuff_Add( LCUI_Widget *widget, wchar_t *text, int pos_type )
{
	LCUI_TextBox *textbox;
	int i, j, len, size;
	wchar_t *text_buff;
	LCUI_TextBlock text_block;
	
	textbox = Widget_GetPrivData( widget );
	len = wcslen( text );
	//_DEBUG_MSG("len = %d\n", len);
	switch( pos_type ) {
		case AT_TEXT_LAST: 
		case AT_CURSOR_POS:
		text_block.pos_type = pos_type;
		break;
		default: return -1;
	}
	for(i=0; i<len; ++i) {
		if( len-i > textbox->block_size ) {
			size = textbox->block_size;
		} else {
			size = len-i +1;
		}
		text_buff = malloc( sizeof(wchar_t) * size );
		if( !text_buff ) {
			return -2;
		}
		for( j=0; j<len; ++j,++i ) {
			/* 如果大于当前块大小 */
			if( j >= textbox->block_size ) {
					break;
			}
			text_buff[j] = text[i];
			//_DEBUG_MSG("char: %d, count: %d\n", new_text[i], count);
		}
		--i;
		text_buff[j] = 0;
		text_block.text = text_buff;
		/* 添加文本块至缓冲区 */
		Queue_Add( &textbox->text_block_buff, &text_block );
	}
	return 0;
}

static void
TextBox_Text_Clear( LCUI_Widget *widget )
{
	LCUI_TextLayer *layer;
	
	layer = TextBox_GetTextLayer( widget );
	TextLayer_Text_Clear( layer );
}


/* 设定文本框显示的文本 */
LCUI_API void
TextBox_TextW( LCUI_Widget *widget, wchar_t *unicode_text )
{
	LCUI_TextBox *tb;
	LCUI_TextLayer *layer;
	
	TextBox_Text_Clear( widget );	/* 清空显示的文本 */
	tb = Widget_GetPrivData( widget );
	/* 把文本分割成块，加入至缓冲队列，让文本框分段显示 */
	TextBox_TextBuff_Add( widget, unicode_text, AT_TEXT_LAST );
	if( tb->show_placeholder ) {
		layer = TextBox_GetTextLayer( widget );
		tb->show_placeholder = FALSE;
		/* 恢复文本框的文本样式以及屏蔽符 */
		TextLayer_Text_SetPasswordChar( layer, tb->password_char_bak );
		TextLayer_Text_SetDefaultStyle( layer, tb->textstyle_bak );
	}
	Widget_Update( widget );
}

LCUI_API void
TextBox_Text( LCUI_Widget *widget, char *utf8_text )
{
	wchar_t *unicode_text;
	LCUICharset_UTF8ToUnicode( utf8_text, &unicode_text );
	TextBox_TextW( widget, unicode_text );
	free( unicode_text );
}

LCUI_API void
TextBox_TextLayer_SetOffset( LCUI_Widget *widget, LCUI_Pos offset_pos )
/* 为文本框内的文本图层设置偏移 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	if( TextLayer_SetOffset( layer, offset_pos ) == 0 ) {
		Widget_Update( widget );
	}
	/* 需要更新光标的显示位置 */
	TextBox_Cursor_Update( widget );
}

/* 在光标处添加文本 */
LCUI_API void
TextBox_Text_AddW( LCUI_Widget *widget, wchar_t *unicode_text )
{
	LCUI_TextBox *tb;
	LCUI_TextLayer *layer;
	
	tb = Widget_GetPrivData( widget );
	/* 把文本分割成若干块，加入至缓冲队列，让文本框分段处理显示 */ 
	TextBox_TextBuff_Add( widget, unicode_text, AT_CURSOR_POS );
	if( tb->show_placeholder ) {
		layer = TextBox_GetTextLayer( widget );
		TextLayer_Text_Clear( layer );
		TextLayer_Text_SetPasswordChar( layer, tb->password_char_bak );
		TextLayer_Text_SetDefaultStyle( layer, tb->textstyle_bak );
		tb->show_placeholder = FALSE;
	}
	Widget_Update( widget );
}

LCUI_API void
TextBox_Text_AddA( LCUI_Widget *widget, char *ascii_text )
{
	wchar_t *unicode_text;
	LCUICharset_ASCIIToUnicode( ascii_text, &unicode_text );
	TextBox_Text_AddW( widget, unicode_text );
	free( unicode_text );
}

LCUI_API void
TextBox_Text_Add( LCUI_Widget *widget, char *utf8_text )
{
	wchar_t *unicode_text;
	LCUICharset_UTF8ToUnicode( utf8_text, &unicode_text );
	TextBox_Text_AddW( widget, unicode_text );
	free( unicode_text );
}


/* 在文本末尾追加文本 */
LCUI_API void
TextBox_Text_AppendW( LCUI_Widget *widget, wchar_t *unicode_text )
{
	LCUI_TextBox *tb;
	LCUI_TextLayer *layer;
	
	tb = Widget_GetPrivData( widget );
	TextBox_TextBuff_Add( widget, unicode_text, AT_TEXT_LAST );
	if( tb->show_placeholder ) {
		layer = TextBox_GetTextLayer( widget );
		TextLayer_Text_Clear( layer );
		TextLayer_Text_SetPasswordChar( layer, tb->password_char_bak );
		TextLayer_Text_SetDefaultStyle( layer, tb->textstyle_bak );
		tb->show_placeholder = FALSE;
	}
	Widget_Update( widget );
}

/* 在文本末尾追加文本 */
LCUI_API void
TextBox_Text_Append( LCUI_Widget *widget, char *utf8_text )
{
	wchar_t *unicode_text;
	LCUICharset_UTF8ToUnicode( utf8_text, &unicode_text );
	TextBox_Text_AppendW( widget, unicode_text );
	free( unicode_text );
}

LCUI_API int
TextBox_Text_Paste(LCUI_Widget *widget)
/* 将剪切板的内容粘贴至文本框 */
{
	return 0;
}

LCUI_API int
TextBox_Text_Backspace(LCUI_Widget *widget, int n)
/* 删除光标左边处n个字符 */
{
	LCUI_Pos cur_pos;
	LCUI_TextLayer *layer;
	
	layer = TextBox_GetTextLayer( widget );
	TextLayer_Text_Backspace( layer, n );
	cur_pos = TextLayer_Cursor_GetPos( layer );
	TextBox_Cursor_Move( widget, cur_pos );
	Widget_Update( widget );
	return 0;
}

LCUI_API int
TextBox_Text_Delete(LCUI_Widget *widget, int n)
/* 删除光标右边处n个字符 */
{
	return 0;
}

LCUI_API LCUI_Pos
TextBox_Cursor_Update( LCUI_Widget *widget )
/* 更新文本框的光标，返回该光标的像素坐标 */
{
	LCUI_Pos pixel_pos;
	LCUI_Widget *cursor;
	LCUI_TextLayer *layer;
	LCUI_TextBox *textbox;
	LCUI_Size size;
	
	textbox = Widget_GetPrivData( widget );
	layer = TextBox_GetTextLayer( widget );
	/* 如果显示了占位符，那么就复位光标的位置 */
	if( textbox->show_placeholder ) {
		TextLayer_Cursor_SetPos( layer, Pos(0,0) );
	}
	cursor = TextBox_GetCursor( widget );
	size.w = 1;
	size.h = TextLayer_CurRow_GetMaxHeight( layer );
	pixel_pos = TextLayer_Cursor_GetPixelPos( layer );
	Widget_Move( cursor, pixel_pos );
	Widget_Resize( cursor, size );
	/* 若当前文本框处于焦点状态，则让光标在更新时显示 */
	if( active_textbox == widget ) {
		Widget_Show( cursor );
	}
	return pixel_pos;
}

LCUI_API LCUI_Pos
TextBox_Cursor_Move( LCUI_Widget *widget, LCUI_Pos new_pos )
/* 移动文本框内的光标 */
{
	LCUI_Pos pixel_pos;
	LCUI_Widget *cursor;
	LCUI_TextLayer *layer;
	LCUI_Size size;
	
	layer = TextBox_GetTextLayer( widget );
	cursor = TextBox_GetCursor( widget );
	size.w = 1;
	size.h = TextLayer_CurRow_GetMaxHeight( layer );
	pixel_pos = TextLayer_Cursor_SetPos( layer, new_pos );
	Widget_Move( cursor, pixel_pos );
	Widget_Resize( cursor, size );
	if( active_textbox == widget ) {
		Widget_Show( cursor );
	}
	/* 更新文本显示区域 */
	TextBox_ViewArea_Update( widget );
	return pixel_pos;
}

LCUI_API int
TextBox_GetSelectedText( LCUI_Widget *widget, char *out_text )
/* 获取文本框内被选中的文本 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	return TextLayer_GetSelectedText( layer, out_text );
}

LCUI_API int
TextBox_CopySelectedText(LCUI_Widget *widget)
/* 复制文本框内被选中的文本 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	return TextLayer_CopySelectedText( layer );
}

LCUI_API int
TextBox_CutSelectedText(LCUI_Widget *widget)
/* 剪切文本框内被选中的文本 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	return TextLayer_CutSelectedText( layer );
}

LCUI_API void
TextBox_Using_StyleTags(LCUI_Widget *widget, LCUI_BOOL flag)
/* 指定文本框是否处理控制符 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	TextLayer_UsingStyleTags( layer, flag );
}

LCUI_API void
TextBox_Multiline( LCUI_Widget *widget, LCUI_BOOL flag )
/* 指定文本框是否启用多行文本显示 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	TextLayer_Multiline( layer, flag );
}

LCUI_API void
TextBox_Text_SetDefaultStyle( LCUI_Widget *widget, LCUI_TextStyle style )
/* 设定文本框内的文本的缺省样式 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	TextLayer_Text_SetDefaultStyle( layer, style );
}


LCUI_API void
TextBox_Text_SetMaxLength( LCUI_Widget *widget, int max )
/* 设置文本框中能够输入的最大字符数 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	TextLayer_Text_Set_MaxLength( layer, max );
}

LCUI_API void
TextBox_Text_SetPasswordChar( LCUI_Widget *widget, wchar_t ch )
/* 为文本框设置屏蔽字符 */
{
	LCUI_TextLayer *layer;
	layer = TextBox_GetTextLayer( widget );
	TextLayer_Text_SetPasswordChar( layer, ch );
	TextLayer_Refresh( layer );
	Widget_Update( widget );
}

LCUI_API void
TextBox_Text_SetPlaceHolderW(	LCUI_Widget *widget, 
				LCUI_TextStyle *style,
				const wchar_t *str )
{
	LCUI_TextBox *textbox;
	
	textbox = Widget_GetPrivData( widget );
	_LCUIWString_Copy( &textbox->placeholder, str );
	if( style ) {
		textbox->placeholder_style = *style;
	}
}
/* 为文本框设置占位符 */
LCUI_API void
TextBox_Text_SetPlaceHolder(	LCUI_Widget *widget, 
				LCUI_TextStyle *style,
				const char *str )
{
	wchar_t *unicode_str;
	LCUICharset_UTF8ToUnicode( str, &unicode_str );
	TextBox_Text_SetPlaceHolderW( widget, style, unicode_str );
	free( unicode_str );
}

/* 
 * 功能：限制能对文本框输入的字符
 * 说明：str中存放限制范围内的字符，设置后，文本框只接受输入该字符串内的字符
 * */
LCUI_API void
TextBox_LimitInput( LCUI_Widget *widget, wchar_t *str )
{
	LCUI_TextBox *textbox;
	
	textbox = Widget_GetPrivData( widget );
	_LCUIWString_Copy( &textbox->allow_input_char, str );
}
/*--------------------------- End Public -----------------------------*/
