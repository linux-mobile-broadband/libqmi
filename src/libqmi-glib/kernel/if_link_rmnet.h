
#ifndef _QMI_IF_LINK_RMNET_H
#define _QMI_IF_LINK_RMNET_H

#include <linux/types.h>
#include <linux/netlink.h>

/* safe guard to avoid redefininig all these symbols if
 * the system provided kernel or libc headers already
 * define them */

#if !defined IFLA_RMNET_MAX

#define RMNET_FLAGS_INGRESS_DEAGGREGATION         (1U << 0)
#define RMNET_FLAGS_INGRESS_MAP_COMMANDS          (1U << 1)
#define RMNET_FLAGS_INGRESS_MAP_CKSUMV4           (1U << 2)
#define RMNET_FLAGS_EGRESS_MAP_CKSUMV4            (1U << 3)

enum {
	IFLA_RMNET_UNSPEC,
	IFLA_RMNET_MUX_ID,
	IFLA_RMNET_FLAGS,
	__IFLA_RMNET_MAX,
};

#define IFLA_RMNET_MAX	(__IFLA_RMNET_MAX - 1)

struct ifla_rmnet_flags {
	__u32	flags;
	__u32	mask;
};

#endif /* IFLA_RMNET_MAX */

#endif /* _QMI_IF_LINK_RMNET_H */
