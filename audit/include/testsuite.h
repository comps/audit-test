/* =======================================================================
 *   (c) Copyright Hewlett-Packard Development Company, L.P., 2005 
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   with this package; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * ======================================================================= 
 */

#ifndef _TESTSUITE_H
#define _TESTSUITE_H

#define TEST_USER "testuser"
#define TEST_TMP_TEMPLATE "/tmp/audit_testXXXXXX"

#define MAX_TESTCASE_NAME 25
#define MAX_VARIATION_NAME 25

/* Testsuite exit codes */
typedef enum {
    TEST_EXPECTED = 0,  /* test completed as expected */
    TEST_UNEXPECTED,    /* test completed with unexpected result */
    TEST_ERROR,         /* test did not complete due to error */
} ts_exit;

#endif	/* _TESTSUITE_H */
