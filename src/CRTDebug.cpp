/* vim:set ts=2 nowrap: ****************************************************

 librtdebug - A C++ based thread-safe Runtime Debugging Library
 Copyright (C) 2003-2019 Jens Maus <mail@jens-maus.de>

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

***************************************************************************/

#include "CRTDebug.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cctype>

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "config.h"

#if defined(HAVE_LIBPTHREAD)
#include <pthread.h>
#endif

// define variables for using ANSI colors in our debugging scheme
#define ANSI_ESC_CLR        "\033[0m"
#define ANSI_ESC_BOLD       "\033[1m"
#define ANSI_ESC_UNDERLINE  "\033[4m"
#define ANSI_ESC_BLINK      "\033[5m"
#define ANSI_ESC_REVERSE    "\033[7m"
#define ANSI_ESC_INVISIBLE  "\033[8m"
#define ANSI_ESC_FG_BLACK   "\033[0;30m"
#define ANSI_ESC_FG_RED     "\033[0;31m"
#define ANSI_ESC_FG_GREEN   "\033[0;32m"
#define ANSI_ESC_FG_BROWN   "\033[0;33m"
#define ANSI_ESC_FG_BLUE    "\033[0;34m"
#define ANSI_ESC_FG_PURPLE  "\033[0;35m"
#define ANSI_ESC_FG_CYAN    "\033[0;36m"
#define ANSI_ESC_FG_LGRAY   "\033[0;37m"
#define ANSI_ESC_FG_DGRAY   "\033[1;30m"
#define ANSI_ESC_FG_LRED    "\033[1;31m"
#define ANSI_ESC_FG_LGREEN  "\033[1;32m"
#define ANSI_ESC_FG_YELLOW  "\033[1;33m"
#define ANSI_ESC_FG_LBLUE   "\033[1;34m"
#define ANSI_ESC_FG_LPURPLE "\033[1;35m"
#define ANSI_ESC_FG_LCYAN   "\033[1;36m"
#define ANSI_ESC_FG_WHITE   "\033[1;37m"
#define ANSI_ESC_BG         "\033[0;4"    // background esc-squ start with 4x
#define ANSI_ESC_BG_BLACK   "\033[0;40m"
#define ANSI_ESC_BG_RED     "\033[0;41m"
#define ANSI_ESC_BG_GREEN   "\033[0;42m"
#define ANSI_ESC_BG_BROWN   "\033[0;43m"
#define ANSI_ESC_BG_BLUE    "\033[0;44m"
#define ANSI_ESC_BG_PURPLE  "\033[0;45m"
#define ANSI_ESC_BG_CYAN    "\033[0;46m"
#define ANSI_ESC_BG_LGRAY   "\033[0;47m"

// define the colors for each debug class
#define DBC_CTRACE_COLOR    ANSI_ESC_FG_BROWN
#define DBC_REPORT_COLOR    ANSI_ESC_FG_PURPLE
#define DBC_ASSERT_COLOR    ANSI_ESC_FG_RED
#define DBC_TIMEVAL_COLOR   ANSI_ESC_FG_BLUE
#define DBC_DEBUG_COLOR     ANSI_ESC_FG_GREEN
#define DBC_ERROR_COLOR     ANSI_ESC_FG_RED
#define DBC_WARNING_COLOR   ANSI_ESC_FG_YELLOW

#define PROCESS_ID          m_pData->m_PID
#define PROCESS_WIDTH       5
#define PROCESS_PREFIX      std::setw(PROCESS_WIDTH) << std::setfill(' ') << std::dec << PROCESS_ID

#if defined(HAVE_GETTIMEOFDAY)
#define GET_TIMEINFO(tp) \
  if(gettimeofday((tp), NULL) != 0) \
    return std::cerr
#elif defined(HAVE_GETTICKCOUNT)
#define GET_TIMEINFO(tp) \
  (tp)->tv_sec = GetTickCount() / MILLISEC; \
  (tp)->tv_usec = 0
#else
  #error "no supported time measurement function found!"
#endif

#if defined(HAVE_LOCALTIME_R)
  #define LOCALTIME(tm_time, tt_time) localtime_r((tt_time), (tm_time))
#elif defined(HAVE_LOCALTIME_S)
  #define LOCALTIME(tm_time, tt_time) localtime_s((tm_time), (tt_time))
#elif defined(HAVE_LOCALTIME)
  #define LOCALTIME(tm_time, tt_time) memcpy((tm_time), localtime((tt_time)), sizeof(struct tm))
#else
  #error "no matching localtime() function"
#endif

#if defined(HAVE_STRFTIME)
#define GET_TIMESTR(tp) \
  time_t tt_time = (tp)->tv_sec + ((tp)->tv_usec/MICROSEC); \
  struct tm tm_time; \
  LOCALTIME(&tm_time, &tt_time); \
  char fmtTime[20]; \
  { \
    char fmtBuf[10]; \
    strftime(fmtBuf, sizeof(fmtBuf), "%T", &tm_time); \
    snprintf(fmtTime, sizeof(fmtTime), "%s.%06ld", fmtBuf, (tp)->tv_usec); \
  }
#define TIME_PREFIX       "[" << fmtTime << "] "
#define TIME_PREFIX_COLOR ANSI_ESC_FG_GREEN << TIME_PREFIX
#else
#define GET_TIMESTR(tp) (void(0))
#define TIME_PREFIX
#define TIME_PREFIX_COLOR
#endif

#define UPDATE_TIMEINFO \
  struct timeval newtp; \
  GET_TIMEINFO(&newtp); \
  GET_TIMESTR(&newtp)

// some often used macros to output the thread number at the beginning of each
// debug output so that we know from which thread this output came.
#if defined(HAVE_LIBPTHREAD)

#define THREAD_TYPE         pthread_self()
#define THREAD_ID           m_pData->m_ThreadID[THREAD_TYPE]
#define THREAD_WIDTH        2
#define THREAD_PREFIX       PROCESS_PREFIX << "." << std::setw(THREAD_WIDTH) << std::setfill('0') << std::dec << THREAD_ID << ": "
#define THREAD_PREFIX_COLOR ANSI_ESC_FG_YELLOW << PROCESS_PREFIX << "." << ANSI_ESC_BG << std::dec << THREAD_ID%6 << "m" << \
                            std::setw(THREAD_WIDTH) << std::setfill('0') << std::dec << THREAD_ID << ANSI_ESC_CLR << ": "
#define THREAD_ID_CHECK     if(m_pData->m_ThreadID[THREAD_TYPE] == 0)\
                              m_pData->m_ThreadID[THREAD_TYPE] = ++(m_pData->m_iThreadCount)
#define INDENT_OUTPUT       std::string(m_pData->m_IdentLevel[THREAD_TYPE], ' ')
#define LOCK_OUTPUTSTREAM   pthread_mutex_lock(&(m_pData->m_pCoutMutex))
#define UNLOCK_OUTPUTSTREAM pthread_mutex_unlock(&(m_pData->m_pCoutMutex))

#else

#define THREAD_TYPE         0
#define THREAD_PREFIX       PROCESS_PREFIX << ": "
#define THREAD_PREFIX_COLOR THREAD_PREFIX
#define THREAD_ID_CHECK     (void(0))
#define INDENT_OUTPUT       std::string(m_pData->m_IdentLevel[THREAD_TYPE], ' ')
#define LOCK_OUTPUTSTREAM   (void(0))
#define UNLOCK_OUTPUTSTREAM (void(0))

#warning "no pthread library found/supported. librtdebug is compiled without being thread-safe!"
#endif

// define how MICRO and MILLI are related to normal
#define MILLISEC 1000L    // 10^-3
#define MICROSEC 1000000L // 10^-6

#define STRINGSIZE 10000

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CRTDebugPrivate
{
  // methods
  public:
    bool matchDebugSpec(const int cl, const char* module, const char* file);
    bool matchInfoSpec(const int cl, const char* module, const char* file);

  // data
  public:
    pid_t m_PID;                              //!< process identification number
    std::map<pthread_t, unsigned int>   m_ThreadID;           //!< thread identification number
    std::map<pthread_t, unsigned int>   m_IdentLevel;         //!< different ident levels for different threads
    std::map<pthread_t, struct timeval> m_TimeMeasure;        //!< for measuring the time we need more structs
    bool                                m_bHighlighting;      //!< text ANSI highlighting?
    bool                                m_bDebugMode;         //!< is compile-time debugging enabled
    unsigned int                        m_iDebugClasses;      //!< the currently active debug classes
    std::map<std::string, bool>         m_DebugModules;       //!< to map actual specified debug modules
    std::map<std::string, bool>         m_DebugFiles;         //!< to map actual specified source file names*/
    unsigned int                        m_iDebugFlags;        //!< the currently active debug flags
    unsigned int                        m_iInfoClasses;       //!< the currently active debug classes
    std::map<std::string, bool>         m_InfoModules;        //!< to map actual specified debug modules
    std::map<std::string, bool>         m_InfoFiles;          //!< to map actual specified source file names*/
    unsigned int                        m_iInfoFlags;         //!< the currently active debug flags
    unsigned int                        m_iThreadCount;       //!< counter of total number of threads processing

    #if defined(HAVE_LIBPTHREAD)
    pthread_mutex_t                     m_pCoutMutex;         //!< a mutex to sync cout output
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
void CRTDebug::init(const char* variable, const bool debugMode)
{
  CRTDebug* rtdebug = CRTDebug::instance();

  if(debugMode == true)
    std::cerr << "*** " << PROJECT_LONGNAME << " v" << PROJECT_VERSION << " (" << __DATE__ << ") runtime debugging framework startup ***********" << std::endl;

  // if the user has specified an environment variable we
  // go and parse it accordingly.
  if(variable != NULL)
  {
    if(debugMode == true)
    {
      std::cerr << "*** parsing ENV variable: '" << variable << "' for debug options." << std::endl
                << "*** for tokens: '@' class, '+' flags, '&' name, '%' module" << std::endl
                << "*** --------------------------------------------------------------------------" << std::endl;
    }

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
            static const struct { const char* token; const unsigned int flag; } dbclasses[] =
            {
              { "ctrace", DBC_CTRACE  },
              { "report", DBC_REPORT  },
              { "assert", DBC_ASSERT  },
              { "timeval",DBC_TIMEVAL },
              { "debug",  DBC_DEBUG   },
              { "error",  DBC_ERROR   },
              { "warning",DBC_WARNING },
              { "all",    DBC_ALL     },
              { NULL,     0           }
            };

            for(int i=0; dbclasses[i].token; i++)
            {
              if(strncasecmp(s+1, dbclasses[i].token, strlen(dbclasses[i].token)) == 0)
              {
                if(debugMode == true)
                  std::cerr << "*** @dbclass: " << (!negate ? "show" : "hide") << " '" << dbclasses[i].token << "' output" << std::endl;

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
            static const struct { const char* token; const unsigned int flag; } dbflags[] =
            {
              { "always", DBF_ALWAYS  },
              { "startup",DBF_STARTUP },
              { "all",    DBF_ALL     },
              { NULL,     0           }
            };

            for(int i=0; dbflags[i].token; i++)
            {
              if(strncasecmp(s+1, dbflags[i].token, strlen(dbflags[i].token)) == 0)
              {
                if(debugMode == true)
                  std::cerr << "*** +dbflag.: " << (!negate ? "show" : "hide") << " '" << dbflags[i].token << "' output" << std::endl;

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
            if(debugMode == true)
              std::cerr << "*** &name...: " << (!negate ? "show" : "hide") << " '" << token << "' output" << std::endl;
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
            if(debugMode == true)
              std::cerr << "*** %module.: " << (!negate ? "show" : "hide") << " '" << token << "' output" << std::endl;
          }
          break;

          default:
          {
            if(strncasecmp(s, "ansi", 4) == 0)
            {
              if(debugMode == true)
                std::cerr << "*** switching " << (!negate ? "on" : "off") << " ANSI color output" << std::endl;

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

      if(debugMode == true)
        std::cerr << "*** --------------------------------------------------------------------------" << std::endl;
    }

    if(debugMode == true)
    {
      std::cerr << "*** active debug classes/flags: 0x" << std::setw(8) << std::setfill('0') << std::hex << rtdebug->debugClasses()
                << "/0x" << std::setw(8) << std::setfill('0') << std::hex << rtdebug->debugFlags() << std::dec << std::endl
                << "*** Normal processing follows ************************************************" << std::endl;
    }
  }

  // save compile-time debug mode flag
  rtdebug->m_pData->m_bDebugMode = debugMode;
}

//  Class:       CRTDebug
//  Constructor: CRTDebug
//!
//! Construct a CRTDebug object.
//!
////////////////////////////////////////////////////////////////////////////////
CRTDebug::CRTDebug(const int dbclasses, const int dbflags,
                   const int infoclasses, const int infoflags)
{
  // allocate data from our private instance class
  m_pData = new CRTDebugPrivate();

  // set some default values
  m_pData->m_PID = getpid();
  m_pData->m_bHighlighting = true;
  m_pData->m_iDebugClasses = dbclasses;
  m_pData->m_iDebugFlags = dbflags;
  m_pData->m_iInfoClasses = infoclasses;
  m_pData->m_iInfoFlags = infoflags;
  m_pData->m_iThreadCount = 0;

  #if defined(HAVE_LIBPTHREAD)
  pthread_mutex_init(&(m_pData->m_pCoutMutex), NULL);
  #endif

  // now we see if we have to apply some default settings or not.
  if(m_pData->m_iDebugClasses == 0)
    m_pData->m_iDebugClasses = DBC_ERROR | DBC_DEBUG | DBC_WARNING | DBC_ASSERT | DBC_REPORT | DBC_TIMEVAL;

  if(m_pData->m_iDebugFlags == 0)
    m_pData->m_iDebugFlags = DBF_ALWAYS | DBF_STARTUP;

  // now we see if we have to apply some default settings or not.
  if(m_pData->m_iInfoClasses == 0)
  {
    m_pData->m_iInfoClasses = INC_INFO | INC_WARNING | INC_ERROR | INC_FATAL;
    if(m_pData->m_bDebugMode == true)
      m_pData->m_iInfoClasses |= INC_VERBOSE | INC_DEBUG;
  }

  if(m_pData->m_iInfoFlags == 0)
    m_pData->m_iInfoFlags = INF_ALWAYS | INF_STARTUP;
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

  if(m_pData->m_bDebugMode == true)
    std::cerr << "*** " << PROJECT_LONGNAME << " framework shutdowned *********************************************" << std::endl;
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
std::ostream& CRTDebug::Enter(const int c, const char* m, const char* file, long line,
                              const char* function)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_CTRACE_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
              << std::dec << line << ":Entering " << function << "()"
              << ANSI_ESC_CLR << std::endl;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
              << std::dec << line << ":Entering " << function << "()" << std::endl;
  }

  // increase the indention level
  m_pData->m_IdentLevel[THREAD_TYPE] = m_pData->m_IdentLevel[THREAD_TYPE] + 1;

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
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
std::ostream& CRTDebug::Leave(const int c, const char* m, const char *file, int line,
                              const char *function)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  if(m_pData->m_IdentLevel[THREAD_TYPE] > 0)
    m_pData->m_IdentLevel[THREAD_TYPE]--;

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_CTRACE_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
              << std::dec << line << ":Leaving " << function << "()"
              << ANSI_ESC_CLR << std::endl;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
              << std::dec << line << ":Leaving " << function << "()" << std::endl;
  }

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
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
std::ostream& CRTDebug::Return(const int c, const char* m, const char *file, int line,
                               const char *function, long result)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  if(m_pData->m_IdentLevel[THREAD_TYPE] > 0)
    m_pData->m_IdentLevel[THREAD_TYPE]--;

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_CTRACE_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
              << std::dec << line << ":Leaving " << function << "() (result 0x"
              << std::hex << std::setw(8) << std::setfill('0') << result << ", "
              << std::dec << result << ")" << ANSI_ESC_CLR << std::dec << std::endl;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file) << ":"
              << std::dec << line << ":Leaving " << function << "() (result 0x"
              << std::hex << std::setw(8) << std::setfill('0') << result << ", "
              << std::dec << result << ")" << std::dec << std::endl;
  }

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
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
//! @param       c      the debug class on which this output should be placed
//! @param       value  the variable on which we want to view the value
//! @param       size   the size of the variable so that we can do some hex output
//! @param       name   the name of the variable for our output
//! @param       file   the file name of the source code where we placed SHOWVALUE()
//! @param       line   the line number on which the SHOWVALUE() is.
////////////////////////////////////////////////////////////////////////////////
std::ostream& CRTDebug::ShowValue(const int c, const char* m, long long value, int size,
                                  const char *name, const char *file, long line)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_REPORT_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << name << " = " << value
              << ", 0x" << std::hex << std::setw(size*2) << std::setfill('0')
              << value;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
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
      std::cerr << ", '" << std::hex << std::setw(2) << std::setfill('0') << value
                << "'";
    }
    else
    {
      std::cerr << ", '" << (char)value << "'";
    }
  }

  if(m_pData->m_bHighlighting)
    std::cerr << ANSI_ESC_CLR;

  std::cerr << std::dec << std::endl;

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
}

//  Class:       CRTDebug
//  Method:      ShowPointer
//!
//! Method to give a developer the possibility to view a currently set
//! pointer variable. It outputs the current address of that pointer.
//!
//! It is normally invoked by the SHOWPOINTER() macro.
//!
//! @param       c        the debug class
//! @param       pointer  the pointer variable which we want to output
//! @param       name     the name of the variable
//! @param       file     the file name of the source we have the SHOWPOINTER()
//! @param       line     the line number on which the SHOWPOINTER() is.
////////////////////////////////////////////////////////////////////////////////
std::ostream& CRTDebug::ShowPointer(const int c, const char* m, void* pointer,
                                    const char* name, const char* file, long line)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_REPORT_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << name << " = ";
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << name << " = ";
  }

  if(pointer != NULL)
    std::cerr << "0x" << std::hex << std::setw(8) << std::setfill('0') << pointer;
  else
    std::cerr << "NULL";

  if(m_pData->m_bHighlighting)
    std::cerr << ANSI_ESC_CLR;

  std::cerr << std::dec << std::endl;

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
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
//! @param       c      the debug class
//! @param       string the string to output
//! @param       name   the variable name of the string
//! @param       file   the file name were the SHOWSTRING() is
//! @param       line   the line number on which the SHOWSTRING() is
////////////////////////////////////////////////////////////////////////////////
std::ostream& CRTDebug::ShowString(const int c, const char* m, const char* string,
                                   const char* name, const char* file, long line)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_REPORT_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << name << " = 0x" << std::hex
              << std::setw(8) << std::setfill('0') << string << " \""
              << string << "\"" << ANSI_ESC_CLR << std::dec << std::endl;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << name << " = 0x" << std::hex
              << std::setw(8) << std::setfill('0') << string << " \""
              << string << "\"" << std::dec << std::endl;
  }

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
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
//! @param       c      the debug class
//! @param       string the string to output
//! @param       file   the filename of the source
//! @param       line   the line number where we have placed the SHOWMSG()
////////////////////////////////////////////////////////////////////////////////
std::ostream& CRTDebug::ShowMessage(const int c, const char* m, const char* string,
                                    const char* file, long line)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_REPORT_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << string << ANSI_ESC_CLR
              << std::endl;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << string << std::endl;
  }

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
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
//! @param       c      the debug class
//! @param       string the additional string to identify the clock
//! @param       file   the filename of the source code
//! @param       line   the line number on which we have the STARTCLOCK()
////////////////////////////////////////////////////////////////////////////////
std::ostream& CRTDebug::StartClock(const int c, const char* m, const char* string,
                                   const char* file, long line)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // lets get the current time of the day
  time_t starttime = newtp.tv_sec + (newtp.tv_usec/MICROSEC);

  // now we convert that starttime to something human readable
  struct tm brokentime;
  #if defined(HAVE_LOCALTIME_R)
  localtime_r(&starttime, &brokentime);
  #elif defined(HAVE_LOCALTIME_S)
  localtime_s(&brokentime, &starttime);
  #elif defined(HAVE_LOCALTIME)
  memcpy(&brokentime, localtime(&starttime), sizeof(brokentime));
  #endif
  char buf[10];
  strftime(buf, sizeof(buf), "%T", &brokentime);
  char formattedTime[40];
  snprintf(formattedTime, sizeof(formattedTime), "%s.%06ld", buf, newtp.tv_usec);

  // save time measurement
  memcpy(&(m_pData->m_TimeMeasure[THREAD_TYPE]), &newtp, sizeof(struct timeval));

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_TIMEVAL_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << string << " started@"
              << formattedTime << ANSI_ESC_CLR << std::endl;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << string << " started@"
              << formattedTime << std::endl;
  }

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
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
//! @param       c      the debug class
//! @param       string the additional string to output
//! @param       file   the filename of the source code file
//! @param       line   the line number on which we placed the STOPCLOCK()
////////////////////////////////////////////////////////////////////////////////
std::ostream& CRTDebug::StopClock(const int c, const char* m, const char* string,
                                  const char* file, long line)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // now we calculate the timedifference
  struct timeval* oldtp = &(m_pData->m_TimeMeasure[THREAD_TYPE]);
  struct timeval  difftp;
  #if defined(timersub)
  timersub(&newtp, oldtp, &difftp);
  #else
  difftp.tv_sec = newtp.tv_sec - oldtp->tv_sec;
  difftp.tv_usec = newtp.tv_usec - oldtp->tv_usec;
  if(difftp.tv_usec < 0)
  {
    --difftp.tv_sec;
    difftp.tv_usec += MICROSEC;
  }
  #endif

  // convert the actual and diff time in a human readable format
  time_t stoptime = newtp.tv_sec + (newtp.tv_usec/MICROSEC);
  float difftime = (float)difftp.tv_sec + ((float)difftp.tv_usec/(float)MICROSEC);

  // now we convert that stoptime to something human readable
  struct tm brokentime;
  #if defined(HAVE_LOCALTIME_R)
  localtime_r(&stoptime, &brokentime);
  #elif defined(HAVE_LOCALTIME_S)
  localtime_s(&brokentime, &stoptime);
  #elif defined(HAVE_LOCALTIME)
  memcpy(&brokentime, localtime(&stoptime), sizeof(brokentime));
  #endif
  char buf[10];
  strftime(buf, sizeof(buf), "%T", &brokentime);
  char formattedTime[40];
  snprintf(formattedTime, sizeof(formattedTime), "%s.%06ld", buf, newtp.tv_usec);

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << DBC_TIMEVAL_COLOR
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << string << " stopped@"
              << formattedTime << " = " << std::fixed << std::setprecision(6) << difftime << "s"
              << ANSI_ESC_CLR << std::endl;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << string << " stopped@"
              << formattedTime << " = " << std::fixed << std::setprecision(6) << difftime << "s"
              << std::endl;
  }

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  return std::cerr;
}

//  Class:       CRTDebug
//  Method:      dprintf
//!
//! Autoformatting debug message method which will format a provided formatstring
//! in a printf()-like format and output the resulting string together with the
//! default debugging header.
//!
//! This method is invoked by the D() E() and W() macros to output a simple string
//! within a predefined debugging class and group.
//!
//! @param       c    the debug class on which the output should be done.
//! @param       g    the debug group in which this message belongs.
//! @param       file the filename of the source file.
//! @param       line the line number on which the macro was placed
//! @param       fmt  the format string
//! @param       ...  a vararg list of parameters.
////////////////////////////////////////////////////////////////////////////////
std::ostream& CRTDebug::dprintf(const int c, const char* m, const char* file,
                                long line, const char* fmt, ...)
{
  // check if we should really output something
  if(m_pData->matchDebugSpec(c, m, file) == false)
    return std::cerr;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // now we go and create the output string by using the dynamic
  // vasprintf() function
  va_list args;
  va_start(args, fmt);
  char *buf;
  #if defined(HAVE_VASPRINTF)
  if(vasprintf(&buf, fmt, args) == -1)
    return std::cerr;
  #else
  if((buf = (char *)malloc(STRINGSIZE)) == NULL)
    return std::cerr;
  vsnprintf(buf, STRINGSIZE, fmt, args);
  #endif
  va_end(args);

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  if(m_pData->m_bHighlighting)
  {
    const char *highlight;
    switch(c)
    {
      case DBC_DEBUG:   highlight = DBC_DEBUG_COLOR;    break;
      case DBC_ERROR:   highlight = DBC_ERROR_COLOR;    break;
      case DBC_WARNING: highlight = DBC_WARNING_COLOR;  break;
      default:          highlight = ANSI_ESC_FG_WHITE;  break;
    }

    std::cerr << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << highlight
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << buf << ANSI_ESC_CLR
              << std::endl;
  }
  else
  {
    std::cerr << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":" << buf
              << std::endl;
  }

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  // free the memory vasprintf() allocated for us
  free(buf);

  return std::cerr;
}

//  Class:       CRTDebug
//  Method:      printf
//!
//! Autoformatting debug message method which will format a provided formatstring
//! in a printf()-like format and output the resulting string together with the
//! default debugging header.
//!
//! This method is invoked by the D() E() and W() macros to output a simple string
//! within a predefined debugging class and group.
//!
//! @param       c    the debug class on which the output should be done.
//! @param       g    the debug group in which this message belongs.
//! @param       file the filename of the source file.
//! @param       line the line number on which the macro was placed
//! @param       fmt  the format string
//! @param       ...  a vararg list of parameters.
////////////////////////////////////////////////////////////////////////////////
std::ostream& CRTDebug::printf(const int c, const char* m, const char* file,
                               long line, const char* fmt, ...)
{
  // check if we should really output something
  if(m_pData->matchInfoSpec(c, m, file) == false)
    return std::cout;

  // lock the output stream
  LOCK_OUTPUTSTREAM;

  // update time information
  UPDATE_TIMEINFO;

  // now we go and create the output string by using the dynamic
  // vasprintf() function
  va_list args;
  va_start(args, fmt);
  char *buf;
  #if defined(HAVE_VASPRINTF)
  if(vasprintf(&buf, fmt, args) == -1)
    return std::cout;
  #else
  if((buf = (char *)malloc(STRINGSIZE)) == NULL)
    return std::cout;
  vsnprintf(buf, STRINGSIZE, fmt, args);
  #endif
  va_end(args);

  // check if the call is issued from a new thread or if this is an already
  // known one for which we have assigned an own ID
  THREAD_ID_CHECK;

  // output different prefixes depending on the info class
  const char* highlight;
  const char* prefix;
  std::ostream* stream = nullptr;
  switch(c)
  {
    case INC_DEBUG:   highlight = DBC_DEBUG_COLOR;   prefix = "DEBUG: ";   stream = &std::cerr; break;
    case INC_ERROR:   highlight = DBC_ERROR_COLOR;   prefix = "ERROR: ";   stream = &std::cerr; break;
    case INC_FATAL:   highlight = DBC_ERROR_COLOR;   prefix = "FATAL: ";   stream = &std::cerr; break;
    case INC_WARNING: highlight = DBC_WARNING_COLOR; prefix = "WARNING: "; stream = &std::cerr; break;
    case INC_VERBOSE: highlight = ANSI_ESC_FG_WHITE; prefix = ""; stream = &std::cout; break;
    default:          highlight = ""; prefix = "";  stream = &std::cout; break;
  }

  if(m_pData->m_bHighlighting)
  {
    if(file != NULL)
    {
      *stream << TIME_PREFIX_COLOR
              << THREAD_PREFIX_COLOR
              << INDENT_OUTPUT << highlight
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":"
              << prefix
              << buf << ANSI_ESC_CLR
              << std::endl;
    }
    else
    {
      *stream << highlight
              << prefix
              << buf << ANSI_ESC_CLR 
              << std::endl;
    }
  }
  else
  {
    if(file != NULL)
    {
      *stream << TIME_PREFIX
              << THREAD_PREFIX
              << INDENT_OUTPUT
              << (strrchr(file, '/') ? strrchr(file, '/')+1 : file)
              << ":" << std::dec << line << ":";
    }
    
    *stream << prefix
            << buf
            << std::endl;
  }

  // unlock the output stream
  UNLOCK_OUTPUTSTREAM;

  // free the memory vasprintf() allocated for us
  free(buf);

  // abort anything that follows if this is a Fatal()
  // call
  if(c == INC_FATAL)
  {
    *stream << std::flush;
    abort();
  }

  return *stream;
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

unsigned int CRTDebug::infoClasses() const
{
  return m_pData->m_iInfoClasses;
}

unsigned int CRTDebug::infoFlags() const
{
  return m_pData->m_iInfoFlags;
}

const char* CRTDebug::infoFiles() const
{
  std::string result;
  std::map<std::string, bool>::iterator it = m_pData->m_InfoFiles.begin();

  while(it != m_pData->m_InfoFiles.end())
  {
    if(it != m_pData->m_InfoFiles.begin())
      result += " ";

    if((*it).second == true)
      result += (*it).first;
    else
      result += "!" + (*it).first;

    ++it;
  }

  return result.c_str();
}

const char* CRTDebug::infoModules() const
{
  std::string result;
  std::map<std::string, bool>::iterator it = m_pData->m_InfoModules.begin();

  while(it != m_pData->m_InfoModules.end())
  {
    if(it != m_pData->m_InfoModules.begin())
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

void CRTDebug::setInfoClass(unsigned int cl)
{
  m_pData->m_iInfoClasses |= cl;
}

void CRTDebug::setInfoFlag(unsigned int fl)
{
  m_pData->m_iInfoFlags |= fl;
}

void CRTDebug::setInfoFile(const char* filename, bool show)
{
  // convert the C-string to an STL std::string
  std::string token = filename;
  std::transform(token.begin(),
                 token.end(),
                 token.begin(), tolower);

  m_pData->m_InfoFiles[token] = show;
}

void CRTDebug::setInfoModule(const char* module, bool show)
{
  // convert the C-string to an STL std::string
  std::string token = module;
  std::transform(token.begin(),
                 token.end(),
                 token.begin(), tolower);

  m_pData->m_InfoModules[token] = show;
}

void CRTDebug::clearInfoClass(unsigned int cl)
{
  m_pData->m_iInfoClasses &= ~cl;
}

void CRTDebug::clearInfoFlag(unsigned int fl)
{
  m_pData->m_iInfoFlags &= ~fl;
}

void CRTDebug::clearInfoFile(const char* filename)
{
  m_pData->m_InfoFiles.erase(filename);
}

void CRTDebug::clearInfoModule(const char* module)
{
  m_pData->m_InfoModules.erase(module);
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

bool CRTDebugPrivate::matchInfoSpec(const int cl, const char* module, const char* file)
{
  bool result = false;

  // first we check if we need to process this debug message or not,
  // depending on the currently set debug level
  if(((m_iInfoClasses & cl)) != 0)
    result = true;

  // now we search through our sourcefileMap and see if we should suppress
  // the output or force it.
  if(file != NULL)
  {
    NameCompare cmp(file);
    std::map<std::string, bool>::iterator iter = find_if(m_InfoFiles.begin(), m_InfoFiles.end(), cmp);
    if(iter != m_InfoFiles.end())
    {
      if((*iter).second == true)
        result = true;
      else
        result = false;
    }
  }

  if(module != NULL)
  {
    std::map<std::string, bool>::iterator it = m_InfoModules.find(module);
    if(it != m_InfoModules.end())
    {
      if((*it).second == true)
        result = true;
      else
        result = false;
    }
  }

  return result;
}
