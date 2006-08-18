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

#ifndef CRTDEBUG_H
#define CRTDEBUG_H

#include <map>
#include <pthread.h>

// debug classes
#define DBC_CTRACE		(1<<0) // call tracing (ENTER/LEAVE etc.)
#define DBC_REPORT		(1<<1) // reports (SHOWVALUE/SHOWSTRING etc.)
#define DBC_ASSERT		(1<<2) // asserts (ASSERT)
#define DBC_TIMEVAL		(1<<3) // time evaluations (STARTCLOCK/STOPCLOCK)
#define DBC_DEBUG			(1<<4) // debugging output D()
#define DBC_ERROR			(1<<5) // error output     E()
#define DBC_WARNING		(1<<6) // warning output   W()
#define DBC_ALL				(0xffffffff)

// debug flags
#define DBF_ALWAYS		(1<<0)
#define DBF_STARTUP		(1<<1)
#define DBF_ALL				(0xffffffff)

// debug modules
#define DBM_NONE			NULL
#define DBM_ALL				"all"

//  Classname:   CRTDebug
//! @brief debugging purpose class
//! @ingroup debug 
//! 
//! This class manages the whole debugging system of an application. It will
//! print out specified debugging information depending on the currently set
//! debugging level.
//! To use this class properly it is necessary that you specify your own
//! debugging levels first (specify the above class defines and the blow enum Levels)
//! so that you can specify which debug message belongs to which class and the
//! below "enum Level" should contain the different bit combinations of those
//! debug "classes", so that you can set a different debug level on runtime.
//!
//! Last, but not least you need to include the file "debug.h" everywhere you
//! want to put debugmessages which directly uses this debug class. Always use
//! the predefined macros in "debug.h" for specifying your debug messages.
//! There you will fine uppercase-only macros which will automatically removed
//! when you compile your program without the global "DEBUG" define, and you got
//! mixed-case macros which will stay in your code, but behave differently if
//! compiled without the "DEBUG" define.
//! Normally the mixed-case macros are used for terminal output which should
//! stay and be user configurable and should not be placed in time consuming
//! situations. The uppercase macros are really for debugging purposes only and
//! will completely vanish if you build a release version.
//! The rule is: Use the uppercase macros to print out your geek debug
//!              information so that if you compile a release version a normal
//!              user wouldn't be bothered with. The mixed-case macros will
//!              output user-dependant debug information.
//!
//! Please note that this debug class is fully threadsafe - which means that
//! it will take care of overlapping output from different threads. It will
//! sync it with a mutal exclusion and as an nice addition also output the
//! threadnumber in front of every debug output to easily identify to which
//! thread a output belongs to.
////////////////////////////////////////////////////////////////////////////////
class CRTDebug
{
	public:
		// the static singleton instance method
		static CRTDebug* instance();
		static void destroy();

		// for initialization via ENV variables
		static void init(const char* variable=0);

		// our main debug output methods
		void Enter(const int c, const char* m, const char* file, long line, const char* function);
		void Leave(const int c, const char* m, const char* file, int line, const char* function);
		void Return(const int c, const char* m, const char* file, int line, const char* function, long result);
		void ShowValue(const int c, const char* m, long value, int size, const char* name, const char* file, long line);
		void ShowPointer(const int c, const char* m, void* pointer, const char* name, const char* file, long line);
		void ShowString(const int c, const char* m, const char* string, const char* name, const char* file, long line);
		void ShowMessage(const int c, const char* m, const char* string, const char* file, long line);
		void StartClock(const int c, const char* m, const char* string, const char* file, long line);
		void StopClock(const int c, const char* m, const char* string, const char* file, long line);

		// some raw methods to format text like printf() does
		void dprintf_header(const int c, const char* m, const char* file, long line, const char* fmt, ...);
		void dprintf(const int c, const char* m, const char* fmt, ...);

		// general public methods to control debug class
		unsigned int debugClasses() const;
		unsigned int debugFlags() const;
		bool highlighting() const;
		void setDebugClasses(unsigned int classes);
		void setDebugFlags(unsigned int flags);
		void setHighlighting(bool on);
		
	protected:
		CRTDebug(const int dbclasses=0, const int dbflags=0);
		~CRTDebug();

		bool matchDebugSpec(const int cl, const char* module, const char* file);
	
	private:
		static CRTDebug*										m_pSingletonInstance;	//!< the singleton instance
		std::map<pthread_t, unsigned int>		m_ThreadID;						//!< thread identification number
		std::map<pthread_t, unsigned int>		m_IdentLevel;					//!< different ident levels for different threads
		std::map<pthread_t, struct timeval>	m_TimeMeasure;				//!< for measuring the time we need more structs
		bool																m_bHighlighting;			//!< text ANSI highlighting?
		pthread_mutex_t											m_pCoutMutex;					//!< a mutex to sync cout output
		unsigned int												m_iDebugClasses;			//!< the currently active debug classes
		std::map<std::string, bool>					m_DebugModules;				//!< to map actual specified debug modules
		std::map<std::string, bool>					m_DebugFiles;					//!< to map actual specified source file names
		unsigned int												m_iDebugFlags;				//!< the currently active debug flags
		unsigned int												m_iThreadCount;				//!< counter of total number of threads processing
};

#endif // CRTDEBUG_H
