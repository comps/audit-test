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

#include "includes.h"

void init_globals()
{

#ifdef CONFIG_AUDIT_LAUS
    audit_ops.audit_start = &laus_start;
    audit_ops.audit_stop = &laus_stop;
    audit_ops.audit_clear_logs = &laus_clear_logs;
    audit_ops.audit_reload = &laus_reload;
    audit_ops.audit_set_filters = &laus_set_filters;
    audit_ops.audit_verify_log = &laus_verify_log;
#else
    audit_ops.audit_start = &laf_start;
    audit_ops.audit_stop = &laf_stop;
    audit_ops.audit_clear_logs = &laf_clear_logs;
    audit_ops.audit_reload = &laf_reload;
    audit_ops.audit_set_filters = &laf_set_filters;
    audit_ops.audit_verify_log = &laf_verify_log;
#endif

}

int audit_start()
{
    return (audit_ops.audit_start)();
};

int audit_stop()
{
    return (audit_ops.audit_stop)();
};

int audit_clear_logs()
{
    return (audit_ops.audit_clear_logs)();
};

int audit_reload()
{
    return (audit_ops.audit_reload)();
};

int audit_set_filters(log_options logOption)
{
    return (audit_ops.audit_set_filters)(logOption);
};

int audit_verify_log(laus_data *dataPtr, log_options logOption)
{
    return (audit_ops.audit_verify_log)(dataPtr, logOption);
};
