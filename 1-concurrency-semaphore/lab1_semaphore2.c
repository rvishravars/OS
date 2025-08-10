#include <linux/module.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/errno.h>

extern struct semaphore my_sem;   /* previously defined with DEFINE_SEMAPHORE in mod #1 */

static char *modname = __stringify(KBUILD_BASENAME);
static bool locked_by_me;

static int __init my_init(void)
{
	int rc;

	printk(KERN_INFO "Trying to load module %s\n", modname);

	/* Report current availability without changing final state */
	rc = down_trylock(&my_sem);
	if (rc == 0) {
		printk(KERN_INFO "%s start: semaphore was UNLOCKED (peeked)\n", modname);
		up(&my_sem); /* restore */
	} else {
		printk(KERN_INFO "%s start: semaphore is currently LOCKED\n", modname);
	}

	/* Try to acquire without sleeping; fail to load if busy */
	rc = down_trylock(&my_sem);
	if (rc != 0) { /* nonzero => couldn't acquire immediately */
		printk(KERN_INFO "%s: semaphore is busy; refusing to load\n", KBUILD_MODNAME);
		return -EBUSY;
	}

	locked_by_me = true;
	printk(KERN_INFO "%s: semaphore acquired\n", KBUILD_MODNAME);

	return 0;
}

static void __exit my_exit(void)
{
	if (locked_by_me) {
		up(&my_sem);
		locked_by_me = false;
		printk(KERN_INFO "%s: semaphore released\n", KBUILD_MODNAME);
	}

	/* Final availability snapshot */
	if (down_trylock(&my_sem) == 0) {
		printk(KERN_INFO "%s end: semaphore UNLOCKED (peeked)\n", modname);
		up(&my_sem);
	} else {
		printk(KERN_INFO "%s end: semaphore still LOCKED by someone else\n", modname);
	}
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Tatsuo Kawasaki");
MODULE_DESCRIPTION("lab1_semaphore2.c");
MODULE_LICENSE("GPL v2");
