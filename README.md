# librtdebug
librtdebug is a small but powerfull debugging purpose library for C++
developers. Its purpose is to provide the developer with some set of
C-language macro definitions which can be easily inserted in all kind of
applications. These macros then call internal class methods of the debugging
library which output the desired data during the execution of the program.

In addition to that it takes care of the correct indention of the output,
providing the developer a nice and easy way to track possible problems due
to execution output at the standard output devices.

Aside from that, it also automatically keeps track if it is called out of
different threads, using different indention levels and other means of
variations to make it easy to recognize if output is comming from the
context of another thread.

## Requirements:
As the library is fully written in C++ you at least require a modern, uptodate
C++ compiler, preferably GCC in its latest 3.4+ version.
In addition the build system is done with the qmake utility of the Qt framework
kit, which raises the requirement for a recent Qt3 version to make it compile
out of the box. However, the code itself doesn't require any Qt classes or
link any of them to the library itself. This allows to use librtdebug also
with non Qt applications.

## Features:
- small and easy macro based interface
- fully object oriented design and implementation using C++
- thread-safe implementation allowing to track the output of a certain thread
- highly portable implementation with direct implementations existing for SunOS, MacOSX and Linux. (more to come)
- Doxygen based class documentation and a deeper implementation documentation based on the MSc thesis of my computer science study.
- Released under LGPL (Lesser General Public License) for a maximum available flexibility and developer support aswell as the possibility to use the library in commercial applications.

## Future plans:
Have a look at the TODO file.

Please feel free to contact me if you have any questions about the
implementation of librtdebug and it's underlying algorithms. But please make
sure that you have checked the whole documentation first.
