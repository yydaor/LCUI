/* ***************************************************************************
 * widget_event.c -- LCUI widget event module
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
 * LCUI_Work.c -- LCUI部件的事件模块
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
#include LC_ERROR_H
#include LC_INPUT_H
#include LC_INPUT_METHOD_H
#include LC_WIDGET_H
#include LC_CURSOR_H

/***************************** Func ***********************************/
LCUI_API void
NULL_Func()
/* 功能：空函数，不做任何操作 */
{
	return;
}

LCUI_API void
FuncQueue_Init(LCUI_Queue *queue)
/* 功能：初始化函数指针队列 */
{
	Queue_Init(queue, sizeof(LCUI_Func), NULL);
}

/****************** 处理部件拖动/点击事件的相关代码 ************************/
static LCUI_Widget *click_widget = NULL;
static LCUI_Pos __offset_pos = {0, 0};  /* 点击部件时保存的偏移坐标 */

static int
WidgetQueue_Get_Pos(LCUI_Queue *queue, LCUI_Widget *widget)
/* 功能：从部件队列中获取指定部件的排列位置 */
{
	LCUI_Widget *temp;
	int i, result = -1, total;

	total = Queue_GetTotal(queue);
	for(i = 0; i < total; ++i) {
		temp = Queue_Get(queue, i);
		if(temp == widget) {
			result = i;
			break;
		}
	}
	return result;
}
/* 将回调函数与部件的指定事件进行关联 */
LCUI_API int
Widget_Event_Connect(	LCUI_Widget *widget, WidgetEventType event_id,
			void (*func)(LCUI_Widget*, LCUI_WidgetEvent*) )
{
	LCUI_Func func_data;
	if( !widget ) {
		return -1;
	}
	if( !Get_FuncData(&func_data, (CallBackFunc)func, widget, NULL) ) {
		return -2;
	}
	//_DEBUG_MSG("widget: %p, event id: %d, callback func: %p\n",
	//		widget, event_id, func_data->func);
	EventSlots_Add( &widget->event, event_id, &func_data );
	return 0;
}

/* 处理与部件事件关联的回调函数 */
int Widget_DispatchEvent( LCUI_Widget *widget, LCUI_WidgetEvent *event )
{
	int i,n;
	char str[256];
	LCUI_EventSlot *slot;
	LCUI_Task *task;
	LCUI_WidgetEvent *p_buff;

	if( !widget ) {
		return -1;
	}

	slot = EventSlots_Find( &widget->event, event->type );
	if( !slot ) {
		return -2;
	}
	n = Queue_GetTotal( &slot->func_data );
	for(i=0; i<n; ++i) {
		task = Queue_Get( &slot->func_data, i );
		p_buff = malloc( sizeof(LCUI_WidgetEvent) );
		if( !p_buff ) {
			sprintf( str,"%s()", __FUNCTION__ );
			perror( str );
			abort();
		}
		*p_buff = *event;
		task->arg[1] = p_buff;
		task->destroy_arg[1] = TRUE;
		AppTasks_Add( task );
	}
	return 0;
}

static LCUI_BOOL
Widget_Have_Event(LCUI_Widget *widget, int event_id)
/* 检测部件是否关联了指定事件 */
{
	LCUI_EventSlot *event;

	if( !widget ) {
		return FALSE;
	}
	event = EventSlots_Find( &widget->event, event_id );
	if( !event ) {
		return FALSE;
	}
	return TRUE;
}

static LCUI_Widget *
Get_ResponseEvent_Widget(LCUI_Widget *widget, int event_id)
/*
 * 功能：查找能响应事件的部件
 * 说明：此函数用于检查部件以及它的上级所有父部件，第一个有响应指定事件的部件，它的指针
 * 将会作为本函数的返回值
 * */
{
	if( !widget ) {
		return NULL;
	}
	if( Widget_Have_Event(widget, event_id) ) {
		return widget;
	}
	if( !widget->parent ) {
		return NULL;
	} else {
		return Get_ResponseEvent_Widget(widget->parent, event_id);
	}
}

static void
_DragEvent_Start( LCUI_Widget *widget, LCUI_MouseButtonEvent *event )
{
	LCUI_WidgetEvent widget_event;
	widget_event.type = EVENT_DRAG;
	widget_event.drag.cursor_pos.x = event->x;
	widget_event.drag.cursor_pos.y = event->y;
	/* 用全局坐标减去部件的全局坐标，得到偏移坐标 */
	__offset_pos = Widget_GetGlobalPos( widget );
	__offset_pos = Pos_Sub( widget_event.drag.cursor_pos, __offset_pos );
	/* 得出部件的新全局坐标 */
	widget_event.drag.new_pos = Pos_Sub( widget_event.drag.cursor_pos, __offset_pos );
	widget_event.drag.first_click = TRUE;
	widget_event.drag.end_click = FALSE;
	/* 处理部件的拖动事件 */
	Widget_DispatchEvent( widget, &widget_event );
}

static void
_DragEvent_Do( LCUI_Widget *widget, LCUI_MouseMotionEvent *event )
{
	LCUI_WidgetEvent widget_event;
	widget_event.type = EVENT_DRAG;
	widget_event.drag.cursor_pos.x = event->x;
	widget_event.drag.cursor_pos.y = event->y;
	widget_event.drag.new_pos = Pos_Sub( widget_event.drag.cursor_pos, __offset_pos );
	widget_event.drag.first_click = TRUE;
	widget_event.drag.end_click = FALSE;
	Widget_DispatchEvent( widget, &widget_event );
}

static void
_DragEvent_End( LCUI_Widget *widget, LCUI_MouseButtonEvent *event )
{
	LCUI_WidgetEvent widget_event;
	widget_event.type = EVENT_DRAG;
	widget_event.drag.cursor_pos.x = event->x;
	widget_event.drag.cursor_pos.y = event->y;
	widget_event.drag.new_pos = Pos_Sub( widget_event.drag.cursor_pos, __offset_pos );
	widget_event.drag.first_click = FALSE;
	widget_event.drag.end_click = TRUE;
	Widget_DispatchEvent( widget, &widget_event );
}

typedef struct {
	LCUI_BOOL need_delete;
	LCUI_Widget *widget;
} widget_item;

static LCUI_Queue widget_list;

static void
widget_list_reset( void )
{
	int i, n;
	widget_item *item;
	n = Queue_GetTotal( &widget_list );
	for(i=0; i<n; ++i) {
		item = Queue_Get( &widget_list, i );
		item->need_delete = TRUE;
	}
}

/* 清理部件列表中需要移除的部件，并将部件状态设置为NORMAL */
static void
widget_list_clear( void )
{
	int i, n;
	widget_item *item;
	n = Queue_GetTotal( &widget_list );
	for(i=0; i<n; ++i) {
		item = Queue_Get( &widget_list, i );
		if( !item->need_delete ) {
			continue;
		}
		Widget_SetState( item->widget, WIDGET_STATE_NORMAL );
		Queue_Delete( &widget_list, i );
		n = Queue_GetTotal( &widget_list );
		--i;
	}
}

/* 添加部件至列表里，如果已存在，则标记该部件不需要删除 */
static int
widget_list_add( LCUI_Widget *widget )
{
	int i, n;
	widget_item new_item, *item;
	n = Queue_GetTotal( &widget_list );
	for(i=0; i<n; ++i) {
		item = Queue_Get( &widget_list, i );
		if( item->widget == widget ) {
			item->need_delete = FALSE;
			return 1;
		}
	}
	new_item.need_delete = FALSE;
	new_item.widget = widget;
	Queue_Add( &widget_list, &new_item );
	return 0;
}

/* 判断指定部件是否被允许响应事件 */
static LCUI_BOOL widget_allow_response( LCUI_Widget *widget )
{
	int i, n;
	LCUI_Queue *child_list;
	LCUI_Widget *child, *up_widget;

	if( widget == NULL ) {
		return TRUE;
	}
	/* 开始判断该部件的上级部件 */
	up_widget = widget->parent;
	while( widget ) {
		if( up_widget == NULL ) {
			child_list = &LCUI_Sys.widget_list;
		} else {
			child_list = &up_widget->child;
		}
		n = Queue_GetTotal( child_list );
		for(i=0; i<n; ++i) {
			child = Queue_Get( child_list, i );
			if( !child || !child->visible || !child->modal ) {
				continue;
			}
			if( child == widget ) {
				break;
			} else {
				return FALSE;
			}
		}
		/* 记录这一级部件 */
		widget = up_widget;
		if( widget ) {
			/* 切换至上级部件 */
			up_widget = widget->parent;
		}
	}
	return TRUE;
}

/* 设定部件状态，该函数会将该部件及上级所有父部件添加至列表里，每次调用该函数时，会更新
 * 列表中的部件记录，若部件在更新前后都在列表中有记录，则该部件及上级所有父部件都会应用
 * 此状态，否则，移除多余的部件，并恢复部件的状态为NORMAL */
static void
widget_list_set_state( LCUI_Widget *widget, WIDGET_STATE state )
{
	widget_list_reset();
	while( widget ) {
		widget_list_add( widget );
		Widget_SetState( widget, state );
		widget = widget->parent;
	}
	widget_list_clear();
}

/* 响应鼠标按键按下事件 */
static void
LCUI_HandleMouseButtonDown( LCUI_MouseButtonEvent *event )
{
	LCUI_Widget *widget, *tmp_widget;
	LCUI_Pos pos;
	LCUI_WidgetEvent wdg_event;

	if( !event || event->state != LCUIKEYSTATE_PRESSED ) {
		return;
	}

	pos.x = event->x;
	pos.y = event->y;
	widget = Widget_At( NULL, pos );

	if( !widget_allow_response(widget) ) {
		return;
	}
	/* 获取能够响应此事件的部件 */
	tmp_widget = Get_ResponseEvent_Widget( widget, EVENT_MOUSEBUTTON );
	if( widget != NULL && tmp_widget != NULL ) {
		/* 开始准备事件数据 */
		pos = Widget_ToRelPos( tmp_widget, pos );
		wdg_event.type = EVENT_MOUSEBUTTON;
		wdg_event.mouse_button.x = pos.x;
		wdg_event.mouse_button.y = pos.y;
		wdg_event.mouse_button.button = event->button;
		wdg_event.mouse_button.state = event->state;
		/* 派发事件 */
		Widget_DispatchEvent( tmp_widget, &wdg_event );
	}
	if( event->button != LCUIKEY_LEFTBUTTON ) {
		return;
	}
	click_widget = widget;
	tmp_widget = Get_ResponseEvent_Widget( widget, EVENT_DRAG );
	/* 焦点转移给该部件 */
	Set_Focus( click_widget );
	if( widget != NULL && tmp_widget != NULL ) {
		DEBUG_MSG("start drag\n");
		/* 开始处理部件的拖动 */
		_DragEvent_Start( tmp_widget, event );
	}
	widget_list_set_state( widget, WIDGET_STATE_ACTIVE );
}

/* 响应鼠标按键释放事件 */
static void
LCUI_HandleMouseButtonUp( LCUI_MouseButtonEvent *event )
{
	LCUI_Widget *tmp_widget, *widget;
	LCUI_WidgetEvent tmp_event;
	LCUI_Pos pos;

	if( !event || event->state != LCUIKEYSTATE_RELEASE ) {
		return;
	}
	pos.x = event->x;
	pos.y = event->y;
	widget = Widget_At( NULL, pos );

	if( !widget_allow_response(widget) ) {
		return;
	}
	if( widget != NULL ) {
		pos = Widget_ToRelPos( widget, pos );
		tmp_event.type = EVENT_MOUSEBUTTON;
		tmp_event.mouse_button.x = pos.x;
		tmp_event.mouse_button.y = pos.y;
		tmp_event.mouse_button.button = event->button;
		tmp_event.mouse_button.state = event->state;
		Widget_DispatchEvent( widget, &tmp_event );
	}

	if( event->button != LCUIKEY_LEFTBUTTON ) {
		return;
	}
	DEBUG_MSG("mouse left button free\n");
	if( !click_widget ) {
		/* 如果是点击屏幕空白处，则复位焦点 */
		Reset_Focus( NULL );
		return;
	}
	tmp_widget = Get_ResponseEvent_Widget( click_widget, EVENT_DRAG );
	if( tmp_widget != NULL ) {
		DEBUG_MSG("end drag\n");
		_DragEvent_End( tmp_widget, event );
	}
	if( click_widget == widget ) {
		/*
		 * 如果点击时和点击后都在同一个按钮部件内进行的,
		 * 触发CLICKED事件，将部件中关联该事件的回调函数发送至
		 * 任务队列，使之在主循环中执行
		 * */
		tmp_widget = Get_ResponseEvent_Widget( widget, EVENT_CLICKED );
		if( tmp_widget && tmp_widget->enabled ) {
			tmp_event.type = EVENT_CLICKED;
			pos.x = event->x;
			pos.y = event->y;
			tmp_event.clicked.rel_pos = Widget_ToRelPos( tmp_widget, pos );
			Widget_DispatchEvent( tmp_widget, &tmp_event );
		}
		widget_list_set_state (widget, WIDGET_STATE_ACTIVE);
	}
	widget_list_set_state (widget, WIDGET_STATE_OVERLAY);
	click_widget = NULL;
}

static void
LCUI_HandleMouseButton( LCUI_MouseButtonEvent *event, void *unused )
{
	if( event->state == LCUIKEYSTATE_PRESSED ) {
		LCUI_HandleMouseButtonDown( event );
	} else {
		LCUI_HandleMouseButtonUp( event );
	}
}

static void
LCUI_HandleMouseMotion( LCUI_MouseMotionEvent *event, void *unused )
{
	LCUI_Pos pos;
	LCUI_Widget *tmp_widget, *widget;
	LCUI_WidgetEvent wdg_event;

	pos.x = event->x;
	pos.y = event->y;
	/* 获取当前鼠标游标覆盖到的部件的指针 */
	widget = Widget_At (NULL, pos);
	if( widget ) {
		tmp_widget = Get_ResponseEvent_Widget( widget, EVENT_MOUSEMOTION );
		if( tmp_widget ) {
			wdg_event.type = EVENT_MOUSEMOTION;
			pos = Widget_ToRelPos( tmp_widget, pos );
			wdg_event.mouse_motion.rel_pos = pos;
			Widget_DispatchEvent( tmp_widget, &wdg_event );
		}
	}
	/* 如果没有部件处于按住状态 */
	if( widget && widget_allow_response(widget) && !click_widget ) {
		widget_list_set_state (widget, WIDGET_STATE_OVERLAY);
	}
	/* 如果之前点击过部件，并且现在鼠标左键还处于按下状态，那就处理部件拖动 */
	tmp_widget = Get_ResponseEvent_Widget( click_widget, EVENT_DRAG );
	if( tmp_widget != NULL && event->state == LCUIKEYSTATE_PRESSED ) {
		DEBUG_MSG("doing drag\n");
		_DragEvent_Do( tmp_widget, event );
	}
}

static int
Widget_DispatchKeyboardEvent(	LCUI_Widget *widget,
				LCUI_KeyboardEvent *event )
{
	int i,n;
	LCUI_EventSlot *slot;
	LCUI_Task *task;
	LCUI_KeyboardEvent *p_buff;

	if( !widget ) {
		return -1;
	}

	slot = EventSlots_Find( &widget->event, EVENT_KEYBOARD );
	if( !slot ) {
		return -2;
	}
	n = Queue_GetTotal( &slot->func_data );
	for(i=0; i<n; ++i) {
		task = Queue_Get( &slot->func_data, i );
		p_buff = malloc( sizeof(LCUI_KeyboardEvent) );
		if( !p_buff ) {
			char str[256];
			sprintf( str,"%s()", __FUNCTION__ );
			perror( str );
			abort();
		}
		*p_buff = *event;
		task->arg[1] = p_buff;
		task->destroy_arg[1] = TRUE;
		AppTasks_Add( task );
	}
	return 0;
}

static void
WidgetFocusProc( LCUI_KeyboardEvent *event, void *arg );

/* 初始化部件模块 */
LCUI_API void
LCUIModule_Widget_Init( void )
{
	WidgetQueue_Init( &LCUI_Sys.widget_list );
	WidgetUpdateBuff_Init( &LCUI_Sys.update_buff );
	Queue_Init( &widget_list, sizeof(widget_item), NULL );
	LCUI_MouseButtonEvent_Connect( LCUI_HandleMouseButton, NULL );
	LCUI_MouseMotionEvent_Connect( LCUI_HandleMouseMotion, NULL );
	LCUI_KeyboardEvent_Connect( WidgetFocusProc, NULL );
	WidgetStyle_LibraryInit();
}

/* 停用部件模块 */
LCUI_API void
LCUIModule_Widget_End( void )
{
	Queue_Destroy( &LCUI_Sys.widget_list );
	Queue_Destroy( &LCUI_Sys.update_buff );
	Queue_Destroy( &widget_list );
	WidgetStyle_LibraryDestroy();
}
/*************************** Event End *********************************/


/*--------------------------- Focus Proc ------------------------------*/
static LCUI_Widget *root_focus_widget = NULL;

LCUI_API LCUI_BOOL
Set_Focus( LCUI_Widget *widget )
/*
 * 功能：为部件设置焦点
 * 说明：上个获得焦点的部件会得到EVENT_FOCUSOUT事件，而当前获得焦点的部件会得到
 * EVENT_FOCUSIN事件。
 * */
{
	LCUI_Widget **focus_widget;
	LCUI_WidgetEvent event;

	if( widget ) {
		/* 先处理上级部件的焦点 */
		if( widget->parent ) {
			Set_Focus( widget->parent );
		}
		if( !widget->focus ) {
			return FALSE;
		}
	} else {
		return FALSE;
	}

	if( widget->parent ) {
		focus_widget = &widget->parent->focus_widget;
	} else {
		focus_widget = &root_focus_widget;
	}
	if( *focus_widget ) {
		/* 如果上次和这次的部件不一样 */
		if( *focus_widget != widget ) {
			event.type = EVENT_FOCUSOUT;
			Widget_DispatchEvent( *focus_widget, &event );
		}
	}
	event.type = EVENT_FOCUSIN;
	Widget_DispatchEvent( widget, &event );
	/* 保存新焦点位置 */
	*focus_widget = widget;
	return TRUE;
}

/* 设定部件是否能够获取焦点 */
LCUI_API void
Widget_SetFocus( LCUI_Widget *widget, LCUI_BOOL flag )
{
	/* 如果该部件已经获得焦点，并且要设置它不能获取焦点，则取消当前焦点 */
	if( Widget_GetFocus(widget) && !flag ) {
		Cancel_Focus( widget );
	}
	widget->focus = flag;
}

/* 获取指定部件内的已获得焦点的子部件 */
LCUI_API LCUI_Widget*
Get_FocusWidget( LCUI_Widget *widget )
{
	int i, focus_pos, total;
	LCUI_Widget **focus_widget;
	LCUI_Queue *queue_ptr;

	//printf( "Get_FocusWidget(）： widget: %p\n", widget );
	//print_widget_info( widget );
	if( !widget ) {
		queue_ptr = &LCUI_Sys.widget_list;
		focus_widget = &root_focus_widget;
	} else {
		/* 如果部件不需要焦点，则返回NULL */
		if( !widget->focus ) {
			return NULL;
		}
		queue_ptr = &widget->child;
		focus_widget = &widget->focus_widget;
	}

	if( !focus_widget ) {
		return NULL;
	}

	total = Queue_GetTotal( queue_ptr );
	if( total <= 0 ) {
		return NULL;
	}
	focus_pos = WidgetQueue_Get_Pos( queue_ptr, *focus_widget );
	if( focus_pos < 0 ) {
		*focus_widget = NULL;
		return NULL;
	}
	/* 查找可获取焦点的有效部件 */
	for( i=focus_pos; i<total; ++i ) {
		widget = Queue_Get( queue_ptr, i );
		if( widget && widget->focus ) {
			break;
		}
	}
	if( i>=total ) {
		*focus_widget = NULL;
		widget = NULL;
	}

	return widget;
}

LCUI_API LCUI_BOOL
Cancel_Focus( LCUI_Widget *widget )
/*
 * 功能：取消指定部件的焦点
 * 说明：该部件会得到EVENT_FOCUSOUT事件，并且，会将焦点转移至其它部件
 * */
{
	int i, total, focus_pos;
	LCUI_Widget *other_widget, **focus_widget;
	LCUI_Queue *queue_ptr;
	LCUI_WidgetEvent event;

	if( !widget || !widget->focus ) {
		return FALSE;
	}

	if( widget->parent ) {
		focus_widget = &widget->parent->focus_widget;
		queue_ptr = &widget->parent->child;
	} else {
		focus_widget = &root_focus_widget;
		queue_ptr = &LCUI_Sys.widget_list;
	}
	/* 如果该部件并没获得焦点 */
	if( *focus_widget != widget ) {
		return FALSE;
	}
	event.type = EVENT_FOCUSOUT;
	Widget_DispatchEvent( widget, &event );
	/* 寻找可获得焦点的其它部件 */
	total = Queue_GetTotal( queue_ptr );
	focus_pos = WidgetQueue_Get_Pos( queue_ptr, *focus_widget );
	for( i=0; i<focus_pos; ++i ) {
		other_widget = Queue_Get( queue_ptr, i);
		if( other_widget && other_widget->visible
		 && other_widget->focus ) {
			event.type = EVENT_FOCUSIN;
			Widget_DispatchEvent( widget, &event );
			*focus_widget = other_widget;
			break;
		}
	}
	if( i < focus_pos ) {
		return TRUE;
	}
	/* 排在该部件前面的符合条件的部件没找到，就找排在该部件后面的 */
	for( i=focus_pos+1; i<total; ++i ) {
		other_widget = Queue_Get( queue_ptr, i);
		if( other_widget && other_widget->visible
		 && other_widget->focus ) {
			event.type = EVENT_FOCUSIN;
			Widget_DispatchEvent( other_widget, &event );
			*focus_widget = other_widget;
			break;
		}
	}
	/* 没找到就复位焦点 */
	if( i >= total ) {
		*focus_widget = NULL;
	}
	return TRUE;
}

LCUI_API LCUI_BOOL
Reset_Focus( LCUI_Widget* widget )
/* 复位指定部件内的子部件的焦点 */
{
	LCUI_Widget** focus_widget;
	LCUI_WidgetEvent event;

	if( widget ) {
		focus_widget = &widget->focus_widget;
	} else {
		focus_widget = &root_focus_widget;
	}
	if( *focus_widget ) {
		event.type = EVENT_FOCUSOUT;
		Widget_DispatchEvent( *focus_widget, &event );
	}

	*focus_widget = NULL;
	return TRUE;
}

LCUI_API int
Next_FocusWidget()
{
	return 0;
}

LCUI_API int
Prev_FocusWidget()
{
	return 0;
}

LCUI_API int
Return_FocusToParent()
{
	return 0;
}

static void
WidgetFocusProc( LCUI_KeyboardEvent *event, void *unused )
{
	LCUI_Widget *widget = NULL, *tmp = NULL, *focus_widget;
	/* 如果输入法需要处理这个键，则退出本函数 */
	if( LCUIIME_ProcessKey( event ) ) {
		return;
	}
	/* 否则，将这个键盘消息发送给当前获得焦点的部件 */
	while( 1 ) {
		/* 获取当前容器中已获得焦点的部件 */
		focus_widget = Get_FocusWidget( widget );
		//_DEBUG_MSG("focus_widget: %p\n", focus_widget);
		//print_widget_info( focus_widget );
		if( !focus_widget ) {
			if( !tmp ) {
				break;
			}
			Widget_DispatchKeyboardEvent( tmp, event );
			break;
		}
		/* 切换到子部件 */
		widget = focus_widget;
		/* 保存已关联按键事件的部件指针 */
		if( EventSlots_Find( &widget->event, EVENT_KEYBOARD ) ) {
			tmp = widget;
		}
	}
}
/*------------------------- End Focus Proc ----------------------------*/
