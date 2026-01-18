#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>

#include "kernel_stack.h"
#include "stack.h"
#include "stack_ops.h"

static struct kobject *ks_kobj;
static struct stack ks;
static DEFINE_MUTEX(ks_lock);

/* push (write) */
static ssize_t push_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t count)
{
    int v, ret;

    ret = kstrtoint(buf, 10, &v);
    if (ret)
        return ret;

    mutex_lock(&ks_lock);
    ret = stack_push(&ks, v);
    mutex_unlock(&ks_lock);

    if (ret != STACK_OK)
        return -ENOMEM;

    return count;
}

static struct kobj_attribute push_attr = __ATTR_WO(push);

/* pop (read) */
static ssize_t pop_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
    int v, ret;

    mutex_lock(&ks_lock);
    ret = stack_pop(&ks, &v);
    mutex_unlock(&ks_lock);

    if (ret == STACK_EMPTY)
        return scnprintf(buf, PAGE_SIZE, "EMPTY\n");
    if (ret != STACK_OK)
        return scnprintf(buf, PAGE_SIZE, "ERROR\n");

    return scnprintf(buf, PAGE_SIZE, "%d\n", v);
}

static struct kobj_attribute pop_attr = __ATTR_RO(pop);

/* peek (read) */
static ssize_t peek_show(struct kobject *kobj, struct kobj_attribute *attr,
                         char *buf)
{
    int v, ret;

    mutex_lock(&ks_lock);
    ret = stack_peek(&ks, &v);
    mutex_unlock(&ks_lock);

    if (ret == STACK_EMPTY)
        return scnprintf(buf, PAGE_SIZE, "EMPTY\n");
    if (ret != STACK_OK)
        return scnprintf(buf, PAGE_SIZE, "ERROR\n");

    return scnprintf(buf, PAGE_SIZE, "%d\n", v);
}

static struct kobj_attribute peek_attr = __ATTR_RO(peek);

/* size (read) */
static ssize_t size_show(struct kobject *kobj, struct kobj_attribute *attr,
                         char *buf)
{
    int s;

    mutex_lock(&ks_lock);
    s = stack_size(&ks);
    mutex_unlock(&ks_lock);

    return scnprintf(buf, PAGE_SIZE, "%d\n", s);
}

static struct kobj_attribute size_attr = __ATTR_RO(size);

/* is_empty (read) */
static ssize_t is_empty_show(struct kobject *kobj, struct kobj_attribute *attr,
                             char *buf)
{
    int e;

    mutex_lock(&ks_lock);
    e = stack_is_empty(&ks);
    mutex_unlock(&ks_lock);

    return scnprintf(buf, PAGE_SIZE, "%d\n", e);
}

static struct kobj_attribute is_empty_attr = __ATTR_RO(is_empty);

/* clear (write) */
static ssize_t clear_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
    mutex_lock(&ks_lock);
    stack_clear(&ks);
    mutex_unlock(&ks_lock);

    return count;
}

static struct kobj_attribute clear_attr = __ATTR_WO(clear);

static struct attribute *ks_attrs[] = {
    &push_attr.attr,
    &pop_attr.attr,
    &peek_attr.attr,
    &size_attr.attr,
    &is_empty_attr.attr,
    &clear_attr.attr,
    NULL,
};

static struct attribute_group ks_attr_group = {
    .attrs = ks_attrs,
};

int kernel_stack_sysfs_init(void)
{
    int ret;

    stack_init(&ks);

    ks_kobj = kobject_create_and_add("kernel_stack", kernel_kobj);
    if (!ks_kobj)
        return -ENOMEM;

    ret = sysfs_create_group(ks_kobj, &ks_attr_group);
    if (ret) {
        kobject_put(ks_kobj);
        ks_kobj = NULL;
        return ret;
    }

    pr_info("sysfs: /sys/kernel/kernel_stack created\n");
    return 0;
}

void kernel_stack_sysfs_exit(void)
{
    if (!ks_kobj)
        return;

    /* на выходе обязательно чистим память стека */
    mutex_lock(&ks_lock);
    stack_clear(&ks);
    mutex_unlock(&ks_lock);

    sysfs_remove_group(ks_kobj, &ks_attr_group);
    kobject_put(ks_kobj);
    ks_kobj = NULL;

    pr_info("sysfs removed\n");
}
