#ifndef SCULL_H
#define SCULL_H

#include <linux/fs.h>
#include <linux/cdev.h>

#ifndef SCULL
#define SCULL "scull"
#endif

#ifndef SCULL_MAJOR
#define SCULL_MAJOR 0
#endif

#ifndef SCULL_NUM_DEV
#define SCULL_NUM_DEV 4
#endif

#ifndef SCULL_MINOR
#define SCULL_MINOR 0
#endif

#ifndef SCULL_QSET
#define SCULL_QSET 1000
#endif

#ifndef SCULL_QUANTUM
#define SCULL_QUANTUM 4000
#endif

loff_t llseek(struct file *filp, loff_t off, int whence);
ssize_t scull_read(struct file *filp, char __user *buffer, size_t count, loff_t *f_pos);
ssize_t scull_write(struct file *filp, char __user *buffer, size_t count, loff_t *f_pos);
int scull_ioctl(struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg);
int scull_open(struct inode *node, struct file *filp);
int scull_release(struct inode *node, struct fiel *filp);

struct scull_qset {
    void **data;             //array of bytes, scull_qset pointers to areas of scull_quantum bytes
    struct scull_qset *next; //the next set of data
};

struct scull_dev {
    struct scull_qset *data;    //pointer to the first set of data
    int quantum;                //the number of bytes stored in each array index
    int qset;                   //length of each array of data
    unsigned long size;         //number of total bytes stored
    struct cdev cdev;           //linux char device
};

int scull_setup_cdev(struct scull_dev *scull_dev, int index);
#endif
