#include <linux/cdev.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/in.h>
#include <net/arp.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/icmp.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/proc_fs.h>

// #define DEBUG    

#define LOG(msg) "%s: " msg "\n", THIS_MODULE->name
#ifdef DEBUG
#define DLOG(...) printk(__VA_ARGS__)
#else
#define DLOG(...)
#endif




#define DEV_FIRST_MAJOR 0
#define DEV_COUNT 1
static char* link = "enp0s3";
static char* ifname = "vni%d";
static char* proc_file_name = "vni_stats";

module_param(link, charp, 0);




static struct net_device_stats stats;

static struct net_device *child = NULL;
struct priv {
    struct net_device *parent;
};

static void printk_ip_addr(__be32 addr_num) {
    __u8* bytes = (void*)&addr_num;
    addr_num = ntohl(addr_num);
    printk(KERN_CONT "%d.%d.%d.%d", (int)bytes[3], (int)bytes[2], (int)bytes[1], (int)bytes[0]);
}

// IPv6 was planned to be suppurted. But I somehow failed to do so.
// And beside the type of echo request of ICMPv6 is 128
// which is not in the variant :))
static void printk_ipv6_addr(struct in6_addr addr) {
    int i;
    for (i = 0; i < 8; ++i) {
        if (i) printk(KERN_CONT ":");
        printk(KERN_CONT "%x", (int)addr.s6_addr16[i]);
    }
}
static void dump_data(unsigned char* data, size_t length) {
    const size_t width = 12;
    const size_t sep = width / 2;
    size_t row, i;
    unsigned char ch;
    
    for (row = 0; row < length; row += width) {
        printk("%04zu | ", row);
        for (i = row; i < row + width; ++i) {
            if (i % width == sep) {
                printk(KERN_CONT "  ");
            }
            if (i < length) {
                printk(KERN_CONT "%02hhX ", data[i]);
            } else {
                printk(KERN_CONT "   ");  // 3 spaces
            }
        }
        printk(KERN_CONT "| ");
        for (i = row; i < row + width; ++i) {
            if (i % width == sep) {
                printk(KERN_CONT " ");
            }
            if (i < length) {
                ch = data[i];
                if (ch > 127 || !isprint(ch)) {
                    ch = '.';
                }
                printk(KERN_CONT "%c", ch);
            } else {
                printk(KERN_CONT " ");
            }
        }
    }
    printk("\n");  // just to flush the ring buffer
}

enum icmp_frame_type8_checking_result {
    OK,
    WRONG_CODE,
    NOT_ICMP_TYPE8_FRAME,
};
static enum icmp_frame_type8_checking_result check_icmp_frame_type8(struct sk_buff *skb) {
    struct ethhdr *eth = eth_hdr(skb);
    struct iphdr *ip = NULL;
    struct ipv6hdr *ipv6 = NULL;
    struct icmphdr *icmp = NULL;
    
    int icmp_echo_request_type;
    int data_len;
    unsigned char *user_data_ptr = NULL;
    
    __be16 eth_proto = ntohs(eth->h_proto);
    if (eth_proto == ETH_P_IP) { 
        ip = ip_hdr(skb); 
        DLOG("GOT IP FRAME %d", (int)ip->protocol); 
        if (ip->protocol != IPPROTO_ICMP) { 
            return NOT_ICMP_TYPE8_FRAME; 
        } 
        icmp_echo_request_type = 8;
    }
    //// well the driver actually got IPv6 frames, but I failed to test it
    //// so let's just leave it.
    // else if (eth_proto == ETH_P_IPV6) {  
        // ipv6 = ipv6_hdr(skb);  
        // DLOG("GOT IPv6 FRAME %d", (int)ipv6->nexthdr);  
        // if (ipv6->nexthdr != IPPROTO_ICMPV6) {  
            // return NOT_ICMP_TYPE8_FRAME;  
        // }  
        // icmp_echo_request_type = 128; 
    // } 
    else { 
        // DLOG("NOT AN IP FRAME");  
        return NOT_ICMP_TYPE8_FRAME; 
    } 
    // printk(KERN_INFO"%p %p", eth, ip); 
    
    icmp = icmp_hdr(skb);
    // printk(KERN_INFO "Icmp type %d", (int)icmp->type, ); 
    if (icmp->type != icmp_echo_request_type) {
        DLOG("NOT AN ICMP FRAME");
        return NOT_ICMP_TYPE8_FRAME;
    }
    if (icmp->code != 0) {
        DLOG("WRONG ICMP CODE FOR TYPE 8");
        return WRONG_CODE;
    }
    printk(KERN_INFO "Captured ICMP datagram\n");
    if (ip) {
        printk("IPv4\n");
        printk(KERN_INFO "saddr: ");
        printk_ip_addr(ip->saddr);
        
        printk(KERN_INFO "daddr: ");
        printk_ip_addr(ip->daddr);
    } else {
        // assert(ipv6);
        printk("IPv6");
        printk(KERN_INFO "saddr: ");
        printk_ipv6_addr(ipv6->saddr);
        
        printk(KERN_INFO "daddr: ");
        printk_ipv6_addr(ipv6->daddr);
    }
    user_data_ptr = (unsigned char*)icmp + sizeof(struct icmphdr);
    data_len = skb_tail_pointer(skb) - user_data_ptr;
    printk(KERN_INFO "Data length: %d\n", data_len);
    dump_data(user_data_ptr, data_len);
    return OK;
}

static void update_stats(struct sk_buff *skb) {
    enum icmp_frame_type8_checking_result res = check_icmp_frame_type8(skb);
    if (res == NOT_ICMP_TYPE8_FRAME) {
        return ;
    }
    stats.rx_packets++;
    stats.rx_bytes += skb->len;
    if (res != OK) {
        stats.rx_errors++;
    }
}

static rx_handler_result_t handle_frame(struct sk_buff **pskb) {
   // if (child) {
        update_stats(*pskb);
        (*pskb)->dev = child;
        return RX_HANDLER_ANOTHER;
    //}   
    return RX_HANDLER_PASS; 
} 

static int net_dev_open(struct net_device *dev) {
    netif_start_queue(dev);
    printk(KERN_INFO "%s: device opened", dev->name);
    return 0; 
} 

static int net_dev_stop(struct net_device *dev) {
    netif_stop_queue(dev);
    printk(KERN_INFO "%s: device closed", dev->name);
    return 0; 
} 

static netdev_tx_t net_dev_start_xmit(struct sk_buff *skb, struct net_device *dev) {
    struct priv *priv = netdev_priv(dev);
    update_stats(skb);
    if (priv->parent) {
        skb->dev = priv->parent;
        skb->priority = 1;
        dev_queue_xmit(skb);
        return 0;
    }
    return NETDEV_TX_OK;
}

static struct net_device_stats* net_dev_get_stats(struct net_device *dev) {
    return &stats;
} 

static struct net_device_ops crypto_net_device_ops = {
    .ndo_open = net_dev_open,
    .ndo_stop = net_dev_stop,
    .ndo_get_stats = net_dev_get_stats,
    .ndo_start_xmit = net_dev_start_xmit
};

static void net_dev_setup(struct net_device *dev) {
    int i;
    ether_setup(dev);
    memset(netdev_priv(dev), 0, sizeof(struct priv));
    dev->netdev_ops = &crypto_net_device_ops;

    //fill in the MAC address with a phoney
    for (i = 0; i < ETH_ALEN; i++)
        dev->dev_addr[i] = (char)i;
} 





static struct proc_dir_entry* proc_entry;

static ssize_t char_dev_read_with_str(
    const char* str, ssize_t str_len,
    struct file *file, char __user *ubuf, size_t count, loff_t* ppos
) {
    if (*ppos >= str_len) {
        return 0;
    }
    if (str_len < 0) {
        printk(KERN_ERR LOG("no mem"));
        return -ENOMEM;
    }
    if (count + *ppos > str_len) {
        count = str_len - *ppos;
    }
    if (copy_to_user(ubuf, str + *ppos, count) != 0) {
        printk(KERN_ERR LOG("not all copied"));
        return -EFAULT;
    }
    *ppos += count;
    return count;
}




static ssize_t proc_file_read(struct file *file, char __user *ubuf, size_t count, loff_t* ppos) {
    char buff[300];
    ssize_t len;
    len = snprintf(buff, 300, (
            "rx packets: %lu\n"
            "tx packets: %lu\n"
            "rx bytes: %lu\n"
            "rx errors: %lu\n"
    ), stats.rx_packets, stats.tx_packets, stats.rx_bytes, stats.rx_errors);
    return char_dev_read_with_str(buff, len, file, ubuf, count, ppos);
}

static struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .read = proc_file_read,
};




int __init vni_init(void) {
    int err = 0;
    struct priv *priv;
    
	proc_entry = proc_create(proc_file_name, S_IRUSR|S_IRGRP|S_IROTH, NULL, &proc_fops);
    if (!proc_entry) { 
        printk(KERN_ERR "Error while initializing proc file.\n"); 
        return -1; 
    } 
    
    child = alloc_netdev(sizeof(struct priv), ifname, NET_NAME_UNKNOWN, net_dev_setup);
    
    
    if (child == NULL) {
        printk(KERN_ERR "%s: allocate error", THIS_MODULE->name);
        return -ENOMEM;
    }
    priv = netdev_priv(child);
    priv->parent = __dev_get_by_name(&init_net, link); //parent interface
    if (!priv->parent) {
        printk(KERN_ERR "%s: no such net: %s", THIS_MODULE->name, link);
        free_netdev(child);
        return -ENODEV;
    }
    if (priv->parent->type != ARPHRD_ETHER && priv->parent->type != ARPHRD_LOOPBACK) {
        printk(KERN_ERR "%s: illegal net type", THIS_MODULE->name); 
        free_netdev(child);
        return -EINVAL;
    }

    //copy IP, MAC and other information
    memcpy(child->dev_addr, priv->parent->dev_addr, ETH_ALEN);
    memcpy(child->broadcast, priv->parent->broadcast, ETH_ALEN);
    if ((err = dev_alloc_name(child, child->name))) {
        printk(KERN_ERR "%s: allocate name, error %i", THIS_MODULE->name, err);
        free_netdev(child);
        return -EIO;
    }

    register_netdev(child);
    rtnl_lock();
    netdev_rx_handler_register(priv->parent, &handle_frame, NULL);
    rtnl_unlock();
    printk(KERN_INFO "Module %s loaded", THIS_MODULE->name);
    printk(KERN_INFO "%s: create link %s", THIS_MODULE->name, child->name);
    printk(KERN_INFO "%s: registered rx handler for %s", THIS_MODULE->name, priv->parent->name);
    return 0; 
}

void __exit vni_exit(void) {
    struct priv *priv = netdev_priv(child);
    if (priv->parent) {
        rtnl_lock();
        netdev_rx_handler_unregister(priv->parent);
        rtnl_unlock();
        printk(KERN_INFO "%s: unregister rx handler for %s", THIS_MODULE->name, priv->parent->name);
    }
    unregister_netdev(child);
    free_netdev(child);
    proc_remove(proc_entry);
    printk(KERN_INFO "Module %s unloaded", THIS_MODULE->name); 
} 


module_init(vni_init);
module_exit(vni_exit);

MODULE_AUTHOR("Author");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Description");
