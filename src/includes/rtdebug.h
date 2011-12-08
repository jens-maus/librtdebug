/* vim:set ts=2 nowrap: ****************************************************

 librtdebug - A C++ based thread-safe Runtime Debugging Library
 Copyright (C) 2003-2006 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef RTDEBUG_H
#define RTDEBUG_H

//! This header file is part of the CDebug class mechanism to allow
//! developers to output debugging relevant information to a terminal
//! with automatically taking respect of different a multithreaded
//! environment.
//!
//! Please take a look at the CRTDebug.h header file for a more
//! detailed information about the ideas behind that debugging mechanism

// redefine the Qt's own debug system
#ifdef qDebug
#undef qDebug
#define qDebug	D
#endif
#ifdef qWarning
#undef qWarning
#define qWarning W
#endif
#ifdef qFatal
#undef qFatal
#define qFatal E
#endif

#include <CRTDebug.h>

// first we make sure all previously defined symbols are undefined now so
// that no other debug system interferes with ours.
#if defined(ENTER)
#undef ENTER
#endif
#if defined(LEAVE)
#undef LEAVE
#endif
#if defined(RETURN)
#undef RETURN
#endif
#if defined(SHOWVALUE)
#undef SHOWVALUE
#endif
#if defined(SHOWPOINTER)
#undef SHOWPOINTER
#endif
#if defined(SHOWSTRING)
#undef SHOWSTRING
#endif
#if defined(SHOWMSG)
#undef SHOWMSG
#endif
#if defined(STARTCLOCK)
#undef STARTCLOCK
#endif
#if defined(STOPCLOCK)
#undef STOPCLOCK
#endif
#if defined(D)
#undef D
#endif
#if defined(E)
#undef E
#endif
#if defined(W)
#undef W
#endif
#if defined(ASSERT)
#undef ASSERT
#endif
#if defined(Debug)
#undef Debug
#endif
#if defined(Verbose)
#undef Verbose
#endif
#if defined(Warning)
#undef Warning
#endif
#if defined(Error)
#undef Error
#endif
#if defined(Info)
#undef Info
#endif

#if defined(DEBUG)

#include <stdlib.h>

#if !defined(DEBUG_MODULE)
#define DEBUG_MODULE DBM_NONE
#endif

// Core class information class messages
#define ENTER()					CRTDebug::instance()->Enter(DBC_CTRACE, DEBUG_MODULE, __FILE__, __LINE__, __FUNCTION__)
#define LEAVE()					CRTDebug::instance()->Leave(DBC_CTRACE, DEBUG_MODULE, __FILE__, __LINE__, __FUNCTION__)
#define RETURN(r)				CRTDebug::instance()->Return(DBC_CTRACE, DEBUG_MODULE, __FILE__, __LINE__, __FUNCTION__, (long)r)
#define SHOWVALUE(v)		CRTDebug::instance()->ShowValue(DBC_REPORT, DEBUG_MODULE, (long)v, sizeof(v), #v, __FILE__, __LINE__)
#define SHOWPOINTER(p)	CRTDebug::instance()->ShowPointer(DBC_REPORT, DEBUG_MODULE, p, #p, __FILE__, __LINE__)
#define SHOWSTRING(s)		CRTDebug::instance()->ShowString(DBC_REPORT, DEBUG_MODULE, s, #s, __FILE__, __LINE__)
#define SHOWMSG(m)			CRTDebug::instance()->ShowMessage(DBC_REPORT, DEBUG_MODULE, m, __FILE__, __LINE__)
#define STARTCLOCK(s)		CRTDebug::instance()->StartClock(DBC_TIMEVAL, DEBUG_MODULE,  s, __FILE__, __LINE__)
#define STOPCLOCK(s)		CRTDebug::instance()->StopClock(DBC_TIMEVAL, DEBUG_MODULE, s, __FILE__, __LINE__)
#define D(s, vargs...)	CRTDebug::instance()->dprintf_header(DBC_DEBUG, DEBUG_MODULE, __FILE__, __LINE__, s, ## vargs)
#define E(s, vargs...)	CRTDebug::instance()->dprintf_header(DBC_ERROR, DEBUG_MODULE, __FILE__, __LINE__, s, ## vargs)
#define W(s, vargs...)	CRTDebug::instance()->dprintf_header(DBC_WARNING, DEBUG_MODULE, __FILE__, __LINE__, s, ## vargs)
#define ASSERT(expression)      \
	((void)                       \
	 ((expression) ? 0 :          \
	  (														\
	   CRTDebug::instance()->dprintf_header(DBC_ASSERT,		\
																					DEBUG_MODULE, \
																					__FILE__,			\
																					__LINE__,			\
																					"failed assertion '%s'", #expression), \
	   abort(),                   \
	   0                          \
	  )                           \
	 )                            \
	)

#else // DEBUG

#define ENTER()							(void(0))
#define LEAVE()							(void(0))
#define RETURN(r)						(void(0))
#define SHOWVALUE(v)				(void(0))
#define SHOWPOINTER(p)			(void(0))
#define SHOWSTRING(s)				(void(0))
#define SHOWMSG(m)					(void(0))
#define STARTCLOCK(s)				(void(0))
#define STOPCLOCK(s)				(void(0))
#define D(s, vargs...)			(void(0))
#define E(s, vargs...)			(void(0))
#define W(s, vargs...)			(void(0))
#define ASSERT(expression)	(void(0))

#endif // DEBUG
#endif // RTDEBUG_H
