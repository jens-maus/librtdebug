/* vim:set ts=2 nowrap: ****************************************************

 librtdebug - A C++ based thread-safe Runtime Debugging Library
 Copyright (C) 2003-2005 by Jens Langner <Jens.Langner@light-speed.de>

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

#include "CRTDebug.h"

#include <pthread.h>
#include <cstdarg>
#include <string>
#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include <stdio.h>

// define variables for using ANSI colors in our debugging scheme
#define ANSI_ESC_CLR				"\033[0m"
#define ANSI_ESC_BOLD				"\033[1m"
#define ANSI_ESC_UNDERLINE	"\033[4m"
#define ANSI_ESC_BLINK			"\033[5m"
#define ANSI_ESC_REVERSE		"\033[7m"
#define ANSI_ESC_INVISIBLE	"\033[8m"
#define ANSI_ESC_FG_BLACK		"\033[0;30m"
#define ANSI_ESC_FG_RED			"\033[0;31m"
#define ANSI_ESC_FG_GREEN		"\033[0;32m"
#define ANSI_ESC_FG_BROWN		"\033[0;33m"
#define ANSI_ESC_FG_BLUE		"\033[0;34m"
#define ANSI_ESC_FG_PURPLE	"\033[0;35m"
#define ANSI_ESC_FG_CYAN		"\033[0;36m"
#define ANSI_ESC_FG_LGRAY		"\033[0;37m"
#define ANSI_ESC_FG_DGRAY		"\033[1;30m"
#define ANSI_ESC_FG_LRED		"\033[1;31m"
#define ANSI_ESC_FG_LGREEN	"\033[1;32m"
#define ANSI_ESC_FG_YELLOW	"\033[1;33m"
#define ANSI_ESC_FG_LBLUE		"\033[1;34m"
#define ANSI_ESC_FG_LPURPLE	"\033[1;35m"
#define ANSI_ESC_FG_LCYAN		"\033[1;36m"
#define ANSI_ESC_FG_WHITE		"\033[1;37m"
#define ANSI_ESC_BG					"\033[0;4"		// background esc-squ start with 4x
#define ANSI_ESC_BG_BLACK		"\033[0;40m"
#define ANSI_ESC_BG_RED			"\033[0;41m"
#define ANSI_ESC_BG_GREEN		"\033[0;42m"
#define ANSI_ESC_BG_BROWN		"\033[0;43m"
#define ANSI_ESC_BG_BLUE		"\033[0;44m"
#define ANSI_ESC_BG_PURPLE	"\033[0;45m"
#define ANSI_ESC_BG_CYAN		"\033[0;46m"
#define ANSI_ESC_BG_LGRAY		"\033[0;47m"

// define the colors for each debug group
#define DBF_GR_NONE_COLOR				ANSI_ESC_FG_WHITE
#define DBF_GR_DEBUG_COLOR			ANSI_ESC_FG_PURPLE
#define DBF_GR_VERBOSE_COLOR		ANSI_ESC_FG_BLUE
#define DBF_GR_WARNING_COLOR		ANSI_ESC_FG_YELLOW
#define DBF_GR_ERROR_COLOR			ANSI_ESC_FG_RED
#define DBF_GR_INFO_COLOR				ANSI_ESC_FG_CYAN
#define DBF_GR_CALLTRACE_COLOR	ANSI_ESC_FG_BROWN
#define DBF_GR_CORE_COLOR				ANSI_ESC_FG_GREEN
#define DBF_GR_CLOCK_COLOR			ANSI_ESC_FG_BLUE

// some often used macros to output the thread number at the beginning of each
// debug output so that we know from which thread this output came.
#define THREAD_ID						m_ThreadID[pthread_self()]
#define THREAD_WIDTH				2
#define THREAD_PREFIX				std::setw(THREAD_WIDTH) << std::setfill('0') << THREAD_ID << ": "
#define THREAD_PREFIX_COLOR	ANSI_ESC_BG << THREAD_ID%6 << "m" << std::setw(THREAD_WIDTH) \
														<< std::setfill('0') << THREAD_ID << ":" << ANSI_ESC_CLR << " "
#define THREAD_ID_CHECK			if(m_ThreadID[pthread_self()] == 0)\
															m_ThreadID[pthread_self()] = ++m_iThreadCount

//  Class:       CRTDebug
//  Method:      instance
//! 
//! Return the current instance for the debug class. If no one exists 
//! creates one.
//! 
//! @return      pointer to current instance of CRTDebug
////////////////////////////////////////////////////////////////////////////////
CRTDebug* CRTDebug::m_pSingletonInstance = 0; 
CRTDebug* CRTDebug::instance()
{
	if(m_pSingletonInstance == 0)
	{
		m_pSingletonInstance = new CRTDebug();
	}

	return m_pSingletonInstance;
}

//  Class:       CRTDebug
//  Method:      destroy
//! 
//! Destroys (free) the singleton object and set the singleton Instance
//! variable to 0 so that the next call to instance() will create a new
//! singleton object.
//! 
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::destroy()
{
	if(m_pSingletonInstance)
	{
		delete m_pSingletonInstance;
		m_pSingletonInstance = 0;
	}
}

//  Class:       CRTDebug
//  Constructor: CRTDebug
//! 
//! Construct a CRTDebug object.
//! 
////////////////////////////////////////////////////////////////////////////////
CRTDebug::CRTDebug(Level dLevel)
	: m_bHighlighting(true),
	  m_CurDebugLevel(dLevel),
		m_iThreadCount(0)
{
	pthread_mutex_init(&m_pCoutMutex, NULL);
}

//  Class:       CRTDebug
//  Destructor:  CRTDebug
//! 
//! Destruct a CRTDebug object.
//! 
////////////////////////////////////////////////////////////////////////////////
CRTDebug::~CRTDebug()
{
	pthread_mutex_destroy(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      Enter
//! 
//! This method will print out a short information string which should signal
//! the developer that the program flow passed a new method/function entry.
//!
//! It will normally be used by the uppercase ENTER() macros in debug.h which
//! should be placed at every method/function entry.
//! 
//! @param       c the debug class on which the output should be placed
//! @param       file file name of the source code file
//! @param       line the line number on which the ENTER() was placed
//! @param       function name of the function in which the ENTER() was placed.
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::Enter(const int c, const char *file, long line, const char *function)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if(((m_CurDebugLevel & c)) == 0) return;

	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CALLTRACE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Entering " << function << "()"
							<< ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ')
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Entering " << function << "()" << std::endl;
	}

	pthread_mutex_unlock(&m_pCoutMutex);

  m_IdentLevel[pthread_self()] = m_IdentLevel[pthread_self()] + 1;
}

//  Class:       CRTDebug
//  Method:      Leave
//!
//! This method will print out a short information string which should signal
//! the developer that the program flow is actually leaving a method/function.
//!
//! It will normally be used by the uppercase LEAVE() macros in debug.h which
//! should be placed at every method/function entry.
//! 
//! @param       c the debug class on which the output should be placed
//! @param       file file name of the source code file
//! @param       line the line number on which the LEAVE() was placed
//! @param       function name of the function in which the LEAVE() was placed.
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::Leave(const int c, const char *file, int line, const char *function)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	if(m_IdentLevel[pthread_self()] > 0)
		m_IdentLevel[pthread_self()]--;

	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CALLTRACE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Leaving " << function << "()"
							<< ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ')
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Leaving " << function << "()" << std::endl;
	}

	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      Return
//!
//! This method will print out a short information string which should signal
//! the developer that the program flow is actually leaving a method/function.
//!
//! It will normally be used by the uppercase RETURN() macros in debug.h which
//! should be placed at every method/function entry.
//! 
//! @param       c the debug class on which the output should be placed
//! @param       file file name of the source code file
//! @param       line the line number on which the RETURN() was placed
//! @param       function name of the function in which the RETURN() was placed.
//! @param       return the return value
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::Return(const int c, const char *file, int line,
										const char *function, long result)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
  if(m_IdentLevel[pthread_self()] > 0)
		m_IdentLevel[pthread_self()]--;

	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CALLTRACE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Leaving " << function << "() (result 0x"
							<< std::hex << std::setw(8) << std::setfill('0') << result << ", "
							<< std::dec << result << ")" << ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ')
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Leaving " << function << "() (result 0x"
							<< std::hex << std::setw(8) << std::setfill('0') << result << ", "
							<< std::dec << result << ")" << std::endl;
	}

	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      ShowValue
//! 
//! This method will output "show" a value on the terminal after formating
//! the debugging string accordingly.
//!
//! It is normally executed by the SHOWVALUE() macro which should easily
//! give a developer a way to output any variable to the terminal to check
//! it's current value on runtime.
//! 
//! @param       c			the debug class on which this output should be placed
//! @param       value	the variable on which we want to view the value
//! @param       size		the size of the variable so that we can do some hex output
//! @param       name		the name of the variable for our output
//! @param       file		the file name of the source code where we placed SHOWVALUE()
//! @param       line		the line number on which the SHOWVALUE() is.
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::ShowValue(const int c, long value, int size, 
											 const char *name, const char *file, long line)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CORE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = " << value
							<< ", 0x" << std::hex << std::setw(size*2) << std::setfill('0')
							<< value;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ')
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = " << value
							<< ", 0x" << std::hex << std::setw(size*2) << std::setfill('0')
							<< value;
	}
	
	if(size == 1 && value < 256)
	{
		if(value < ' ' || (value >= 127 && value <= 160))
		{
			std::cout << ", '" << std::hex << std::setw(2) << std::setfill('0') << value
								<< "'";
		}
		else
		{
			std::cout << ", '" << (char)value << "'";
		}
	}

	if(m_bHighlighting) std::cout << ANSI_ESC_CLR;

	std::cout << std::endl;

	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      ShowPointer
//! 
//! Method to give a developer the possibility to view a currently set
//! pointer variable. It outputs the current address of that pointer.
//!
//! It is normally invoked by the SHOWPOINTER() macro.
//! 
//! @param       c				the debug class
//! @param       pointer	the pointer variable which we want to output
//! @param       name			the name of the variable
//! @param       file			the file name of the source we have the SHOWPOINTER()
//! @param       line			the line number on which the SHOWPOINTER() is.
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::ShowPointer(const int c, void *pointer, const char *name,
												 const char *file, long line)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CORE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = ";
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ')
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = ";
	}

	if(pointer != NULL)	std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << pointer;
	else								std::cout << "NULL";

	if(m_bHighlighting) std::cout << ANSI_ESC_CLR;
	std::cout << std::endl;

	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      ShowString
//! 
//! Method to show the user a string on the terminal. This string will
//! not be preparsed or altered in anyway, this is done by another method.
//!
//! This method is invoked by the SHOWSTRING() method which gives a developer
//! the possibility to output any string on runtime.
//! In contrast to the ShowMsg() method, this method will output the name
//! of the used string aswell as the address of it.
//! 
//! @param       c			the debug class
//! @param       string the string to output
//! @param       name		the variable name of the string
//! @param       file		the file name were the SHOWSTRING() is
//! @param       line		the line number on which the SHOWSTRING() is
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::ShowString(const int c, const char *string,
												const char *name, const char *file, long line)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CORE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = 0x" << std::hex
							<< std::setw(8) << std::setfill('0') << string << " \""
							<< string << "\"" << ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ')
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = 0x" << std::hex
							<< std::setw(8) << std::setfill('0') << string << " \""
							<< string << "\"" << std::endl;
	}

	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      ShowMessage
//! 
//! Method to just output a string. The string will not be preparsed or altered
//! in any form.
//!
//! This method is invoked by the SHOWMSG() macro to give the developer the
//! possibility to output any string within the debug environment.
//! 
//! @param       c			the debug class
//! @param       string	the string to output
//! @param       file		the filename of the source
//! @param       line		the line number where we have placed the SHOWMSG()
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::ShowMessage(const int c, const char *string,
												 const char *file, long line)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CORE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << ANSI_ESC_CLR
							<< std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') 
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << std::endl;
	}
	
	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      StartClock
//! 
//! Method to output the current time in seconds.milliseconds since 1.1.1970
//! and to automatically start a timer to measure the time between a StartClock()
//! and a StopClock() call.
//!
//! This method is invoked by the STARTCLOCK() macro and should be followed
//! by a later STOPCLOCK() call within the same thread, so that this debug
//! class can output the measured time between those two calls.
//! 
//! @param       c			the debug class
//! @param       string the additional string to identify the clock
//! @param       file		the filename of the source code
//! @param       line		the line number on which we have the STARTCLOCK()
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::StartClock(const int c, const char *string,
												const char *file, long line)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	// lets get the current time of the day
	struct timeval* tp = &m_TimeMeasure[pthread_self()];
	if(gettimeofday(tp, NULL) != 0) return;

	// convert the timeval in some human readable format
	double starttime = (double)tp->tv_sec + ((double)tp->tv_usec/(double)MICROSEC);

	// now we convert that starttime to something human readable
	struct tm brokentime;
	time_t curTime = (time_t)starttime;
	localtime_r(&curTime, &brokentime);
	char buf[10];
	strftime(&buf[0], 10, "%T", &brokentime);
	char formattedTime[40];
	snprintf(&formattedTime[0], 40, "%s'%03d", buf, (int)((starttime-((int)starttime))*1000.0));

	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CLOCK_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << " started@"
							<< formattedTime << ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') 
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << " started@"
							<< formattedTime << std::endl;
	}
	
	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      StopClock
//! 
//! Method to output the current time in seconds.milliseconds since 1.1.1970
//! and to calculate the passed time since the last StartClock() call.
//!
//! This method is invoked by the STOPCLOCK() macro and calculates the time
//! since the last executation of STARTCLOCK(). It will then output the
//! difference in seconds.milliseconds format.
//! 
//! @param       c			the debug class
//! @param       string	the additional string to output
//! @param       file		the filename of the source code file
//! @param       line		the line number on which we placed the STOPCLOCK()
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::StopClock(const int c, const char *string,
											 const char *file, long line)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	// lets get the current time of the day
	struct timeval	newtp;
	if(gettimeofday(&newtp, NULL) != 0) return;

	// now we calculate the timedifference
	struct timeval* oldtp = &m_TimeMeasure[pthread_self()];
	struct timeval	difftp;
	difftp.tv_sec		= newtp.tv_sec - oldtp->tv_sec;
	difftp.tv_usec	= newtp.tv_usec - oldtp->tv_usec;
	if(difftp.tv_usec < 0)
	{
		difftp.tv_sec--;
		difftp.tv_usec += 1000000;
  }

	// convert the actual and diff time in a human readable format
	double stoptime = (double)newtp.tv_sec + ((double)newtp.tv_usec/(double)MICROSEC);
	double difftime = (double)difftp.tv_sec + ((double)difftp.tv_usec/(double)MICROSEC);

	// now we convert that starttime to something human readable
	struct tm brokentime;
	time_t curTime = (time_t)stoptime;
	localtime_r(&curTime, &brokentime);
	char buf[10];
	strftime(&buf[0], 10, "%T", &brokentime);
	char formattedTime[40];
	snprintf(&formattedTime[0], 40, "%s'%03d", buf, (int)((stoptime-((int)stoptime))*1000.0));

	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << DBF_GR_CLOCK_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << " stopped@"
							<< formattedTime << " = " << std::fixed << std::setprecision(3) << difftime << "s"
							<< ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') 
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << " stopped@"
							<< formattedTime << " = " << std::fixed << std::setprecision(3) << difftime << "s"
							<< std::endl;
	}
	
	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      dprintf_header
//! 
//! Autoformatting debug message method which will format a provided formatstring
//! in a printf()-like format and output the resulting string together with the
//! default debugging header.
//!
//! This method is invoked by the D() E() and W() macros to output a simple string
//! within a predefined debugging class and group.
//! 
//! @param       c		the debug class on which the output should be done.
//! @param       g		the debug group in which this message belongs.
//! @param       file	the filename of the source file.
//! @param       line the line number on which the macro was placed
//! @param       fmt  the format string
//! @param       ...  a vararg list of parameters.
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::dprintf_header(const int c, const int g, const char *file,
														long line, const char *fmt, ...)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_bHighlighting)
	{
		char *highlight = "";
		switch(g)
		{
			case DBF_GR_NONE:			highlight = DBF_GR_NONE_COLOR;		break;
			case DBF_GR_DEBUG:		highlight = DBF_GR_DEBUG_COLOR;		break;
			case DBF_GR_VERBOSE:	highlight = DBF_GR_VERBOSE_COLOR; break;
			case DBF_GR_WARNING:	highlight	= DBF_GR_WARNING_COLOR;	break;
			case DBF_GR_ERROR:		highlight = DBF_GR_ERROR_COLOR;		break;
			case DBF_GR_INFO:			highlight = DBF_GR_INFO_COLOR;		break;
			case DBF_GR_CORE:			highlight = DBF_GR_CORE_COLOR;		break;
		}

		std::cout << THREAD_PREFIX_COLOR;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ') << highlight
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << buf << ANSI_ESC_CLR
							<< std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX;
		std::cout << std::string(m_IdentLevel[pthread_self()], ' ')
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << buf
							<< std::endl;
	}

	pthread_mutex_unlock(&m_pCoutMutex);
}

//  Class:       CRTDebug
//  Method:      dprintf
//!
//! Autoformatting debug message method which will format a provided formatstring
//! in a printf()-like format and output the resulting string.
//!
//! This method is invoked by the mixed-case macros to output user dependant
//! information strings to the user. It will also take care of ANSI color
//! highlighting if enabled.
//! 
//! @param       c	 the debug class on which the output should be done.
//! @param       g   the debug group in which this message belongs.
//! @param       fmt the format string
//! @param       ... a vararg list of parameters.
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::dprintf(const int c, const int g, const char *fmt, ...)
{
	// first we check if need to process this debug message or not,
	// depending on the currently set debug level
	if((m_CurDebugLevel & c) == 0) return;
	
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

	pthread_mutex_lock(&m_pCoutMutex);

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	#ifdef DEBUG
	if(m_bHighlighting)
	{
		switch(g)
		{
			case DBF_GR_DEBUG:		std::cout << DBF_GR_DEBUG_COLOR		<< "Debug..: ";	break;
			case DBF_GR_VERBOSE:	std::cout << DBF_GR_VERBOSE_COLOR << "Verbose: "; break;
			case DBF_GR_WARNING:	std::cout << DBF_GR_WARNING_COLOR << "Warning: ";	break;
			case DBF_GR_ERROR:		std::cout << DBF_GR_ERROR_COLOR   << "Error..: ";	break;
			case DBF_GR_INFO:			std::cout << DBF_GR_INFO_COLOR		<< "Info...: ";	break;
		}
	}
	else
	{
		switch(g)
		{
			case DBF_GR_DEBUG:		std::cout << "Debug..: ";	break;
			case DBF_GR_VERBOSE:	std::cout << "Verbose: "; break;
			case DBF_GR_WARNING:	std::cout << "Warning: ";	break;
			case DBF_GR_ERROR:		std::cout << "Error..: ";	break;
			case DBF_GR_INFO:			std::cout << "Info...: ";	break;
		}
	}
	#else
	if(m_bHighlighting)
	{
		switch(g)
		{
			case DBF_GR_DEBUG:		std::cout << DBF_GR_DEBUG_COLOR;		break;
			case DBF_GR_VERBOSE:	std::cout << DBF_GR_VERBOSE_COLOR;	break;
			case DBF_GR_WARNING:	std::cout << DBF_GR_WARNING_COLOR;	break;
			case DBF_GR_ERROR:		std::cout << DBF_GR_ERROR_COLOR;		break;
			case DBF_GR_INFO:			std::cout << DBF_GR_INFO_COLOR;			break;
		}
	}
	#endif

	std::cout << buf << ANSI_ESC_CLR << std::endl;

	pthread_mutex_unlock(&m_pCoutMutex);
}
