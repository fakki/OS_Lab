#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init_task.h>

static int pinfo_init(void)
{
	struct task_struct *p;
	p = &init_task;
	printk(KERN_ALERT"NAME	PID   STAT   PRIO  PPID\n");
	for_each_process(p){
		if(p->mm ==NULL)
			printk(KERN_ALERT"%s\t%d\t%ld\t%d\t%d\n", p->comm, p->pid, p->state, p->normal_prio, p->parent->pid);
	}
	return 0;
}

static void pinfo_exit(void)
{
	printk(KERN_ALERT"Goodbye!(module pinfo)\n");
}

module_init(pinfo_init);
module_exit(pinfo_exit);

MODULE_LICENSE("GPL");
