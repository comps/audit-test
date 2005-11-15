##########################################################################
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
#  PURPOSE: The default "all" target will build all targets within this
#           makefile.
#
#           The "clean" target removes all targets and all intermediate
#           files.
#
#
#  HISTORY:
#    08/03 originated by Tom Lendacky (toml@us.ibm.com)
#
##########################################################################

TOPDIR		= ../..
TESTDIR		= ..

TESTS_AR	= filter_conf_tests.a
TESTS_OBJS	= test_int_cmp.o	\
		  test_str_cmp.o	\
		  test_syscall_attrs.o	\
		  test_process_attrs.o	\
		  test_file_attrs.o	\
		  test_socket_attrs.o	\
		  test_netlink_attrs.o	\
		  test_user_msg.o	\
		  test_login.o

ALL_OBJS	= $(TESTS_OBJS)

INCLUDES	= -I$(TOPDIR)/include -I$(TESTDIR)/include

all: deps $(TESTS_AR)

$(TESTS_AR): $(TESTS_OBJS)
	$(AR) rc $@ $(TESTS_OBJS)

clean:
	$(RM) -r .deps
	$(RM) *.a *.o

include $(TOPDIR)/rules.mk
