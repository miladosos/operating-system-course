#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Jiffies-OS-Proj1");
MODULE_AUTHOR("Milad Noroozi");
MODULE_VERSION("0.01");


#define BUFFER_SIZE 256
#define PROC_NAME "jiffies"

struct proc_dir_entry *procDirEntry;

ssize_t read_info(struct file *, char __user *, size_t, loff_t *);

ssize_t write_info(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .read = read_info,
        .write = write_info,
};

int init_module(void) {
    int ret = 0;

    procDirEntry = proc_create(PROC_NAME, 0666, NULL, &fops);

    if (procDirEntry == NULL) {
        ret = -1;
        printk(KERN_WARNING "%s could not be created\n", PROC_NAME);
    } else {
        printk(KERN_INFO "%s could not be created\n", PROC_NAME);
    }

    return ret;
}

void exit_module(void) {
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "%s unloaded.\n", PROC_NAME);
}


ssize_t write_info(struct file *filep, const char __user *ubuffer, size_t size, loff_t *offset) {
    int write_result = write_into_proc(ubuffer, jiffies);
    return write_result;
}

ssize_t read_info(struct file *filep, char __user *ubuffer, size_t size, loff_t *offset) {
    if (write_into_proc(ubuffer, jiffies)) {
        printk(KERN_WARNING, "could not write to proc <%s> while reading", PROC_NAME);
    }
    return 0;
}

int write_into_proc(char __user *ubuffer, unsigned long int number) {
    char buffer[BUFFER_SIZE];

    int rv = sprintf(buffer, "%lu\n", number);
    int result = copy_to_user(ubuffer, buffer, rv);
    return result;
}

module_init(init_module)
module_exit(exit_module)
