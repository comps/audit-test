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
# (existing) syscalls on various architectures.
#
# For more, see docs/syscall-relevancy.txt.
#

# This parser uses non-recursive alias resolution - it unwraps aliases on each
# archlist during processing and thus never needs to resolve more than one level
# of nested aliases - this also takes care of the alias-aliasing-itself infinite
# recursion as it never happens - the alias name for itself won't be resolved as
# an alias and it's thus left intact as an archspec name.

import sys
from fnmatch import fnmatchcase
from collections import namedtuple

# syscall relevancy "data types" (refer to docs/, syntax desc.)
# - ArchList / ArchListItem / ArchSpec separation is because of nesting
#   (resolved aliases), ie. arch1,!(arch2,!arch3),!arch4

#comma-separated list of archspecs (or nested archlists)
ArchList = list
ArchListItem = namedtuple('ArchListItem', ['neg', 'archspec'])
#arch/bitness specification itself
ArchSpec = namedtuple('ArchSpec', ['arch', 'bits'])

# for passing match metadata
MatchTarget = namedtuple('MatchTarget', ['syscall', 'arch', 'bits'])

class Parser():
    """Parse an existing relevancy file and match against its rules.

    To use, first parse() an input relevancy file, then match() or filter()
    against the loaded rules.

    Compared to an unprocessed relevancy file, this class doesn't store
    aliases - it instead resolves them at parse-time as nested lists.
    """
    def __init__(self, infile=None):
        self.lineno = 0
        self.syscalls = dict()
        if infile:
            self.parse(infile)
    def __iter__(self):
        return iter(self.syscalls)

    def _synerr(self, msg=None):
        """Syntax error handling wrapper.
        """
        line = "syntax error on line {0}".format(self.lineno)
        if msg:
            line += ": {0}".format(msg)
        raise AssertionError(line)

    def _parse_archspec(self, spec):
        spec = spec.split(':', 1)
        arch = spec[0]
        bits = spec[1] if len(spec) > 1 else None
        return ArchSpec(arch, bits)

    def _parse_neg(self, tok):
        if tok and tok[0] == '!':
            return (tok[1:], True)
        return (tok, False)

    def _parse_archlist(self, aliases, archstr):
        for archspec in archstr.split(','):
            # negation sign - outside of archspec
            (archspec, neg) = self._parse_neg(archspec)
            # is an alias, use its target value instead
            if archspec in aliases:
                archspec = aliases[archspec]
            else:
                archspec = self._parse_archspec(archspec)
            yield ArchListItem(neg, archspec)

    def parse(self, infile):
        """Parse an input file using the syscall relevancy definitions syntax
        and create an internal representation of the (parsed) rules.
        """
        aliases = dict()
        with open(infile, 'r') as f:
            for lineno, line in enumerate(f):
                self.lineno = lineno+1;

                # sanitize, remove comments
                line = line.strip().split('#')[0]
                if not line:
                    continue

                line = line.split(None, 3)

                # if it's an alias, store it for later
                if line[0] == 'alias':
                    if len(line) != 3:
                        self._synerr("missing/extra columns for alias")
                    (name, archlist) = line[1:]
                    aliases[name] = \
                        ArchList(self._parse_archlist(aliases, archlist))
                    continue

                if len(line) < 2:
                    # valid - just syscall name specified, skip it - it won't
                    # appear in the result & will be treated as not relevant
                    continue
                elif len(line) > 2:
                    self._synerr("unexpected extra column(s)")

                (syscall, archlist) = line
                self.syscalls[syscall] = \
                    ArchList(self._parse_archlist(aliases, archlist))

    def _match_arch(self, name, pattern):
        if not name:
            return False
        if pattern == 'all':
            return True
        return fnmatchcase(name, pattern)

    def _match_archspec(self, target, archspec):
        if not self._match_arch(target.arch, archspec.arch):
            return False
        if archspec.bits:
            return fnmatchcase(target.bits, archspec.bits)
        return True

    def _match_archlistitem(self, target, archlist):
        for architem in archlist:
            (neg, archspec) = architem
            if isinstance(archspec, ArchList):
                (match, verdict) = self._match_archlistitem(target, archspec)
                if match:
                    return (True, neg ^ verdict)
            else:
                match = self._match_archspec(target, archspec)
                if match:
                    return (True, not neg)  # technically neg ^ match
        return (False, False)

    def match(self, syscall, arch, bits=None):
        """Match a syscall/arch/bits against a previously-parsed ruleset.

        If 'bits' is not specified, return True only if all bit variations
        of the given 'arch' are relevant, eg. if the rules don't specify
        bitness either.
        """
        rules = self.syscalls.get(syscall)
        if not rules:
            return False
        target = MatchTarget(syscall, arch, bits)
        (match, verdict) = self._match_archlistitem(target, rules)
        return True if match and verdict else False

    def filter(self, arch, bits=None):
        """Iterate over syscalls relevant on given 'arch' with 'bits'.

        This is equivalent to listing all syscalls and applying match().
        """
        for sc in self.syscalls.iterkeys():
            if self.match(sc, arch, bits):
                yield sc

def main():
    #from pprint import PrettyPrinter
    #pp = PrettyPrinter(indent=4)

    if len(sys.argv) < 4:
        print >> sys.stderr, \
            "usage: {0} <filename> <arch> <mode>".format(sys.argv[0])
        sys.exit(2)
    (infile, arch, mode) = sys.argv[1:]
    p = Parser(infile)
    print '\n'.join(p.filter(arch, mode))

if __name__ == '__main__':
    main()
