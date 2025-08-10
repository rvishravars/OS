/* **************** LDD:1.0 s_12/lab1_mutex1.c **************** */
/*
 * The code herein is: Copyright Jerry Cooperstein, 2009
 *
 * This Copyright is retained for the purpose of protecting free
 * redistribution of source.
 *
 *     URL:    http://www.coopj.com
 *     email:  coop@coopj.com
 *
 * The primary maintainer for this code is Jerry Cooperstein
 * The CONTRIBUTORS file (distributed with this
 * file) lists those known to have contributed to the source.
 *
 * This code is distributed under Version 2 of the GNU General Public
 * License, which you should have received with the source.
 *
 */
/*
 * Mutex Contention
 *
 * Now do the same thing using semaphores instead of mutexes
 @*/

#include <linux/module.h>
#include <linux/init.h>

DEFINE_MUTEX(my_mutex);
EXPORT_SYMBOL(my_mutex);

static int __init my_init(void)
{
	printk(KERN_INFO "\nInit mutex in unlocked state: %d lock owner: %u\n",
	       mutex_is_locked(&my_mutex), atomic_read(&my_mutex.owner));
	return 0;
}

static void __exit my_exit(void)
{
	printk(KERN_INFO "\nExiting with  mutex state: %d lock owner: %u\n",
	       mutex_is_locked(&my_mutex), atomic_read(&my_mutex.owner));
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Tatsuo Kawasaki");
MODULE_DESCRIPTION("LDD:1.0 s_12/lab1_mutex1.c");
MODULE_LICENSE("GPL v2");
