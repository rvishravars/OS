#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#define DEVICE_NAME "asgn1"

static int __init asgn1_init(void) {
    pr_info("asgn1 module loaded\n");
    return 0;
}

static void __exit asgn1_exit(void) {
    pr_info("asgn1 module unloaded\n");
}

module_init(asgn1_init);
module_exit(asgn1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vishravars R");
MODULE_DESCRIPTION("Virtual Ramdisk Assignment");
