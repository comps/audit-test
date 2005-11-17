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
CPPFLAGS 	+= -I$(TOPDIR)/include -I$(UTILSDIR)/include

UTILS_AR	= testutils.a
UTILS_OBJ	= core/context.o		\
		  core/create_temp_file.o	\
		  core/create_temp_file_name.o	\
		  core/create_temp_user_name.o	\
		  core/create_temp_group_name.o	\
		  core/create_temp_dir.o	\
		  core/encode_syscall_args.o	\
		  core/file_utils.o		\
		  core/get_identifiers.o	\
		  core/get_laus_data.o		\
		  core/get_login_uid.o		\
		  core/get_pid.o		\
		  core/identifiers.o		\
		  core/mysprintf.o		\
		  core/run.o			\
		  core/verify.o

LAF_OBJ		= laf/laf_clear.o		\
		  laf/laf_config.o		\
		  laf/laf_read.o		\
		  laf/laf_reload.o		\
		  laf/laf_start.o		\
		  laf/laf_stop.o		\
		  laf/laf_verify.o

SEM_EXE		= semaphore_poster
SEM_EXE2	= semaphore_poster2
SEM_OBJ		= bin/semaphore_poster.o

RC_EXE		= rc_return
RC_OBJ		= bin/rc_return.o

PTS_EXE		= get_pts
PTS_OBJ		= bin/get_pts.o
PTS_LINK	= /tmp/$(PTS_EXE)

EXIT_EXE	= exit_with_pid_status
EXIT_OBJ	= bin/exit_with_pid_status.o

ALL_OBJ		= $(UTILS_OBJ) $(LAF_OBJ) $(SEM_OBJ) $(RC_OBJ) $(PTS_OBJ) $(EXIT_OBJ)
ALL_EXE		= $(SEM_EXE) $(SEM_EXE2) $(RC_EXE) $(PTS_EXE) $(PTS_LINK) $(EXIT_EXE)
ALL_AR		= $(UTILS_AR)

all: deps $(ALL_AR) $(ALL_EXE)

$(UTILS_AR): $(UTILS_OBJ) $(LAF_OBJ)
	$(LINK_AR)

$(SEM_EXE): $(SEM_OBJ)
	$(LINK_EXE)

$(SEM_EXE2): $(SEM_EXE)
	cp $(SEM_EXE) $(SEM_EXE2)
	chmod 700 $(SEM_EXE2)

$(EXIT_EXE): $(EXIT_OBJ)
	$(LINK_EXE)

$(RC_EXE): $(RC_OBJ)
	$(LINK_EXE)

$(PTS_EXE): $(PTS_OBJ)
	$(LINK_EXE)

.PHONY: $(PTS_LINK)
$(PTS_LINK):
	-ln -sfn `pwd`/$(PTS_EXE) $@

include $(TOPDIR)/rules.mk
