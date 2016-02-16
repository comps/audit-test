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

import sys, os
import argparse
import time
import textwrap
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

class Relevancy():
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
    def __len__(self):
        return len(self.syscalls)

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

    def list(self, arch, bits=None):
        """Iterate over syscalls relevant on given 'arch' with 'bits'.

        This is equivalent to listing all syscalls and applying match().
        """
        for sc in self.syscalls.iterkeys():
            if self.match(sc, arch, bits):
                yield sc

class Generator():
    """Load and look up syscalls from multiple files/architectures.
    """
    def __init__(self):
        # a dict() indexed by syscall name, with each value pointing to another
        # dict(), indexed by arch, where each value is a set() of bits
        self.syscalls = dict()
        # all arch/bits seen during syscall processing
        # - structure same as the values of self.syscalls, for easy comparison
        self.arches = dict()
    def __iter__(self):
        return iter(self.syscalls)
    def __len__(self):
        return len(self.syscalls)

    def loadfile(self, arch, bits, infile):
        """Load a list of newline-separated syscall names from a file.
        """
        with open(infile, 'r') as f:
            # record arch bitnesses for later
            try:
                self.arches[arch].update([bits])
            except KeyError:
                self.arches[arch] = set([bits])
            # add arch bitness for the syscall
            for sc in f:
                sc = sc.strip()
                if not sc in self.syscalls:
                    self.syscalls[sc] = dict()
                try:
                    self.syscalls[sc][arch].update([bits])
                except KeyError:
                    self.syscalls[sc][arch] = set([bits])

    def loaddir(self, indir):
        """Like loadfile, but use directory contents automagically.

        The file names are used as architecture names, with `:' in file name
        delimiting the arch name and bitness.
        """
        for infile in os.listdir(indir):
            try:
                (arch, bits) = infile.split(':')
            except ValueError:
                raise ValueError("file {0} has no or extra `:' in name"
                                 .format(infile))
            if not arch or not bits:
                raise ValueError("arch or bits empty for {0}".format(infile))
            self.loadfile(arch, bits, os.path.join(indir, infile))

    def _relevant(self, relevancy, syscall):
        """Test if the list of relevant / all arches matches relevancy rules.
        """
        relevant = dict()
        for arch, bits in self.arches.iteritems():
            for bit in bits:
                if relevancy.match(syscall, arch, bit):
                    try:
                        relevant[arch].update([bit])
                    except KeyError:
                        relevant[arch] = set([bit])
        return self.syscalls[syscall] == relevant

    def _arches_to_list(self, arches):
        """Given a dict() with set()s of bits as values, return a relevancy
        rules-like comma-separated string.
        """
        rules = list()
        for arch, bits in arches.iteritems():
            for bit in bits:
                rules.append('{0}:{1}'.format(arch, bit))
        return rules

    def _arches_to_list_smart(self, arches):
        """Like _arches_to_list, but take self.arches into consideration - use it
        as the reference for 'all' - as if self.arches defined all in existence.
        """
        # all arches: relevant everywhere
        if arches == self.arches:
            return ['all']

        if sorted(arches.iterkeys()) == sorted(self.arches.iterkeys()):
            # one bitness on all arches: relevant everywhere on that bitness
            if all(len(x) == 1 for x in arches.itervalues()):
                bits = next(arches.itervalues())
                bit = next(iter(bits))
                if all(next(iter(x)) == bit for x in arches.itervalues()):
                    return ['all:{0}'.format(bit)]

            # everywhere except one arch + one bitness: use negation
            diff = [(arch,bit) for arch,bits in self.arches.iteritems() \
                                for bit in bits
                                 if bit not in arches[arch]]
            (darches, dbits) = zip(*diff)
            if len(set(darches)) == 1 and len(set(dbits)) == 1:
                return ['!{0}:{1},all'
                        .format(next(iter(darches)), next(iter(dbits)))]

        # everywhere except one arch: use negation
        if len(arches) == len(self.arches)-1 and len(self.arches) > 2:
            setar = set(arches.iterkeys())
            setsar = set(self.arches.iterkeys())
            if setar.issubset(setsar):
                diff = next(iter(setsar.difference(setar)))
                arsame = self.arches.copy()
                del arsame[diff]
                if arches == arsame:
                    return ['!{0},all'.format(diff)]

        rules = list()

        # matching bits for arch: relevant everywhere on that arch
        arches = arches.copy()
        for arch in list(arches.keys()):
            if arches[arch] == self.arches[arch]:
                rules.append(arch)
                del arches[arch]

        # fallback: just enumerate arch:bits
        rules += self._arches_to_list(arches)
        return rules

    def mkrelevancy(self, oldrel=None, dumb=False):
        """Generate a relevancy ruleset based on loaded arch/bits metadata.

        If old (reference) relevancy is provided, show only the diff.
        With dumb=True, don't do smart arch/bits list optimizations.
        """
        # like str().ljust(), but with tabs + extra space if too long
        def tabljust(text, tcnt, tsize=8):
            tabs = tcnt - int(len(text) / tsize)
            tabs = 0 if tabs < 0 else tabs
            space = ' ' if not tabs else ''
            return text+space+'\t'*tabs

        text = "# relevancy auto-generated on {0}\n#\n"\
               .format(time.strftime("%F %T"))

        tw = textwrap.TextWrapper(initial_indent='#   ',
                                  subsequent_indent='#   ',
                                  break_long_words=False,
                                  break_on_hyphens=False)
        text += "# all architectures:\n"
        text += tw.fill(', '.join(sorted(self.arches.iterkeys())))
        text += "\n"
        text += "# all bitnesses:\n"
        text += tw.fill(', '.join(sorted(self._arches_to_list(self.arches))))
        text += "\n#"

        for syscall, arches in sorted(self.syscalls.iteritems()):
            # skip syscalls exactly matching existing relevancy
            if oldrel and self._relevant(oldrel, syscall):
                continue
            text += "\n{0}".format(tabljust(syscall, 3))
            if dumb:
                text += ','.join(sorted(self._arches_to_list(arches)))
            else:
                text += ','.join(sorted(self._arches_to_list_smart(arches)))

        return text

def parse_args(cmdline):
    def xsplit(arg, sep, cnt):
        res = arg.split(sep, cnt+1)
        if len(res) != cnt+1:
            raise ValueError("wrong `{0}' count, expected {1}, got {2}"
                             .format(sep, cnt, arg.count(sep)))
        return res

    p = argparse.ArgumentParser(prog=cmdline[0])
    subp = p.add_subparsers(dest='subcmd')

    lookup = subp.add_parser('lookup', help="Look up relevancy info from a file")
    desc = """
    When specified multiple times,
      --match does AND between specifications, uses exitval as result,
      --list prints out union of syscalls that match all specifications.
    """
    g = lookup.add_argument_group('Relevancy lookup', desc)
    g.add_argument('--rel', type=str, metavar='<rules>', required=True, help="Input file with relevancy rules")
    x = g.add_mutually_exclusive_group()
    x.add_argument('--match', type=lambda x: xsplit(x, ',', 2), action='append', metavar='<s,a,b>', help="Match syscall `s' against arch `a' / bitness `b'")
    x.add_argument('--list', type=lambda x: xsplit(x, ',', 1), action='append', metavar='<a,b>', help="List all syscalls for arch `a' / bitness `b'")

    gen = subp.add_parser('gen', help="Generate a reference relevancy file")
    desc = """
    When specified multiple times, both --load and --loaddir simply make
    a union of all found syscalls, architectures and bitnesses.
    """
    g = gen.add_argument_group('Relevancy generation', desc)
    g.add_argument('--rel', type=str, metavar='<rules>', help="Use existing relevancy rules, produce diff")
    g.add_argument('--load', type=lambda x: xsplit(x, ',', 2), action='append', metavar='<f,a,b>', help="Load syscalls from file `f' as arch `a' / bitness `b'")
    g.add_argument('--loaddir', type=str, action='append', metavar='<dir>', help="Load syscalls from arch:bits files in `dir'")
    g.add_argument('--dumb', action='store_true', help="Don't do smart arch/bits optimizations")

    (opts, nonopts) = p.parse_known_args(cmdline[1:])

    # additional post-processing: treat unknown option-like arguments before
    # `--' as errors (like getopt)
    for arg in nonopts:
        if arg == '--':
            break
        if arg.startswith('-'):
            raise ValueError("unknown option argument: {0}".format(arg))

    # strip possible leading `--' from non-opt args
    if len(nonopts) > 0 and nonopts[0] == '--':
        nonopts.pop(0)

    return (opts, nonopts)

def main():
    (opts, nonopts) = parse_args(sys.argv)

    if opts.subcmd == 'lookup':
        rel = Relevancy(opts.rel)
        if opts.match:
            if all(rel.match(sc,arch,bits) for sc,arch,bits in opts.match):
                sys.exit(0)
            else:
                sys.exit(2)
        elif opts.list:
            res = set().union(*(rel.list(arch,bits) for arch,bits in opts.list))
            if res:
                print '\n'.join(sorted(res))

    elif opts.subcmd == 'gen':
        gen = Generator()
        if opts.load:
            for f in opts.load:
                (f, arch, bits) = f
                gen.loadfile(arch, bits, f)
        if opts.loaddir:
            for d in opts.loaddir:
                gen.loaddir(d)
        rel = None
        if opts.rel:
            rel = Relevancy(opts.rel)
        if len(gen) > 0:
            print gen.mkrelevancy(rel, opts.dumb)

if __name__ == '__main__':
    main()
