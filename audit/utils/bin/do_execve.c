/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of version 2 the GNU General Public License as
 *  published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include <sys/wait.h>

int main(int argc, char **argv)
{
    int result, error, status;
    pid_t pid;

    if (argc != 2) {
	fprintf(stderr, "Usage:\n%s <path>\n", argv[0]);
	return 1;
    }

    pid = fork();
    if (pid < 0) {
	fprintf(stderr, "Error: fork(): %s\n", strerror(errno));
	return 1;
    }
    if (pid == 0) {
	execve(argv[1], NULL, NULL);
	_exit(errno);
    } else {
	if (waitpid(pid, &status, 0) < 0) {
	    fprintf(stderr, "Error: waitpid(): %s\n", strerror(errno));
	    return 1;
	}
    }

    error = WIFEXITED(status) ? WEXITSTATUS(status) : EINTR;
    result = !!error;

    printf("%d %d %d\n", result, result ? error : 0, pid);
    return result;
}
