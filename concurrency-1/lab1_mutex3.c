/* **************** LDD:1.0 s_12/lab1_mutex3.c **************** */
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
 * Semaphore Contention
 *
 * second and third module to test semaphores
 @*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <asm/atomic.h>
#include <linux/errno.h>

extern struct mutex my_mutex;

static char *modname = __stringify(KBUILD_BASENAME);

static bool locked_by_me;

static int __init my_init(void)
{
    printk(KERN_INFO "Trying to load module %s\n", modname);
    printk(KERN_INFO "%s start mutex state: %d owner(raw): %lx\n",
           modname,
           mutex_is_locked(&my_mutex),
           (unsigned long)atomic_long_read(&my_mutex.owner));

    if (!mutex_trylock(&my_mutex)) {
        printk(KERN_INFO "%s: mutex is busy; refusing to load\n", KBUILD_MODNAME);
        return -EBUSY;
    }
	
    locked_by_me = true;

    printk(KERN_INFO "%s mutex locked state: %d owner(raw): %lx\n",
           modname,
           mutex_is_locked(&my_mutex),
           (unsigned long)atomic_long_read(&my_mutex.owner));

    return 0;
}

static void __exit my_exit(void)
{
	/* START SKELETON */
	if (locked_by_me) {
		mutex_unlock(&my_mutex);
		locked_by_me = false;
		printk(KERN_INFO "%s: mutex released\n", KBUILD_MODNAME);
	}
	/* END SKELETON */

	printk(KERN_INFO "\n%s mutex end state: %d lock owner: %u\n",
	       modname, mutex_is_locked(&my_mutex), atomic_read(&my_mutex.owner));
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Tatsuo Kawasaki");
MODULE_DESCRIPTION("LDD:1.0 s_12/lab1_mutex2.c");
MODULE_LICENSE("GPL v2");
