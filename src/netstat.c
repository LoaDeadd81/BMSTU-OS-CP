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

void print_qdisc_data(struct seq_file *seq, struct Qdisc *q){
	if(q == NULL) return;

	struct gnet_stats_queue *stats;

	seq_printf(seq, "\tQdisc: %d %d\n", q->flags, q->limit);

	for (int i = 0; i < nr_cpu_ids; i++)
	{
		if (cpu_online(i)) {
			stats = per_cpu_ptr(q->cpu_qstats, i);
			if(stats == NULL) continue;
			seq_printf(seq, "\tQdisc cpu#%d: %d %d %d %d\n", i, stats->qlen, stats->backlog, stats->drops, stats->overlimits);
		}
	}
}

int get_cq_len(struct sk_buff *cq) {
	if(cq == NULL) return 0;
	
	int cnt = 0;

	while (cq->next != NULL) {
		cnt++;
		cq = cq->next;
	}

	return cnt;
}

void print_softnet_data(struct seq_file *seq, struct softnet_data *sd){
	seq_printf(seq, "softnet_data cpu #%d: %d %d %d %d %d %d\n", 
	sd->cpu, sd->processed, sd->time_squeeze, sd->received_rps, sd->dropped, get_cq_len(sd->completion_queue), sd->in_net_rx_action);
	print_qdisc_data(seq, sd->output_queue);
}

void print_dev_data(struct seq_file *seq, struct net_device *dev){
	if (dev == NULL) return;
	seq_printf(seq, "\tnetdev: %s %ld %d\n", dev->name, dev->state, dev->mtu);

	// if(dev == NULL || dev->netdev_ops == NULL || dev->netdev_ops->ndo_get_stats == NULL) return;
	// struct net_device_stats* stats = dev->netdev_ops->ndo_get_stats(dev);
	// seq_printf(seq, "\tnetdev rx: %ld %ld %ld %ld\n", stats->rx_packets, stats->rx_bytes, stats->rx_errors, stats->rx_dropped);
	// seq_printf(seq, "\tnetdev tx: %ld %ld %ld %ld\n", stats->tx_packets, stats->tx_bytes, stats->tx_errors, stats->tx_dropped);
}

void print_napi_data(struct seq_file *seq, struct napi_struct *n){
	if(n == NULL) return;
	
	seq_printf(seq, "\tnapi: %ld %d %d %d\n", n->state, n->weight, n->list_owner, n->rx_count);
	print_dev_data(seq, n->dev);
}

void print_poll_list_data(struct seq_file *seq, struct softnet_data *sd){
	long i = 0;
	for(i = 0; i < INT_MAX; i++) {
		if(list_empty(&sd->poll_list)){
			
		}
		else{
			LIST_HEAD(list);
			list_splice_init(&sd->poll_list, &list);
			if(list_empty(&list)) break;

			struct list_head *head = &list;
			struct list_head *cur = &list;
			struct napi_struct *n;

			list_for_each(cur, head){
				n = list_entry(cur, struct napi_struct, poll_list);
				print_napi_data(seq, n);
			}
			return;
		} 
	}

	seq_printf(seq, "\tpoll_list is empty %ld\n", i);
}

void print_backlog_data(struct seq_file *seq, struct softnet_data *sd) {
	seq_printf(seq, "\tbacklog_q_len: %d\n", skb_queue_len(&sd->input_pkt_queue));
	print_napi_data(seq, &sd->backlog);
}

void print_netdata(struct seq_file *seq, struct softnet_data *sd) {
	print_softnet_data(seq, sd);
	print_poll_list_data(seq, sd);
	print_backlog_data(seq, sd);
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