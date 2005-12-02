/* =======================================================================
 * Copyright (C) 2005 Hewlett-Packard Company
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

#include "includes.h"
#include <libaudit.h>

int get_auditarch()
{
#if defined(__i386__)
    return MACH_X86;
#elif defined(__x86_64__)
    return MACH_86_64;
#elif defined(__ia64__)
    return MACH_IA64;
#elif defined(__powerpc64__)
    return MACH_PPC64;
#elif defined(__powerpc__)
    return MACH_PPC;
#elif defined(__s390x__)
    return MACH_S390X;
#elif defined(__s390__)
    return MACH_S390;
#else
    return -1;
#endif
}
