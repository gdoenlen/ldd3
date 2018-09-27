#include <linux/module.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/uaccess.h> //ldd text says to use asm/uaccess.h

#include "scull.h"

MODULE_LICENSE("GPL")

int scull_num_devices = SCULL_NUM_DEV;
int scull_major = SCULL_MAJOR;
int scull_minor = SCULL_MINOR;
int scull_qset = SCULL_QSET;
int scull_quantum = SCULL_QUANTUM;
struct scull_dev *scull_devices;

loff_t llseek(struct file *filp, loff_t off, int whence)
{
    return off;
}

/**
 * @brief scull_read
 * @param filp file pointer supplied by the kernel
 * @param buffer where to put the data
 * @param count the amount of data to be read
 * @param f_pos the position in the driver to read from.
 * @return the amount of bytes read
 */
ssize_t scull_read(struct file *filp, char __user *buffer, size_t count, loff_t *f_pos)
{
    ssize_t ret = 0;
    scull_dev *dev = filp->private_data;
    int quantum = dev->quantum;
    int qset = dev->qset;
    int item_size = quantum * qset;


    //trying to read past the buffer
    //return 0 since we're at EOF`
    if (*f_pos >= dev->size) {
        return 0;
    }

    //if the current position + the current amount to be read is
    //greater than the amount of data we have we need to
    //set count to only read the amount we can give it
    if (*f_pos + count > dev->size) {
        count = dev->size - *f_pos;
    }

    int lstIndex = (long)*f_pos / item_size;
    int rest = (long)*f_pos % item_size;
    int dataIndex = rest / quantum;
    int bytePosition = rest % quantum;

    struct scull_qset *lst = dev->qset;

    while (lstIndex--) {
        lst = lst->next;
    }

    if (NULL == lst || !lst->data || !lst->data[dataIndex]) {
        return 0;
    }

    //only read to the end of this list item
    if (count > quantum - bytePosition) {
        count = quantum - bytePosition;
    }

    return copy_to_user(buffer, lst->data[dataIndex] + bytePosition, count);
}

ssize_t scull_write(struct file *filp, char __user *buffer, size_t count, loff_t *f_pos)
{

    return count;
}

int scull_ioctl(struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    return cmd;
}

int scull_trim(struct scull_dev *dev)
{
    struct qset *data, *next;
    int qset = dev->qset;
    int i;

    for (data = dev->data; data; data = next) {
        if (data->data) {
            for (i = 0; i < qset; i++) {
                kfree(data->data[i]);
            }
            kfree(data->data);
            data->data = NULL;
        }
        next = data->next;
        kfree(data);
    }

    dev->size = 0;
    dev->data = NULL;
    dev->qset = scull_qset;
    dev->quantum = scull_quantum;
    return 0;
}

/**
 * @brief scull_open
 * @param node
 * @param filp
 * @return
 */
int scull_open(struct inode *node, struct file *filp)
{
    struct scull_dev *dev = container_of(node->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;

    //opened for writing, we need to free all the data
    if (filp->f_mode == FMODE_WRITE) {
        scull_trim(dev);
    }

    return 0;
}


int scull_release(struct inode *node, struct file *filp)
{
    return 0;
}

static struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .llseek = scull_llseek,
    .read = scull_read,
    .write = scull_write,
    .ioctl = scull_ioctl,
    .open = scull_open,
    .release = scull_release
};

int scull_setup_cdev(struct scull_dev *scull_dev, int index)
{
    dev_t dev = MKDEV(scull_major, scull_minor + index);
    cdev_init(&scull_dev->cdev, &scull_fops);
    scull_dev->cdev.owner = THIS_MODULE;
    scull_dev->cdev.ops = &scull_fops;

    return cdev_add(&scull_dev->cdev, dev, 1);
}

static int __init scull_init(void)
{
    int result, i;
    dev_t dev = 0;

    if (scull_major) {
        dev = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(dev, scull_num_devices, SCULL);
    } else {
        result = alloc_chrdev_region(&dev, scull_minor, scull_num_devices, SCULL);
        scull_major = MAJOR(dev);
    }
    if (result < 0) {
        printk(KERN_WARNING "ERROR allocating chr dev region");
        return result;
    }

    scull_devices = kzalloc(scull_num_devices * sizeof(struct scull_dev), GFP_KERNEL);
    if (scull_devices) {
        return -ENOMEM;
    }

    for (i = 0; i < scull_num_devices; i++) {
        if (scull_setup_cdev(scull_devices, i)) {
            //fail
        }
    }
    return result;
}

static void scull_exit(void)
{
    dev_t dev = MKDEV(scull_major, scull_minor);
    unregister_chrdev_region(dev, scull_num_devices);
    scull_trim(scull_devices)
    //todo free scull_dev
}

module_init(scull_init)
module_exit(scull_exit)
