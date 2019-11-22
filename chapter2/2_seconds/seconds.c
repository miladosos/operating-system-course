#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Seconds-OS-Proj1");
MODULE_AUTHOR("Milad Noroozi");
MODULE_VERSION("0.01");


#define BUFFER_SIZE 128
#define PROC_NAME "seconds"

ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos);

static struct file_operations proc_ops = {
        .owner = THIS_MODULE,
        .read = proc_read,
};

int proc_init(void) {
    proc_create(PROC_NAME, 0666, NULL, &proc_ops);

    return 0;
}

void proc_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
}


ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos) {
    char buffer[BUFFER_SIZE];

    int rv = sprintf(buffer, "%lu\n", jiffies / HZ);

    copy_to_user(usr_buf, buffer, rv);

    return rv;
}

module_init(proc_init)
module_exit(proc_exit)
