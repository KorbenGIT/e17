/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
#define MODE_NOTHING        0
#define MODE_GEOMETRY       1
#define MODE_LOCKS          2
#define MODE_GEOMETRY_LOCKS 3
#define MODE_ALL            4
struct _E_Config_Dialog_Data
{
   E_Border *border;
   /*- BASIC -*/
   int mode;
   int warned;
   /*- ADVANCED -*/
   char *name;
   char *class;
   char *title;
   char *role;
   char *command;
   struct {
      int match_name;
      int match_class;
      int match_title;
      int match_role;
      int match_type;
      int match_transient;
      int apply_first_only;
      int apply_pos;
      int apply_size;
      int apply_layer;
      int apply_locks;
      int apply_border;
      int apply_sticky;
      int apply_desktop;
      int apply_shade;
      int apply_fullscreen;
      int apply_zone;
      int apply_skip_winlist;
      int apply_skip_pager;
      int apply_skip_taskbar;
      int apply_run;
      int apply_icon_pref;
      int set_focus_on_start;
   } remember;
};

/* a nice easy setup function that does the dirty work */
EAPI void
e_int_border_remember(E_Border *bd)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (v)
      {
         /* methods */
         v->create_cfdata           = _create_data;
         v->free_cfdata             = _free_data;
         v->basic.apply_cfdata      = _basic_apply_data;
         v->basic.create_widgets    = _basic_create_widgets;
         v->advanced.apply_cfdata   = _advanced_apply_data;
         v->advanced.create_widgets = _advanced_create_widgets;
	 v->override_auto_apply = 1;

         /* create config dialog for bd object/data */
         cfd = e_config_dialog_new(bd->zone->container,
				   _("Window Remember"),
				   "E", "_border_remember_dialog",
				   NULL, 0, v, bd);
         bd->border_remember_dialog = cfd;
      }
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Border *bd;
   E_Remember *rem;

   bd = cfdata->border;
   rem = bd->remember;

   if (!bd->remember) cfdata->mode = MODE_NOTHING;
   else if ((cfdata->remember.apply_pos) && (cfdata->remember.apply_size) &&
	    (cfdata->remember.apply_locks) && (cfdata->remember.apply_layer) &&
	    (cfdata->remember.apply_border) && (cfdata->remember.apply_sticky) &&
	    (cfdata->remember.apply_desktop) && (cfdata->remember.apply_shade) &&
	    (cfdata->remember.apply_zone) && (cfdata->remember.apply_skip_winlist) &&
	    (cfdata->remember.apply_skip_pager) && (cfdata->remember.apply_fullscreen) &&
	    (cfdata->remember.apply_skip_taskbar))
     cfdata->mode = MODE_ALL;
   else if ((cfdata->remember.apply_pos) && (cfdata->remember.apply_size) &&
	    (cfdata->remember.apply_locks))
     cfdata->mode = MODE_GEOMETRY_LOCKS;
   else if ((cfdata->remember.apply_pos) && (cfdata->remember.apply_size))
     cfdata->mode = MODE_GEOMETRY;
   else if ((cfdata->remember.apply_locks))
     cfdata->mode = MODE_LOCKS;
   else cfdata->mode = MODE_NOTHING;

   if (bd->client.icccm.name && bd->client.icccm.name[0])
     cfdata->name = strdup(bd->client.icccm.name);
   if (bd->client.icccm.class && bd->client.icccm.class[0])
     cfdata->class = strdup(bd->client.icccm.class);
   if (bd->client.netwm.name && bd->client.netwm.name[0])
     cfdata->title = strdup(bd->client.netwm.name);
   else if (bd->client.icccm.title && bd->client.icccm.title[0])
     cfdata->title = strdup(bd->client.icccm.title);
   if (bd->client.icccm.window_role && bd->client.icccm.window_role[0])
     cfdata->role = strdup(bd->client.icccm.window_role);

   if ((bd->client.icccm.command.argc > 0) &&
       (bd->client.icccm.command.argv))
     {
	char buf[4096];
	int i, j, k;

	buf[0] = 0;
	k = 0;
	for (i = 0; i < bd->client.icccm.command.argc; i++)
	  {
	     if (i > 0)
	       {
		  buf[k] = ' ';
		  k++;
	       }
	     for (j = 0; bd->client.icccm.command.argv[i][j]; j++)
	       {
		  if (k >= (sizeof(buf) - 10))
		    {
		       buf[k] = 0;
		       goto done;
		    }
		  if ((bd->client.icccm.command.argv[i][j] == ' ') ||
		      (bd->client.icccm.command.argv[i][j] == '\t') ||
		      (bd->client.icccm.command.argv[i][j] == '\\') ||
		      (bd->client.icccm.command.argv[i][j] == '\"') ||
		      (bd->client.icccm.command.argv[i][j] == '\'') ||
		      (bd->client.icccm.command.argv[i][j] == '$') ||
		      (bd->client.icccm.command.argv[i][j] == '%'))
		    {
		       buf[k] = '\\';
		       k++;
		    }
		  buf[k] = bd->client.icccm.command.argv[i][j];
		  k++;
	       }
	  }
	buf[k] = 0;
     done:
	cfdata->command = strdup(buf);
     }

   if (rem)
     {
	if (rem->apply_first_only) cfdata->remember.apply_first_only = 1;
	if (rem->match & E_REMEMBER_MATCH_NAME)         cfdata->remember.match_name = 1;
	if (rem->match & E_REMEMBER_MATCH_CLASS)        cfdata->remember.match_class = 1;
	if (rem->match & E_REMEMBER_MATCH_TITLE)        cfdata->remember.match_title = 1;
	if (rem->match & E_REMEMBER_MATCH_ROLE)         cfdata->remember.match_role = 1;
	if (rem->match & E_REMEMBER_MATCH_TYPE)         cfdata->remember.match_type = 1;
	if (rem->match & E_REMEMBER_MATCH_TRANSIENT)    cfdata->remember.match_transient = 1;
	if (rem->apply & E_REMEMBER_APPLY_POS)          cfdata->remember.apply_pos = 1;
	if (rem->apply & E_REMEMBER_APPLY_SIZE)         cfdata->remember.apply_size = 1;
	if (rem->apply & E_REMEMBER_APPLY_LAYER)        cfdata->remember.apply_layer = 1;
	if (rem->apply & E_REMEMBER_APPLY_LOCKS)        cfdata->remember.apply_locks = 1;
	if (rem->apply & E_REMEMBER_APPLY_BORDER)       cfdata->remember.apply_border = 1;
	if (rem->apply & E_REMEMBER_APPLY_STICKY)       cfdata->remember.apply_sticky = 1;
	if (rem->apply & E_REMEMBER_APPLY_DESKTOP)      cfdata->remember.apply_desktop = 1;
	if (rem->apply & E_REMEMBER_APPLY_SHADE)        cfdata->remember.apply_shade = 1;
	if (rem->apply & E_REMEMBER_APPLY_FULLSCREEN)   cfdata->remember.apply_fullscreen = 1;
	if (rem->apply & E_REMEMBER_APPLY_ZONE)         cfdata->remember.apply_zone = 1;
	if (rem->apply & E_REMEMBER_APPLY_SKIP_WINLIST) cfdata->remember.apply_skip_winlist = 1;
	if (rem->apply & E_REMEMBER_APPLY_SKIP_PAGER)   cfdata->remember.apply_skip_pager = 1;
	if (rem->apply & E_REMEMBER_APPLY_SKIP_TASKBAR) cfdata->remember.apply_skip_taskbar = 1;
	if (rem->apply & E_REMEMBER_APPLY_RUN)          cfdata->remember.apply_run = 1;
	if (rem->apply & E_REMEMBER_APPLY_ICON_PREF)    cfdata->remember.apply_icon_pref = 1;
	if (rem->apply & E_REMEMBER_SET_FOCUS_ON_START) cfdata->remember.set_focus_on_start = 1;
     }
}


static void *
_create_data(E_Config_Dialog *cfd)
{
   /* Create cfdata - cfdata is a temporary block of config data that this
    * dialog will be dealing with while configuring. it will be applied to
    * the running systems/config in the apply methods
    */
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->border = cfd->data;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
   if (cfdata->name) free(cfdata->name);
   if (cfdata->class) free(cfdata->class);
   if (cfdata->title) free(cfdata->title);
   if (cfdata->role) free(cfdata->role);
   if (cfdata->command) free(cfdata->command);
   if (cfdata->border)
     cfdata->border->border_remember_dialog = NULL;
   free(cfdata);
}

/**--APPLY--**/
static int
_check_matches(E_Border *bd, int matchflags)
{
   Eina_List *l;
   E_Border *bd2;
   int n = 0;
   const char *title;

   title = e_border_name_get(bd);
   EINA_LIST_FOREACH(e_border_client_list(), l, bd2)
     {
	int required_matches;
	int matches;
	const char *title2;

        matches = 0;
	required_matches = 0;
	if (matchflags & E_REMEMBER_MATCH_NAME) required_matches++;
	if (matchflags & E_REMEMBER_MATCH_CLASS) required_matches++;
	if (matchflags & E_REMEMBER_MATCH_TITLE) required_matches++;
	if (matchflags & E_REMEMBER_MATCH_ROLE) required_matches++;
	if (matchflags & E_REMEMBER_MATCH_TYPE) required_matches++;
	if (matchflags & E_REMEMBER_MATCH_TRANSIENT) required_matches++;
	title2 = e_border_name_get(bd2);
        if ((matchflags & E_REMEMBER_MATCH_NAME) &&
	    ((!e_util_strcmp(bd->client.icccm.name, bd2->client.icccm.name)) ||
	     (e_util_both_str_empty(bd->client.icccm.name, bd2->client.icccm.name))))
	  matches++;
	if ((matchflags & E_REMEMBER_MATCH_CLASS) &&
	    ((!e_util_strcmp(bd->client.icccm.class, bd2->client.icccm.class)) ||
	     (e_util_both_str_empty(bd->client.icccm.class, bd2->client.icccm.class))))
	  matches++;
	if ((matchflags & E_REMEMBER_MATCH_TITLE) &&
	    ((!e_util_strcmp(title, title2)) ||
	     (e_util_both_str_empty(title, title2))))
	  matches++;
	if ((matchflags & E_REMEMBER_MATCH_ROLE) &&
	    ((!e_util_strcmp(bd->client.icccm.window_role, bd2->client.icccm.window_role)) ||
	     (e_util_both_str_empty(bd->client.icccm.window_role, bd2->client.icccm.window_role))))
	  matches++;
	if ((matchflags & E_REMEMBER_MATCH_TYPE) &&
	    (bd->client.netwm.type == bd2->client.netwm.type))
	  matches++;
	if ((matchflags & E_REMEMBER_MATCH_TRANSIENT) &&
	    (((bd->client.icccm.transient_for) && (bd2->client.icccm.transient_for != 0)) ||
	     ((!bd->client.icccm.transient_for) && (bd2->client.icccm.transient_for == 0))))
	  matches++;
	if (matches >= required_matches) n++;
     }
   return n;
}

static void
_remember_update(E_Remember *rem, E_Config_Dialog_Data *cfdata)
{
   if (rem->name) eina_stringshare_del(rem->name);
   if (rem->class) eina_stringshare_del(rem->class);
   if (rem->title) eina_stringshare_del(rem->title);
   if (rem->role) eina_stringshare_del(rem->role);
   if (rem->prop.command) eina_stringshare_del(rem->prop.command);
   rem->name = NULL;
   rem->class = NULL;
   rem->title = NULL;
   rem->role = NULL;
   rem->prop.command = NULL;
   if (cfdata->name  && cfdata->name[0])
     rem->name = eina_stringshare_add(cfdata->name);
   if (cfdata->class && cfdata->class[0])
     rem->class = eina_stringshare_add(cfdata->class);
   if (cfdata->title && cfdata->title[0])
     rem->title = eina_stringshare_add(cfdata->title);
   if (cfdata->role  && cfdata->role[0])
     rem->role = eina_stringshare_add(cfdata->role);
   if (cfdata->command && cfdata->command[0])
     rem->prop.command = eina_stringshare_add(cfdata->command);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   E_Remember *rem;

   if (cfdata->mode == MODE_NOTHING)
     {
	if (cfdata->border->remember)
	  {
	     rem = cfdata->border->remember;
	     cfdata->border->remember = NULL;
	     e_remember_unuse(rem);
	     e_remember_del(rem);
	  }
	e_config_save_queue();
	return 1;
     }
   if (!cfdata->warned)
     {
	int matches = 0;

	if ((cfdata->border->client.icccm.name) &&
	    (cfdata->border->client.icccm.class) &&
	    (cfdata->border->client.icccm.name[0] != 0) &&
	    (cfdata->border->client.icccm.class[0] != 0))
	  matches = _check_matches(cfdata->border, E_REMEMBER_MATCH_NAME | E_REMEMBER_MATCH_CLASS | E_REMEMBER_MATCH_ROLE | E_REMEMBER_MATCH_TYPE | E_REMEMBER_MATCH_TRANSIENT);
	else
	  matches = _check_matches(cfdata->border, E_REMEMBER_MATCH_TITLE | E_REMEMBER_MATCH_ROLE | E_REMEMBER_MATCH_TYPE | E_REMEMBER_MATCH_TRANSIENT);
	if (matches > 1)
	  {
	     E_Dialog *dia;

	     dia = e_dialog_new(cfd->con, "E", "_border_remember_error_multi_dialog");
	     e_dialog_title_set(dia, _("Window properties are not a unique match"));
	     e_dialog_text_set
	       (dia,
		_("You are trying to ask Enlightenment to remember to apply<br>"
		  "properties (such as size, location, border style etc.) to<br>"
		  "a window that <hilight>does not have unique properties</hilight>.<br>"
		  "<br>"
		  "This means it shares Name/Class, Transience, Role etc. properties<br>"
		  "with more than 1 other window on the screen and remembering<br>"
		  "properties for this window will apply to all other windows<br>"
		  "that match these properties.<br>"
		  "<br>"
		  "This is just a warning in case you did not intend this to happen.<br>"
		  "If you did, simply press <hilight>Apply</hilight> or <hilight>OK</hilight> buttons<br>"
		  "and your settings will be accepted. Press <hilight>Cancel</hilight> if you<br>"
		  "are not sure and nothing will be affected.")
		);
	     e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	     e_win_centered_set(dia->win, 1);
	     e_dialog_show(dia);
	     cfdata->warned = 1;
	     return 0;
	  }
     }

   rem = cfdata->border->remember;

   if (!rem)
     {
	rem = e_remember_new();
	if (rem)
	  e_remember_use(rem);
     }

   if (rem)
     {
	rem->match = e_remember_default_match(cfdata->border);

	if (cfdata->mode == MODE_GEOMETRY)
	  rem->apply = E_REMEMBER_APPLY_POS | E_REMEMBER_APPLY_SIZE;
	else if (cfdata->mode == MODE_LOCKS)
	  rem->apply = E_REMEMBER_APPLY_LOCKS;
	else if (cfdata->mode == MODE_GEOMETRY_LOCKS)
	  rem->apply = E_REMEMBER_APPLY_POS | E_REMEMBER_APPLY_SIZE | E_REMEMBER_APPLY_LOCKS;
	else if (cfdata->mode == MODE_ALL)
	  rem->apply = E_REMEMBER_APPLY_POS | E_REMEMBER_APPLY_SIZE | E_REMEMBER_APPLY_LAYER |
					    E_REMEMBER_APPLY_LOCKS | E_REMEMBER_APPLY_BORDER | E_REMEMBER_APPLY_STICKY |
					    E_REMEMBER_APPLY_DESKTOP | E_REMEMBER_APPLY_SHADE | E_REMEMBER_APPLY_ZONE |
					    E_REMEMBER_APPLY_SKIP_WINLIST | E_REMEMBER_APPLY_SKIP_PAGER | E_REMEMBER_APPLY_SKIP_TASKBAR |
	                                    E_REMEMBER_APPLY_FULLSCREEN | E_REMEMBER_APPLY_ICON_PREF;
	rem->apply_first_only = 0;

	_remember_update(rem, cfdata);

	e_remember_update(rem, cfdata->border);
     }

   cfdata->border->remember = rem;

   e_config_save_queue();
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   E_Border *bd = cfdata->border;
   E_Remember *rem = bd->remember;

   if (!((cfdata->remember.apply_pos) || (cfdata->remember.apply_size) ||
	 (cfdata->remember.apply_locks) || (cfdata->remember.apply_layer) ||
	 (cfdata->remember.apply_border) || (cfdata->remember.apply_sticky) ||
	 (cfdata->remember.apply_desktop) || (cfdata->remember.apply_shade) ||
	 (cfdata->remember.apply_zone) || (cfdata->remember.apply_skip_winlist) ||
	 (cfdata->remember.apply_skip_pager) || (cfdata->remember.apply_skip_taskbar) ||
	 (cfdata->remember.apply_run) || (cfdata->remember.apply_icon_pref) ||
	 (cfdata->remember.set_focus_on_start) || (cfdata->remember.apply_fullscreen)))
     {
	if (rem)
	  {
	     e_remember_unuse(rem);
	     e_remember_del(rem);
	     cfdata->border->remember = NULL;
	  }
	e_config_save_queue();
	return 1;
     }

   if (cfdata->remember.match_name) cfdata->remember.match_class = 1;
   else cfdata->remember.match_class = 0;

   if (!((cfdata->remember.match_name)  || (cfdata->remember.match_class) ||
	 (cfdata->remember.match_title) || (cfdata->remember.match_role) ||
	 (cfdata->remember.match_type)  || (cfdata->remember.match_transient)))
     {
	E_Dialog *dia;

	dia = e_dialog_new(cfd->con, "E", "_border_remember_error_noprop_dialog");
	e_dialog_title_set(dia, _("No match properties set"));
	e_dialog_text_set
	  (dia,
	   _("You are trying to ask Enlightenment to remember to apply<br>"
	     "properties (such as size, location, border style etc.) to<br>"
	     "a window <hilight>without specifying how to remember it</hilight>.<br>"
	     "<br>"
	     "You must specify at least 1 way of remembering this window.")
	   );
	e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	e_win_centered_set(dia->win, 1);
	e_dialog_show(dia);
	return 0;
     }
   if (!cfdata->warned)
     {
	int matchflags = 0;

	if (cfdata->remember.match_name)  matchflags |= E_REMEMBER_MATCH_NAME;
	if (cfdata->remember.match_class) matchflags |= E_REMEMBER_MATCH_CLASS;
	if (cfdata->remember.match_title) matchflags |= E_REMEMBER_MATCH_TITLE;
	if (cfdata->remember.match_role)  matchflags |= E_REMEMBER_MATCH_ROLE;
	if (cfdata->remember.match_type)  matchflags |= E_REMEMBER_MATCH_TYPE;
	if (cfdata->remember.match_transient) matchflags |= E_REMEMBER_MATCH_TRANSIENT;
	if ((!cfdata->remember.apply_first_only) &&
	    (_check_matches(cfdata->border, matchflags) > 1))
	  {
	     E_Dialog *dia;

	     dia = e_dialog_new(cfd->con, "E", "_border_remember_error_no_remember_dialog");
	     e_dialog_title_set(dia, _("No match properties set"));
	     e_dialog_text_set
	       (dia,
		_("You are trying to ask Enlightenment to remember to apply<br>"
		  "properties (such as size, location, border style etc.) to<br>"
		  "a window that <hilight>does not have unique properties</hilight>.<br>"
		  "<br>"
		  "This means it shares Name/Class, Transience, Role etc. properties<br>"
		  "with more than 1 other window on the screen and remembering<br>"
		  "properties for this window will apply to all other windows<br>"
		  "that match these properties.<br>"
		  "<br>"
		  "You may wish to enable the <hilight>Match only one window</hilight> option if<br>"
		  "you only intend one instance of this window to be modified, with<br>"
		  "additional instances not being modified.<br>"
		  "<br>"
		  "This is just a warning in case you did not intend this to happen.<br>"
		  "If you did, simply press <hilight>Apply</hilight> or <hilight>OK</hilight> buttons<br>"
		  "and your settings will be accepted. Press <hilight>Cancel</hilight> if you<br>"
		  "are not sure and nothing will be affected.")
		);
	     e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	     e_win_centered_set(dia->win, 1);
	     e_dialog_show(dia);
	     cfdata->warned = 1;
	     return 0;
	  }
     }

   if (!rem)
     {
	rem = e_remember_new();
	if (rem)
	   e_remember_use(rem);
     }

   if (rem)
     {
	rem->apply = 0;
	rem->match = 0;
	if (cfdata->remember.match_name)         rem->match |= E_REMEMBER_MATCH_NAME;
	if (cfdata->remember.match_class)        rem->match |= E_REMEMBER_MATCH_CLASS;
	if (cfdata->remember.match_title)        rem->match |= E_REMEMBER_MATCH_TITLE;
	if (cfdata->remember.match_role)         rem->match |= E_REMEMBER_MATCH_ROLE;
	if (cfdata->remember.match_type)         rem->match |= E_REMEMBER_MATCH_TYPE;
	if (cfdata->remember.match_transient)    rem->match |= E_REMEMBER_MATCH_TRANSIENT;
	if (cfdata->remember.apply_pos)          rem->apply |= E_REMEMBER_APPLY_POS;
	if (cfdata->remember.apply_size)         rem->apply |= E_REMEMBER_APPLY_SIZE;
	if (cfdata->remember.apply_layer)        rem->apply |= E_REMEMBER_APPLY_LAYER;
	if (cfdata->remember.apply_locks)        rem->apply |= E_REMEMBER_APPLY_LOCKS;
	if (cfdata->remember.apply_border)       rem->apply |= E_REMEMBER_APPLY_BORDER;
	if (cfdata->remember.apply_sticky)       rem->apply |= E_REMEMBER_APPLY_STICKY;
	if (cfdata->remember.apply_desktop)      rem->apply |= E_REMEMBER_APPLY_DESKTOP;
	if (cfdata->remember.apply_shade)        rem->apply |= E_REMEMBER_APPLY_SHADE;
	if (cfdata->remember.apply_fullscreen)   rem->apply |= E_REMEMBER_APPLY_FULLSCREEN;
	if (cfdata->remember.apply_zone)         rem->apply |= E_REMEMBER_APPLY_ZONE;
	if (cfdata->remember.apply_skip_winlist) rem->apply |= E_REMEMBER_APPLY_SKIP_WINLIST;
	if (cfdata->remember.apply_skip_pager)   rem->apply |= E_REMEMBER_APPLY_SKIP_PAGER;
	if (cfdata->remember.apply_skip_taskbar) rem->apply |= E_REMEMBER_APPLY_SKIP_TASKBAR;
	if (cfdata->remember.apply_run)          rem->apply |= E_REMEMBER_APPLY_RUN;
	if (cfdata->remember.apply_icon_pref)    rem->apply |= E_REMEMBER_APPLY_ICON_PREF;
	if (cfdata->remember.set_focus_on_start) rem->apply |= E_REMEMBER_SET_FOCUS_ON_START;

	rem->apply_first_only = cfdata->remember.apply_first_only;

	_remember_update(rem, cfdata);

	cfdata->border->remember = rem;

	e_remember_update(rem, cfdata->border);
     }

   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ob;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->mode));
   ob = e_widget_radio_add(evas, _("Nothing"), MODE_NOTHING, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Size and Position"), MODE_GEOMETRY, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Locks"), MODE_LOCKS, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Size, Position and Locks"), MODE_GEOMETRY_LOCKS, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Everything"), MODE_ALL, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o, *ob, *of;

   o = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("Remember using"), 0);
   if (cfdata->name)
     {
	ob = e_widget_check_add(evas, _("Window name"), &(cfdata->remember.match_name));
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_entry_add(evas, &cfdata->name, NULL, NULL, NULL);
	e_widget_entry_readonly_set(ob, 1);
	e_widget_framelist_object_append(of, ob);
     }
   else
     {
	cfdata->remember.match_name = 0;
     }
   if (cfdata->class)
     {
	ob = e_widget_check_add(evas, _("Window class"), &(cfdata->remember.match_class));
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_entry_add(evas, &cfdata->class, NULL, NULL, NULL);
	e_widget_entry_readonly_set(ob, 1);
	e_widget_framelist_object_append(of, ob);
     }
   else
     {
	cfdata->remember.match_class = 0;
     }
   if (cfdata->title)
     {
	ob = e_widget_check_add(evas, _("Title"), &(cfdata->remember.match_title));
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_entry_add(evas, &cfdata->title, NULL, NULL, NULL);
	e_widget_entry_readonly_set(ob, 1);
	e_widget_framelist_object_append(of, ob);
     }
   else
     {
	cfdata->remember.match_title = 0;
     }
   if (cfdata->role)
     {
	ob = e_widget_check_add(evas, _("Window Role"), &(cfdata->remember.match_role));
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_entry_add(evas, &cfdata->role, NULL, NULL, NULL);
	e_widget_entry_readonly_set(ob, 1);
	e_widget_framelist_object_append(of, ob);
     }
   else
     {
	cfdata->remember.match_role = 0;
     }
   if (cfdata->border->client.netwm.type != ECORE_X_WINDOW_TYPE_UNKNOWN)
     {
	ob = e_widget_check_add(evas, _("Window type"), &(cfdata->remember.match_type));
	e_widget_framelist_object_append(of, ob);
     }
   else
     {
	cfdata->remember.match_type = 0;
     }
   ob = e_widget_check_add(evas, _("Transience"), &(cfdata->remember.match_transient));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(o, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_frametable_add(evas, _("Properties to remember"), 0);
   ob = e_widget_check_add(evas, _("Position"), &(cfdata->remember.apply_pos));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Size"), &(cfdata->remember.apply_size));
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Stacking"), &(cfdata->remember.apply_layer));
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Locks"), &(cfdata->remember.apply_locks));
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Border style"), &(cfdata->remember.apply_border));
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Icon Preference"), &(cfdata->remember.apply_icon_pref));
   e_widget_frametable_object_append(of, ob, 0, 5, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Stickiness"), &(cfdata->remember.apply_sticky));
   e_widget_frametable_object_append(of, ob, 0, 6, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Virtual Desktop"), &(cfdata->remember.apply_desktop));
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Shaded state"), &(cfdata->remember.apply_shade));
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Fullscreen state"), &(cfdata->remember.apply_fullscreen));
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Current Screen"), &(cfdata->remember.apply_zone));
   e_widget_frametable_object_append(of, ob, 1, 3, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Skip Window List"), &(cfdata->remember.apply_skip_winlist));
   e_widget_frametable_object_append(of, ob, 1, 4, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Skip Pager"), &(cfdata->remember.apply_skip_pager));
   e_widget_frametable_object_append(of, ob, 1, 5, 1, 1, 1, 1, 1, 1);
   ob = e_widget_check_add(evas, _("Skip Taskbar"), &(cfdata->remember.apply_skip_taskbar));
   e_widget_frametable_object_append(of, ob, 1, 6, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(o, of, 1, 0, 1, 1, 1, 1, 1, 1);

   ob = e_widget_check_add(evas, _("Match only one window"), &(cfdata->remember.apply_first_only));
   e_widget_table_object_append(o, ob, 0, 1, 1, 1, 1, 1, 1, 1);

   if (cfdata->command)
     {
	ob = e_widget_check_add(evas, _("Start this program on login"), &(cfdata->remember.apply_run));
	e_widget_table_object_append(o, ob, 1, 2, 1, 1, 1, 1, 1, 1);
     }

   ob = e_widget_check_add(evas, _("Always focus on start"), &(cfdata->remember.set_focus_on_start));
   e_widget_table_object_append(o, ob, 1, 1, 1, 1, 1, 1, 1, 1);

   return o;
}
