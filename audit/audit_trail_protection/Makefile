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
#  PURPOSE: The default "all" target uses the gcc compiler to compile
#           the audit trail protection test program.
#
#           The "clean" target removes all files the end in ".o" (object
#           files).
#
#
#  HISTORY:
#    09/03 Originated by Michael A. Halcrow <mike@halcrow.us>
#
##########################################################################

TOPDIR		= ..

INCLUDES 	= -I$(TOPDIR)/include

AUDIT_TRAIL_EXE  = audit_trail_protection
AUDIT_TRAIL_OBJS = audit_trail_protection.o

EXE = $(AUDIT_TRAIL_EXE)

all: $(EXE)

$(EXE): $(AUDIT_TRAIL_OBJS)
	$(CC) $(CFLAGS) -o $@ $(AUDIT_TRAIL_OBJS)

clean:
	$(RM) *.o

clobber: clean
	$(RM) $(EXE)

run: all
	./$(EXE) &> ./$(EXE).run.log

include $(TOPDIR)/rules.mk
