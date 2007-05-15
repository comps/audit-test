/*  (c) Copyright Hewlett-Packard Development Company, L.P., 2007
 *
 *  This program is free software;  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *  the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "includes.h"
#include <signal.h>

static void signal_handler(int sig)
{
    int save_errno;

    switch (sig) {
    /* add other signals here, or below */
    case SIGUSR1:
        /* re-register this signal handler for next time */
        save_errno = errno;
        signal(sig, signal_handler);
        errno = save_errno;
        break;
    }

    return;
}

int main(void)
{
    if (signal(SIGUSR1, signal_handler) == SIG_ERR)
        perror("do_dummy: signal");

    for (;;) sleep(1);

    /* not reached */
    return 1;
}
