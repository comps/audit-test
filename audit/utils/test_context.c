/* =======================================================================
 *   (c) Copyright Hewlett-Packard Development Company, L.P., 2006
 *   Written by Paul Moore <paul.moore@hp.com>
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
#include <selinux/flask.h>

#define ERR_CTX_STR                    "error_no_context"
#define ERR_CTX_RC                     1

/**
 * err_fatal - Output an invalid context and exit
 *
 * Description:
 * Print an invalid, error context and exit.
 *
 */
void err_fatal(void)
{
  printf("%s\n", ERR_CTX_STR);
  exit(ERR_CTX_RC);
}

/*
 * main
 */
int main(int argc, char *argv[])
{
  char *test_path;
  security_context_t test_file_ctx = NULL;
  security_context_t test_domain_ctx = NULL;
  security_context_t self_domain_ctx = NULL;

  if (argc != 2)
    err_fatal();
  test_path = argv[1];

  if (getcon(&self_domain_ctx) < 0)
    err_fatal();
  if (getfilecon(test_path, &test_file_ctx) < 0)
    err_fatal();
  if (security_compute_create(self_domain_ctx,
			      test_file_ctx,
			      SECCLASS_PROCESS,
			      &test_domain_ctx) < 0)
    err_fatal();

  printf("%s\n", test_domain_ctx);

  exit(0);
}
