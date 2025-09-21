#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/ioctl.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
#  define CLASS_CREATE(name) class_create(name)
#else
#  define CLASS_CREATE(name) class_create(THIS_MODULE, name)
#endif

#define DRV_NAME "mychar"
#define DEFAULT_BUF_SIZE 4096

/* ioctl 命令（用户态要同样的定义） */
#define MYCHAR_IOC_MAGIC     'M'
#define MYCHAR_IOC_SET_BUFSZ _IOW(MYCHAR_IOC_MAGIC, 1, unsigned long)
#define MYCHAR_IOC_GET_BUFSZ _IOR(MYCHAR_IOC_MAGIC, 2, unsigned long)
#define MYCHAR_IOC_CLEAR     _IO(MYCHAR_IOC_MAGIC,  3)
#define MYCHAR_IOC_GET_LEN   _IOR(MYCHAR_IOC_MAGIC, 4, unsigned long)

static dev_t devno;
static struct cdev my_cdev;
static struct class *my_class;

/* 驱动的“内存盒子”与其当前有效长度/容量 */
static char  *buf;            // kmalloc/krealloc 的缓冲
static size_t data_len;       // 0..buf_cap
static size_t buf_cap;        // ★ 运行期可变容量

/* 互斥锁：保护 buf / data_len / buf_cap 的并发访问 */
static DEFINE_MUTEX(buf_lock);

static int my_open(struct inode *inode, struct file *filp)
{
    pr_info(DRV_NAME ": open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
    pr_info(DRV_NAME ": release\n");
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
    ssize_t ret;

    mutex_lock(&buf_lock);

    if (data_len == 0) {
        ret = 0;    /* EOF */
        goto out;
    }

    if (count > data_len)
        count = data_len;

    if (copy_to_user(ubuf, buf, count)) {
        ret = -EFAULT;
        goto out;
    }

    /* 队列语义：读走前 count 字节，剩余整体左移 */
    memmove(buf, buf + count, data_len - count);
    data_len -= count;

    ret = count;

out:
    mutex_unlock(&buf_lock);
    return ret;
}

static ssize_t my_write(struct file *filp, const char __user *ubuf, size_t count, loff_t *ppos)
{
    size_t free_space;
    ssize_t ret;

    mutex_lock(&buf_lock);

    free_space = buf_cap - data_len;      // ★ 可变容量
    if (free_space == 0) {
        ret = -ENOSPC;
        goto out;
    }

    if (count > free_space)
        count = free_space;

    if (copy_from_user(buf + data_len, ubuf, count)) {
        ret = -EFAULT;
        goto out;
    }

    data_len += count;
    ret = count;

out:
    mutex_unlock(&buf_lock);
    return ret;
}

/* ---------- ioctl：控制面 ---------- */
static long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret = 0;

    if (_IOC_TYPE(cmd) != MYCHAR_IOC_MAGIC)
        return -ENOTTY;

    mutex_lock(&buf_lock);

    switch (cmd) {
    case MYCHAR_IOC_SET_BUFSZ:
    {
        unsigned long new_cap_ul;
        size_t new_cap;
        char *new_buf;

        if (copy_from_user(&new_cap_ul, (void __user *)arg, sizeof(new_cap_ul))) {
            ret = -EFAULT;
            break;
        }
        if (new_cap_ul == 0 || new_cap_ul > (1UL << 30)) { // up to 1GiB
            ret = -EINVAL;
            break;
        }
        new_cap = (size_t)new_cap_ul;

        if (new_cap == buf_cap)
            break;

        new_buf = krealloc(buf, new_cap, GFP_KERNEL);
        if (!new_buf) {
            ret = -ENOMEM;
            break;
        }
        buf = new_buf;
        buf_cap = new_cap;

        if (data_len > buf_cap)   // 缩小时按队列语义截尾
            data_len = buf_cap;

        pr_info(DRV_NAME ": resized to %zu bytes (data_len=%zu)\n", buf_cap, data_len);
        break;
    }

    case MYCHAR_IOC_GET_BUFSZ:
    {
        unsigned long v = (unsigned long)buf_cap;
        if (copy_to_user((void __user *)arg, &v, sizeof(v)))
            ret = -EFAULT;
        break;
    }

    case MYCHAR_IOC_GET_LEN:
    {
        unsigned long v = (unsigned long)data_len;
        if (copy_to_user((void __user *)arg, &v, sizeof(v)))
            ret = -EFAULT;
        break;
    }

    case MYCHAR_IOC_CLEAR:
        data_len = 0;
        break;

    default:
        ret = -ENOTTY;
        break;
    }

    mutex_unlock(&buf_lock);
    return ret;
}

/* 文件操作表 */
static const struct file_operations my_fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .read           = my_read,
    .write          = my_write,
    .unlocked_ioctl = my_ioctl,   // ★ 新增
    // .compat_ioctl = my_ioctl,  // (可选)
};

/* 模块加载入口 */
static int __init my_init(void)
{
    int ret;

    /* 1) 分配默认容量的缓冲 */
    buf_cap = DEFAULT_BUF_SIZE;
    buf = kmalloc(buf_cap, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;
    data_len = 0;

    /* 2) 申请设备号 */
    ret = alloc_chrdev_region(&devno, 0, 1, DRV_NAME);
    if (ret)
        goto err_alloc;

    /* 3) 注册 cdev */
    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, devno, 1);
    if (ret)
        goto err_cdev;

    /* 4) /sys/class + /dev 节点 */
    my_class = CLASS_CREATE(DRV_NAME);
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        goto err_class;
    }

    if (IS_ERR(device_create(my_class, NULL, devno, NULL, DRV_NAME))) {
        ret = -EINVAL;
        goto err_dev;
    }

    pr_info(DRV_NAME ": loaded (major=%d minor=%d, cap=%zu bytes)\n",
            MAJOR(devno), MINOR(devno), buf_cap);
    return 0;

err_dev:
    class_destroy(my_class);
err_class:
    cdev_del(&my_cdev);
err_cdev:
    unregister_chrdev_region(devno, 1);
err_alloc:
    kfree(buf);
    return ret;
}

/* 模块卸载入口 */
static void __exit my_exit(void)
{
    device_destroy(my_class, devno);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(devno, 1);
    kfree(buf);
    pr_info(DRV_NAME ": unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("you");
MODULE_DESCRIPTION("Char driver: queue buffer with dynamic capacity via ioctl (Chinese comments)");

