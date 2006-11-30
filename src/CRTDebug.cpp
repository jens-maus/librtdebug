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

#include "CRTDebug.h"

#include <cstdarg>
#include <string>
#include <map>
#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <cctype>

#include "config.h"

#if defined(HAVE_LIBPTHREAD)
#include <pthread.h>
#endif

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

// define the colors for each debug class
#define DBC_CTRACE_COLOR		ANSI_ESC_FG_BROWN
#define DBC_REPORT_COLOR		ANSI_ESC_FG_GREEN
#define DBC_ASSERT_COLOR		ANSI_ESC_FG_RED
#define DBC_TIMEVAL_COLOR		ANSI_ESC_FG_BLUE
#define DBC_DEBUG_COLOR			ANSI_ESC_FG_PURPLE
#define DBC_ERROR_COLOR			ANSI_ESC_FG_RED
#define DBC_WARNING_COLOR		ANSI_ESC_FG_YELLOW

// some often used macros to output the thread number at the beginning of each
// debug output so that we know from which thread this output came.
#if defined(HAVE_LIBPTHREAD)

#define THREAD_TYPE					pthread_self()
#define THREAD_ID						m_pData->m_ThreadID[THREAD_TYPE]
#define THREAD_WIDTH				2
#define THREAD_PREFIX				std::setw(THREAD_WIDTH) << std::setfill('0') << THREAD_ID << ": "
#define THREAD_PREFIX_COLOR	ANSI_ESC_BG << THREAD_ID%6 << "m" << std::setw(THREAD_WIDTH) \
														<< std::setfill('0') << THREAD_ID << ":" << ANSI_ESC_CLR << " "
#define THREAD_ID_CHECK			if(m_pData->m_ThreadID[THREAD_TYPE] == 0)\
															m_pData->m_ThreadID[THREAD_TYPE] = ++(m_pData->m_iThreadCount)
#define INDENT_OUTPUT				std::string(m_pData->m_IdentLevel[THREAD_TYPE], ' ')
#define LOCK_OUTPUTSTREAM		pthread_mutex_lock(&(m_pData->m_pCoutMutex))
#define UNLOCK_OUTPUTSTREAM	pthread_mutex_unlock(&(m_pData->m_pCoutMutex))

#else

#define THREAD_TYPE					0
#define THREAD_PREFIX				""
#define THREAD_PREFIX_COLOR	""
#define THREAD_ID_CHECK			(void(0))
#define INDENT_OUTPUT				std::string(m_pData->m_IdentLevel[THREAD_TYPE], ' ')
#define LOCK_OUTPUTSTREAM		(void(0))
#define UNLOCK_OUTPUTSTREAM	(void(0))

#warning "no pthread library found/supported. librtdebug is compiled without being thread-safe!"
#endif

// some systems doesn't have the MICROSEC macros
#ifndef MICROSEC
#define MICROSEC 1000
#endif
#ifndef MILLISEC
#define MILLISEC 1000000
#endif

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CRTDebugPrivate
{
	// methods
	public:
		bool matchDebugSpec(const int cl, const char* module, const char* file);
	
	// data
	public:
		std::map<pthread_t, unsigned int>		m_ThreadID;						//!< thread identification number
		std::map<pthread_t, unsigned int>		m_IdentLevel;					//!< different ident levels for different threads
		std::map<pthread_t, struct timeval>	m_TimeMeasure;				//!< for measuring the time we need more structs
		bool																m_bHighlighting;			//!< text ANSI highlighting?
		unsigned int												m_iDebugClasses;			//!< the currently active debug classes
		std::map<std::string, bool>					m_DebugModules;				//!< to map actual specified debug modules
		std::map<std::string, bool>					m_DebugFiles;					//!< to map actual specified source file names*/
		unsigned int												m_iDebugFlags;				//!< the currently active debug flags
		unsigned int												m_iThreadCount;				//!< counter of total number of threads processing		

		#if defined(HAVE_LIBPTHREAD)
		pthread_mutex_t											m_pCoutMutex;					//!< a mutex to sync cout output
		#endif
};

//!
//! The following "NameCompare" inlined class is a small helper class to please
//! the damned STL find_if() method so that we can "easily" do a lowercase
//! substring search in an std::map<>. What a shit this STL is....
//!
////////////////////////////////////////////////////////////////////////////////
class NameCompare
{
	public:
		NameCompare(std::string searchPattern)
			: m_sSearchPattern(searchPattern)
		{ 
			// well, believe it or not, but this is how you have
			// to convert a std::string to lowercase... :(
			std::transform(m_sSearchPattern.begin(), 
										 m_sSearchPattern.end(), 
										 m_sSearchPattern.begin(), tolower);
		}

		bool operator()(std::pair<std::string, bool> cur);

	private:
		std::string m_sSearchPattern;
};

bool NameCompare::operator()(std::pair<std::string, bool> cur)
{
	std::string::size_type pos = m_sSearchPattern.find(cur.first, 0);
	return pos != std::string::npos;
}

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
		m_pSingletonInstance = new CRTDebug();

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
//  Method:      init
//! 
//! initializes the debugging framework by parsing a specified environment variable
//! via getenv() so that only certain predefined debug messages will be output,
//! 
////////////////////////////////////////////////////////////////////////////////
void CRTDebug::init(const char* variable)
{
	CRTDebug* rtdebug = CRTDebug::instance();

	// if the user has specified an environment variable we
	// go and parse it accordingly.
	if(variable != NULL)
	{
		std::cout << "*** parsing ENV variable: '" << variable << "' for debug options." << std::endl
		          << "*** for tokens: '@' class, '+' flags, '&' name, '%' module" << std::endl
							<< "*** --------------------------------------------------------------------------" << std::endl;
		
		char* var = getenv(variable);
		if(var != NULL)
		{
			char* s = var;

			// now we iterate through the env-variable
			while(*s)
			{
				char firstchar;
				char* e;
				bool negate = false;

				if((e = strpbrk(s, " ,;")) == NULL)
					e = s+strlen(s);

				// analyze the first two characters of the
				// found token
				firstchar = s[0];
				if(firstchar != '\0')
				{
					if(firstchar == '!')
					{
						negate = true;
						firstchar = s[1];
						s++;
					}
					else if(s[1] == '!')
					{
						negate = true;
						s++;
					}
				}
				
				switch(firstchar)
				{
					// class definition
					case '@':
					{
						static const struct { char* token; unsigned int flag; } dbclasses[] =
						{
							{ "ctrace",	DBC_CTRACE	},
							{ "report", DBC_REPORT  },
							{ "assert", DBC_ASSERT  },
							{ "timeval",DBC_TIMEVAL },
							{ "debug",  DBC_DEBUG   },
							{ "error",  DBC_ERROR   },
							{ "warning",DBC_WARNING },
							{ "all",    DBC_ALL			},
							{ NULL,			0						}
						};

						for(int i=0; dbclasses[i].token; i++)
						{
							if(strncasecmp(s+1, dbclasses[i].token, strlen(dbclasses[i].token)) == 0)
							{
								std::cout << "*** @dbclass: " << (!negate ? "show" : "hide") << " '" << dbclasses[i].token << "' output" << std::endl;

								if(negate)
									rtdebug->m_pData->m_iDebugClasses &= ~dbclasses[i].flag;
								else
									rtdebug->m_pData->m_iDebugClasses |= dbclasses[i].flag;
							}
						}
					}
					break;

					// flags definition
					case '+':
					{
						static const struct { char* token; unsigned int flag; } dbflags[] =
						{
							{ "always",	DBF_ALWAYS	},
							{ "startup",DBF_STARTUP },
							{ "all",    DBF_ALL			},
							{ NULL,			0						}
						};

						for(int i=0; dbflags[i].token; i++)
						{
							if(strncasecmp(s+1, dbflags[i].token, strlen(dbflags[i].token)) == 0)
							{
								std::cout << "*** +dbflag.: " << (!negate ? "show" : "hide") << " '" << dbflags[i].token << "' output" << std::endl;

								if(negate)
									rtdebug->m_pData->m_iDebugFlags &= ~dbflags[i].flag;
								else
									rtdebug->m_pData->m_iDebugFlags |= dbflags[i].flag;
							}
						}
					}
					break;

					// file definition
					case '&':
					{
						char* tk = strdup(s+1);
						char* t;

						if((t = strpbrk(tk, " ,;")))
							*t = '\0';

						// convert the C-string to an STL std::string
						std::string token = tk;
						std::transform(token.begin(), 
													 token.end(), 
													 token.begin(), tolower);						
						free(tk);

						// lets add the lowercase token to our sourcefilemap.
						rtdebug->m_pData->m_DebugFiles[token] = !negate;
						std::cout << "*** &name...: " << (!negate ? "show" : "hide") << " '" << token << "' output" << std::endl;
					}
					break;

					// module definition
					case '%':
					{
						char* tk = strdup(s+1);
						char* t;

						if((t = strpbrk(tk, " ,;")))
							*t = '\0';

						// convert the C-string to an STL std::string
						std::string token = tk;
						std::transform(token.begin(), 
													 token.end(), 
													 token.begin(), tolower);						
						free(tk);

						// lets add the lowercase token to our sourcefilemap.
						rtdebug->m_pData->m_DebugModules[token] = !negate;
						std::cout << "*** %module.: " << (!negate ? "show" : "hide") << " '" << token << "' output" << std::endl;

					}
					break;

					default:
					{
						if(strncasecmp(s, "ansi", 4) == 0)
						{
							std::cout << "*** switching " << (!negate ? "on" : "off") << " ANSI color output" << std::endl;
							rtdebug->m_pData->m_bHighlighting = !negate;
						}
					}
				}

				// set the next start to our last search
				if(*e)
					s = ++e;
				else
					break;
			}

			std::cout << "*** --------------------------------------------------------------------------" << std::endl;
		}

		std::cout << "*** active debug classes/flags: 0x" << std::setw(8) << std::setfill('0') << std::hex << rtdebug->debugClasses() 
							<< "/0x" << std::setw(8) << std::setfill('0') << std::hex << rtdebug->debugFlags() << std::endl;
		std::cout << "*** Normal processing follows ************************************************" << std::endl;
	}
}

//  Class:       CRTDebug
//  Constructor: CRTDebug
//! 
//! Construct a CRTDebug object.
//! 
////////////////////////////////////////////////////////////////////////////////
CRTDebug::CRTDebug(const int dbclasses, const int dbflags)
{
	// allocate data from our private instance class
	m_pData = new CRTDebugPrivate();

	// set some default values
	m_pData->m_bHighlighting = true;
	m_pData->m_iDebugClasses = dbclasses;
	m_pData->m_iDebugFlags = dbflags;
	m_pData->m_iThreadCount = 0;

	#if defined(HAVE_LIBPTHREAD)
	pthread_mutex_init(&(m_pData->m_pCoutMutex), NULL);
	#endif

	// now we see if we have to apply some default settings or not.
	if(m_pData->m_iDebugClasses == 0)
		m_pData->m_iDebugClasses = DBC_ERROR | DBC_DEBUG | DBC_WARNING | DBC_ASSERT | DBC_REPORT;

	if(m_pData->m_iDebugFlags == 0)
		m_pData->m_iDebugFlags = DBF_ALWAYS | DBF_STARTUP; 

	std::cout << "*** rtdebug v" << PACKAGE_VERSION << " (" << __DATE__ << ") runtime debugging framework startup ***********" << std::endl;
}

//  Class:       CRTDebug
//  Destructor:  CRTDebug
//! 
//! Destruct a CRTDebug object.
//! 
////////////////////////////////////////////////////////////////////////////////
CRTDebug::~CRTDebug()
{
	#if defined(HAVE_LIBPTHREAD)
	pthread_mutex_destroy(&(m_pData->m_pCoutMutex));
	#endif

	std::cout << "*** rtdebug framework shutdowned *********************************************" << std::endl;	
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
void CRTDebug::Enter(const int c, const char* m, const char* file, long line, 
										 const char* function)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	// lock the output stream
	LOCK_OUTPUTSTREAM;
	
	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
						  << INDENT_OUTPUT << DBC_CTRACE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Entering " << function << "()"
							<< ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX
							<< INDENT_OUTPUT
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Entering " << function << "()" << std::endl;
	}

	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;

	// increase the indention level
  m_pData->m_IdentLevel[THREAD_TYPE] = m_pData->m_IdentLevel[THREAD_TYPE] + 1;
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
void CRTDebug::Leave(const int c, const char* m, const char *file, int line,
										 const char *function)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	if(m_pData->m_IdentLevel[THREAD_TYPE] > 0)
		m_pData->m_IdentLevel[THREAD_TYPE]--;

	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
							<< INDENT_OUTPUT << DBC_CTRACE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Leaving " << function << "()"
							<< ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX
						  << INDENT_OUTPUT
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Leaving " << function << "()" << std::endl;
	}

	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::Return(const int c, const char* m, const char *file, int line,
											const char *function, long result)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
  if(m_pData->m_IdentLevel[THREAD_TYPE] > 0)
		m_pData->m_IdentLevel[THREAD_TYPE]--;

	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
						  << INDENT_OUTPUT << DBC_CTRACE_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Leaving " << function << "() (result 0x"
							<< std::hex << std::setw(8) << std::setfill('0') << result << ", "
							<< std::dec << result << ")" << ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX
							<< INDENT_OUTPUT
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
							<< std::dec << line << ":Leaving " << function << "() (result 0x"
							<< std::hex << std::setw(8) << std::setfill('0') << result << ", "
							<< std::dec << result << ")" << std::endl;
	}

	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::ShowValue(const int c, const char* m, long value, int size, 
												 const char *name, const char *file, long line)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
							<< INDENT_OUTPUT << DBC_REPORT_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = " << value
							<< ", 0x" << std::hex << std::setw(size*2) << std::setfill('0')
							<< value;
	}
	else
	{
		std::cout << THREAD_PREFIX
						  << INDENT_OUTPUT
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

	if(m_pData->m_bHighlighting)
		std::cout << ANSI_ESC_CLR;

	std::cout << std::endl;

	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::ShowPointer(const int c, const char* m, void* pointer,
													 const char* name, const char* file, long line)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
						  << INDENT_OUTPUT << DBC_REPORT_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = ";
	}
	else
	{
		std::cout << THREAD_PREFIX
							<< INDENT_OUTPUT
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = ";
	}

	if(pointer != NULL)
		std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << pointer;
	else
		std::cout << "NULL";

	if(m_pData->m_bHighlighting)
		std::cout << ANSI_ESC_CLR;
	
	std::cout << std::endl;

	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::ShowString(const int c, const char* m, const char* string,
													const char* name, const char* file, long line)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
							<< INDENT_OUTPUT << DBC_REPORT_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = 0x" << std::hex
							<< std::setw(8) << std::setfill('0') << string << " \""
							<< string << "\"" << ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX
							<< INDENT_OUTPUT
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << name << " = 0x" << std::hex
							<< std::setw(8) << std::setfill('0') << string << " \""
							<< string << "\"" << std::endl;
	}

	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::ShowMessage(const int c, const char* m, const char* string,
													 const char* file, long line)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
							<< INDENT_OUTPUT << DBC_REPORT_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << ANSI_ESC_CLR
							<< std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX
							<< INDENT_OUTPUT 
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << std::endl;
	}
	
	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::StartClock(const int c, const char* m, const char* string,
													const char* file, long line)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	// lets get the current time of the day
	#if defined(HAVE_GETTIMEOFDAY)
	struct timeval* tp = &(m_pData->m_TimeMeasure[THREAD_TYPE]);
	if(gettimeofday(tp, NULL) != 0)
		return;

	// convert the timeval in some human readable format
	double starttime = static_cast<double>(tp->tv_sec) + (static_cast<double>(tp->tv_usec)/MICROSEC);
	#elif defined(HAVE_GETTICKCOUNT)
	double starttime = static_cast<double>(GetTickCount()) / MICROSEC;
	#else
	double starttime = 0.0;
	#warning "no supported time measurement function found!"
	#endif

	// now we convert that starttime to something human readable
	struct tm brokentime;
	time_t curTime = tp->tv_sec+(tp->tv_usec/MICROSEC);
	localtime_r(&curTime, &brokentime);
	char buf[10];
	strftime(&buf[0], 10, "%T", &brokentime);
	char formattedTime[40];
	snprintf(&formattedTime[0], 40, "%s'%03d", buf, (int)((starttime-((int)starttime))*1000.0));

	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
							<< INDENT_OUTPUT << DBC_TIMEVAL_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << " started@"
							<< formattedTime << ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX
							<< INDENT_OUTPUT 
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << " started@"
							<< formattedTime << std::endl;
	}
	
	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::StopClock(const int c, const char* m, const char* string,
												 const char* file, long line)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	// lets get the current time of the day
	struct timeval newtp;
	#if defined(HAVE_GETTIMEOFDAY)
	if(gettimeofday(&newtp, NULL) != 0)
		return;
	#elif defined(HAVE_GETTICKCOUNT)
	newtp.tv_sec = GetTickCount();
	newtp.tv_usec = 0;
	#warning "no supported time measurement function found!"
	#endif

	// now we calculate the timedifference
	struct timeval* oldtp = &(m_pData->m_TimeMeasure[THREAD_TYPE]);
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
	time_t curTime = newtp.tv_sec+(newtp.tv_usec/MICROSEC);
	localtime_r(&curTime, &brokentime);
	char buf[10];
	strftime(&buf[0], 10, "%T", &brokentime);
	char formattedTime[40];
	snprintf(&formattedTime[0], 40, "%s'%03d", buf, (int)((stoptime-((int)stoptime))*1000.0));

	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		std::cout << THREAD_PREFIX_COLOR
							<< INDENT_OUTPUT << DBC_TIMEVAL_COLOR
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << " stopped@"
							<< formattedTime << " = " << std::fixed << std::setprecision(3) << difftime << "s"
							<< ANSI_ESC_CLR << std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX
							<< INDENT_OUTPUT 
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << string << " stopped@"
							<< formattedTime << " = " << std::fixed << std::setprecision(3) << difftime << "s"
							<< std::endl;
	}
	
	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::dprintf_header(const int c, const char* m, const char* file,
															long line, const char* fmt, ...)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, file) == false)
		return;
	
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	if(m_pData->m_bHighlighting)
	{
		char *highlight;
		switch(c)
		{
			case DBC_DEBUG:		highlight = DBC_DEBUG_COLOR;		break;
			case DBC_ERROR:		highlight = DBC_ERROR_COLOR;		break;
			case DBC_WARNING:	highlight	= DBC_WARNING_COLOR;	break;
			default:					highlight = ANSI_ESC_FG_WHITE;	break;
		}

		std::cout << THREAD_PREFIX_COLOR
							<< INDENT_OUTPUT << highlight
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << buf << ANSI_ESC_CLR
							<< std::endl;
	}
	else
	{
		std::cout << THREAD_PREFIX
							<< INDENT_OUTPUT
							<< (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
							<< ":" << std::dec << line << ":" << buf
							<< std::endl;
	}

	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
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
void CRTDebug::dprintf(const int c, const char* m, const char* fmt, ...)
{
	// check if we should really output something
	if(m_pData->matchDebugSpec(c, m, NULL) == false)
		return;
	
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

	// lock the output stream
	LOCK_OUTPUTSTREAM;

	// check if the call is issued from a new thread or if this is an already
	// known one for which we have assigned an own ID
	THREAD_ID_CHECK;
	
	#ifdef DEBUG
	if(m_pData->m_bHighlighting)
	{
		switch(c)
		{
			case DBC_DEBUG:		std::cout << DBC_DEBUG_COLOR << "Debug..: "; break;
			case DBC_ERROR:		std::cout << DBC_ERROR_COLOR << "Error..: "; break;
			case DBC_WARNING:	std::cout << DBC_WARNING_COLOR << "Warning: "; break;
			default:					std::cout << ANSI_ESC_FG_WHITE << "Info...: "; break;
		}
	}
	else
	{
		switch(c)
		{
			case DBC_DEBUG:		std::cout << "Debug..: "; break;
			case DBC_ERROR:		std::cout << "Error..: "; break;
			case DBC_WARNING:	std::cout << "Warning: "; break;
			default:					std::cout << "Info...: "; break;
		}
	}
	#else
	if(m_pData->m_bHighlighting)
	{
		switch(c)
		{
			case DBC_DEBUG:		std::cout << DBC_DEBUG_COLOR; break;
			case DBC_ERROR:		std::cout << DBC_ERROR_COLOR; break;
			case DBC_WARNING:	std::cout << DBC_WARNING_COLOR; break;
			default:					std::cout << ANSI_ESC_FG_WHITE; break;
		}
	}
	#endif

	std::cout << buf << ANSI_ESC_CLR << std::endl;

	// unlock the output stream
	UNLOCK_OUTPUTSTREAM;
}

unsigned int CRTDebug::debugClasses() const
{ 
	return m_pData->m_iDebugClasses;
}

unsigned int CRTDebug::debugFlags() const
{ 
	return m_pData->m_iDebugFlags;
}

const char* CRTDebug::debugFiles() const
{
	std::string result;
	std::map<std::string, bool>::iterator it = m_pData->m_DebugFiles.begin();

	while(it != m_pData->m_DebugFiles.end())
	{
		if(it != m_pData->m_DebugFiles.begin())
			result += " ";
		
		if((*it).second == true)
			result += (*it).first;
		else
			result += "!" + (*it).first;

		++it;
	}

	return result.c_str();
}

const char* CRTDebug::debugModules() const
{
	std::string result;
	std::map<std::string, bool>::iterator it = m_pData->m_DebugModules.begin();

	while(it != m_pData->m_DebugModules.end())
	{
		if(it != m_pData->m_DebugModules.begin())
			result += " ";

		if((*it).second == true)
			result += (*it).first;
		else
			result += "!" + (*it).first;

		++it;
	}

	return result.c_str();
}

bool CRTDebug::highlighting() const
{ 
	return m_pData->m_bHighlighting;
}

void CRTDebug::setDebugClass(unsigned int cl)
{ 
	m_pData->m_iDebugClasses |= cl;
}

void CRTDebug::setDebugFlag(unsigned int fl)
{ 
	m_pData->m_iDebugFlags |= fl;
}

void CRTDebug::setDebugFile(const char* filename, bool show)
{
	// convert the C-string to an STL std::string
	std::string token = filename;
	std::transform(token.begin(), 
								 token.end(), 
								 token.begin(), tolower);						
	
	m_pData->m_DebugFiles[token] = show;
}

void CRTDebug::setDebugModule(const char* module, bool show)
{
	// convert the C-string to an STL std::string
	std::string token = module;
	std::transform(token.begin(), 
								 token.end(), 
								 token.begin(), tolower);						
	
	m_pData->m_DebugModules[token] = show;
}

void CRTDebug::clearDebugClass(unsigned int cl)
{ 
	m_pData->m_iDebugClasses &= ~cl;
}

void CRTDebug::clearDebugFlag(unsigned int fl)
{ 
	m_pData->m_iDebugFlags &= ~fl;
}

void CRTDebug::clearDebugFile(const char* filename)
{
	m_pData->m_DebugFiles.erase(filename);
}

void CRTDebug::clearDebugModule(const char* module)
{
	m_pData->m_DebugModules.erase(module);
}

void CRTDebug::setHighlighting(bool on)
{ 
	m_pData->m_bHighlighting = on;
}	

bool CRTDebugPrivate::matchDebugSpec(const int cl, const char* module, const char* file)
{
	bool result = false;

	// first we check if we need to process this debug message or not,
	// depending on the currently set debug level
	if(((m_iDebugClasses & cl)) != 0)
		result = true;

	// now we search through our sourcefileMap and see if we should suppress
	// the output or force it.
	if(file != NULL)
	{
		NameCompare cmp(file);
		std::map<std::string, bool>::iterator iter = find_if(m_DebugFiles.begin(), m_DebugFiles.end(), cmp);
		if(iter != m_DebugFiles.end())
		{
			if((*iter).second == true)
				result = true;
			else
				result = false;
		}
	}

	if(module != NULL)
	{
		std::map<std::string, bool>::iterator it = m_DebugModules.find(module);
		if(it != m_DebugModules.end())
		{
			if((*it).second == true)
				result = true;
			else
				result = false;			
		}
	}

	return result;
}
