/* utils.c

Copyright (C) 1999,2000 Tom Gilbert.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "utils.h"
#include "geist.h"

/* eprintf: print error message and exit */
void
eprintf(char *fmt, ...)
{
   va_list args;

   fflush(stdout);
   fprintf(stderr, "%s ERROR: ", PACKAGE);

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
      fprintf(stderr, "%s", strerror(errno));
   fprintf(stderr, "\n");
   exit(2);
}

/* weprintf: print warning message and continue */
void
weprintf(char *fmt, ...)
{
   va_list args;

   fflush(stdout);
   fprintf(stderr, "%s WARNING: ", PACKAGE);

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
      fprintf(stderr, "%s", strerror(errno));
   fprintf(stderr, "\n");
}

/* estrdup: duplicate a string, report if error */
char *
estrdup(char *s)
{
   char *t;
   if(!s)
      return NULL;
   t = (char *) g_malloc(strlen(s) + 1);
   if (t == NULL)
      eprintf("estrdup(\"%.20s\") failed:", s);
   strcpy(t, s);
   return t;
}

/* emalloc: malloc and report if error */
void *
_emalloc(size_t n)
{
   void *p;

   p = g_malloc(n);
   if (p == NULL)
      eprintf("g_malloc of %u bytes failed:", n);
   return p;
}

/* erealloc: realloc and report if error */
void *
_erealloc(void *ptr, size_t n)
{
   void *p;

   p = g_realloc(ptr, n);
   if (p == NULL)
      eprintf("g_realloc of %p by %u bytes failed:", ptr, n);
   return p;
}

/* efree: just do the free for now */
void
_efree(void *p)
{
   g_free(p);
}

char *
stroflen(char c, int l)
{
   static char buf[1024];
   int i = 0;

   buf[0] = '\0';
   while (l--)
      buf[i++] = c;
   buf[i] = '\0';
   return buf;
}
