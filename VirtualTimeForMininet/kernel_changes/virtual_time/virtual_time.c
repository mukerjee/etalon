#include <linux/kernel.h>

/* well, this is just a toy */
asmlinkage long sys_helloworld(void)
{
    printk("Hello world from Jiaqi\n");
    return 0;
}
