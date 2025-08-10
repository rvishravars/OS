#include <linux/module.h>
#include <linux/init.h>
#include <linux/semaphore.h>

DEFINE_SEMAPHORE(my_sem, 1);   // <-- note the initial count argument
EXPORT_SYMBOL(my_sem);

static int __init my_init(void)
{
        int rc = down_trylock(&my_sem);
        if (rc == 0) {
                pr_info("Init: semaphore was UNLOCKED (peeked, restoring)\n");
                up(&my_sem);
        } else {
                pr_info("Init: semaphore is currently LOCKED\n");
        }
        return 0;
}

static void __exit my_exit(void)
{
        if (down_trylock(&my_sem) == 0) {
                pr_info("Exit: semaphore UNLOCKED (peeked, restoring)\n");
                up(&my_sem);
        } else {
                pr_info("Exit: semaphore still LOCKED\n");
        }
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Tatsuo Kawasaki");
MODULE_DESCRIPTION("lab1_semaphore1.c");
MODULE_LICENSE("GPL v2");
