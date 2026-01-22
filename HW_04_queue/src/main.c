// src/main.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/* внешние функции из fifo_ops.c */
int fifo_init(int max_size);
void fifo_free(void);
void fifo_clear(void);

/* ёмкость FIFO (в элементах int), задаётся при загрузке */
static int max_size = 16;
module_param(max_size, int, 0444);
MODULE_PARM_DESC(max_size, "FIFO capacity in int elements");

static int __init kernel_fifo_init(void)
{
    int ret = fifo_init(max_size);

    if (ret < 0) {
        pr_err("init failed: %d\n", ret);
        return -ENOMEM;
    }

    pr_info("init\n");
    return 0;
}

static void __exit kernel_fifo_exit(void)
{
    //fifo_clear();
    fifo_free();
    pr_info("exit\n");
}

module_init(kernel_fifo_init);
module_exit(kernel_fifo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("you");
MODULE_DESCRIPTION("kernel_fifo: FIFO via kfifo + module_param_cb");
