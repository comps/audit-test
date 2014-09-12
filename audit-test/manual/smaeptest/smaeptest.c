/* Copyright (c) 2014 Red Hat, Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 2 the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static struct dentry *dir;

static ssize_t
smap_write(struct file *file, const char *buffer, size_t len, loff_t *off)
{
	/* simply read the address directly, without copy_from_user() */
	volatile char junk = *buffer;
	(void)junk;
	return len;
}

static ssize_t
smep_write(struct file *file, const char *buffer, size_t len, loff_t *off)
{
	/* assume the passed pointer contains a valid function */
	void (*ptr)(void) = (void*)buffer;
	ptr();
	return len;
}

static struct file_operations smap_ops = {
	.owner   = THIS_MODULE,
	.write   = smap_write,
};
static struct file_operations smep_ops = {
	.owner   = THIS_MODULE,
	.write   = smep_write,
};

int init_module(void)
{
	dir = debugfs_create_dir(THIS_MODULE->name, NULL);
	if (!dir)
		return -EINVAL;

	if (!debugfs_create_file("smap", 0222, dir, NULL, &smap_ops))
		return -EINVAL;

	if (!debugfs_create_file("smep", 0222, dir, NULL, &smep_ops))
		return -EINVAL;

	return 0;
}

void cleanup_module(void)
{
	debugfs_remove_recursive(dir);
}
