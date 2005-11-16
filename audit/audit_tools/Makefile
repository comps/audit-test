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
#  PURPOSE: The default "all" target uses the gcc compiler to compile all
#           test files in the current directory.
#
#           The "clean" target removes all files the end in ".o" (object
#           files).
#
#
#  HISTORY:
#    08/03 Originated by Michael A. Halcrow <mike@halcrow.us>
#
##########################################################################

TOPDIR		= ..
CPPFLAGS	+= -I$(TOPDIR)/include  -I$(TOPDIR)/syscalls/include \
		  -I$(TOPDIR)/trustedprograms/include

all: test_setxattr test_groupadd test_bind

utils:
	make -C ../utils

syscalls_utils:
	make -C ../syscalls/utils

trustedprograms_utils:
	make -C ../trustedprograms/utils

test_setxattr: test_setxattr.o utils syscalls_utils
	$(CC) $(CFLAGS) -o test_setxattr test_setxattr.o \
	$(TOPDIR)/utils/testutils.a \
	$(TOPDIR)/syscalls/utils/syscall_test_utils.a $(CPPFLAGS)

test_setxattr.o: test_setxattr.c $(TOPDIR)/syscalls/include/syscalls.h
	$(CC) $(CFLAGS) -c -o test_setxattr.o test_setxattr.c $(CPPFLAGS)

test_bind: test_bind.o utils syscalls_utils
	$(CC) $(CFLAGS) -o test_bind test_bind.o \
	$(TOPDIR)/utils/testutils.a \
	$(TOPDIR)/syscalls/utils/syscall_test_utils.a $(CPPFLAGS)

test_bind.o: test_bind.c $(TOPDIR)/syscalls/include/syscalls.h
	$(CC) $(CFLAGS) -c -o test_bind.o test_bind.c $(CPPFLAGS)

test_groupadd: test_groupadd.o utils trustedprograms_utils
	$(CC) $(CFLAGS) -o test_groupadd test_groupadd.o \
	$(TOPDIR)/utils/testutils.a \
	$(TOPDIR)/trustedprograms/utils/trustedprogram_test_utils.a $(CPPFLAGS)

test_groupadd.o: test_groupadd.c $(TOPDIR)/trustedprograms/include/trustedprograms.h
	$(CC) $(CFLAGS) -c -o test_groupadd.o test_groupadd.c $(CPPFLAGS)

clean:
	$(RM) *.o

clobber: clean
	$(RM) test_bind test_groupadd test_setxattr

run: all
	$(AUDIT_CLEAN_LOG)
#	./au_login &> ./au_login.run.log
	./au_netlink &> ./au_netlink.run.log
	./au_syscalls &> ./au_syscalls.run.log
	./au_trustedpgms &> ./au_trustedpgms.run.log
	./au_restart &> ./au_restart.run.log

include $(TOPDIR)/rules.mk
