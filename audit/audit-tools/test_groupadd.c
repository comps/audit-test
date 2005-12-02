/**********************************************************************
 **   Copyright (C) International Business Machines  Corp., 2003
 **
 **   This program is free software;  you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation; either version 2 of the License, or
 **   (at your option) any later version.
 **
 **   This program is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 **   the GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with this program;  if not, write to the Free Software
 **   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 **
 **
 **
 **  FILE       : test_groupadd.c
 **
 **  PURPOSE    : To help test the aucat tool.
 **
 **  HISTORY    :
 **    08/2003 Originated by Michael A. Halcrow <mike@halcrow.us>
 **
 **********************************************************************/

#include "includes.h"

int debug = 2;

int aucat_test_groupadd( int succeed );

int main( int argc, char* argv[] ) {
  int succeed = 1;

  if( argc > 1 ) {
    succeed = atoi( argv[1] );
  } else {
    aucat_test_groupadd( 1 );
    //aucat_test_groupadd( 0 );
    return 0;
  }

  return aucat_test_groupadd( succeed );

}

int aucat_test_groupadd( int succeed ) {

  int rc = 0;
  //int test = 1;
  //char* command;
  char* group;
  int gid;

  if( succeed ) {
  createTempGroupName( &group, &gid );
  } else {
    gid = -1;
    group = "if_you_see_this_then_something_went_wrong_with_aucat_test_groupadd";
  }

  RUNCOMMAND( "/usr/sbin/groupadd -g %d %s", gid, group );

  if( succeed ) {
    RUNCOMMAND( "/usr/sbin/groupdel %s", group );
    free( group );
  }

  rc = 0;

 //EXIT:
  printf5("Returning from test_groupadd()\n");
  return rc;
}


