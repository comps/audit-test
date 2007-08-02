/* =======================================================================
 *   (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of version 2 the GNU General Public License as
 *   published by the Free Software Foundation.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <selinux/selinux.h>
#include <selinux/context.h>
#include <string.h>

int main(int argc, char **argv)
{
	int rc;
	security_context_t orig_scon;
	security_context_t new_scon;
	security_context_t final_scon;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s new_scon\n", argv[0]);
		exit(1);
	}

	rc = getcon(&orig_scon);
	if (rc < 0) {
		perror("getcon");
		exit(1);
	}
	new_scon = argv[1];
	rc = setcon(new_scon);
	if (rc < 0) {
		perror("setcon");
		exit(1);
	}
	rc = getcon(&final_scon);
	if (rc < 0) {
		perror("getcon");
		exit(1);
	}
	if (strcmp(new_scon, final_scon) != 0) {
		printf("Failed to change from %s to %s\n",orig_scon,final_scon);
		exit(1);
	} 
	printf("Changed from %s to %s\n",orig_scon,final_scon);
	exit(0);
}
