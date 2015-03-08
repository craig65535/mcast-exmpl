/* craig65535@gmail.com */

#include <linux/module.h>
#include <linux/kprobes.h>
#include <net/sock.h>       /* must include this before inet_common.h */
#include <net/inet_common.h>
#include <net/genetlink.h>

#include "mcast-exmpl.h"

MODULE_LICENSE("GPL");

static int _dummy(struct sk_buff *skb, struct genl_info *info);

static atomic_t _g_genlmsg_seq = ATOMIC_INIT(0);

static struct genl_family _g_genl_family = {
    .id          = GENL_ID_GENERATE,
    .hdrsize     = 0,
    .name        = MCAST_EXMPL_GENL_FAM_NAME,
    .version     = MCAST_EXMPL_GENL_VERSION,
    .maxattr     = MCAST_EXMPL_ATTR_COUNT-1,
};

static struct genl_multicast_group _g_genl_mc_groups[] = {
    {
        .name    = MCAST_EXMPL_GENL_MC_GRP_NAME,
    },
};

static struct nla_policy _g_no_attrs_pol[MCAST_EXMPL_ATTR_COUNT] __read_mostly = {
};

static struct genl_ops _g_genl_ops[] = {
    {
        .cmd     = MCAST_EXMPL_CMD_DUMMY,
        .flags   = GENL_ADMIN_PERM,
        .policy  = _g_no_attrs_pol,
        .doit    = _dummy
    },
};

static struct jprobe _g_connect_probe;

static int _dummy(struct sk_buff *skb, struct genl_info *info)
{
    return 0;
}

static int _send_rec(uint32_t num)
{
    int rc;
    int ret = 0;
    struct sk_buff *skb;
    void *genl_msg;

    skb = nlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (!skb) {
        printk(KERN_ERR "%s: nlmsg_new failed\n", __func__);
        ret = -1;
        goto done;
    }
    atomic_inc(&_g_genlmsg_seq);
    genl_msg = genlmsg_put(skb,
                           0 /* portid */,
                           atomic_read(&_g_genlmsg_seq),
                           &_g_genl_family,
                           0 /* flags */,
                           MCAST_EXMPL_CMD_SEND_NUM
                          );
    if (genl_msg == NULL) {
        printk(KERN_ERR "%s: genlmsg_put failed\n", __func__);
        ret = -1;
        goto done;
    }

    rc = nla_put_u32(skb, MCAST_EXMPL_ATTR_NUM, num);
    if (rc != 0) {
        printk(KERN_ERR "%s: nla_put_u32(num) failed\n", __func__);
        ret = rc;
        goto done;
    }
    (void)genlmsg_end(skb, genl_msg);

    printk(KERN_INFO "sending a MCAST_EXMPL_CMD_SEND_NUM record\n");
    rc = genlmsg_multicast(&_g_genl_family,
                           skb,
                           0 /* portid */,
                           0 /* the index of the mcast grp */,
                           GFP_KERNEL
                          );
    /* don't free skb after handing it off to genlmsg_multicast,
       even if the function returns an error */
    skb = NULL;
    /* genlmsg_multicast returns -ESRCH if there are no listeners */
    if (rc != 0 &&
            rc != -ESRCH) {
        printk(KERN_ERR "%s: genlmsg_multicast failed\n", __func__);
        ret = rc;
        goto done;
    }

done:
    if (skb) {
        nlmsg_free(skb);
        skb = NULL;
    }
    return ret;
}

static void _connect_probe_cb(struct socket *sock,
                              struct sockaddr * uaddr,
                              int addr_len, int flags)
{
    (void)_send_rec(55555);
    jprobe_return();
}

int init_module(void)
{
    int rc;
    int ret = 0;

    printk(KERN_INFO "starting mcast-exmpl\n");

    /* register generic netlink family */
    rc = genl_register_family_with_ops_groups(&_g_genl_family,
                                              _g_genl_ops,
                                              _g_genl_mc_groups);
    if (rc != 0) {
        printk(KERN_ERR "%s: genl_register_family_with_ops_groups failed\n", __func__);
        ret = rc;
        goto done;
    }
    printk(KERN_INFO "_g_genl_family.id %u\n", _g_genl_family.id);

    _g_connect_probe.kp.addr = (kprobe_opcode_t *)inet_stream_ops.connect;
    _g_connect_probe.entry = (kprobe_opcode_t *)_connect_probe_cb;
    rc = register_jprobe(&_g_connect_probe);
    if (rc != 0) {
        ret = rc;
        printk(KERN_ERR "%s: register_jprobe failed\n", __func__);
    }

done:
    return ret;
}

void cleanup_module(void)
{
    int rc;

    unregister_jprobe(&_g_connect_probe);

    /* unregister generic netlink family */
    rc = genl_unregister_family(&_g_genl_family);
    if (rc != 0) {
        printk(KERN_ERR "%s: genl_unregister_family failed\n", __func__);
    }

    printk(KERN_INFO "stopping mcast-exmpl\n");
}

