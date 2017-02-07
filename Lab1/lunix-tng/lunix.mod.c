#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xef0eea15, "module_layout" },
	{ 0xe007f632, "cdev_del" },
	{ 0x83768e64, "per_cpu__current_task" },
	{ 0x6a52c3b3, "kmalloc_caches" },
	{ 0x5a34a45c, "__kmalloc" },
	{ 0x405c1144, "get_seconds" },
	{ 0x20c6b2b8, "cdev_init" },
	{ 0x9b388444, "get_zeroed_page" },
	{ 0x6980fe91, "param_get_int" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x973873ab, "_spin_lock" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0xfc4f55f3, "down_interruptible" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xff964b25, "param_set_int" },
	{ 0x712aa29b, "_spin_lock_irqsave" },
	{ 0xf929c015, "nonseekable_open" },
	{ 0xa120d33c, "tty_unregister_ldisc" },
	{ 0xffc7c184, "__init_waitqueue_head" },
	{ 0xea147363, "printk" },
	{ 0x85f8a266, "copy_to_user" },
	{ 0x16305289, "warn_slowpath_null" },
	{ 0x4b07e779, "_spin_unlock_irqrestore" },
	{ 0xdae996c8, "cdev_add" },
	{ 0x7dceceac, "capable" },
	{ 0x7a795a78, "kmem_cache_alloc" },
	{ 0x1000e51, "schedule" },
	{ 0x4302d0eb, "free_pages" },
	{ 0x642e54ac, "__wake_up" },
	{ 0x37a0cba, "kfree" },
	{ 0x33d92f9a, "prepare_to_wait" },
	{ 0x3f1899f1, "up" },
	{ 0x9ccb2622, "finish_wait" },
	{ 0x9edbecae, "snprintf" },
	{ 0xa6c83b8c, "tty_register_ldisc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

