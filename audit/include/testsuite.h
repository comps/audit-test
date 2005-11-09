/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
 * Written by Amy Griffis <amy.griffis@hp.com>
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

#define SKIP_TEST_CASE 256

#define LOGOPTION_INDEX_ALL 0
#define LOGOPTION_INDEX_SUCCESS_ONLY 1
#define LOGOPTION_INDEX_FAIL_ONLY 2
#define LOGOPTION_INDEX_NOTHING 3

typedef enum {FALSE, TRUE} Boolean; 

typedef struct {
    Boolean logSuccess;
    Boolean logFailure;
} log_options;

struct test_counters {
    int passed;
    int failed;
    int skipped;
};

#endif	/* _TESTSUITE_H */
