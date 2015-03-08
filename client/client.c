/* craig65535@gmail.com
   derived from the libmnl example genl-family-get.c */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <libmnl/libmnl.h>
#include <linux/genetlink.h>
#include "../mcast-exmpl.h"

struct cb_data {
    uint16_t family_id;
    uint32_t mcast_grp;
};

static int _parse_mc_grps_cb(const struct nlattr *attr, void *data)
{
    const struct nlattr **tb = data;
    uint16_t type;
    int ret = MNL_CB_OK;

    if (mnl_attr_type_valid(attr, CTRL_ATTR_MCAST_GRP_MAX) < 0) {
        perror("mnl_attr_type_valid");
        ret = MNL_CB_ERROR;
        goto done;
    }

    type = mnl_attr_get_type(attr);
    switch(type) {
        case CTRL_ATTR_MCAST_GRP_ID:
            if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0) {
                perror("mnl_attr_validate");
                ret = MNL_CB_ERROR;
                goto done;
            }
            break;
        case CTRL_ATTR_MCAST_GRP_NAME:
            if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0) {
                perror("mnl_attr_validate");
                ret = MNL_CB_ERROR;
                goto done;
            }
            break;
        default:
            break;
    }
    tb[type] = attr;

done:
    return ret;
}

static void _parse_genl_mc_grps(struct nlattr *nested, struct cb_data *cb_data)
{
    struct nlattr *pos;

    mnl_attr_for_each_nested(pos, nested) {
        struct nlattr *tb[CTRL_ATTR_MCAST_GRP_MAX+1] = {};

        mnl_attr_parse_nested(pos, _parse_mc_grps_cb, tb);
        if (tb[CTRL_ATTR_MCAST_GRP_ID]) {
            if (cb_data->mcast_grp == 0) {
                /* use the first group ID returned */
                cb_data->mcast_grp = mnl_attr_get_u32(tb[CTRL_ATTR_MCAST_GRP_ID]);
            }
        }
    }
}

static int _genl_ctrl_attr_cb(const struct nlattr *attr, void *data)
{
    const struct nlattr **tb = data;
    uint16_t type;
    int ret = MNL_CB_OK;

    if (mnl_attr_type_valid(attr, CTRL_ATTR_MAX) < 0) {
        perror("mnl_attr_type_valid");
        ret = MNL_CB_ERROR;
        goto done;
    }

    type = mnl_attr_get_type(attr);
    switch(type) {
        case CTRL_ATTR_FAMILY_NAME:
            if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0) {
                perror("mnl_attr_validate");
                ret = MNL_CB_ERROR;
                goto done;
            }
            break;
        case CTRL_ATTR_FAMILY_ID:
            if (mnl_attr_validate(attr, MNL_TYPE_U16) < 0) {
                perror("mnl_attr_validate");
                ret = MNL_CB_ERROR;
                goto done;
            }
            break;
        case CTRL_ATTR_MCAST_GROUPS:
            if (mnl_attr_validate(attr, MNL_TYPE_NESTED) < 0) {
                perror("mnl_attr_validate");
                ret = MNL_CB_ERROR;
                goto done;
            }
            break;
        default:
            break;
    }
    tb[type] = attr;

done:
    return ret;
}

static int _rec_attr_cb(const struct nlattr *attr, void *data)
{
    const struct nlattr **tb = data;
    uint16_t type;
    int ret = MNL_CB_OK;

    if (mnl_attr_type_valid(attr, MCAST_EXMPL_ATTR_COUNT) < 0) {
        perror("mnl_attr_type_valid");
        ret = MNL_CB_ERROR;
        goto done;
    }

    type = mnl_attr_get_type(attr);
    switch(type) {
        case MCAST_EXMPL_ATTR_NUM:
            if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0) {
                perror("mnl_attr_validate");
                ret = MNL_CB_ERROR;
                goto done;
            }
            break;
        default:
            break;
    }
    tb[type] = attr;

done:
    return ret;
}

static int _data_cb(const struct nlmsghdr *nlh, void *data)
{
    struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);
    struct cb_data *cb_data = data;
    int err;
    int ret = MNL_CB_OK;

    if (nlh->nlmsg_type == cb_data->family_id) {
        struct nlattr *tb[MCAST_EXMPL_ATTR_COUNT] = {};
        printf("mcast-exmpl msg\n");
        err = mnl_attr_parse(nlh, sizeof(*genl), _rec_attr_cb, tb);
        if (err != MNL_CB_OK) {
            ret = err;
            goto done;
        }
        switch(genl->cmd) {
            case MCAST_EXMPL_CMD_SEND_NUM:
                printf("SEND_NUM");
                break;
            default:
                printf("???");
                break;
        }
        if (tb[MCAST_EXMPL_ATTR_NUM]) {
            printf(" %d", mnl_attr_get_u32(tb[MCAST_EXMPL_ATTR_NUM]));
        }
        printf("\n");

    } else if (nlh->nlmsg_type == GENL_ID_CTRL) {
        struct nlattr *tb[CTRL_ATTR_MAX+1] = {};
        printf("genl ctrl msg\n");
        err = mnl_attr_parse(nlh, sizeof(*genl), _genl_ctrl_attr_cb, tb);
        if (err != MNL_CB_OK) {
            ret = err;
            goto done;
        }
        if (tb[CTRL_ATTR_FAMILY_ID]) {
            cb_data->family_id = mnl_attr_get_u16(tb[CTRL_ATTR_FAMILY_ID]);
        }
        if (tb[CTRL_ATTR_MCAST_GROUPS]) {
            _parse_genl_mc_grps(tb[CTRL_ATTR_MCAST_GROUPS], cb_data);
        }
    }

done:
    return ret;
}

int main(void)
{
    struct mnl_socket *nl;
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genl;
    int ret = EXIT_SUCCESS;
    unsigned int seq, portid;
    struct cb_data cb_data = { 0, 0 };
    int n;

    /* connect to generic netlink */
    nl = mnl_socket_open(NETLINK_GENERIC);
    if (nl == NULL) {
        perror("mnl_socket_open");
        ret = EXIT_FAILURE;
        goto done;
    }
    if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
        perror("mnl_socket_bind");
        ret = EXIT_FAILURE;
        goto done;
    }
    portid = mnl_socket_get_portid(nl);

    /* get the family and mcast group IDs for MCAST_EXMPL_GENL_FAM_NAME */
    nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type = GENL_ID_CTRL;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq = seq = time(NULL);

    genl = mnl_nlmsg_put_extra_header(nlh, sizeof(struct genlmsghdr));
    genl->cmd = CTRL_CMD_GETFAMILY;
    genl->version = 1;

    mnl_attr_put_u32(nlh, CTRL_ATTR_FAMILY_ID, GENL_ID_CTRL);
    mnl_attr_put_strz(nlh, CTRL_ATTR_FAMILY_NAME, MCAST_EXMPL_GENL_FAM_NAME);

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
        perror("mnl_socket_sendto");
        ret = EXIT_FAILURE;
        goto done;
    }

    n = mnl_socket_recvfrom(nl, buf, sizeof(buf));
    while (n > 0) {
        n = mnl_cb_run(buf, n, seq, portid, _data_cb, &cb_data);
        if (n <= 0) {
            if (n < 0) {
                perror("mnl_cb_run");
            }
            break;
        }
        n = mnl_socket_recvfrom(nl, buf, sizeof(buf));
        if (n < 0) {
            perror("mnl_socket_recvfrom");
            ret = EXIT_FAILURE;
            goto done;
        }
    }
    if (n < 0) {
        ret = EXIT_FAILURE;
        goto done;
    }

    printf("Family ID: %d\n", cb_data.family_id);
    printf("Mcast group ID: %d\n", cb_data.mcast_grp);

    /* join multicast group */
    if (mnl_socket_setsockopt(nl, NETLINK_ADD_MEMBERSHIP, &cb_data.mcast_grp, sizeof(cb_data.mcast_grp)) != 0) {
        perror("mnl_socket_setsockopt");
        ret = EXIT_FAILURE;
        goto done;
    }

    /* start receiving records */
    /* blocking recvs */
    do {
        n = mnl_socket_recvfrom(nl, buf, sizeof(buf));
        while (n > 0) {
            n = mnl_cb_run(buf, n, 0, 0, _data_cb, &cb_data);
            if (n <= 0) {
                if (n < 0) {
                    perror("mnl_cb_run");
                }
                break;
            }
            n = mnl_socket_recvfrom(nl, buf, sizeof(buf));
            if (n < 0) {
                perror("mnl_socket_recvfrom");
                ret = EXIT_FAILURE;
                goto done;
            }
        }
    } while (1);

done:
    if (nl) {
        mnl_socket_close(nl);
        nl = NULL;
    }

    return ret;
}

