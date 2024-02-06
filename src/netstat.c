#include <linux/module.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/netdevice.h>
#include <linux/percpu-defs.h>
#include <linux/list.h>
#include <net/sch_generic.h>
#include <uapi/linux/gen_stats.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("George Volkov");

#define FILENAME "netstat"

void print_backlog_napi_data(struct seq_file *seq, struct napi_struct *n){
	if(n == NULL) return;
	
	seq_printf(seq, "\tbacklog_napi: %ld %d %d %d %d\n", n->state, n->weight, n->list_owner, n->rx_count, n->napi_id);
	seq_printf(seq, "\tgro_buckets: ");
	for(int i = 0; i < GRO_HASH_BUCKETS; i++) seq_printf(seq, "%d ", n->gro_hash->count);
	seq_printf(seq, "\n");
}

void print_backlog_data(struct seq_file *seq, struct softnet_data *sd) {
	seq_printf(seq, "\tbacklog: %d %d %d %d\n", sd->received_rps, skb_queue_len(&sd->input_pkt_queue), skb_queue_len(&sd->process_queue), sd->input_queue_tail);
}

void print_softnet_data(struct seq_file *seq, struct softnet_data *sd){
	seq_printf(seq, "softnet_data cpu #%d: %d %d %d %d\n", sd->cpu, sd->processed, sd->time_squeeze, sd->dropped, sd->in_net_rx_action);
}

void print_netdata(struct seq_file *seq, struct softnet_data *sd) {
	for(int i = 0; i < INT_MAX; i++) {
		if(!list_empty(&sd->poll_list)){
			print_softnet_data(seq, sd);
			print_backlog_data(seq, sd);
			print_backlog_napi_data(seq, &sd->backlog);
			break;
		}
	}
}

int netstat_show(struct seq_file *seq, void *v)
{
	struct softnet_data *sd = NULL;

	for (int i = 0; i < nr_cpu_ids; i++)
	{
		if (cpu_online(i)) {
			sd = &per_cpu(softnet_data, i);
			print_netdata(seq, sd);
		}
	}

	return 0;
}

ssize_t netstat_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{	
	return seq_read(file, buf, size, ppos);
}

int netstat_open(struct inode *inode, struct file *file)
{
	return single_open(file, netstat_show, NULL);
}

int netstat_release(struct inode *inode, struct file *file)
{
	return single_release(inode, file);
}


static struct proc_ops fops = {
	.proc_read = netstat_read,
	.proc_open = netstat_open,
	.proc_release = netstat_release
};

static int __init mod_init(void)
{
	if (!proc_create(FILENAME, 0666, NULL, &fops))
	{
		printk(KERN_ERR "%s: file create failed!\n", FILENAME);
		return - 1;
	}

	printk(KERN_INFO "%s: module loaded\n", FILENAME);
	return 0;
}

static void __exit mod_exit(void)
{
	remove_proc_entry(FILENAME, NULL);
	printk(KERN_INFO "%s: module unloaded\n", FILENAME);
}

module_init(mod_init);
module_exit(mod_exit);