#!/usr/bin/perl
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
#  FILE       : au_params.pm
#
#  PURPOSE    : Global augrep test parameters
#
#  HISTORY    :
#    09/2003 Originated by Michael A. Halcrow <mike@halcrow.us>

package au_params;

use strict;

# MH: Make locations parameterizable
sub filter_dot_conf_fullpath { "/etc/audit/filter.conf"; }
sub filter_dot_conf_backup_fullpath { "/etc/audit/filter.conf.bak"; }
sub aucat_executable { "/usr/sbin/aucat"; }
sub augrep_executable { "/usr/sbin/augrep"; }
sub audit_log_fullpath { "/var/log/audit.d/bin.0"; }
sub audit_logs_fullpath { "/var/log/audit.d/bin.*"; }
sub audit_log_link_fullpath { "/var/log/audit"; }
sub auditd_executable { "/sbin/auditd"; }
sub debug { 5; }

1;
