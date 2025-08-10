// Assignment 1: Virtual Ramdisk using pages + list_head
// Features in this commit:
// - register_chrdev()-based chardev
// - read/write across arbitrarily many pages allocated via alloc_page()
// - double-linked list of pages using kernel list utilities
// - truncate-on-open when opened O_WRONLY (frees all pages)
// - llseek (SEEK_SET / SEEK_CUR / SEEK_END)
// TODO (next): ioctl to control max concurrent users; optional mmap; kmem_cache

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/highmem.h>   // kmap_local_page, kunmap_local, clear_highpage
#include <linux/mutex.h>
#include <linux/types.h>

#define DEVICE_NAME   "asgn1"
#define DEFAULT_MAJOR 0  // dynamic major

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vishravars R");
MODULE_DESCRIPTION("Assignment 1: Virtual Ramdisk (page-backed)");
MODULE_VERSION("0.2");

// One node per page of storage
struct rd_page {
    struct page *page;
    struct list_head list; // double-linked list node
};

// Device state
struct rd_device {
    struct list_head pages;   // head of list of rd_page nodes (in order)
    size_t           npages;  // count of pages currently allocated
    loff_t           size;    // logical file size in bytes
    struct mutex     lock;    // serialize access
};

static int major_number;
static struct rd_device device;

// ----- Helpers -----

/**
 * rd_free_all_pages_locked - Frees all pages in the ramdisk.
 * @d: Pointer to the ramdisk device structure.
 * Caller must hold device.lock.
 */
static void rd_free_all_pages_locked(struct rd_device *d)
{
    struct rd_page *rp, *tmp;

    list_for_each_entry_safe(rp, tmp, &d->pages, list) {
        if (rp->page)
            __free_page(rp->page);
        list_del(&rp->list);
        kfree(rp);
    }
    d->npages = 0;
    d->size   = 0;
}

/**
 * rd_ensure_capacity_locked - Ensures at least needed_pages are allocated.
 * @d: Pointer to the ramdisk device structure.
 * @needed_pages: Number of pages required.
 * Allocates and appends new pages if necessary.
 * Caller must hold device.lock.
 * Returns 0 on success or -ENOMEM on failure.
 */
static int rd_ensure_capacity_locked(struct rd_device *d, size_t needed_pages)
{
    while (d->npages < needed_pages) {
        struct rd_page *rp = kzalloc(sizeof(*rp), GFP_KERNEL);
        if (!rp)
            return -ENOMEM;

        rp->page = alloc_page(GFP_KERNEL);
        if (!rp->page) {
            kfree(rp);
            return -ENOMEM;
        }

        // Zero the new page so holes read back as zero
        clear_highpage(rp->page);

        list_add_tail(&rp->list, &d->pages);
        d->npages++;
    }
    return 0;
}

/**
 * rd_get_page_locked - Retrieves the rd_page at a given index.
 * @d: Pointer to the ramdisk device structure.
 * @index: Page index (0-based).
 * Returns pointer to rd_page or NULL if index is out of bounds.
 * Caller must hold device.lock.
 */
static struct rd_page *rd_get_page_locked(struct rd_device *d, size_t index)
{
    size_t i = 0;
    struct rd_page *rp;

    if (index >= d->npages)
        return NULL;

    list_for_each_entry(rp, &d->pages, list) {
        if (i == index)
            return rp;
        i++;
    }
    return NULL; // shouldn't happen if index < npages
}

// ----- File operations -----

/**
 * ramdiskext_open - File open operation.
 * Truncates the ramdisk if opened with O_WRONLY.
 */
static int ramdiskext_open(struct inode *inode, struct file *file)
{
    // If opened write-only, truncate by freeing all pages (per assignment)
    // O_ACCMODE masks the access mode
    if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
        mutex_lock(&device.lock);
        rd_free_all_pages_locked(&device);
        mutex_unlock(&device.lock);
        pr_info("ramdiskext: truncated on O_WRONLY open\n");
    }
    pr_info("ramdiskext: open (flags=0x%x)\n", file->f_flags);
    return 0;
}

/**
 * ramdiskext_release - File release (close) operation.
 */
static int ramdiskext_release(struct inode *inode, struct file *file)
{
    pr_info("ramdiskext: release\n");
    return 0;
}

/**
 * ramdiskext_llseek - File seek operation.
 * Supports SEEK_SET, SEEK_CUR, SEEK_END.
 */
static loff_t ramdiskext_llseek(struct file *file, loff_t off, int whence)
{
    loff_t newpos;

    mutex_lock(&device.lock);
    switch (whence) {
    case SEEK_SET:
        newpos = off;
        break;
    case SEEK_CUR:
        newpos = file->f_pos + off;
        break;
    case SEEK_END:
        newpos = device.size + off;
        break;
    default:
        mutex_unlock(&device.lock);
        return -EINVAL;
    }

    if (newpos < 0) {
        mutex_unlock(&device.lock);
        return -EINVAL;
    }

    // Allow seeking beyond EOF; pages get allocated lazily on write
    file->f_pos = newpos;
    mutex_unlock(&device.lock);
    return newpos;
}

/**
 * ramdiskext_read - File read operation.
 * Reads data from the ramdisk into user buffer.
 */
static ssize_t ramdiskext_read(struct file *file, char __user *buf,
                               size_t count, loff_t *ppos)
{
    ssize_t done = 0;

    if (count == 0)
        return 0;

    mutex_lock(&device.lock);

    // If position is at/after EOF, nothing to read
    if (*ppos >= device.size) {
        mutex_unlock(&device.lock);
        return 0;
    }

    // Clamp count to EOF
    if (*ppos + count > device.size)
        count = device.size - *ppos;

    while (done < count) {
        loff_t pos       = *ppos + done;
        size_t page_idx  = pos >> PAGE_SHIFT;         // pos / PAGE_SIZE
        size_t page_off  = pos & (PAGE_SIZE - 1);     // pos % PAGE_SIZE
        size_t to_copy   = min_t(size_t, PAGE_SIZE - page_off, count - done);
        struct rd_page *rp = rd_get_page_locked(&device, page_idx);
        void *kaddr;

        if (!rp || !rp->page) {
            // Hole (shouldn't happen if device.size set properly), treat as zeros
            // Provide zeroed temp buffer
            static const char zeros[64] = {0};
            size_t chunk = min_t(size_t, to_copy, sizeof(zeros));
            if (copy_to_user(buf + done, zeros, chunk)) {
                mutex_unlock(&device.lock);
                return -EFAULT;
            }
            done += chunk;
            continue;
        }

        kaddr = kmap_local_page(rp->page);
        if (copy_to_user(buf + done, (u8 *)kaddr + page_off, to_copy)) {
            kunmap_local(kaddr);
            mutex_unlock(&device.lock);
            return -EFAULT;
        }
        kunmap_local(kaddr);

        done += to_copy;
    }

    *ppos += done;
    mutex_unlock(&device.lock);
    return done;
}

/**
 * ramdiskext_write - File write operation.
 * Writes data from user buffer into the ramdisk.
 */
static ssize_t ramdiskext_write(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
    ssize_t done = 0;
    int ret;

    if (count == 0)
        return 0;

    mutex_lock(&device.lock);

    // We may need pages up to end_pos (exclusive)
    // end_pos = *ppos + count; needed_pages = ceil(end_pos / PAGE_SIZE)
    {
        loff_t end_pos = *ppos + count;
        size_t needed_pages = (end_pos + PAGE_SIZE - 1) >> PAGE_SHIFT;

        ret = rd_ensure_capacity_locked(&device, needed_pages);
        if (ret) {
            mutex_unlock(&device.lock);
            return ret;
        }
    }

    while (done < count) {
        loff_t pos       = *ppos + done;
        size_t page_idx  = pos >> PAGE_SHIFT;
        size_t page_off  = pos & (PAGE_SIZE - 1);
        size_t to_copy   = min_t(size_t, PAGE_SIZE - page_off, count - done);
        struct rd_page *rp = rd_get_page_locked(&device, page_idx);
        void *kaddr;

        if (!rp || !rp->page) {
            // Shouldn't happen: capacity was ensured
            mutex_unlock(&device.lock);
            return -EIO;
        }

        kaddr = kmap_local_page(rp->page);
        if (copy_from_user((u8 *)kaddr + page_off, buf + done, to_copy)) {
            kunmap_local(kaddr);
            mutex_unlock(&device.lock);
            return -EFAULT;
        }
        kunmap_local(kaddr);

        done += to_copy;
    }

    // Advance file position
    *ppos += done;

    // Update logical file size if we extended past EOF
    if (*ppos > device.size)
        device.size = *ppos;

    mutex_unlock(&device.lock);
    return done;
}

static const struct file_operations ramdiskext_fops = {
    .owner   = THIS_MODULE,
    .open    = ramdiskext_open,
    .release = ramdiskext_release,
    .llseek  = ramdiskext_llseek,
    .read    = ramdiskext_read,
    .write   = ramdiskext_write,
};

/**
 * ramdiskext_init - Module initialization.
 * Registers the character device and initializes state.
 */
static int __init ramdiskext_init(void)
{
    INIT_LIST_HEAD(&device.pages);
    device.npages = 0;
    device.size   = 0;
    mutex_init(&device.lock);

    major_number = register_chrdev(DEFAULT_MAJOR, DEVICE_NAME, &ramdiskext_fops);
    if (major_number < 0) {
        pr_err("ramdiskext: register_chrdev failed: %d\n", major_number);
        return major_number;
    }

    pr_info("ramdiskext: registered with major %d\n", major_number);
    pr_info("ramdiskext: create node: mknod /dev/%s c %d 0 && chmod 666 /dev/%s\n",
            DEVICE_NAME, major_number, DEVICE_NAME);
    return 0;
}

/**
 * ramdiskext_exit - Module cleanup.
 * Unregisters the device and frees all pages.
 */
static void __exit ramdiskext_exit(void)
{
    unregister_chrdev(major_number, DEVICE_NAME);

    mutex_lock(&device.lock);
    rd_free_all_pages_locked(&device);
    mutex_unlock(&device.lock);

    pr_info("ramdiskext: unloaded, all pages freed\n");
}

module_init(ramdiskext_init);
module_exit(ramdiskext_exit);