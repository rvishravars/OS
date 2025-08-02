#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>   // copy_to_user, copy_from_user
#include <linux/kernel.h>    // printk
#include <linux/slab.h>      // kmalloc/kfree

#define DEVICE_NAME "asgn1"
#define DEFAULT_MAJOR 0       // 0 = dynamic major number

static int major_number;
static char *device_buffer;
static size_t buffer_size = 0;  // how many bytes stored
#define MAX_BUFFER_SIZE 4096    // just for this stub — will replace with page-linked list later

// Called when process opens the device
static int ramdiskext_open(struct inode *inode, struct file *file) {
    pr_info("ramdiskext: device opened\n");
    return 0;
}

// Called when process releases the device
static int ramdiskext_release(struct inode *inode, struct file *file) {
    pr_info("ramdiskext: device closed\n");
    return 0;
}

// Called when data is read from the device
static ssize_t ramdiskext_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    if (*ppos >= buffer_size)
        return 0; // EOF

    if (*ppos + count > buffer_size)
        count = buffer_size - *ppos;

    if (copy_to_user(buf, device_buffer + *ppos, count))
        return -EFAULT;

    *ppos += count;
    pr_info("ramdiskext: read %zu bytes at offset %lld\n", count, *ppos);
    return count;
}

// Called when data is written to the device
static ssize_t ramdiskext_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    if (!device_buffer)
        device_buffer = kmalloc(MAX_BUFFER_SIZE, GFP_KERNEL);

    if (!device_buffer)
        return -ENOMEM;

    if (count > MAX_BUFFER_SIZE)
        count = MAX_BUFFER_SIZE;

    if (copy_from_user(device_buffer, buf, count))
        return -EFAULT;

    buffer_size = count;
    *ppos = count;

    pr_info("ramdiskext: wrote %zu bytes\n", count);
    return count;
}

static const struct file_operations ramdiskext_fops = {
    .owner = THIS_MODULE,
    .read = ramdiskext_read,
    .write = ramdiskext_write,
    .open = ramdiskext_open,
    .release = ramdiskext_release,
};

static int __init ramdiskext_init(void) {
    major_number = register_chrdev(DEFAULT_MAJOR, DEVICE_NAME, &ramdiskext_fops);
    if (major_number < 0) {
        pr_err("ramdiskext: failed to register char device\n");
        return major_number;
    }

    pr_info("ramdiskext: registered successfully with major number %d\n", major_number);
    pr_info("ramdiskext: create device with: mknod /dev/%s c %d 0\n", DEVICE_NAME, major_number);
    return 0;
}

static void __exit ramdiskext_exit(void) {
    unregister_chrdev(major_number, DEVICE_NAME);
    kfree(device_buffer);
    pr_info("ramdiskext: unregistered device and freed memory\n");
}

module_init(ramdiskext_init);
module_exit(ramdiskext_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vishravars R");
MODULE_DESCRIPTION("Assignment 1: Virtual Ramdisk Character Device");
