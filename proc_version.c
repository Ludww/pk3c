#include "proc_commonconsts.h"
#include "proc_interface.h"


#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/fs.h>		// for basic filesystem
#include <linux/proc_fs.h>	// for the proc filesystem
#include <linux/seq_file.h>	// for sequence files
#include <linux/jiffies.h>	// for jiffies


static char str_proc_version_name[32];
static char str_proc_version_num[32];
static int active_con_cnt = 0;

static struct proc_dir_entry* ver_file;

static int ver_show(struct seq_file *m, void *v)
{
        seq_printf(m, "%s %s\n%i\n",str_proc_version_name, str_proc_version_num, active_con_cnt);
     
        return 0;
}

static int ver_open(struct inode *inode, struct file *file)
{
        return single_open(file, ver_show, NULL);
}

static const struct file_operations ver_fops =
{
        .owner	        = THIS_MODULE,
        .open	        = ver_open,
        .read	        = seq_read,
        .llseek	        = seq_lseek,
        .release	= single_release,
};

int ver_create(char* vername, char* verstr)
{
	strcpy(str_proc_version_name, vername);
	strcpy(str_proc_version_num, verstr);
        ver_file = proc_create(PROCFS_NAME, 0, NULL, &ver_fops);
        if (!ver_file) {
		printk(KERN_ERR "init fails (cannot create /proc/%s entry)\n", PROCFS_NAME);
		return -ENOMEM;
        }
	active_con_cnt = 0;
  
	printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
	return 0;
}

void ver_destroy(void)
{
	remove_proc_entry(PROCFS_NAME, NULL);
}

void set_con_cnt(int i)
{
        active_con_cnt = i;
}
