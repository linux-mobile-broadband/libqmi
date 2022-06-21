
#ifndef _MBIM_WWAN_H
#define _MBIM_WWAN_H

/* safe guard to avoid redefining WWAN symbols if
 * the system provided kernel or libc headers already
 * define them: HAVE_WWAN_KERNEL_HEADER is set by
 * meson build system according to linux/wwan.h presence */
#if !defined HAVE_WWAN_KERNEL_HEADER

enum {
	IFLA_WWAN_UNSPEC,
	IFLA_WWAN_LINK_ID, /* u32 */

	__IFLA_WWAN_MAX
};
#define IFLA_WWAN_MAX (__IFLA_WWAN_MAX - 1)

/* Both IFLA_WWAN_MAX and enum entry IFLA_PARENT_DEV_NAME got included
 * in the same kernel version 5.14-rc1 */
enum {
	/* device (sysfs) name as parent, used instead
	 * of IFLA_LINK where there's no parent netdev
	 */
	IFLA_PARENT_DEV_NAME = 56,
};

#else

#include <linux/wwan.h>

#endif /* HAVE_WWAN_KERNEL_HEADER */

#endif /* _MBIM_WWAN_H */
