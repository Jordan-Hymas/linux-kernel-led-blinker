/*
 * blinkkbd.c - Blink keyboard lock LEDs via /proc/blinkkbd
 *
 * Write to /proc/blinkkbd:
 *   L<0-7>  LED bitmask  (bit0=Num, bit1=Caps, bit2=Scroll)
 *   D<0-9>  blink speed  (HZ / divisor, D0 stops)
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CS430 Student");
MODULE_DESCRIPTION("Blink keyboard lock LEDs via /proc/blinkkbd");

static int led_mask      = 7;
static int delay_divisor = 4;
static int blink_state   = 0;

static struct timer_list blink_timer;
static struct input_handle kbd_handle;
static bool kbd_connected = false;

static void set_leds(int on)
{
    int i;

    if (!kbd_connected)
        return;

    for (i = 0; i < 3; i++) {
        if (led_mask & (1 << i))
            input_inject_event(&kbd_handle, EV_LED, i, on);
    }
    input_inject_event(&kbd_handle, EV_SYN, SYN_REPORT, 0);
}

static void blink_timer_fn(struct timer_list *t)
{
    blink_state ^= 1;
    set_leds(blink_state);

    if (delay_divisor > 0)
        mod_timer(&blink_timer, jiffies + HZ / delay_divisor);
}

static int kbd_connect(struct input_handler *handler, struct input_dev *dev,
                       const struct input_device_id *id)
{
    if (kbd_connected)
        return -EBUSY;

    kbd_handle.dev     = input_get_device(dev);
    kbd_handle.handler = handler;
    kbd_handle.name    = "blinkkbd";

    input_register_handle(&kbd_handle);
    input_open_device(&kbd_handle);
    kbd_connected = true;

    pr_info("blinkkbd: connected to %s\n", dev->name);
    return 0;
}

static void kbd_disconnect(struct input_handle *handle)
{
    kbd_connected = false;
    input_close_device(handle);
    input_unregister_handle(handle);
    input_put_device(handle->dev);
}

static const struct input_device_id kbd_ids[] = {
    { .flags = INPUT_DEVICE_ID_MATCH_EVBIT, .evbit = { BIT_MASK(EV_LED) } },
    { }
};
MODULE_DEVICE_TABLE(input, kbd_ids);

static struct input_handler kbd_handler = {
    .name       = "blinkkbd",
    .connect    = kbd_connect,
    .disconnect = kbd_disconnect,
    .id_table   = kbd_ids,
};

static ssize_t proc_write(struct file *f, const char __user *buf,
                          size_t count, loff_t *pos)
{
    char cmd[3];
    int  val;

    if (count < 2 || count > 3)
        return -EINVAL;

    if (copy_from_user(cmd, buf, count))
        return -EFAULT;

    if (count == 3 && cmd[2] != '\n')
        return -EINVAL;

    if (cmd[1] < '0' || cmd[1] > '9')
        return -EINVAL;

    val = cmd[1] - '0';

    if (cmd[0] == 'L') {
        if (val > 7)
            return -EINVAL;

        if (val == 0) {
            del_timer(&blink_timer);
            led_mask = 7;
            set_leds(0);
            led_mask = 0;
            blink_state = 0;
        } else {
            led_mask = val;
            if (delay_divisor > 0 && !timer_pending(&blink_timer))
                mod_timer(&blink_timer, jiffies + HZ / delay_divisor);
        }
        pr_info("blinkkbd: LED mask = %d\n", led_mask);

    } else if (cmd[0] == 'D') {
        delay_divisor = val;

        if (delay_divisor == 0) {
            del_timer(&blink_timer);
            set_leds(0);
            blink_state = 0;
        } else {
            mod_timer(&blink_timer, jiffies + HZ / delay_divisor);
        }
        pr_info("blinkkbd: delay divisor = %d\n", delay_divisor);

    } else {
        return -EINVAL;
    }

    return count;
}

static const struct proc_ops blinkkbd_fops = {
    .proc_write = proc_write,
};

static int __init blinkkbd_init(void)
{
    int err;

    err = input_register_handler(&kbd_handler);
    if (err)
        return err;

    if (!proc_create("blinkkbd", 0222, NULL, &blinkkbd_fops)) {
        input_unregister_handler(&kbd_handler);
        return -ENOMEM;
    }

    timer_setup(&blink_timer, blink_timer_fn, 0);
    mod_timer(&blink_timer, jiffies + HZ / delay_divisor);

    pr_info("blinkkbd: loaded (led_mask=%d, divisor=%d)\n", led_mask, delay_divisor);
    return 0;
}

static void __exit blinkkbd_exit(void)
{
    del_timer_sync(&blink_timer);
    set_leds(0);
    remove_proc_entry("blinkkbd", NULL);
    input_unregister_handler(&kbd_handler);
    pr_info("blinkkbd: unloaded\n");
}

module_init(blinkkbd_init);
module_exit(blinkkbd_exit);
