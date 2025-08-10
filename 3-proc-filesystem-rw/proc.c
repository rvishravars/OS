/*
* Using the /proc filesystem. (/proc/driver solution)
*
* Write a module that creates a /proc filesystem entry and can read
* and write to it.
*
* When you read from the entry, you should obtain the value of some
* parameter set in your module.
*
* When you write to the entry, you should modify that value, which
* should then be reflected in a subsequent read.
*
* Make sure you remove the entry when you unload your module.  What
* happens if you don't and you try to access the entry after the
* module has been removed?
*/

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>   /* for kstrtoint */

#if 0
#define NODE "my_proc"
#else
#define NODE "driver/my_proc"
#endif

static struct proc_dir_entry *my_proc_dir;

#define MODULE_NAME "proc_test"
#define MAXBUFLEN 100
static char msg[MAXBUFLEN+1]="777\n";
static int dlen=4, wptr=0, rptr=0;
static int param=777;

static int
my_proc_open(struct inode *inode, struct file *filp)
{
  rptr = 0;
  wptr = 0;

  // if not opened in read-only mode, clean message buffer
  if ((filp->f_flags & O_ACCMODE) != O_RDONLY){
    memset(msg, 0, MAXBUFLEN+1);
    dlen = 0;
  }
  return 0;
}

static ssize_t
my_proc_read(struct file *filp, char __user *buf, size_t count, loff_t *offp )
{
  int rc;
  size_t to_copy;

  /* Prepare a fresh snapshot each read-open */
  memset(msg, 0, MAXBUFLEN+1);
  snprintf(msg, MAXBUFLEN, "param = %d\n", param);
  dlen = strlen(msg);

  if (rptr >= dlen)
    return 0; /* EOF */

  to_copy = dlen - rptr;
  if (count < to_copy)
    to_copy = count;

  /* Copy out from msg + rptr into user buffer */
  rc = copy_to_user(buf, msg + rptr, to_copy);
  if (rc) {
    /* rc is the number of bytes that could NOT be copied */
    to_copy -= rc;
  }
  rptr += to_copy;
  return to_copy;
}

static ssize_t
my_proc_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
  size_t space;
  size_t to_copy;
  int rc;

  if (wptr >= MAXBUFLEN)
    return -ENOSPC;

  space = MAXBUFLEN - wptr;
  to_copy = (count > space) ? space : count;

  rc = copy_from_user(msg + wptr, buf, to_copy);
  if (rc) {
    /* rc is the number not copied; accept what we got */
    to_copy -= rc;
  }

  wptr += to_copy;
  /* msg is a string buffer—keep it NUL-terminated if room */
  if (wptr <= MAXBUFLEN)
    msg[wptr] = '\0';

  /* Track data length (up to last written byte) */
  if (wptr > dlen)
    dlen = wptr;

  return to_copy;
}

static int
my_proc_close(struct inode *inode, struct file *filp)
{
  /* if not opened in read-only mode, process the message buffer */
  if ((filp->f_flags & O_ACCMODE) != O_RDONLY){
    /* Sanitize: ensure NUL-terminated, trim whitespace/newlines */
    int new_param;
    char *start = msg;
    char *end;

    msg[MAXBUFLEN] = '\0';

    /* Trim leading spaces */
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')
      start++;

    /* Trim trailing spaces/newlines */
    end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r'))
      *--end = '\0';

    /* Parse integer; accept decimal, allow leading +/- */
    if (kstrtoint(start, 10, &new_param) == 0) {
      param = new_param;
      pr_info("proc rw_info: param has been set to %d\n", param);
    } else {
      pr_warn("proc rw_info: invalid input, expected integer (got '%s')\n", start);
    }
  }
  return 0;
}

static struct proc_ops my_proc_ops = {
  .proc_open    = my_proc_open,
  .proc_read    = my_proc_read,
  .proc_write   = my_proc_write,
  .proc_release = my_proc_close,
};

static int __init my_init(void)
{
  /**
   * create a proc entry with global read access and owner write access
   *  if fails, print an error message and return an error.
   *
   */
  my_proc_dir = proc_mkdir(NODE, NULL);
  if (my_proc_dir == NULL) {
    printk(KERN_WARNING "mod_test_proc: can't add %s to the procfs\n", NODE);
    return -ENOMEM;
  }

  if (proc_create("rw_info",
#if defined(S_IRUGO)
                  S_IRUGO | S_IWUSR,
#else
                  0644,
#endif
                  my_proc_dir, &my_proc_ops) == NULL) {
    remove_proc_entry(NODE, NULL);
    printk(KERN_WARNING "mod_test_proc: can't add %s/rw_info to the procfs\n", NODE);
    return -ENOMEM;
  }

  printk(KERN_WARNING "mod_test_proc successful\n");
  return 0;
}

static void __exit my_exit(void)
{
  /**
   * remove the proc entry
   */
  remove_proc_entry("rw_info", my_proc_dir);
  remove_proc_entry(NODE, NULL);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Zhiyi Huang");
MODULE_DESCRIPTION("Create a readable and writable proc entry in Lab 4 of COSC440");
MODULE_LICENSE("GPL v2");
