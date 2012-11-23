###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

TOPDIR		= ../..
UTILSDIR	= ..
CPPFLAGS	+= -I$(UTILSDIR)/include

SRVR_EXE        = lblnet_tst_server

ALL_EXE		= $(SRVR_EXE)

include $(TOPDIR)/rules.mk

$(SRVR_EXE): LDLIBS += -lselinux

all: $(ALL_EXE)

install: $(SRVR_EXE)
	install -o root -g root -m 0644 lblnet_tst-tcp /etc/xinetd.d

clobber:
	$(RM) $(ALL_EXE)
