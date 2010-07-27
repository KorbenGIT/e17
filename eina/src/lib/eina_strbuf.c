#define _STRBUF_DATA_TYPE         char
#define _STRBUF_CSIZE             sizeof(_STRBUF_DATA_TYPE)
#define _STRBUF_STRUCT_NAME       Eina_Strbuf
#define _STRBUF_STRLEN_FUNC(x)    strlen(x)
#define _STRBUF_STRESCAPE_FUNC(x) eina_str_escape(x)
#define _STRBUF_MAGIC             EINA_MAGIC_STRBUF
static const char _STRBUF_MAGIC_STR[] = "Eina Strbuf";

#define _FUNC_EXPAND(y) eina_strbuf_##y

#define _GNU_SOURCE
#include <stdio.h>


#include "eina_strbuf.h"
#include "eina_strbuf_common.h"
#include "eina_unicode.h"
#include "eina_private.h"

#include "eina_strbuf_template_c.i"



EAPI Eina_Bool
eina_strbuf_append_printf(Eina_Strbuf *buf, const char *fmt, ...)
{
   va_list args;
   char *str;
   size_t len;
   Eina_Bool ret;

   va_start(args, fmt);
   len = vasprintf(&str, fmt, args);
   va_end(args);

   if (len <= 0 || !str)
     return EINA_FALSE;

   ret = eina_strbuf_append_length(buf, str, len);
   free(str);
   return ret;
}

EAPI Eina_Bool
eina_strbuf_append_vprintf(Eina_Strbuf *buf, const char *fmt, va_list args)
{
   char *str;
   size_t len;
   Eina_Bool ret;

   len = vasprintf(&str, fmt, args);

   if (len <= 0 || !str)
     return EINA_FALSE;

   ret = eina_strbuf_append_length(buf, str, len);
   free(str);
   return ret;
}

EAPI Eina_Bool
eina_strbuf_insert_printf(Eina_Strbuf *buf, const char *fmt, size_t pos, ...)
{
   va_list args;
   char *str;
   size_t len;
   Eina_Bool ret;

   va_start(args, pos);
   len = vasprintf(&str, fmt, args);
   va_end(args);

   if (len <= 0 || !str)
     return EINA_FALSE;

   ret = eina_strbuf_insert(buf, str, pos);
   free(str);
   return ret;
}

EAPI Eina_Bool
eina_strbuf_insert_vprintf(Eina_Strbuf *buf, const char *fmt, size_t pos, va_list args)
{
   char *str;
   size_t len;
   Eina_Bool ret;

   len = vasprintf(&str, fmt, args);

   if (len <= 0 || !str)
     return EINA_FALSE;

   ret = eina_strbuf_insert(buf, str, pos);
   free(str);
   return ret;
}