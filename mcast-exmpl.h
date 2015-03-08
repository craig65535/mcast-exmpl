/* craig65535@gmail.com */

/* max GENL_NAMSIZ (16) bytes: */
#define MCAST_EXMPL_GENL_FAM_NAME "mcast-exmpl"
/* max GENL_NAMSIZ (16) bytes: */
#define MCAST_EXMPL_GENL_MC_GRP_NAME "mcast-exmpl-grp"
#define MCAST_EXMPL_GENL_VERSION 1

enum {
    MCAST_EXMPL_CMD_UNSPEC,
    MCAST_EXMPL_CMD_SEND_NUM,
    MCAST_EXMPL_CMD_DUMMY,
};

enum {
    MCAST_EXMPL_ATTR_UNSPEC,
    MCAST_EXMPL_ATTR_NUM,
    /* last entry: */
    MCAST_EXMPL_ATTR_COUNT
};

