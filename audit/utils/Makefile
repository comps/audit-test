#########################################################################
#   Copyright (C) International Business Machines  Corp., 2003
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#
#
#  FILE   : Makefile
#
#  PURPOSE: The default "all" target uses the gcc compiler to compile all
#           files in the current directory that end in ".c" (source code).
#
#           The "clean" target removes all files the end in ".o" (object
#           files).
#
#
#  HISTORY:
#    06/03 originated by Dustin Kirkland (k1rkland@us.ibm.com
#    08/03 Updated by Daniel H. Jones <danjones@us.ibm.com>
#    09/03 Updated by Michael A. Halcrow <mike@halcrow.us>
#    05/04 Updated by Daniel H. Jones <danjones@us.ibm.com>
#
##########################################################################

TOPDIR		= ..
UTILSDIR	= .
CC		= gcc
FLAGS		+= $(CFLAGS)
INCLUDES 	= -I$(TOPDIR)/include -I$(UTILSDIR)/include

AR		= ar
RANLIB		= ranlib

UTILS_AR	= laus_test_utils.a
UTILS_OBJS	= create_dir.o			\
		  create_file.o			\
		  create_temp_file.o		\
		  create_temp_file_name.o	\
		  create_temp_user_name.o	\
		  create_temp_group_name.o	\
		  create_temp_dir.o		\
		  find_next_pts.o		\
		  get_identifiers.o		\
		  get_laus_data.o		\
		  get_login_uid.o		\
		  init_globals.o		\
		  mysprintf.o			\
		  file_utils.o			\
		  run.o				\
		  get_pid.o

LAF_OBJS	= laf/laf_clear.o		\
		  laf/laf_config.o		\
		  laf/laf_read.o		\
		  laf/laf_reload.o		\
		  laf/laf_start.o		\
		  laf/laf_stop.o		\
		  laf/laf_verify.o

LAUS_OBJS	= laus/laus_args.o		\
		  laus/laus_clear.o		\
		  laus/laus_config.o		\
		  laus/laus_read.o		\
		  laus/laus_reload.o		\
		  laus/laus_start.o		\
		  laus/laus_stop.o		\
		  laus/laus_verify.o

SEM_EXE		= semaphore_poster
SEM_EXE2	= semaphore_poster2
SEM_OBJS	= semaphore_poster.o

RC_EXE		= rc_return
RC_OBJS		= rc_return.o

PTS_EXE		= get_pts
PTS_OBJS	= get_pts.o
PTS_LINK	= /tmp/$(PTS_EXE)

EXIT_EXE	= exit_with_pid_status
EXIT_OBJS	= exit_with_pid_status.o

all: deps $(UTILS_AR) $(SEM_EXE) $(RC_EXE) $(PTS_EXE) $(AUDIT_TRAIL_EXE) $(EXIT_EXE)

$(UTILS_AR): $(UTILS_OBJS) $(LAF_OBJS) $(LAUS_OBJS)
	$(AR) rc $@ $(UTILS_OBJS) $(LAF_OBJS) $(LAUS_OBJS)
	$(RANLIB) $@

$(SEM_EXE): $(SEM_OBJS)
	$(CC) $(FLAGS) $(ARCH) -o $@ $(SEM_OBJS)
	cp $(SEM_EXE) $(SEM_EXE2)
	chmod 700 $(SEM_EXE2)

$(EXIT_EXE): $(EXIT_OBJS)
	$(CC) $(FLAGS) $(ARCH) -o $@ $(EXIT_OBJS)

$(RC_EXE): $(RC_OBJS)
	$(CC) $(FLAGS) $(ARCH) -o $@ $(RC_OBJS)

$(PTS_EXE): $(PTS_OBJS)
	$(CC) $(FLAGS) $(ARCH) -o $@ $(PTS_OBJS)
	ln -sf `pwd`/$@ $(PTS_LINK) 

clean:
	$(RM) -r .deps
	$(RM) $(UTILS_OBJS) $(LAF_OBJS) $(LAUS_OBJS)

clobber: clean
	$(RM) $(UTILS_AR) $(SEM_EXE) $(SEM_EXE2) $(RC_EXE) \
	$(PTS_LINK) $(PTS_EXE) $(AUDIT_TRAIL_EXE) $(EXIT_EXE)

include $(TOPDIR)/rules.mk

ifneq (,$(findstring ppc, $(MACHINE)))
	INCLUDES += -I/usr/include
endif


