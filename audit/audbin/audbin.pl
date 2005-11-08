#!/usr/bin/perl
# Copyright (C) International Business Machines Corp., 2003
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details. 
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA 
#
# FILE: audbin.pl
#
# PURPOSE: Verify basic functionality of the audbin utility.
#
# DESCRIPTION: This test directly calls audbin with a series of
#              parameters and verifies that the results are as
#              expected.  The primary goal of these tests is to ensure
#              that audbin does not overwrite or delete something that
#              it should not overwrite or delete.
#
# HISTORY:
#       09/2003 Originated by Michael A. Halcrow <mike@halcrow.us>

use strict;

my $audbin = "/usr/sbin/audbin";
my $aucat = "/usr/sbin/aucat";

my $passed = 0;
my $failed = 0;
my $skipped = 0;

( `id -u` eq "0\n" ) || die "ERROR: You must run this script as the superuser\n";

# First, delete our guinea pig
`rm -f auditlogbin.0`;

# Get the location of the bin.0 file
my $logFilePrefix;
open( FILE, "</etc/audit/audit.conf" );
while( <FILE> ) {
    if( $_ =~ /file-name\s*=\s*\"([^\"]+)\"/ ) {
	$logFilePrefix = $1;
	goto EXIT_AUDIT_CONF_SEARCH;
    }
}
die "Unable to determine prefix for audit log files";
 EXIT_AUDIT_CONF_SEARCH:;
my $auditLogFile = $logFilePrefix.".0";
my $cmd = "$audbin -S ./auditlogbin.0 $auditLogFile";
`$cmd` || die "Cannot obtain audit log file";

# Clean up any cruft
`rm -f save.*`;
`rm -f time.*`;
`rm -f hostname.*`;

# Verify that the %h flag gives the expected results
my $hostname = `/bin/hostname`;
chomp( $hostname );
`$audbin -S hostname.%h auditlogbin.0`;
my $filename = `ls | grep hostname`;
chomp( $filename );
if( $filename ne "hostname.$hostname" ) {
    print "Expected [hostname.$hostname] while testing hostname substitution; got [$filename]\n";
    $failed += 1;
} else {
    $passed += 1;
}

# Verify that a basic full copy can be made
my $orig_md5sum = `/usr/bin/md5sum auditlogbin.0 | /usr/bin/awk '{ print \$1; }'`;
chomp( $orig_md5sum );
`$audbin -S save.0 auditlogbin.0`;
my $dest_md5sum = `/usr/bin/md5sum save.0 | /usr/bin/awk '{ print \$1; }'`;
chomp( $dest_md5sum );
if( !( $orig_md5sum eq $dest_md5sum ) ) {
    print "md5sums did not match up between the original and backup binaries produced by audbin\n";
    print "[$orig_md5sum] != [$dest_md5sum]\n";
    $failed += 1;
} else {
    $passed += 1;
}

# Verify that audbin will not overwrite when the backup file already exists and we give the append flag
`$audbin -a -S save.0 auditlogbin.0`;
$dest_md5sum = `/usr/bin/md5sum save.0 | /usr/bin/awk '{ print \$1; }'`;
chomp( $dest_md5sum );
if( $orig_md5sum eq $dest_md5sum ) {
    print "md5sums matched up between the original and backup binaries produced by audbin; they should have been different\n";
    print "[$orig_md5sum] = [$dest_md5sum]\n";
    $failed += 1;
} else {
    $passed += 1;
}

# Verify that audbin will overwrite when the backup file already exists and we give the overwrite flag
`$audbin -o -S save.0 auditlogbin.0`;
$dest_md5sum = `/usr/bin/md5sum save.0 | /usr/bin/awk '{ print \$1; }'`;
chomp( $dest_md5sum );
if( !( $orig_md5sum eq $dest_md5sum ) ) {
    print "md5sums did not match up between the original and backup binaries produced by audbin; it should have overwritten\n";
    print "[$orig_md5sum] != [$dest_md5sum]\n";
    $failed += 1;
} else {
    $passed += 1;
}

# Verify that audbin will not overwrite when the backup file already
# exists and we give the %u flag
`$audbin -a -S save.0 auditlogbin.0`;
`$audbin -S save.%u auditlogbin.0`;
$dest_md5sum = `/usr/bin/md5sum save.0 | /usr/bin/awk '{ print \$1; }'`;
chomp( $dest_md5sum );
if( $orig_md5sum eq $dest_md5sum ) {
    print "md5sums matched up between the original and backup binaries produced by audbin.  This indicates that audbin overwrote [save.0] rather than advancing to [save.1].\n";
    print "[$orig_md5sum] != [$dest_md5sum]\n";
    $failed += 1;
} else {
    $passed += 1;
}

# Verify that save.1 is identical to auditlogbin.0
$dest_md5sum = `/usr/bin/md5sum save.1 | /usr/bin/awk '{ print \$1; }'`;
chomp( $dest_md5sum );
if( !( $orig_md5sum eq $dest_md5sum ) ) {
    print "md5sums did not match up between the original and backup (save.1) binaries produced by audbin\n";
    print "[$orig_md5sum] != [$dest_md5sum]\n";
    $failed += 1;
} else {
    $passed += 1;
}

# Verify that the log file is cleared after saving its contents
`rm -f save.0`;
`$audbin -C -S save.0 auditlogbin.0`;
#my $result = `$aucat -f auditlogbin.0 2> /dev/stdout`;
my $result = `$aucat -f auditlogbin.0 2>&1 /dev/stdout`;
if( $result ne "Empty audit log file: auditlogbin.0\n" ) {
    print "audbin called with -C flag, but it did not clear the audit log\n";
    $failed += 1;
} else {
    $passed += 1;
}

# Verify that the %t flag gives expected results
my $start_second = `/bin/date +%s`;
chomp( $start_second );
`$audbin -S time.%t auditlogbin.0`;
my $end_second = `/bin/date +%s`;
chomp( $end_second );
$filename = `ls | grep time`;
chomp( $filename );
if( !( $filename =~ /time\.(\d+)/ ) ) {
    print "Time suffix not generated correctly\n";
    $failed += 1;
} else {
    if( $start_second > $1 ) {
	print "Time suffix before start second\n";
	$failed += 1;
    } else {
	if( $end_second < $1 ) {
	    print "Time suffix past end second\n";
	    $failed += 1;	    
	} else {
	    $passed += 1;
	}
    }
}

# Delete the cruft
`rm -f save.*`;
`rm -f time.*`;
`rm -f hostname.*`;
`rm -f auditlogbin.0`;

# Give the report
print "TEST PASSED = $passed, FAILED = $failed, SKIPPED = $skipped\n";

exit 0;
