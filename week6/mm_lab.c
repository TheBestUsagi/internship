// mm_lab.c — kmalloc vs vmalloc 物理连续性实验
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/io.h>        // virt_to_phys
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("you");
MODULE_DESCRIPTION("kmalloc vs vmalloc contiguity lab");

// 模块参数
static unsigned long req_size = 1 << 20;    // 默认 1MB
module_param(req_size, ulong, 0444);
MODULE_PARM_DESC(req_size, "bytes to allocate (default 1MB)");

static bool do_vmalloc;                      // false=kmalloc, true=vmalloc
module_param(do_vmalloc, bool, 0444);
MODULE_PARM_DESC(do_vmalloc, "use vmalloc when true");

static void *buf;

static void check_kmalloc_pages(void *p, size_t n)
{
    unsigned long v = (unsigned long)p;
    phys_addr_t p0 = virt_to_phys((void *)v);
    size_t pages = (n + PAGE_SIZE - 1) >> PAGE_SHIFT;
    size_t disc = 0;

    pr_info("[kmalloc] virt=%px phys(first)=%pa size=%zu\n", p, &p0, n);

    for (size_t i = 1; i < pages; ++i) {
        phys_addr_t pi = virt_to_phys((void *)(v + i * PAGE_SIZE));
        if (pi != p0 + (phys_addr_t)i * PAGE_SIZE) disc++;
    }
    pr_info("[kmalloc] pages=%zu, discontinuities=%zu (0 表示物理连续)\n", pages, disc);
}

static void check_vmalloc_pages(void *p, size_t n)
{
    unsigned long v = (unsigned long)p;
    struct page *pg0 = vmalloc_to_page((void *)v);
    phys_addr_t p0 = page_to_phys(pg0);
    size_t pages = (n + PAGE_SIZE - 1) >> PAGE_SHIFT;
    size_t disc = 0, eq = 0;
    phys_addr_t prev = p0;

    pr_info("[vmalloc] virt=%px phys(first)=%pa size=%zu\n", p, &p0, n);

    for (size_t i = 1; i < pages; ++i) {
        struct page *pgi = vmalloc_to_page((void *)(v + i * PAGE_SIZE));
        phys_addr_t pi = page_to_phys(pgi);
        if (pi != prev + PAGE_SIZE) disc++; else eq++;
        prev = pi;
    }
    pr_info("[vmalloc] pages=%zu, +1 连续=%zu, discontinuities=%zu（通常 >0）\n", pages, eq, disc);
}

static int __init mm_lab_init(void)
{
    size_t n = (size_t)req_size;

    if (n == 0 || n > (1UL<<30)) {
        pr_err("[mm_lab] bad req_size=%lu\n", req_size);
        return -EINVAL;
    }

    if (do_vmalloc) {
        buf = vmalloc(n);
        if (!buf) return -ENOMEM;
        pr_info("[mm_lab] vmalloc ok\n");
        check_vmalloc_pages(buf, n);
    } else {
        buf = kmalloc(n, GFP_KERNEL);
        if (!buf) {
            pr_warn("[mm_lab] kmalloc %lu bytes failed\n", req_size);
            return -ENOMEM;
        }
        pr_info("[mm_lab] kmalloc ok\n");
        check_kmalloc_pages(buf, n);
    }

    // 简单可访问性验证：首尾写入标记并读回
    ((u8 *)buf)[0] = 0x5A;
    ((u8 *)buf)[n - 1] = 0xA5;
    pr_info("[mm_lab] head=%02x tail=%02x\n", ((u8 *)buf)[0], ((u8 *)buf)[n-1]);

    return 0;
}

static void __exit mm_lab_exit(void)
{
    if (buf) {
        if (do_vmalloc) vfree(buf);
        else kfree(buf);
    }
    pr_info("[mm_lab] unloaded\n");
}

module_init(mm_lab_init);
module_exit(mm_lab_exit);

