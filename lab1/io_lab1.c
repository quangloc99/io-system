#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>

#include "config.h"

#define LOG(msg) "%s: " msg "\n", THIS_MODULE->name

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tran Quang Loc");
MODULE_DESCRIPTION("IO system lab 1");
MODULE_VERSION("0.69");

static struct proc_dir_entry* proc_entry;
static struct class *char_drv_class;
static struct cdev char_dev;
static dev_t first_dev;

// The data for this task
static size_t total_read_chars;

// proc file operations
// ====================
static ssize_t proc_write(struct file *file, const char __user * ubuf, size_t count, loff_t* ppos) {
#ifdef PRINT_EVENT
    printk(KERN_INFO LOG("proc write"));
#endif
	printk(KERN_DEBUG LOG("Nice try :))"));
	return -1;
}

static ssize_t proc_read(struct file *file, char __user * ubuf, size_t count, loff_t* ppos) {
    char num_buf[NUM_BUF_SIZE];
    int printed;
#ifdef PRINT_EVENT
    printk(KERN_INFO LOG("proc read"));
#endif
    printed = snprintf(num_buf, NUM_BUF_SIZE, "%zu", total_read_chars);
    if (printed < 0) {
        return -ENOMEM;
    }
    if (*ppos >= printed) {
        return 0;
    }
    if (count > printed - *ppos) {
        count = printed - *ppos;
    }
	if (copy_to_user(ubuf, num_buf + *ppos, count) != 0) {
		return -EFAULT;
	}
	*ppos += count;
	return count;
}

static struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.read = proc_read,
	.write = proc_write,
};

// dev file operations
// ===================

static int dev_open(struct inode *i, struct file *file) {
#ifdef PRINT_EVENT
    printk(KERN_INFO LOG("dev open"));
#endif
    return 0;
}

static int dev_close(struct inode *i, struct file *file) {
#ifdef PRINT_EVENT
    printk(KERN_INFO LOG("dev close"));
#endif
    return 0;
}

static ssize_t dev_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
    static char my_buf[READ_BUF_SIZE];
    size_t read_next;
#ifdef PRINT_EVENT
    printk(KERN_INFO LOG("dev write: count = %zu, pos = %lld"), count, *ppos);
#endif
    read_next = count < READ_BUF_SIZE ? count : READ_BUF_SIZE;
    if (copy_from_user(my_buf, ubuf, read_next) != 0) {
        return -EFAULT;
    }
    total_read_chars += read_next;
    *ppos += read_next;
    return read_next;
}

static ssize_t dev_read(struct file *file, char __user *ubuf, size_t count, loff_t* ppos) {
#ifdef PRINT_EVENT
    printk(KERN_INFO LOG("dev read"));
#endif
    printk("%zu", total_read_chars);
    return 0;
}

static struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_close,
    .read = dev_read,
    .write = dev_write,
};


static int __init lab1_init(void) {
    total_read_chars = 0;
	proc_entry = proc_create(VARIANT_NAME, S_IRUSR|S_IRGRP|S_IROTH, NULL, &proc_fops);
    printk(KERN_INFO LOG("Initializing"));
    if (!proc_entry) {
        printk(KERN_INFO LOG("Failed to create proc dir entry"));
        goto failed_proc_entry;
    }
    if (alloc_chrdev_region(&first_dev, DEV_FIRST_MAJOR, DEV_COUNT, VARIANT_NAME) < 0) {
        printk(KERN_INFO LOG("Failed to alloc character drive region"));
        goto failed_alloc_chrdev_region;
    }
    
    char_drv_class = class_create(THIS_MODULE, VARIANT_NAME);
    if (!char_drv_class) {
        printk(KERN_INFO LOG("Failed to create character device class"));
        goto failed_class_create;
    }
    
    if (!device_create(char_drv_class, NULL, first_dev, NULL, VARIANT_NAME)) {
        printk(KERN_INFO LOG("Failed to create device"));
        goto failed_device_create;
    }
    
    cdev_init(&char_dev, &dev_fops);
    if (cdev_add(&char_dev, first_dev, 1) < 0) {
        printk(KERN_INFO LOG("Failed to add character device"));
        goto failed_cdev_add;
    }
    printk(KERN_INFO LOG("Sucessfully initialized"));
    
	return 0;
    
    // Error handling in reversed order
failed_cdev_add:
    
    device_destroy(char_drv_class, first_dev);
failed_device_create:
    
    class_destroy(char_drv_class);
failed_class_create:
    
    unregister_chrdev_region(first_dev, DEV_COUNT);
failed_alloc_chrdev_region:
    
	proc_remove(proc_entry);
failed_proc_entry:
    
    return -1;
}

static void __exit lab1_exit(void) {
    cdev_del(&char_dev);
    device_destroy(char_drv_class, first_dev);
    class_destroy(char_drv_class);
    unregister_chrdev_region(first_dev, DEV_COUNT);
	proc_remove(proc_entry);
	printk(KERN_INFO LOG("Sucessfully exit"));
}

module_init(lab1_init);
module_exit(lab1_exit);

