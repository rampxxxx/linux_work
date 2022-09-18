#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/list.h> 
#include <linux/slab.h> 
#include <linux/kthread.h> // kthread_create
#include <linux/delay.h> //msleep_interruptible
#include <linux/debugfs.h> // simple_read_from_buffer


// INI : linked list
//struct list_head my_list;
//INIT_LIST_HEAD(my_list);
LIST_HEAD(my_list);
#define RAMP_NODES 10
// END : linked list

// debugfs
static struct dentry *parent, *filen;
static u32 val_file = (u32) RAMP_NODES;
static u32 val32 = (u32) 777;
#define MAXSTRING 32


static ssize_t
my_read(struct file *file, char __user *buf, size_t lbuf, loff_t *ppos)
{
	int nbytes;
	ssize_t rc;
	char *kstring;

	kstring = kmalloc(MAXSTRING, GFP_KERNEL);
	if (IS_ERR(kstring))
		return PTR_ERR(kstring);

	nbytes = sprintf(kstring, "%d\n", val_file);
	rc = simple_read_from_buffer(buf, lbuf, ppos, kstring, nbytes);
	kfree(kstring);

	pr_info("d_inode = %p\n", parent->d_inode);
	return rc;
}

static ssize_t
my_write(struct file *file, const char __user *buf, size_t lbuf, loff_t *ppos)
{
	/*
	int rc;
	u32 tmp;

	rc = kstrtoint_from_user(buf, lbuf, 10, &tmp);
	if (rc)
		return rc;
	val_file = tmp;
	pr_info("\n WRITING function, nbytes=%ld, val_file=%d\n", lbuf, val);
	return lbuf;
	*/
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
};


// INI : thread stuff
#define MSG_SIZE 256
#define NUM_THREADS 1
static struct task_struct *tsk[NUM_THREADS];

struct my_struct {
  int val;
  struct list_head list;
};

// Util to print avoiding preempt , why? I don't know
static void print_in_thread(char *s)
{
	preempt_disable();
	pr_info("%s \n", s);
	preempt_enable();
}


static void debugfs_support(char *prefix)
{
	char debugfs_name[MSG_SIZE];
	snprintf(debugfs_name, MSG_SIZE-1, "%sdir", prefix);
	parent = debugfs_create_dir(debugfs_name, NULL);
	snprintf(debugfs_name, MSG_SIZE-1, "%sval", prefix);
	debugfs_create_u32(debugfs_name, 0644, parent, &val32);
	snprintf(debugfs_name, MSG_SIZE-1, "%sfile", prefix);
	filen = debugfs_create_file(debugfs_name, 0644, parent, NULL, &fops);
	print_in_thread("debugfs infra created ... \n");
}

static int thr_fun(void *thr_arg)
{
	struct my_struct *node;
	char msg[MSG_SIZE];

	print_in_thread("linked list creating ... \n");

	for (int j = 0; j < RAMP_NODES; j++) {
		node = kmalloc(sizeof(struct my_struct), GFP_KERNEL);
		node->val = j;
		list_add(&node->list, &my_list);
	}

	list_for_each_entry(node, &my_list, list) {
		snprintf(msg, MSG_SIZE-1, "each node val (%d)", node->val);
		print_in_thread(msg);
	}
	print_in_thread("linked list debugfs support ... \n");
	debugfs_support("ramp");
	print_in_thread("finish linked list setup... \n");

	// Waiting to end...
	do {

		trace_printk(" Hello from module with trace_printk\n");
		msleep_interruptible(2000);
		print_in_thread("msleep over in Thread Function");
	} while (!kthread_should_stop());
	return 0;
}
// END : thread stuff
 
static int __init my_oops_init(void) { 
	tsk[0]=kthread_create(thr_fun, NULL, "rampxxxx/%d", 0);
	if (!tsk[0]){
		pr_info("Failed to create kernel thread bye!\n");
		return -1;
	}
//	kthread_bind(tsk[0], 0); // Bind to a particular cpu
	pr_info("Starting thread %d", 0);
	wake_up_process(tsk[0]);

       return (0); 
} 
static void __exit my_oops_exit(void) { 
	struct my_struct *node;

	// debugfs
	debugfs_remove(filen);
	debugfs_remove(parent); /* A remove is recursive by default */
	// list
	list_for_each_entry(node,&my_list, list){
		pr_info("FREE each node val (%d)\n", node->val); 
		kfree(node);
	}

	// thread
	pr_info(" Kill Thread %d", 0);
	kthread_stop(tsk[0]);
	pr_info("Goodbye world\n"); 
} 
 
module_init(my_oops_init); 
module_exit(my_oops_exit);
MODULE_LICENSE("GPL");
