/*** BEGIN file-header ***/

#include <glib-object.h>

G_BEGIN_DECLS
/*** END file-header ***/

/*** BEGIN file-production ***/

/* enumerations from "@filename@" */
/*** END file-production ***/

/*** BEGIN value-header ***/
GQuark @enum_name@_quark    (void);
GType  @enum_name@_get_type (void) G_GNUC_CONST;
#define @ENUMNAME@ (@enum_name@_quark ())
#define @ENUMPREFIX@_TYPE_@ENUMSHORT@ (@enum_name@_get_type ())

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
/*** END value-header ***/

/*** BEGIN file-tail ***/
G_END_DECLS

/*** END file-tail ***/
