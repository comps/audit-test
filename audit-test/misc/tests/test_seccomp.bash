#!/bin/bash
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

source testcase.bash || exit 2

set -x

AUDIT_MARK=
audit_mark()
{
	AUDIT_MARK=$(get_audit_mark)
}
audit_check()
{
	augrok -q --seek="$AUDIT_MARK" type==SECCOMP || \
		exit_fail "missing SECCOMP audit entry"
}

exitval=
retval=
errno=
success=
scerrno=
update_results()
{
	exitval=$?

	[ "$exitval" -eq 1 ] && exit_error "seccomp tester terminated with 1"

	# seccomp.c outputs syscall info in a space-separated array,
	# these are the index value/name mappings (from seccomp.c help):
	local arr=( $@ )
	retval=${arr[0]}
	errno=${arr[1]}
	success=${arr[2]}
	scerrno=${arr[3]}
}


# compiled seccomp binary not found/executable
[ -x "./seccomp" ] || exit_error "./seccomp not found/executable"


# loop over the available syscalls, running all tests on each
for syscall in dup2 open
do

# sanity check (no rules, ALLOW default action)
# - execute syscall - should succeed
#   (any action except "allow" uses ALLOW default action)
out=$(./seccomp $syscall trap none none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_error


#
# basic matching/nonmatching action testing, with and without arguments,
# each action has 6 cases:
#  - matching rule, no rule argument
#  - matching rule, matching argument
#  - matching rule, not matching argument
#  - not matching rule, no rule argument
#  - not matching rule, matching argument
#  - not matching rule, not matching argument
#

#
# SECCOMP_RET_KILL
#

# (159 = 128 + SIGSYS(31))

# KILL, case 1: rule matches syscall #NR, no argument matching done
#  - seccomp should kill the process, syscall should not succeed,
#    tester should not return any output
audit_mark
out=$(./seccomp $syscall kill match none none)
update_results "$out"
audit_check
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# KILL, case 2: rule matches syscall #NR and rule argument matches the syscall
#  - seccomp should kill the process, syscall should not succeed,
#    tester should not return any output
audit_mark
out=$(./seccomp $syscall kill match match none)
update_results "$out"
audit_check
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# KILL, case 3: rule matches syscall #NR and rule argument does not match
#               the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall kill match mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# KILL, case 4: rule doesn't match syscall #NR, no argument matching done
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall kill mismatch none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# KILL, case 5: rule doesn't match syscall #NR and rule argument matches
#		the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall kill mismatch match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# KILL, case 6: rule doesn't match syscall #NR and rule argument does not match
#		the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall kill mismatch mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail


#
# SECCOMP_RET_TRAP
#

# TRAP, case 1: rule matches syscall #NR, no argument matching done
#  - seccomp should send SIGSYS to the process, syscall should not succeed
out=$(./seccomp $syscall trap match none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 2 \
 -a "$success" -eq 0 ] || exit_fail

# TRAP, case 2: rule matches syscall #NR and rule argument matches the syscall
#  - seccomp should send SIGSYS to the process, syscall should not succeed
out=$(./seccomp $syscall trap match match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 2 \
 -a "$success" -eq 0 ] || exit_fail

# TRAP, case 3: rule matches syscall #NR and rule argument does not match
#               the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall trap match mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# TRAP, case 4: rule doesn't match syscall #NR, no argument matching done
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall trap mismatch none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# TRAP, case 5: rule doesn't match syscall #NR and rule argument matches
#		the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall trap mismatch match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# TRAP, case 6: rule doesn't match syscall #NR and rule argument does not match
#		the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall trap mismatch mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail


#
# SECCOMP_RET_ERRNO
#

# (exitval is 0 if the tester itself is not killed/traped)

# ERRNO, case 1: rule matches syscall #NR, no argument matching done
#  - syscall should return defined errno (negative), should not succeed
out=$(./seccomp $syscall errno match none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -eq -1 \
 -a "$errno" -eq "$scerrno" \
 -a "$success" -eq 0 ] || exit_fail

# ERRNO, case 2: rule matches syscall #NR and rule argument matches the syscall
#  - syscall should return defined errno (negative), should not succeed
out=$(./seccomp $syscall errno match match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -eq -1 \
 -a "$errno" -eq "$scerrno" \
 -a "$success" -eq 0 ] || exit_fail

# ERRNO, case 3: rule matches syscall #NR and rule argument does not match
#               the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall errno match mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# ERRNO, case 4: rule doesn't match syscall #NR, no argument matching done
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall errno mismatch none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# ERRNO, case 5: rule doesn't match syscall #NR and rule argument matches
#		the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall errno mismatch match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# ERRNO, case 6: rule doesn't match syscall #NR and rule argument does not match
#		the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall errno mismatch mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail


#
# SECCOMP_RET_TRACE
#

# (ENOSYS = errno 38)

# TRACE, case 1: rule matches syscall #NR, no argument matching done
#  - syscall should fail with ENOSYS (no tracer attached), returning -1
out=$(./seccomp $syscall trace match none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -eq -1 \
 -a "$errno" -eq 38  \
 -a "$success" -eq 0 ] || exit_fail

# TRACE, case 2: rule matches syscall #NR and rule argument matches the syscall
#  - syscall should fail with ENOSYS (no tracer attached), returning -1
out=$(./seccomp $syscall trace match match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -eq -1 \
 -a "$errno" -eq 38  \
 -a "$success" -eq 0 ] || exit_fail

# TRACE, case 3: rule matches syscall #NR and rule argument does not match
#               the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall trace match mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# TRACE, case 4: rule doesn't match syscall #NR, no argument matching done
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall trace mismatch none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# TRACE, case 5: rule doesn't match syscall #NR and rule argument matches
#		the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall trace mismatch match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# TRACE, case 6: rule doesn't match syscall #NR and rule argument does not match
#		the syscall
#  - the rule should not match, seccomp should use the default action (allow),
#    syscall should succeed
out=$(./seccomp $syscall trace mismatch mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail


#
# SECCOMP_RET_ALLOW
#

# (the tester uses TRAP default action)

# ALLOW, case 1: rule matches syscall #NR, no argument matching done
#  - seccomp should allow the syscall, syscall should succeed
out=$(./seccomp $syscall allow match none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# ALLOW, case 2: rule matches syscall #NR and rule argument matches the syscall
#  - seccomp should allow the syscall, syscall should succeed
out=$(./seccomp $syscall allow match match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 0 \
 -a "$retval" -ge 0 \
 -a "$errno" -eq 0  \
 -a "$success" -eq 1 ] || exit_fail

# ALLOW, case 3: rule matches syscall #NR and rule argument does not match
#               the syscall
#  - the rule should not match, seccomp should use the default action (trap),
#    syscall should succeed
out=$(./seccomp $syscall allow match mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 2 \
 -a "$success" -eq 0 ] || exit_fail

# ALLOW, case 4: rule doesn't match syscall #NR, no argument matching done
#  - the rule should not match, seccomp should use the default action (trap),
#    syscall should succeed
out=$(./seccomp $syscall allow mismatch none none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 2 \
 -a "$success" -eq 0 ] || exit_fail

# ALLOW, case 5: rule doesn't match syscall #NR and rule argument matches
#		the syscall
#  - the rule should not match, seccomp should use the default action (trap),
#    syscall should succeed
out=$(./seccomp $syscall allow mismatch match none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 2 \
 -a "$success" -eq 0 ] || exit_fail

# ALLOW, case 6: rule doesn't match syscall #NR and rule argument does not match
#		the syscall
#  - the rule should not match, seccomp should use the default action (trap),
#    syscall should succeed
out=$(./seccomp $syscall allow mismatch mismatch none)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 2 \
 -a "$success" -eq 0 ] || exit_fail


#
# precedence related testing
#
# the following cases represent only several meaningful combinations,
# because testing every combination in a generic way is nearly impossible
# due to the fact that we're combining (stacking) not only rules, but also
# default actions - trying to override allow rule (with trap default action)
# using trap rule (with allow default action) might result in two independent
# processes with the same result:
# - the allow rule doesn't match, trap is issued as default action
# - the allow rule is overriden by a trap rule, which matches, issuing trap
# eg. it's impossible to distinguish between a bug in seccomp (rule not maching)
# and a successful override
# - this is somewhat mitigated by the "basic" tests above
#
# the following precedence testing is split into two sections, the first does
# generic (argument-less) / basic precedence verification (cases 1-6),
# the second verifies precedence correctness with regard to argument matching
#

# PRECEDENCE, case 1: generic allow is overriden by generic trap
#  - seccomp should prefer trap, the syscall should not succeed
out=$(./seccomp $syscall allow match none x-before-trap)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 2 \
 -a "$success" -eq 0 ] || exit_fail

# PRECEDENCE, case 2: generic trap is NOT overriden by generic allow
#  - seccomp should prefer trap, the syscall should not succeed
out=$(./seccomp $syscall allow match none x-after-trap)
update_results "$out"
[ "$out" \
 -a "$exitval" -eq 2 \
 -a "$success" -eq 0 ] || exit_fail

# PRECEDENCE, case 3: generic trap is overriden by generic kill
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall trap match none x-before-kill)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# PRECEDENCE, case 4: generic kill is NOT overriden by generic trap
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall trap match none x-after-kill)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# PRECEDENCE, case 5: generic allow is overriden by generic kill
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall allow match none x-before-kill)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# PRECEDENCE, case 6: generic kill is NOT overriden by generic allow
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall allow match none x-after-kill)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail


# PRECEDENCE, case 7: trap with argument is overriden by generic (non-arg) kill
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall trap match match x-before-kill)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# PRECEDENCE, case 8: generic kill is NOT overriden by trap with argument
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall trap match match x-after-kill)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# PRECEDENCE, case 9: allow with argument is overriden by generic (non-arg) kill
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall allow match match x-before-kill)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# PRECEDENCE, case 10: generic kill is NOT overriden by allow with argument
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall allow match match x-after-kill)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# PRECEDENCE, case 11: generic (non-arg) trap is overriden by kill with argument
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall kill match match x-after-trap)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail

# PRECEDENCE, case 12: kill with argument is NOT overriden by generic trap
#  - seccomp should prefer kill, the syscall should not succeed
out=$(./seccomp $syscall kill match match x-before-trap)
update_results "$out"
[ -z "$out" \
 -a "$exitval" -eq 159 ] || exit_fail


done;

exit_pass
