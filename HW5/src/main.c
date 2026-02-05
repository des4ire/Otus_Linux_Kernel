// src/main.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "allocator.h"

static int __init kernel_alloc_init(void)
{
    int ret = allocator_init();

    if (ret != ALLOC_OK) {
        pr_err("init failed: %d\n", ret);
        return -ENOMEM;
    }

    pr_info("init\n");
    return 0;
}

static void __exit kernel_alloc_exit(void)
{
    allocator_cleanup();
    pr_info("exit\n");
}

module_init(kernel_alloc_init);
module_exit(kernel_alloc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey Ogurcov");
MODULE_DESCRIPTION("kernel_alloc: bitmap allocator + module_param_cb");
