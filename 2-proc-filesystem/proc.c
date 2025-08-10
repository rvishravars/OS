/* Using the /proc filesystem. (/proc/driver solution)
 *
 * Write a module that creates a read-only /proc filesystem entry
 * using seq_file implemented in Linux kernel.
 *
 * When you read from the entry, you should obtain the value of some
 * parameter set in your module.
 *
 * Make sure you remove the entry when you unload your module.  What
 * happens if you don't and you try to access the entry after the
 * module has been removed?
 *
 * There are two different solutions given, one which creates the entry
 * in the /proc directory, the other in /proc/driver.
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/slab.h>

#if 0
#define NODE "my_proc"
#else
#define NODE "driver/my_proc"
#endif

static struct proc_dir_entry *my_proc_dir;

#define MODULE_NAME "proc_test"
static int param = 777;

static void *my_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= 1)
		return NULL;
	else
		return &param;
}

static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >= 1)
		return NULL;
	else
		return &param;
}

static void my_seq_stop(struct seq_file *s, void *v)
{
	/* There's nothing to do here! */
}

/**
 * Displays information about current status of the module,
 * which helps debugging.
 */
int my_seq_show(struct seq_file *s, void *v)
{
	seq_printf(s,
		   "param = %d\n",
		   *(int *)v);

	return 0;
}

static struct seq_operations my_seq_ops = {
	.start = my_seq_start,
	.next  = my_seq_next,
	.stop  = my_seq_stop,
	.show  = my_seq_show
};

static int my_proc_open(struct inode *inode, struct file *filp)
{
	return seq_open(filp, &my_seq_ops);
}

static struct proc_ops my_proc_ops = {
	.proc_open    = my_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = seq_release,
};

static int __init my_init(void)
{
	/**
	 * create a proc entry with global read access
	 * if fails, print an error message and return an error.
	 *
	 */
	my_proc_dir = proc_mkdir(NODE, NULL);
	if (my_proc_dir == NULL) {
		printk(KERN_WARNING "mod_test_proc: can't add %s to the procfs\n", NODE);
		return -ENOMEM;
	}

	if (proc_create("ro_info", 0444, my_proc_dir, &my_proc_ops) == NULL) {
		remove_proc_entry(NODE, NULL);
		printk(KERN_WARNING "mod_test_proc: can't add %s/ro_info to the procfs\n", NODE);
		return -ENOMEM;
	}

	printk(KERN_WARNING "mod_test_proc read-only proc entry successful\n");
	return 0;
}

static void __exit my_exit(void)
{
	/**
	 * remove the proc entry
	 */
	remove_proc_entry("ro_info", my_proc_dir);
	remove_proc_entry(NODE, NULL);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Zhiyi Huang");
MODULE_DESCRIPTION("Create a read-only proc entry in Lab 4 of COSC440");
MODULE_LICENSE("GPL v2");
