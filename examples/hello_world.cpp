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

/*
 * This is a small hello world example on how to use librtdebug which
 * should demonstrate how easy it is to integrate rtdebug into everyone's
 * projects. By using just a small set of preprocessor macros (defined in
 * rtdebug.h) a user can easily setup his program for outputting very
 * detailed and sophisticated debugging output.
 *
 * And by using an environment variable, this output is then even dynamically 
 * selectable upon startup of the application.
 *
 * So all I can say it, use "rtdebug" and have a happy debugging session time!
 *
 * cheers,
 * jens
 *
 */

#include <iostream>

// rtdebug main include which will define all our
// preprocessor macros
#include <rtdebug.h>

using namespace std;

// prototypes
void output(const char* text);

// main function
int main(int argc, char* argv[])
{
	int returnCode = EXIT_SUCCESS; // return no error on default

	// for being able to filter the debugging output via
	// an environment variable, a user can use ::init() to
	// define that ENV-variable.
	#if defined(DEBUG)
	CRTDebug::init("hello_world");
	#endif

	// we delay the first function entry marker because of ::init()
	ENTER();

	// we measure the time of the execution from here on
	STARTCLOCK("output() measurement");

	// now we branch into the output() function
	output("Hello to rtDebug!");

	// signal that we are finished
	STOPCLOCK("output() measurement");

	// before we really exit the main function we have to
	// use RETURN() to report the return value to the debug framework
	RETURN(returnCode);

	// for properly cleaning up the debug environment we have to
	// call ::destroy() as the very last method of CRTDebug
	#if defined(DEBUG)
	CRTDebug::destroy();
	#endif

	return returnCode;
}

void output(const char* text)
{
	ENTER();
	bool result = true;

	// for debug purposes we can output the content of a c string
	// via SHOWSTRING()
	SHOWSTRING(text);

	cout << text << endl;

	LEAVE();
}
