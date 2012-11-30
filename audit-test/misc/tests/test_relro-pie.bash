#!/bin/bash
###############################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
#   and conditions of the GNU General Public License version 2.
#
#   This program is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
###############################################################################
#
# Test to exercise RELRO and PIE
#
# AUTHOR: Eduard Benes <ebenes@redhat.com>
#
# DESCRIPTION:
#   Simple test for RELRO, which happens to be a PIE too, but that's only
#   because this kind of example has to be in PIC code to make RELRO relevant,
#   and PIE makes it simpler to write a standalone one-file test than writing
#   a DSO. Refer to relro.c for more details.
#
# Test with RELRO should fail:
#   $ gcc -pie -fPIE -g  -Wl,-z,relro -o relro relro.c
#   $ ./relro
#   Segmentation fault (core dumped)
#
# Test without RELRO should pass:
#   $ gcc -pie -fPIE -g  -Wl,-z,norelro -o no-relro relro.c
#   $ ./no-relro
#

source testcase.bash || exit 2

prepend_cleanup  "rm -f ./relro ./no-relro"

#### main ####

# be verbose
set -x

[ -r relro.c ] || exit_error "Unable to read source code file "

# Good case
/usr/bin/gcc -pie -fPIE -g  -Wl,-z,relro -o relro relro.c || \
    exit_error "Failed to build test program"
./relro
[ ! $? -eq 139 ] && exit_fail "Test is expected to crash with segmentation fault"

# Bad case
/usr/bin/gcc -pie -fPIE -g  -Wl,-z,norelro -o no-relro relro.c || \
    exit_error "Failed to build test program"
./no-relro || exit_fail "Test is expected to pass without RELRO"

exit_pass
