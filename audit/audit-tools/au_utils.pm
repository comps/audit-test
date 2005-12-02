#!/usr/bin/perl

package au_utils;

use strict;
require au_params;

sub backupFilterConf() {
    system( "cp -f ".au_params::filter_dot_conf_fullpath()." ".au_params::filter_dot_conf_backup_fullpath() );
}

sub restoreFilterConf() {
    system( "cp -f ".au_params::filter_dot_conf_backup_fullpath()." ".au_params::filter_dot_conf_fullpath() );
}

sub replaceFilterConf() {
    system("cp -f filter.conf ".au_params::filter_dot_conf_fullpath() );	
}

sub killAuditd() {
    system( "/etc/init.d/audit stop" );
    sleep 3;
}

sub wipeLogs() {
    system( "rm -f ".au_params::audit_logs_fullpath() );
    system( "rm -f ".au_params::audit_log_link_fullpath() );
}

sub verifyRoot() {
    # MH: Verify that we are running as root
    ( `id -u` eq "0\n" ) || die "ERROR: You must run this script as the superuser\n";
}

sub verifyAuditd() {
    # MH: Verify that the auditd program is running
    #     Try to start it if it isn't.
    if( `/bin/ps -e | /usr/bin/grep auditd | /usr/bin/wc -l` =~ /\s0\s/ ) {
	print "Attempting to start auditd...\n";
	system( au_params::auditd_executable() ) && die "ERROR: Cannot start auditd. Make sure the module is loaded.\n";
	sleep 5;
    }    
}

sub preTestSetup() {
    verifyRoot();
    backupFilterConf();
    killAuditd();
    replaceFilterConf();
    wipeLogs();
    verifyAuditd();
    system ( "/etc/init.d/audit start" );
}

sub postTestCleanup() {
    restoreFilterConf();
}

1;
