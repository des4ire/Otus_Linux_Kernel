// hw2.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/mm.h>

#define MAX_SIZE 64

static DEFINE_MUTEX(lock);

/* idx — индекс в массиве (0..MAX_SIZE-2, чтобы оставалось место под '\0') */
static unsigned int idx;

/* ch_val — ASCII-код последнего записанного символа */
static unsigned int ch_val;

/* my_str — итоговая строка (read-only) */
static char my_str[MAX_SIZE];
static size_t my_len;

static int ensure_len(size_t new_len)
{
    size_t i;

    if (new_len > MAX_SIZE - 1)
        return -ERANGE;

    /* заполняем “дыры” пробелами, чтобы строка не обрывалась '\0' */
    for (i = my_len; i < new_len; i++)
        my_str[i] = ' ';

    my_len = new_len;
    my_str[my_len] = '\0';
    return 0;
}

/* ===== idx param callbacks ===== */

static int idx_set(const char *val, const struct kernel_param *kp)
{
    unsigned int v;
    int ret = kstrtouint(val, 10, &v);

    if (ret)
        return ret;

    if (v > MAX_SIZE - 2)
        return -ERANGE;

    mutex_lock(&lock);
    idx = v;
    mutex_unlock(&lock);

    pr_info("idx=%u\n", idx);
    return 0;
}

static int idx_get(char *buf, const struct kernel_param *kp)
{
    unsigned int v;

    mutex_lock(&lock);
    v = idx;
    mutex_unlock(&lock);

    return scnprintf(buf, PAGE_SIZE, "%u", v);
}

static const struct kernel_param_ops idx_ops = {
    .set = idx_set,
    .get = idx_get,
};

module_param_cb(idx, &idx_ops, NULL, 0664);
MODULE_PARM_DESC(idx, "Index in my_str to write to");

/* ===== ch_val param callbacks ===== */

static int ch_val_set(const char *val, const struct kernel_param *kp)
{
    unsigned int v;
    int ret = kstrtouint(val, 10, &v);
    char c;
    size_t pos;

    if (ret)
        return ret;

    /* печатаемый ASCII (включая пробел) */
    if (v > 127)
        return -EINVAL;

    c = (char)v;
    if (!isprint((unsigned char)c))
        return -EINVAL;

    mutex_lock(&lock);

    pos = idx;
    ret = ensure_len(pos + 1);
    if (ret) {
        mutex_unlock(&lock);
        return ret;
    }

    ch_val = v;
    my_str[pos] = c;

    mutex_unlock(&lock);

    pr_info("write my_str[%zu]=%u('%c') -> \"%s\"\n", pos, ch_val, c, my_str);
    return 0;
}

static int ch_val_get(char *buf, const struct kernel_param *kp)
{
    unsigned int v;

    mutex_lock(&lock);
    v = ch_val;
    mutex_unlock(&lock);

    return scnprintf(buf, PAGE_SIZE, "%u", v);
}

static const struct kernel_param_ops ch_val_ops = {
    .set = ch_val_set,
    .get = ch_val_get,
};

module_param_cb(ch_val, &ch_val_ops, NULL, 0664);
MODULE_PARM_DESC(ch_val, "ASCII code of character to write at idx");

/* ===== my_str param callbacks (RO) ===== */

static int my_str_set(const char *val, const struct kernel_param *kp)
{
    /* строго запрещаем запись */
    return -EPERM;
}

static int my_str_get(char *buf, const struct kernel_param *kp)
{
    int ret;

    mutex_lock(&lock);
    ret = scnprintf(buf, PAGE_SIZE, "%s", my_str);
    mutex_unlock(&lock);

    return ret;
}

static const struct kernel_param_ops my_str_ops = {
    .set = my_str_set,
    .get = my_str_get,
};

module_param_cb(my_str, &my_str_ops, NULL, 0444);
MODULE_PARM_DESC(my_str, "Result string (read-only)");

static int __init hw2_init(void)
{
    mutex_lock(&lock);
    idx = 0;
    ch_val = 0;
    my_len = 0;
    my_str[0] = '\0';
    mutex_unlock(&lock);

    pr_info("init\n");
    return 0;
}

static void __exit hw2_exit(void)
{
    pr_info("exit\n");
}

module_init(hw2_init);
module_exit(hw2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Ogurcov");
MODULE_DESCRIPTION("HW2: build 'Hello, World!' via module params");
