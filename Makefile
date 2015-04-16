#/* vim:set ts=2 nowrap: ****************************************************
#
# librtdebug - A C++ based thread-safe Runtime Debugging Library
# Copyright (C) 2003-2015 Jens Maus <mail@jens-maus.de>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id: README 3 2005-01-27 15:21:21Z langner $
#
#***************************************************************************/
#
# Top-level makefile for creating the proper 'build' directory tree
# and running the proper cmake command within
#
# Example:
#
# # to explicitly compile for Windows-64bit
# > make OS=w64
#
# # to compile for Linux-64bit but without debugging
# > make OS=l32 DEBUG=
#

#############################################
# find out the HOST operating system
# on which this makefile is run
HOST ?= $(shell uname)
CPU ?= $(shell uname -m)

# if no host is identifed (no uname tool)
# we assume a Linux-64bit build
ifeq ($(HOST),)
  HOST = Linux
endif

# identify CPU
ifeq ($(CPU), x86_64)
  HOST := $(HOST)64
else
ifeq ($(CPU), i686)
  HOST := $(HOST)32
endif
endif

#############################################
# now we find out the target OS for
# which we are going to compile in case
# the caller didn't yet define OS himself
ifndef (OS)
  ifeq ($(HOST), Linux64)
    OS = l64
  else
  ifeq ($(HOST), Linux32)
    OS = l32
  else
  ifeq ($(HOST), Windows64)
    OS = w64
  else
  ifeq ($(HOST), Windows32)
    OS = w32
  else
  ifeq ($(HOST), Darwin64)
    OS = m64
  else
  ifeq ($(HOST), Darwin32)
    OS = m32
  endif
  endif
  endif
  endif
  endif
  endif
endif

#############################################
# define common commands we use in this
# makefile. Please note that each of them
# might be overridden on the commandline.

# common commands
MAKE    = make
CMAKE   = cmake
RM      = rm -f
RMDIR   = rm -rf
MKDIR   = mkdir -p

# Common Directories
PREFIX    = ./
BUILDDIR  = $(PREFIX)build-$(OS)
MXEDIR    = /usr/local/mxe

#############################################
# lets identify if we are going to cross
# compile and if so we add some options to
# the cmake call
ifeq ($(OS), w64)
  ##############################
  # Windows 64-bit static
  ifneq ($(HOST), Windows64)
    TARGET_PATH = $(MXEDIR)/usr/x86_64-w64-mingw32.shared
  endif
endif

ifeq ($(OS), w64s)
  ##############################
  # Windows 64-bit static
  ifneq ($(HOST), Windows64)
    TARGET_PATH = $(MXEDIR)/usr/x86_64-w64-mingw32.static
  endif
endif

ifeq ($(OS), w32)
  ##############################
  # Windows 32-bit static
  ifneq ($(HOST), Windows64)
    TARGET_PATH = $(MXEDIR)/usr/i686-w64-mingw32.shared
  endif
endif

ifeq ($(OS), w32s)
  ##############################
  # Windows 32-bit static
  ifneq ($(HOST), Windows64)
    TARGET_PATH = $(MXEDIR)/usr/i686-w64-mingw32.static
  endif
endif

# depending on TARGET_PATH we enable cross compiling 
# for cmake or not
ifneq ($(TARGET_PATH),)
  CMAKE_OPTIONS := -DCMAKE_TOOLCHAIN_FILE=$(TARGET_PATH)/share/cmake/mxe-conf.cmake -DCROSS_OS=$(OS)
endif

###################
# main target
.PHONY: all
all: $(BUILDDIR) $(BUILDDIR)/Makefile build

# make the object directories
.NOTPARALLEL: $(BUILDDIR)
$(BUILDDIR):
	@echo "  MKDIR $@"
	@$(MKDIR) $(BUILDDIR)

.NOTPARALLEL: $(BUILDDIR)/Makefile
$(BUILDDIR)/Makefile:
	@echo "  CMAKE $@"
	@(cd $(BUILDDIR) ; $(CMAKE) $(CMAKE_OPTIONS) ..)

.PHONY: build
.NOTPARALLEL: build
build:
	@echo "  MAKE $(BUILDDIR)"
	@$(MAKE) -C $(BUILDDIR)

.PHONY: install
.NOTPARALLEL: install
install:
	@echo "  INSTALL $(BUILDDIR)"
	@$(MAKE) -C $(BUILDDIR) install

# cleanup target
.PHONY: clean
clean:
	@echo "  CLEAN"
	@$(MAKE) -C $(BUILDDIR) clean

.PHONY: cleanall
cleanall:
	@echo "  CLEANALL"
	@$(RMDIR) $(BUILDDIR)/*

# clean all stuff, including our autotools
.PHONY: distclean
distclean:
	@echo "  DISTCLEAN"
	@$(RMDIR) $(PREFIX)build-*
