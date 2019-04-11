#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/pid.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

static int pid;

module_param(pid, int, 0644);

static int __init mypinfo_param_init(void)
{
	struct pid* ppid;
	struct task_struct *task, *parent;
	struct list_head *list;
	int isibling, ichildren;
	
	ppid = find_get_pid(pid);
	task = pid_task(ppid, PIDTYPE_PID);
	
	printk(KERN_ALERT"Start!(pinfo_param pid=%d)", pid);
	parent = task->parent;
	printk(KERN_ALERT"Parent\t%s\t%d\t%ld", parent->comm, parent->pid, parent->state);

	isibling = 0;
	ichildren = 0;
	
	//get info of siblings from parent's children list
	list_for_each(list, &parent->children)
	{
		struct task_struct *p;
		p = list_entry(list, struct task_struct, sibling);
		if(p->pid != pid)
			isibling++;
			printk(KERN_ALERT"Sibling\t%s\t%d\t%ld",  p->comm, p->pid, p->state);
	}
	printk(KERN_ALERT"%d siblings",  isibling);
	
	//get info of children
	list_for_each(list, &task->children)
	{
		struct task_struct *p;
		p = list_entry(list, struct task_struct, sibling);
		ichildren++;
		printk(KERN_ALERT"Child\t%s\t%d\t%ld",  p->comm, p->pid, p->state);
	}
	printk(KERN_ALERT"%d children",  ichildren);

	return 0;
	
}

static void __exit mypinfo_param_exit(void)
{
	printk(KERN_ALERT"Goodbye!(pinfo_param)\n");
}

module_init(mypinfo_param_init);
module_exit(mypinfo_param_exit);
