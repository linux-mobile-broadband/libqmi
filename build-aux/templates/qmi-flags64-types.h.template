/*** BEGIN file-header ***/

#include <glib-object.h>

G_BEGIN_DECLS
/*** END file-header ***/

/*** BEGIN file-production ***/

/* enumerations from "@filename@" */
/*** END file-production ***/

/*** BEGIN value-header ***/

#define __@ENUMNAME@_IS_@TYPE@__

#if defined __@ENUMNAME@_IS_ENUM__
#error Only flags expected, @EnumName@ is an enumeration
#endif

/**
 * @enum_name@_build_string_from_mask:
 * @mask: bitmask of @EnumName@ values.
 *
 * Builds a string containing a comma-separated list of nicknames for
 * each #@EnumName@ in @mask.
 *
 * Returns: (transfer full): a string with the list of nicknames, or %NULL if none given. The returned value should be freed with g_free().
 * Since: @enumsince@
 */
gchar *@enum_name@_build_string_from_mask (@EnumName@ mask);

/*** END value-header ***/

/*** BEGIN file-tail ***/
G_END_DECLS

/*** END file-tail ***/
