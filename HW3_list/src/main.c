#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "kernel_stack.h"

static int __init kernel_stack_init(void)
{
    int ret = kernel_stack_sysfs_init();

    if (ret) {
        pr_err("init failed: %d\n", ret);
        return ret;
    }

    pr_info("init\n");
    return 0;
}

static void __exit kernel_stack_exit(void)
{
    kernel_stack_sysfs_exit();
    pr_info("exit\n");
}

module_init(kernel_stack_init);
module_exit(kernel_stack_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey Ogurcov");
MODULE_DESCRIPTION("Stack on list_head with sysfs interface");
