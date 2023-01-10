/*** BEGIN file-header ***/
#include <glib-object.h>

/*** END file-header ***/

/*** BEGIN file-production ***/

/* enumerations from "@filename@" */
#include "@filename@"
/*** END file-production ***/

/*** BEGIN value-header ***/
GType
@enum_name@_get_type (void)
{
  static gsize static_g_@type@_type_id;

  if (g_once_init_enter (&static_g_@type@_type_id))
    {
      static const G@Type@Value values[] = {
/*** END value-header ***/

/*** BEGIN value-production ***/
            { @VALUENAME@, "@VALUENAME@", "@valuenick@" },
/*** END value-production ***/

/*** BEGIN value-tail ***/
            { 0, NULL, NULL }
      };

      GType g_@type@_type_id = g_@type@_register_static ("@EnumName@", values);

      g_once_init_leave (&static_g_@type@_type_id, g_@type@_type_id);
    }
  return static_g_@type@_type_id;
}

/*** END value-tail ***/
