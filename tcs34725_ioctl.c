#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#define DRIVER_NAME "tcs34725_driver"
#define CLASS_NAME "tcs34725"
#define DEVICE_NAME "tcs34725"

// Registers
#define TCS34725_ENABLE      0x00
#define TCS34725_ATIME       0x01
#define TCS34725_CONTROL     0x0F
#define TCS34725_ID          0x12
#define TCS34725_CDATAL      0x14
#define TCS34725_COMMAND_BIT 0x80

// IOCTL definitions
#define TCS34725_IOCTL_MAGIC     't'
#define TCS34725_IOCTL_READ_CLEAR  _IOR(TCS34725_IOCTL_MAGIC, 1, int)
#define TCS34725_IOCTL_READ_RED    _IOR(TCS34725_IOCTL_MAGIC, 2, int)
#define TCS34725_IOCTL_READ_GREEN  _IOR(TCS34725_IOCTL_MAGIC, 3, int)
#define TCS34725_IOCTL_READ_BLUE   _IOR(TCS34725_IOCTL_MAGIC, 4, int)

static struct i2c_client *tcs34725_client;
static struct class *tcs34725_class;
static struct device *tcs34725_device;
static int major_number;
static DEFINE_MUTEX(tcs34725_mutex);

static int tcs34725_enable(struct i2c_client *client)
{
    int ret = i2c_smbus_write_byte_data(client, TCS34725_COMMAND_BIT | TCS34725_ENABLE, 0x03);
    return ret < 0 ? ret : 0;
}

static int tcs34725_read_color(struct i2c_client *client, int offset)
{
    u8 buf[2];
    int ret;

    ret = i2c_smbus_read_i2c_block_data(client, TCS34725_COMMAND_BIT | (TCS34725_CDATAL + offset), 2, buf);
    if (ret < 0) return ret;

    return (buf[1] << 8) | buf[0];
}

static long tcs34725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int value = 0;

    if (!tcs34725_client) {
        printk(KERN_ERR "TCS34725: client not available\n");
        return -ENODEV;
    }

    mutex_lock(&tcs34725_mutex);

    switch (cmd) {
        case TCS34725_IOCTL_READ_CLEAR:
            value = tcs34725_read_color(tcs34725_client, 0);
            break;
        case TCS34725_IOCTL_READ_RED:
            value = tcs34725_read_color(tcs34725_client, 2);
            break;
        case TCS34725_IOCTL_READ_GREEN:
            value = tcs34725_read_color(tcs34725_client, 4);
            break;
        case TCS34725_IOCTL_READ_BLUE:
            value = tcs34725_read_color(tcs34725_client, 6);
            break;
        default:
            mutex_unlock(&tcs34725_mutex);
            return -EINVAL;
    }

    mutex_unlock(&tcs34725_mutex);

    if (value < 0) return value;

    if (copy_to_user((int __user *)arg, &value, sizeof(value)))
        return -EFAULT;

    return 0;
}

static int tcs34725_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725: Device opened\n");
    return 0;
}

static int tcs34725_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725: Device closed\n");
    return 0;
}

static struct file_operations fops = {
    .open = tcs34725_open,
    .release = tcs34725_release,
    .unlocked_ioctl = tcs34725_ioctl,
    .owner = THIS_MODULE,
};

static int tcs34725_probe(struct i2c_client *client)
{
    int id, i;

    printk(KERN_INFO "TCS34725: Probing at address 0x%02X\n", client->addr);

    for (i = 0; i < 3; i++) {
        id = i2c_smbus_read_byte_data(client, TCS34725_COMMAND_BIT | TCS34725_ID);
        if (id >= 0) break;
        msleep(10);
    }

    if (id < 0) {
        printk(KERN_ERR "TCS34725: Failed to read ID register\n");
        return id;
    }

    if ((id & 0xF0) != 0x40) {
        printk(KERN_ERR "TCS34725: Unexpected ID 0x%02X\n", id);
        return -ENODEV;
    }

    tcs34725_client = client;

    if (tcs34725_enable(client) < 0) {
        printk(KERN_ERR "TCS34725: Failed to enable sensor\n");
        return -EIO;
    }

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "TCS34725: Failed to register char device\n");
        return major_number;
    }

    tcs34725_class = class_create(CLASS_NAME);
    if (IS_ERR(tcs34725_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(tcs34725_class);
    }

    tcs34725_device = device_create(tcs34725_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(tcs34725_device)) {
        class_destroy(tcs34725_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(tcs34725_device);
    }

    mutex_init(&tcs34725_mutex);
    printk(KERN_INFO "TCS34725: Driver loaded, device /dev/%s created\n", DEVICE_NAME);
    return 0;
}

static void tcs34725_remove(struct i2c_client *client)
{
    device_destroy(tcs34725_class, MKDEV(major_number, 0));
    class_destroy(tcs34725_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    mutex_destroy(&tcs34725_mutex);
    printk(KERN_INFO "TCS34725: Driver removed\n");
}

static const struct of_device_id tcs34725_of_match[] = {
    { .compatible = "taos,tcs34725" },
    { }
};
MODULE_DEVICE_TABLE(of, tcs34725_of_match);

static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = tcs34725_of_match,
    },
    .probe = tcs34725_probe,
    .remove = tcs34725_remove,
};

static int __init tcs34725_init(void)
{
    printk(KERN_INFO "TCS34725: Initializing driver\n");
    return i2c_add_driver(&tcs34725_driver);
}

static void __exit tcs34725_exit(void)
{
    printk(KERN_INFO "TCS34725: Exiting driver\n");
    i2c_del_driver(&tcs34725_driver);
}

module_init(tcs34725_init);
module_exit(tcs34725_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mai Minh Duc");
MODULE_DESCRIPTION("TCS34725 I2C Color Sensor Driver with IOCTL interface");
