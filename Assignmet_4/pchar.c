#include<linux/module.h>
#include<linux/device.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/semaphore.h>

#define MAX 32
char buf[MAX];

static dev_t devno;
static struct class *pclass;
static struct cdev cdev;
struct semaphore s;

static int pchar_open(struct inode *pinode, struct file *pfile);
static int pchar_close(struct inode *pinode, struct file *pfile);
static ssize_t pchar_write(struct file *pfile, const char __user *ubuf, size_t size, loff_t *poffset);
static ssize_t pchar_read(struct file *pfile, char __user *ubuf, size_t size, loff_t *poffset);
static loff_t pchar_lseek(struct file *pfile, loff_t offset, int origin);

static struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.open = pchar_open,
	.release = pchar_close,
	.write = pchar_write,
	.read = pchar_read,
	.llseek = pchar_lseek
};

static __init int pchar_init(void)
{
	int ret, major, minor;
	struct device *pdevice;

	printk(KERN_INFO "%s : pchar_init() is called\n", THIS_MODULE->name);
	
	// allocate device number 
	ret = alloc_chrdev_region(&devno, 0, 1, "pchar");
	if(ret < 0)
	{
		printk(KERN_ERR "%s : alloc_chrdev_region() is failed\n", THIS_MODULE->name);
		return ret;
	}
	major = MAJOR(devno);
	minor = MINOR(devno);
	printk(KERN_INFO "%s : alloc_chrdev_region() - allocated devno = %d/%d\n", THIS_MODULE->name, major, minor);

	// create class for devices
	pclass = class_create(THIS_MODULE,"pchar_class");
	if(IS_ERR(pclass))
	{
		printk(KERN_ERR "%s : class_create() is failed\n", THIS_MODULE->name);
		
		unregister_chrdev_region(devno, 1);
		return -1;
	}
	printk(KERN_INFO "%s : class_create() - class is created\n", THIS_MODULE->name);

	// create device
	pdevice = device_create(pclass, NULL, devno, NULL, "pchar%d", 0);
	if(IS_ERR(pdevice))
	{
		printk(KERN_ERR "%s : device_create() is failed\n", THIS_MODULE->name);
		class_destroy(pclass);
		unregister_chrdev_region(devno, 1);
		return -1;
	}
	printk(KERN_INFO "%s : device_create() - device is created\n", THIS_MODULE->name);
	
	// create cdev and add it into cdev map
	cdev_init(&cdev, &f_ops);
	ret = cdev_add(&cdev, devno, 1);
	if(ret < 0)
	{
		printk(KERN_ERR "%s : cdev_add() is failed\n", THIS_MODULE->name);
		
		device_destroy(pclass, devno);
		class_destroy(pclass);
		unregister_chrdev_region(devno, 1);
		return -1;
	}
	printk(KERN_INFO "%s : cdev_add() - device is created\n", THIS_MODULE->name);

	printk(KERN_INFO "%s : pchar_init() is completed\n", THIS_MODULE->name);
     
	
	sema_init(&s, 1);
	printk(KERN_INFO "%s : sema_init() initialized semaphore for the device.\n",THIS_MODULE->name);
    
		return 0;
}

static __exit void pchar_exit(void)
{
	printk(KERN_INFO "%s : pchar_exit() is called\n", THIS_MODULE->name);
	cdev_del(&cdev);
	printk(KERN_INFO "%s : cdev_del() completed\n", THIS_MODULE->name);
	device_destroy(pclass, devno);
	printk(KERN_INFO "%s : device_destroy() completed\n", THIS_MODULE->name);
	class_destroy(pclass);
	printk(KERN_INFO "%s : class_destroy() completed\n", THIS_MODULE->name);
	unregister_chrdev_region(devno, 1);
	printk(KERN_INFO "%s : unregister_chrdev_region() completed\n", THIS_MODULE->name);
	printk(KERN_INFO "%s : pchar_exit() is completed\n", THIS_MODULE->name);
}

// device operations

static int pchar_open(struct inode *pinode, struct file *pfile)
{
	printk(KERN_INFO "%s : pchar_open() is called\n", THIS_MODULE->name);
	down(&s);
	printk(KERN_INFO "%s: semaphore is lock \n",THIS_MODULE->name);
	return 0;
}

static int pchar_close(struct inode *pinode, struct file *pfile)
{
	printk(KERN_INFO "%s : pchar_close() is called\n", THIS_MODULE->name);
	up(&s);
	printk(KERN_INFO "%s: semaphore is unlock \n",THIS_MODULE->name);
	return 0;
}

static ssize_t pchar_write(struct file *pfile, const char __user *ubuf, size_t size, loff_t *poffset)
{
	int avail, bytes_to_write, nbytes;

	printk(KERN_INFO "%s : pchar_write() is called\n", THIS_MODULE->name);
	
	avail = MAX - *poffset;
	printk(KERN_INFO "%s : pchar_write() : avail bytes = %d\n", THIS_MODULE->name, avail);

	bytes_to_write = avail < size ? avail : size;
	printk(KERN_INFO "%s : pchar_write() : bytes to write = %d\n", THIS_MODULE->name, bytes_to_write);
	
	if(bytes_to_write == 0)
	{
		printk(KERN_ERR "%s : pchar_write() : no space available\n", THIS_MODULE->name);
		return -1;
	}

	nbytes = bytes_to_write - copy_from_user(buf + *poffset, ubuf, bytes_to_write);
	printk(KERN_INFO "%s : pchar_write() : nbytes = %d\n", THIS_MODULE->name, nbytes);
	
	// update file position
	*poffset = *poffset + nbytes;

	return nbytes;
}

static ssize_t pchar_read(struct file *pfile, char __user *ubuf, size_t size, loff_t *poffset)
{
	int avail, bytes_to_read, nbytes;

	printk(KERN_INFO "%s : pchar_read() is called\n", THIS_MODULE->name);
	
	avail = MAX - *poffset;
	printk(KERN_INFO "%s : pchar_read() : avail bytes = %d\n", THIS_MODULE->name, avail);

	bytes_to_read = avail < size ? avail : size;
	printk(KERN_INFO "%s : pchar_read() : bytes to read = %d\n", THIS_MODULE->name, bytes_to_read);
	
	if(bytes_to_read == 0)
	{
		printk(KERN_ERR "%s : pchar_read() : no data available\n", THIS_MODULE->name);
		return -1;
	}

	nbytes = bytes_to_read - copy_to_user(ubuf, buf + *poffset, bytes_to_read);
	printk(KERN_INFO "%s : pchar_read() : nbytes = %d\n", THIS_MODULE->name, nbytes);
	
	// update file position
	*poffset = *poffset + nbytes;

	return nbytes;
}

static loff_t pchar_lseek(struct file *pfile, loff_t offset, int origin)
{
	loff_t newpos = -1;
	printk(KERN_INFO "%s : pchar_lseek() is called\n", THIS_MODULE->name);
	
	switch(origin)
	{
		case SEEK_SET:
			newpos = 0 + offset;
			break;

		case SEEK_CUR:
			newpos = pfile->f_pos + offset;
			break;

		case SEEK_END:
			newpos = MAX + offset;
			break;
	}
	if(newpos < 0)
		newpos = 0;
	if(newpos > MAX)
		newpos = MAX;

	pfile->f_pos = newpos;

	return newpos;
}

module_init(pchar_init);
module_exit(pchar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DESD @ SUNBEAM");
MODULE_DESCRIPTION("This is pseudo character device driver");























