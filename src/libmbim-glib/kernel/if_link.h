
#ifndef _MBIM_IF_LINK_H
#define _MBIM_IF_LINK_H

#include <linux/if_link.h>

/* Instead of checking if the libc or kernel provided headers
 * include the enum value we require, in this case we define
 * a new one ourselves. We cannot check if a single enum value
 * was defined in an included header via preprocessor directives.
 */

enum {
	/* device (sysfs) name as parent, used instead
	 * of IFLA_LINK where there's no parent netdev.
	 */
	MBIM_IFLA_PARENT_DEV_NAME = 56,
};

#endif /* _MBIM_IF_LINK_H */
