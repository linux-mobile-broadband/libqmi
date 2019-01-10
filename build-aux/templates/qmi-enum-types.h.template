/*** BEGIN file-header ***/

#include <glib-object.h>

G_BEGIN_DECLS
/*** END file-header ***/

/*** BEGIN file-production ***/

/* enumerations from "@filename@" */
/*** END file-production ***/

/*** BEGIN value-header ***/
GType @enum_name@_get_type (void) G_GNUC_CONST;
#define @ENUMPREFIX@_TYPE_@ENUMSHORT@ (@enum_name@_get_type ())

/* Define type-specific symbols */
#define __@ENUMNAME@_IS_@TYPE@__

#if defined __@ENUMNAME@_IS_ENUM__
/**
 * @enum_name@_get_string:
 * @val: a @EnumName@.
 *
 * Gets the nickname string for the #@EnumName@ specified at @val.
 *
 * Returns: (transfer none): a string with the nickname, or %NULL if not found. Do not free the returned value.
 * Since: @enumsince@
 */
const gchar *@enum_name@_get_string (@EnumName@ val);
#endif

#if defined __@ENUMNAME@_IS_FLAGS__
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
#endif

/*** END value-header ***/

/*** BEGIN file-tail ***/
G_END_DECLS

/*** END file-tail ***/
