#!/usr/bin/python
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
# AUTHOR: Jiri Jaburek <jjaburek@redhat.com>
#
# This is a parser for the 'syscall relevancy' file, which specifies relevant
# (existing) syscalls on various architectures. Based on such info, this parser
# prints out (to stdout) a list of syscall names that are relevant to the
# current architecture.
#
# For more, see docs/syscall-relevancy.txt.
#

import sys
from fnmatch import fnmatch

#
# helpers functions
#

# fatal error
def syntaxerr(msg):
    print >> sys.stderr, "syntax error on line %d:" % linenr, msg
    sys.exit(2)

# strip whitespaces, cut off comments
def sanitize_line(line):
    return line.strip().split('#')[0]

#
# parsing (recursive) functions
#

def parse_archspec(tok):
    tok = tok.split(':', 2)
    if len(tok) > 2:
        syntaxerr("unexpected extra %s" % '`:\'')

    arch = bits = None
    if len(tok) == 2:
        (arch, bits) = tok
    else:
        (arch,) = tok

    # matching
    if fnmatch(in_arch, arch) or arch == 'all':
        if bits:
            return (bits == in_mode)
        else:
            return True
    else:
        return False

def parse_neg(tok):
    if tok and tok[0] == '!':
        return (tok[1:], True)
    return (tok, False)

def parse_archlist(toklist, aliases):
    for tok in toklist.split(','):
        # negation
        (tok, neg) = parse_neg(tok)

        if not tok:
            syntaxerr("empty archspec")

        # if archspec is alias, recurse into it, if it matches, stop
        if aliases.has_key(tok):
            (match, verdict) = parse_archlist(aliases[tok], aliases)
            if match:
                return (match, (verdict ^ neg))
            continue

        # if archspec matches current arch, stop
        if parse_archspec(tok):
            return (True, not neg)

    return (False, False)

def parse_line_syscall(line, aliases):
    # line with only syscall - not relevant anywhere, skip
    if len(line) == 1:
        return
    if len(line) > 2:
        syntaxerr("syscall definition has >2 columns")

    (syscall, archlist) = line

    # if the syscall matched the archlist *and* is relevant
    # (not matched -> exclude, matched negative archspec -> exclude)
    (matched, verdict) = parse_archlist(archlist, aliases)
    if matched and verdict == True:
        print syscall

def parse_line_alias(line):
    # line with only word ('alias')
    if len(line) == 1:
        syntaxerr("missing alias name")
    if len(line) > 3:
        syntaxerr("alias definition >3 columns")

    (name, archlist) = line[1:]
    return dict(((name, archlist),))

def parse_line(line):
    if not hasattr(parse_line, "aliases"):
        parse_line.aliases = {}

    line = sanitize_line(line)
    # line without any useful content
    if not line:
        return

    # len(line) > 0 due to ^^^
    line = line.split(None, 3)

    if line[0] == 'alias':
        alias = parse_line_alias(line)
        parse_line.aliases.update(alias)
    else:
        parse_line_syscall(line, parse_line.aliases)

#
# main
#

if len(sys.argv) < 4:
    print >> sys.stderr, "usage: %s <filename> <arch> <mode>" % sys.argv[0]
    sys.exit(2)

(in_file, in_arch, in_mode) = sys.argv[1:]
linenr = 0

with open(in_file, 'r') as f:
    for line in f:
        linenr += 1
        parse_line(line)
