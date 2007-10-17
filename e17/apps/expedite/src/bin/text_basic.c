#undef FNAME
#undef NAME
#undef ICON

/* metadata */
#define FNAME text_basic_start
#define NAME "Text Basic"
#define ICON "mushroom.png"

#ifndef PROTO
# ifndef UI
#  include "main.h"

/* standard var */
static int done = 0;

/* private data */
static Evas_Object *o_texts[OBNUM];

/* setup */
static void _setup(void)
{
   int i;
   Evas_Object *o;

   for (i = 0; i < OBNUM; i++)
     {
	o = evas_object_text_add(evas);
	o_texts[i] = o;
	evas_object_text_font_set(o, "Vera-Bold", 20);
	evas_object_text_text_set(o, "This is a test string");
	evas_object_color_set(o, 0, 0, 0, 255);
	evas_object_show(o);
     }
   done = 0;
}

/* cleanup */
static void _cleanup(void)
{
   int i;
   for (i = 0; i < OBNUM; i++) evas_object_del(o_texts[i]);
}

/* loop - do things */
static void _loop(double t, int f)
{
   int i;
   Evas_Coord x, y, w, h;
   for (i = 0; i < OBNUM; i++)
     {
	evas_object_geometry_get(o_texts[i], NULL, NULL, &w, &h);
	x = (win_w / 2) - (w / 2);
	x += sin((double)(f + (i * 13)) / (36.7 * SLOW)) * (w / 2);
	y = (win_h / 2) - (h / 2);
	y += cos((double)(f + (i * 28)) / (43.8 * SLOW)) * (w / 2);
	evas_object_move(o_texts[i], x, y);
     }
   FPS_STD(NAME);
}

/* prepend special key handlers if interactive (before STD) */
static void _key(char *key)
{
   KEY_STD;
}












/* template stuff - ignore */
# endif
#endif

#ifdef UI
_ui_menu_item_add(ICON, NAME, FNAME);
#endif

#ifdef PROTO
void FNAME(void);
#endif

#ifndef PROTO
# ifndef UI
void FNAME(void)
{
   ui_func_set(_key, _loop);
   _setup();
}
# endif
#endif
#undef FNAME
#undef NAME
#undef ICON
