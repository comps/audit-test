#!/usr/bin/env bash
#
#   Copyright (c) 2010 International Business Machines Corp.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#


TOPDIR		= ..

include $(TOPDIR)/rules.mk

clean:
	for machine in $$(virsh list --all 2>&1 \
				| grep KVM-Guest- | awk '{print $$2}'); \
		do virsh destroy $$machine; virsh undefine $$machine; \
	done
	for machine in $$(virsh net-list --all 2>&1 \
				| grep KVM-Guest- | awk '{print $$1}'); \
		do virsh net-destroy $$machine; virsh net-undefine $$machine; \
	done
