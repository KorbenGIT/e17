/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef _WIN32
# define _GNU_SOURCE
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca (size_t);
#endif

#include <string.h>

#include "edje_private.h"

static void _edje_emit_cb(Edje *ed, const char *sig, const char *src);
static void _edje_param_copy(Edje_Real_Part *src_part, const char *src_param, Edje_Real_Part *dst_part, const char *dst_param);
static void _edje_param_set(Edje_Real_Part *part, const char *param, const char *value);

int             _edje_anim_count = 0;
Ecore_Animator *_edje_timer = NULL;
Eina_List      *_edje_animators = NULL;


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @addtogroup Edje_program_Group Program
 *
 * @brief These functions provide an abstraction layer between the
 * application code and the interface, while allowing extremely
 * flexible dynamic layouts and animations.
 *
 * @{
 */

/**
 * @brief Set edje trasitions' frame time.
 *
 * @param t The frame time, in seconds. Default value is 1/30.
 *
 * This function sets the edje built-in animations' frame time (thus,
 * affecting their resolution) by calling
 * ecore_animator_frametime_set(). This frame time can be retrieved
 * with edje_frametime_get().
 *
 * @see edje_frametime_get()
 *
 */
EAPI void
edje_frametime_set(double t)
{
   ecore_animator_frametime_set(t);
}

/**
 * @brief Get edje trasitions' frame time.
 *
 * @return The frame time, in seconds.
 *
 * This function returns the edje frame time set by
 * edje_frametime_set().
 *
 * @see edje_frametime_set()
 *
 */
EAPI double
edje_frametime_get(void)
{
   return ecore_animator_frametime_get();
}

/**
 * @brief Add a callback for a signal emitted by @a obj.
 *
 * @param obj A valid Evas_Object handle.
 * @param emission The signal's name.
 * @param source The signal's source.
 * @param func The callback function to be executed when the signal is
 * emitted.
 * @param data A pointer to data to pass in to the callback function.
 *
 * Connects a callback function to a signal emitted by @a obj.
 * In EDC, a program can emit a signal as follows:
 *
 * @code
 * program {
 *   name: "emit_example";
 *   action: SIGNAL_EMIT "a_signal" "a_source";
 * }
 * @endcode
 *
 * Assuming a function with the following declaration is definded:
 *
 * @code
 * void cb_signal(void *data, Evas_Object *o, const char *emission, const char *source);
 * @endcode
 *
 * a callback is attached using:
 *
 * @code
 * edje_object_signal_callback_add(obj, "a_signal", "a_source", cb_signal, data);
 * @endcode
 *
 * Here, @a data is an arbitrary pointer to be used as desired.  Note
 * that @a emission and @a source correspond respectively to the first
 * and the second parameters at the SIGNAL_EMIT action.
 *
 * Internal edje signals can also be attached to, and globs can occur
 * in either the emission or source name, e.g.
 *
 * @code
 * edje_object_signal_callback_add(obj, "mouse,down,*", "button.*", NULL);
 * @endcode
 *
 * Here, any mouse down events on an edje part whose name begins with
 * "button." will trigger the callback. The actual signal and source
 * names will be passed in to the @a emission and @a source parameters
 * of the callback function (e.g. "mouse,down,2" and "button.close").
 */
EAPI void
edje_object_signal_callback_add(Evas_Object *obj, const char *emission, const char *source, void (*func) (void *data, Evas_Object *o, const char *emission, const char *source), void *data)
{
   Edje *ed;
   Edje_Signal_Callback *escb;

   if ((!emission) || (!source) || (!func)) return;
   ed = _edje_fetch(obj);
   if (!ed) return;
   if (ed->delete_me) return;
   escb = calloc(1, sizeof(Edje_Signal_Callback));
   if (emission[0])
     escb->signal = eina_stringshare_add(emission);
   if (source[0])
     escb->source = eina_stringshare_add(source);
   escb->func = func;
   escb->data = data;
   ed->callbacks = eina_list_append(ed->callbacks, escb);
   if (ed->walking_callbacks)
     {
	escb->just_added = 1;
	ed->just_added_callbacks = 1;
     }
   else
     _edje_callbacks_patterns_clean(ed);
}

/**
 * @brief Remove a signal-triggered callback from an object.
 *
 * @param obj A valid Evas_Object handle.
 * @param emission The emission string.
 * @param source The source string.
 * @param func The callback function.
 * @return The data pointer
 *
 * This function removes a callback, previously attached to the
 * emittion of a signal, from the object @a obj. The parameters @a
 * emission, @a source and @a func must match exactly those passed to
 * a previous call to edje_object_signal_callback_add(). The data
 * pointer that was passed to this call will be returned.
 *
 * @see edje_object_signal_callback_add().
 *
 */
EAPI void *
edje_object_signal_callback_del(Evas_Object *obj, const char *emission, const char *source, void (*func) (void *data, Evas_Object *o, const char *emission, const char *source))
{
   Edje *ed;
   Eina_List *l;
   Edje_Signal_Callback *escb;

   if ((!emission) || (!source) || (!func)) return NULL;
   ed = _edje_fetch(obj);
   if (!ed) return NULL;
   if (ed->delete_me) return NULL;
   EINA_LIST_FOREACH(ed->callbacks, l, escb)
     {
	if ((escb->func == func) &&
	    ((!escb->signal && !emission[0]) ||
             (escb->signal && !strcmp(escb->signal, emission))) &&
	    ((!escb->source && !source[0]) ||
             (escb->source && !strcmp(escb->source, source))))
	  {
	     void *data;

	     data = escb->data;
	     if (ed->walking_callbacks)
	       {
		  escb->delete_me = 1;
		  ed->delete_callbacks = 1;
	       }
	     else
	       {
		  _edje_callbacks_patterns_clean(ed);

		  ed->callbacks = eina_list_remove_list(ed->callbacks, l);
		  if (escb->signal) eina_stringshare_del(escb->signal);
		  if (escb->source) eina_stringshare_del(escb->source);
		  free(escb);
	       }
	     return data;
	  }
     }
   return NULL;
}

/**
 * @brief Send a signal to an edje object.
 *
 * @param obj A valid Evas_Object handle.
 * @param emission The signal's name.
 * @param source The signal's source.
 *
 * This function sends a signal to the object @a obj. An edje program
 * can respond to a signal by specifying matching 'signal' and
 * 'source' fields.
 *
 * @code
 * edje_object_signal_emit(obj, "a_signal", "");
 * @endcode
 *
 * will trigger a program whose EDC block is:
 *
 * @code
 * program {
 *  name: "a_program";
 *  signal: "a_signal";
 *  source: "";
 *  action: ...
 * }
 * @endcode
 *
 * FIXME: should this signal be sent to children also?
 */
EAPI void
edje_object_signal_emit(Evas_Object *obj, const char *emission, const char *source)
{
   Edje *ed;

   if ((!emission) || (!source)) return;
   ed = _edje_fetch(obj);
   if (!ed) return;
   if (ed->delete_me) return;
   _edje_emit(ed, (char *)emission, (char *)source);
}

/* FIXDOC: Verify/Expand */
/**
 * @brief Set the edje object to playing or paused states.
 *
 * @param obj A valid Evas_Object handle.
 * @param play Object state (1 to playing, 0 to pauseed).
 *
 * This function sets the edje object @a obj to playing or paused
 * states, depending on the parameter @a play.  This has no effect if
 * the object was already at that state.
 *
 * @see edje_object_play_get().
 *
 */
EAPI void
edje_object_play_set(Evas_Object *obj, Eina_Bool play)
{
   Edje *ed;
   double t;
   Eina_List *l;
   Edje_Running_Program *runp;
   int i;

   ed = _edje_fetch(obj);
   if (!ed) return;
   if (ed->delete_me) return;
   if (play)
     {
	if (!ed->paused) return;
	ed->paused = 0;
	t = ecore_time_get() - ed->paused_at;
	EINA_LIST_FOREACH(ed->actions, l, runp)
	  runp->start_time += t;
     }
   else
     {
	if (ed->paused) return;
	ed->paused = EINA_TRUE;
	ed->paused_at = ecore_time_get();
     }

   for (i = 0; i < ed->table_parts_size; i++)
     {
	Edje_Real_Part *rp;
	rp = ed->table_parts[i];
	if (rp->part->type == EDJE_PART_TYPE_GROUP && rp->swallowed_object)
	  edje_object_play_set(rp->swallowed_object, play);
     }
}

/**
 * @brief Get the edje object's play/pause state.
 *
 * @param obj A valid Evas_Object handle.
 * @return @c EINA_FALSE if the object is not connected, its @c delete_me flag
 * is set, or it is at paused state; @c EINA_TRUE if the object is at playing
 * state.
 *
 * This function tells if an edje object is playing or not. This state
 * is set by edje_object_play_set().
 *
 * @see edje_object_play_set().
 *
 */
EAPI Eina_Bool
edje_object_play_get(const Evas_Object *obj)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed) return EINA_FALSE;
   if (ed->delete_me) return EINA_FALSE;
   if (ed->paused) return EINA_FALSE;
   return EINA_TRUE;
}

/* FIXDOC: Verify/Expand */
/**
 * @brief Set the object's animation state.
 *
 * @param obj A valid Evas_Object handle.
 * @param on Animation State.
 *
 * This function starts or stops an edje object's animation. The
 * information if it's runnig can be retrieved by
 * edje_object_animation_get().
 *
 * @see edje_object_animation_get()
 *
 */
EAPI void
edje_object_animation_set(Evas_Object *obj, Eina_Bool on)
{
   Edje *ed;
   Eina_List *l;
   int i;

   ed = _edje_fetch(obj);
   if (!ed) return;
   if (ed->delete_me) return;
   _edje_block(ed);
   ed->no_anim = !on;
   _edje_freeze(ed);
   if (!on)
     {
	Eina_List *newl = NULL;
	const void *data;

	EINA_LIST_FOREACH(ed->actions, l, data)
	  newl = eina_list_append(newl, data);
	while (newl)
	  {
	     Edje_Running_Program *runp;

	     runp = eina_list_data_get(newl);
	     newl = eina_list_remove(newl, eina_list_data_get(newl));
	     _edje_program_run_iterate(runp, runp->start_time + TO_DOUBLE(runp->program->tween.time));
	     if (_edje_block_break(ed))
	       {
		  eina_list_free(newl);
		  goto break_prog;
	       }
	  }
     }
   else
     {
	_edje_emit(ed, "load", NULL);
	if (evas_object_visible_get(obj))
	  {
	     evas_object_hide(obj);
	     evas_object_show(obj);
	  }
     }
   break_prog:

   for (i = 0; i < ed->table_parts_size; i++)
     {
	Edje_Real_Part *rp;
	rp = ed->table_parts[i];
	if (rp->part->type == EDJE_PART_TYPE_GROUP && rp->swallowed_object)
	  edje_object_animation_set(rp->swallowed_object, on);
     }

   _edje_thaw(ed);
   _edje_unblock(ed);
}

/**
 * @brief Get the edje object's animation state.
 *
 * @param obj A valid Evas_Object handle.
 * @return @c EINA_FALSE on error or if object is not animated;
 * @c EINA_TRUE if animated.
 *
 * This function returns if the animation is playing or not. The
 * animation state is set by edje_object_play_set().
 *
 * @see edje_object_animation_set().
 *
 */

EAPI Eina_Bool
edje_object_animation_get(const Evas_Object *obj)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed) return EINA_FALSE;
   if (ed->delete_me) return EINA_FALSE;
   if (ed->no_anim) return EINA_FALSE;
   return EINA_TRUE;
}

/* Private Routines */

Eina_Bool
_edje_program_run_iterate(Edje_Running_Program *runp, double tim)
{
   FLOAT_T t, total;
   Eina_List *l;
   Edje *ed;
   Edje_Program_Target *pt;
   Edje_Real_Part *rp;

   ed = runp->edje;
   if (ed->delete_me) return EINA_FALSE;
   _edje_block(ed);
   _edje_ref(ed);
   _edje_freeze(ed);
   t = FROM_DOUBLE(tim - runp->start_time);
   total = runp->program->tween.time;
   t = DIV(t, total);
   if (t > FROM_INT(1)) t = FROM_INT(1);
   EINA_LIST_FOREACH(runp->program->targets, l, pt)
     {
	if (pt->id >= 0)
	  {
	     rp = ed->table_parts[pt->id % ed->table_parts_size];
	     if (rp) _edje_part_pos_set(ed, rp,
					runp->program->tween.mode, t);
	  }
     }
   if (t >= FROM_INT(1))
     {
        Edje_Program_After *pa;

        EINA_LIST_FOREACH(runp->program->targets, l, pt)
	  {
	     if (pt->id >= 0)
	       {
		  rp = ed->table_parts[pt->id % ed->table_parts_size];
		  if (rp)
		    {
		       _edje_part_description_apply(ed, rp,
						    runp->program->state,
						    runp->program->value,
						    NULL,
						    0.0);
		       _edje_part_pos_set(ed, rp,
					  runp->program->tween.mode, ZERO);
		       rp->program = NULL;
		    }
	       }
	  }
	_edje_recalc(ed);
	runp->delete_me = 1;
	if (!ed->walking_actions)
	  {
	     _edje_anim_count--;
	     ed->actions = eina_list_remove(ed->actions, runp);
	     if (!ed->actions)
	       _edje_animators = eina_list_remove(_edje_animators, ed);
	  }
//	_edje_emit(ed, "program,stop", runp->program->name);
	if (_edje_block_break(ed))
	  {
	     if (!ed->walking_actions) free(runp);
	     goto break_prog;
	  }
	EINA_LIST_FOREACH(runp->program->after, l, pa)
	  {
	     Edje_Program *pr;

	     if (pa->id >= 0)
	       {
		  pr = ed->table_programs[pa->id % ed->table_programs_size];
		  if (pr) _edje_program_run(ed, pr, 0, "", "");
		  if (_edje_block_break(ed))
		    {
		       if (!ed->walking_actions) free(runp);
		       goto break_prog;
		    }
	       }
	  }
	_edje_thaw(ed);
	_edje_unref(ed);
	if (!ed->walking_actions) free(runp);
	_edje_unblock(ed);
	return  EINA_FALSE;
     }
   break_prog:
   _edje_recalc(ed);
   _edje_thaw(ed);
   _edje_unref(ed);
   _edje_unblock(ed);
   return EINA_TRUE;
}

void
_edje_program_end(Edje *ed, Edje_Running_Program *runp)
{
   Eina_List *l;
   Edje_Program_Target *pt;
   const char *pname = NULL;
   int free_runp = 0;

   if (ed->delete_me) return;
   _edje_ref(ed);
   _edje_freeze(ed);
   EINA_LIST_FOREACH(runp->program->targets, l, pt)
     {
	Edje_Real_Part *rp;

	if (pt->id >= 0)
	  {
	     rp = ed->table_parts[pt->id % ed->table_parts_size];
	     if (rp)
	       {
		  _edje_part_description_apply(ed, rp,
					       runp->program->state,
					       runp->program->value,
					       NULL,
					       0.0);
		  _edje_part_pos_set(ed, rp,
				     runp->program->tween.mode, ZERO);
		  rp->program = NULL;
	       }
	  }
     }
   _edje_recalc(ed);
   runp->delete_me = 1;
   pname = runp->program->name;
   if (!ed->walking_actions)
     {
	_edje_anim_count--;
	ed->actions = eina_list_remove(ed->actions, runp);
	free_runp = 1;
	if (!ed->actions)
	  {
	     _edje_animators = eina_list_remove(_edje_animators, ed);
	  }
     }
//   _edje_emit(ed, "program,stop", pname);
   _edje_thaw(ed);
   _edje_unref(ed);
   if (free_runp) free(runp);
}

void
_edje_program_run(Edje *ed, Edje_Program *pr, Eina_Bool force, const char *ssig, const char *ssrc)
{
   Eina_List *l;
   Edje_Real_Part *rp;
   Edje_Program_Target *pt;
   Edje_Program *pr2;
   Edje_Program_After *pa;
   /* limit self-feeding loops in programs to 64 levels */
   static int recursions = 0;
   static int recursion_limit = 0;

   if (ed->delete_me) return;
   if ((pr->in.from > 0.0) && (pr->in.range >= 0.0) && (!force))
     {
	Edje_Pending_Program *pp;
	double r = 0.0;

	pp = calloc(1, sizeof(Edje_Pending_Program));
	if (!pp) return;
	if (pr->in.range > 0.0) r = ((double)rand() / RAND_MAX);
	pp->timer = ecore_timer_add(pr->in.from + (pr->in.range * r),
				    _edje_pending_timer_cb, pp);
	if (!pp->timer)
	  {
	     free(pp);
	     return;
	  }
	pp->edje = ed;
	pp->program = pr;
	ed->pending_actions = eina_list_append(ed->pending_actions, pp);
	return;
     }
   if ((recursions >= 64) || (recursion_limit))
     {
	ERR("Programs recursing up to recursion limit of %i. Disabled.",
	    64);
	recursion_limit = 1;
	return;
     }
   recursions++;
   _edje_block(ed);
   _edje_ref(ed);
   _edje_freeze(ed);
   if (pr->action == EDJE_ACTION_TYPE_STATE_SET)
     {
	if ((pr->tween.time > ZERO) && (!ed->no_anim))
	  {
	     Edje_Running_Program *runp;

	     runp = calloc(1, sizeof(Edje_Running_Program));
	     EINA_LIST_FOREACH(pr->targets, l, pt)
	       {
		  if (pt->id >= 0)
		    {
		       rp = ed->table_parts[pt->id % ed->table_parts_size];
		       if (rp)
			 {
			    if (rp->program)
			      _edje_program_end(ed, rp->program);
			    _edje_part_description_apply(ed, rp,
							 rp->param1.description->common.state.name,
							 rp->param1.description->common.state.value,
							 pr->state,
							 pr->value);
			    _edje_part_pos_set(ed, rp, pr->tween.mode, ZERO);
			    rp->program = runp;
			 }
		    }
	       }
//	     _edje_emit(ed, "program,start", pr->name);
	     if (_edje_block_break(ed))
	       {
		  ed->actions = eina_list_append(ed->actions, runp);
		  goto break_prog;
	       }
	     if (!ed->actions)
	       _edje_animators = eina_list_append(_edje_animators, ed);
	     ed->actions = eina_list_append(ed->actions, runp);
	     runp->start_time = ecore_loop_time_get();
	     runp->edje = ed;
	     runp->program = pr;
	     if (!_edje_timer)
	       _edje_timer = ecore_animator_add(_edje_timer_cb, NULL);
	     _edje_anim_count++;
	  }
	else
	  {
	     EINA_LIST_FOREACH(pr->targets, l, pt)
	       {
		  if (pt->id >= 0)
		    {
		       rp = ed->table_parts[pt->id % ed->table_parts_size];
		       if (rp)
			 {
			    if (rp->program)
			      _edje_program_end(ed, rp->program);
			    _edje_part_description_apply(ed, rp,
							 pr->state,
							 pr->value,
							 NULL,
							 0.0);
			    _edje_part_pos_set(ed, rp, pr->tween.mode, ZERO);
			 }
		    }
	       }
//	     _edje_emit(ed, "program,start", pr->name);
	     if (_edje_block_break(ed)) goto break_prog;
//	     _edje_emit(ed, "program,stop", pr->name);
	     if (_edje_block_break(ed)) goto break_prog;

	     EINA_LIST_FOREACH(pr->after, l, pa)
	       {
		  if (pa->id >= 0)
		    {
		       pr2 = ed->table_programs[pa->id % ed->table_programs_size];
		       if (pr2) _edje_program_run(ed, pr2, 0, "", "");
		       if (_edje_block_break(ed)) goto break_prog;
		    }
	       }
	     _edje_recalc(ed);
	  }
     }
   else if (pr->action == EDJE_ACTION_TYPE_ACTION_STOP)
     {
//	_edje_emit(ed, "program,start", pr->name);
        EINA_LIST_FOREACH(pr->targets, l, pt)
	  {
	     Eina_List *ll;
	     Edje_Running_Program *runp;
	     Edje_Pending_Program *pp;

	     EINA_LIST_FOREACH(ed->actions, ll, runp)
	       {
		  if (pt->id == runp->program->id)
		    {
		       _edje_program_end(ed, runp);
//		       goto done;
		    }
	       }
	     EINA_LIST_FOREACH(ed->pending_actions, ll, pp)
	       {
		  if (pt->id == pp->program->id)
		    {
		       ed->pending_actions = eina_list_remove(ed->pending_actions, pp);
		       ecore_timer_del(pp->timer);
		       free(pp);
//		       goto done;
		    }
	       }
//	     done:
//	        continue;
	  }
//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
     }
   else if (pr->action == EDJE_ACTION_TYPE_SIGNAL_EMIT)
     {
//	_edje_emit(ed, "program,start", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
	_edje_emit(ed, pr->state, pr->state2);
	if (_edje_block_break(ed)) goto break_prog;
//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
     }
   else if (pr->action == EDJE_ACTION_TYPE_DRAG_VAL_SET)
     {
//	_edje_emit(ed, "program,start", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
	EINA_LIST_FOREACH(pr->targets, l, pt)
	  {
	     if (pt->id >= 0)
	       {
		  rp = ed->table_parts[pt->id % ed->table_parts_size];
		  if ((rp) && (rp->drag) && (rp->drag->down.count == 0))
		    {
		       rp->drag->val.x = pr->value;
		       rp->drag->val.y = pr->value2;
		       if      (rp->drag->val.x < 0.0) rp->drag->val.x = 0.0;
		       else if (rp->drag->val.x > 1.0) rp->drag->val.x = 1.0;
		       if      (rp->drag->val.y < 0.0) rp->drag->val.y = 0.0;
		       else if (rp->drag->val.y > 1.0) rp->drag->val.y = 1.0;
		       _edje_dragable_pos_set(ed, rp, rp->drag->val.x, rp->drag->val.y);
		       _edje_emit(ed, "drag,set", rp->part->name);
		       if (_edje_block_break(ed)) goto break_prog;
		    }
	       }
	  }
//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
     }
   else if (pr->action == EDJE_ACTION_TYPE_DRAG_VAL_STEP)
     {
//	_edje_emit(ed, "program,start", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
	EINA_LIST_FOREACH(pr->targets, l, pt)
	  {
	     if (pt->id >= 0)
	       {
		  rp = ed->table_parts[pt->id % ed->table_parts_size];
		  if ((rp) && (rp->drag) && (rp->drag->down.count == 0))
		    {
		       rp->drag->val.x += pr->value * rp->drag->step.x * rp->part->dragable.x;
		       rp->drag->val.y += pr->value2 * rp->drag->step.y * rp->part->dragable.y;
		       if      (rp->drag->val.x < 0.0) rp->drag->val.x = 0.0;
		       else if (rp->drag->val.x > 1.0) rp->drag->val.x = 1.0;
		       if      (rp->drag->val.y < 0.0) rp->drag->val.y = 0.0;
		       else if (rp->drag->val.y > 1.0) rp->drag->val.y = 1.0;
		       _edje_dragable_pos_set(ed, rp, rp->drag->val.x, rp->drag->val.y);
		       _edje_emit(ed, "drag,step", rp->part->name);
		       if (_edje_block_break(ed)) goto break_prog;
		    }
	       }
	  }
//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
     }
   else if (pr->action == EDJE_ACTION_TYPE_DRAG_VAL_PAGE)
     {
//	_edje_emit(ed, "program,start", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
	EINA_LIST_FOREACH(pr->targets, l, pt)
	  {
	     if (pt->id >= 0)
	       {
		  rp = ed->table_parts[pt->id % ed->table_parts_size];
		  if ((rp) && (rp->drag) && (rp->drag->down.count == 0))
		    {
		       rp->drag->val.x += pr->value * rp->drag->page.x * rp->part->dragable.x;
		       rp->drag->val.y += pr->value2 * rp->drag->page.y * rp->part->dragable.y;
		       if      (rp->drag->val.x < 0.0) rp->drag->val.x = 0.0;
		       else if (rp->drag->val.x > 1.0) rp->drag->val.x = 1.0;
		       if      (rp->drag->val.y < 0.0) rp->drag->val.y = 0.0;
		       else if (rp->drag->val.y > 1.0) rp->drag->val.y = 1.0;
		       _edje_dragable_pos_set(ed, rp, rp->drag->val.x, rp->drag->val.y);
		       _edje_emit(ed, "drag,page", rp->part->name);
		       if (_edje_block_break(ed)) goto break_prog;
		    }
	       }
	  }
//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
     }
   else if (pr->action == EDJE_ACTION_TYPE_SCRIPT)
     {
	char fname[128];

//	_edje_emit(ed, "program,start", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
	snprintf(fname, sizeof(fname), "_p%i", pr->id);
	_edje_embryo_test_run(ed, fname, ssig, ssrc);
//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
	_edje_recalc_do(ed);
     }
   else if (pr->action == EDJE_ACTION_TYPE_LUA_SCRIPT)
     {
	//printf ("running Lua program script %i\n", pr->id);

//	_edje_emit(ed, "program,start", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
        
#ifdef LUA2
	_edje_lua2_script_init(ed);
#else        
	if (ed->L == NULL) /* private state does not yet exist, create it */
	  {
	     ed->L = _edje_lua_new_thread(ed, _edje_lua_state_get());
	  }
	lua_State *L = ed->L;
	lua_pushnumber(L, pr->id);
	lua_gettable(L, LUA_GLOBALSINDEX);
	if (!lua_isnil(L, -1))
	  {
	     int err_code;

	     lua_pushvalue(L, LUA_GLOBALSINDEX); /* set function environment from collection thread to edje object thread */
	     lua_setfenv(L, -2);
	     _edje_lua_get_reg(L, ed);
	     if (lua_isnil(L, -1)) /* group object does not yet exist, create it */
	       {
		  lua_pop(L, 1);
		  _edje_lua_group_fn_new (ed);
	       }
	     lua_pushstring(L, ssig);
	     lua_pushstring(L, ssrc);

	     if ((err_code = lua_pcall(L, 3, 0, 0)))
	       _edje_lua_error(L, err_code);
	  }
#endif
	//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
	_edje_recalc_do(ed);
     }
   else if (pr->action == EDJE_ACTION_TYPE_FOCUS_SET)
     {
	if (!pr->targets)
	  {
	     ed->focused_part = NULL;
	  }
	else
	  {
	     EINA_LIST_FOREACH(pr->targets, l, pt)
	       {
		  if (pt->id >= 0)
		    {
		       rp = ed->table_parts[pt->id % ed->table_parts_size];
		       if (rp)
			 {
			    if (ed->focused_part != rp)
			      {
				 if (ed->focused_part)
				   _edje_emit(ed, "focus,part,out", 
					      ed->focused_part->part->name);
				 ed->focused_part = rp;
				 _edje_emit(ed, "focus,part,in",
					    ed->focused_part->part->name);
			      }
 			 }
		    }
	       }
	  }
     }
   else if (pr->action == EDJE_ACTION_TYPE_FOCUS_OBJECT)
     {
	if (!pr->targets)
	  {
	     Evas_Object *focused;

	     focused = evas_focus_get(evas_object_evas_get(ed->obj));
	     if (focused)
	       {
		  int i;

		  /* Check if the current swallowed object is one of my child. */
		  for (i = 0; i < ed->table_parts_size; ++i)
		    {
		       rp = ed->table_parts[i];
		       if (rp && rp->swallowed_object == focused)
			 {
			    evas_object_focus_set(focused, EINA_FALSE);
			    break;
			 }
		    }
	       }
	  }
	else
	  {
	     EINA_LIST_FOREACH(pr->targets, l, pt)
	       {
		  if (pt->id >= 0)
		    {
		       rp = ed->table_parts[pt->id % ed->table_parts_size];
		       if (rp && rp->swallowed_object)
			 {
			    evas_object_focus_set(rp->swallowed_object, EINA_TRUE);
			 }
		    }
	       }
	  }
     }
   else if (pr->action == EDJE_ACTION_TYPE_PARAM_COPY)
     {
	Edje_Real_Part *src_part, *dst_part;

//	_edje_emit(ed, "program,start", pr->name);
	if (_edje_block_break(ed)) goto break_prog;

	src_part = ed->table_parts[pr->param.src % ed->table_parts_size];
	dst_part = ed->table_parts[pr->param.dst % ed->table_parts_size];
	_edje_param_copy(src_part, pr->state, dst_part, pr->state2);

	if (_edje_block_break(ed)) goto break_prog;
//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
     }
   else if (pr->action == EDJE_ACTION_TYPE_PARAM_SET)
     {
	Edje_Real_Part *part;

//	_edje_emit(ed, "program,start", pr->name);
	if (_edje_block_break(ed)) goto break_prog;

	part = ed->table_parts[pr->param.dst % ed->table_parts_size];
	_edje_param_set(part, pr->state, pr->state2);

	if (_edje_block_break(ed)) goto break_prog;
//	_edje_emit(ed, "program,stop", pr->name);
	if (_edje_block_break(ed)) goto break_prog;
     }
   else
     {
//	_edje_emit(ed, "program,start", pr->name);
//	_edje_emit(ed, "program,stop", pr->name);
     }
   if (!((pr->action == EDJE_ACTION_TYPE_STATE_SET)
	 /* hmm this fucks somethgin up. must look into it later */
	 /* && (pr->tween.time > ZERO) && (!ed->no_anim))) */
	 ))
     {
        EINA_LIST_FOREACH(pr->after, l, pa)
	  {
	     if (pa->id >= 0)
	       {
		  pr2 = ed->table_programs[pa->id % ed->table_programs_size];
		  if (pr2) _edje_program_run(ed, pr2, 0, "", "");
		  if (_edje_block_break(ed)) goto break_prog;
	       }
	  }
     }
   break_prog:
   _edje_thaw(ed);
   _edje_unref(ed);
   recursions--;
   if (recursions == 0) recursion_limit = 0;
   _edje_unblock(ed);
}

void
_edje_emit(Edje *ed, const char *sig, const char *src)
{
   Edje_Message_Signal emsg;
   Eina_List *l;
   Evas_Object *obj;
   const char *sep;

   if (ed->delete_me) return;

   sep = strchr(sig, EDJE_PART_PATH_SEPARATOR);

   /* If we are not sending the signal to a part of the child, the
    * signal if for ourself
    */
   if (sep)
     {
	char *idx;
        size_t length;
        char *part;
       /* the signal contains a colon, split the signal into "part:signal",
	* and deliver it to "part" (if there is a GROUP or EXTERNAL part named "part")
	*/
       length = strlen(sig) + 1;
       part = alloca(length);
       if (part)
	 {
            char *newsig;
	    int i;

	    memcpy(part, sig, length);

	    /* The part contain a [index], retrieve it */
	    idx = strchr(sig, EDJE_PART_PATH_SEPARATOR_INDEXL);
	    if (idx == NULL || sep < idx) newsig = part + (sep - sig);
	    else newsig = part + (idx - sig);

	    *newsig = '\0';
	    newsig++;

            for (i = 0; i < ed->table_parts_size; i++)
              {
                 Edje_Real_Part *rp = ed->table_parts[i];
                 if ((((rp->part->type == EDJE_PART_TYPE_GROUP
			|| rp->part->type == EDJE_PART_TYPE_EXTERNAL)
		       && (rp->swallowed_object))
		      || rp->part->type == EDJE_PART_TYPE_BOX) &&
                     (rp->part) && (rp->part->name) &&
                     (strcmp(rp->part->name, part) == 0))
                   {
		      if (rp->part->type == EDJE_PART_TYPE_GROUP)
			{
			   Edje *ed2 = _edje_fetch(rp->swallowed_object);
			   if (ed2) _edje_emit(ed2, newsig, src);
			   return; /* stop processing.
				    * XXX maybe let signal be processed anyway?
				    * XXX in this case, just comment this line
				    */
			}
		      else if (rp->part->type == EDJE_PART_TYPE_EXTERNAL)
			{
			   _edje_external_signal_emit(rp->swallowed_object, newsig, src);
			   return;
			}
		      else if (rp->part->type == EDJE_PART_TYPE_BOX
			       || rp->part->type == EDJE_PART_TYPE_TABLE)
			{
			   const char *partid;
			   Evas_Object *child;
			   Edje *ed2 = NULL;

			   idx = strchr(newsig, EDJE_PART_PATH_SEPARATOR_INDEXR);

			   if (!idx) return ;
			   if (idx[1] != ':') return ;
			   if (!rp->object) return;

			   partid = newsig;
			   newsig = idx;

			   *newsig = '\0';
			   newsig += 2; /* we jump over ']' and ':' */

			   child = _edje_children_get(rp, partid);

			   if (child) ed2 = _edje_fetch(child);
			   if (ed2) _edje_emit(ed2, newsig, src);

			   return;
			}
		   }
              }
         }
     }

   emsg.sig = sig;
   emsg.src = src;
   _edje_message_send(ed, EDJE_QUEUE_SCRIPT, EDJE_MESSAGE_SIGNAL, 0, &emsg);
   EINA_LIST_FOREACH(ed->subobjs, l, obj)
     {
        Edje *ed2;
        
        ed2 = _edje_fetch(obj);
        if (!ed2) continue;
        if (ed2->delete_me) continue;
        _edje_message_send(ed2, EDJE_QUEUE_SCRIPT, EDJE_MESSAGE_SIGNAL, 0, &emsg);
     }
}

struct _Edje_Program_Data
{
#ifdef EDJE_PROGRAM_CACHE
  Eina_List     *matches;
  int            matched;
#endif
  Edje          *ed;
  const char    *signal;
  const char    *source;
};

static Eina_Bool _edje_glob_callback(Edje_Program *pr, void *dt)
{
   struct _Edje_Program_Data *data = dt;
   Edje_Real_Part *rp = NULL;
   Eina_Bool exec = EINA_TRUE;

#ifdef EDJE_PROGRAM_CACHE
   data->matched++;
#endif

   if (pr->filter.state)
     {
	rp = _edje_real_part_get(data->ed, pr->filter.part ? pr->filter.part : data->source);
	if (rp)
	  exec = (rp->chosen_description->common.state.name == pr->filter.state);
     }

   if (exec)
     _edje_program_run(data->ed, pr, 0, data->signal, data->source);

   if (_edje_block_break(data->ed))
     {
#ifdef EDJE_PROGRAM_CACHE
        eina_list_free(data->matches);
        data->matches = NULL;
#endif
        return EINA_TRUE;
     }

#ifdef EDJE_PROGRAM_CACHE
   data->matches = eina_list_append(data->matches, pr);
#endif

   return EINA_FALSE;
}


void
_edje_callbacks_patterns_clean(Edje *ed)
{
   _edje_signals_sources_patterns_clean(&ed->patterns.callbacks);

   eina_rbtree_delete(ed->patterns.callbacks.exact_match,
		      EINA_RBTREE_FREE_CB(edje_match_signal_source_free),
		      NULL);
   ed->patterns.callbacks.exact_match = NULL;

   ed->patterns.callbacks.globing = eina_list_free(ed->patterns.callbacks.globing);
}

static void
_edje_callbacks_patterns_init(Edje *ed)
{
   Edje_Signals_Sources_Patterns *ssp = &ed->patterns.callbacks;

   if ((ssp->signals_patterns) || (ssp->sources_patterns) ||
       (ssp->globing) || (ssp->exact_match))
     return;

   ssp->globing = edje_match_callback_hash_build(ed->callbacks,
						 &ssp->exact_match);

   ssp->signals_patterns = edje_match_callback_signal_init(ssp->globing);
   ssp->sources_patterns = edje_match_callback_source_init(ssp->globing);
}

/* FIXME: what if we delete the evas object??? */
void
_edje_emit_handle(Edje *ed, const char *sig, const char *src)
{
   if (ed->delete_me) return;
   if (!sig) sig = "";
   if (!src) src = "";
//   printf("EDJE EMIT: signal: \"%s\" source: \"%s\"\n", sig, src);
   _edje_block(ed);
   _edje_ref(ed);
   _edje_freeze(ed);
#ifdef LUA2
   if (ed->collection && ed->L)
     _edje_lua2_script_func_signal(ed, sig, src);
#endif   
   if (ed->collection)
     {
	Edje_Part_Collection *ec;
#ifdef EDJE_PROGRAM_CACHE
	char *tmps;
	int l1, l2;
#endif
	int done;

	ec = ed->collection;
#ifdef EDJE_PROGRAM_CACHE
	l1 = strlen(sig);
	l2 = strlen(src);
	tmps = alloca(l1 + l2 + 3); /* \0, \337, \0 */
	strcpy(tmps, sig);
	tmps[l1] = '\377';
	strcpy(&(tmps[l1 + 1]), src);
#endif
	done = 0;

#ifdef EDJE_PROGRAM_CACHE
	  {
	     Eina_List *matches;
	     Eina_List *l;
	     Edje_Program *pr;

	     if (eina_hash_find(ec->prog_cache.no_matches, tmps))
	       {
		  done = 1;
	       }
	     else if ((matches = eina_hash_find(ec->prog_cache.matches, tmps)))
	       {
		 EINA_LIST_FOREACH(matches, l, pr)
		    {
		       _edje_program_run(ed, pr, 0, sig, src);
		       if (_edje_block_break(ed))
			 {
			    goto break_prog;
			 }
		    }
		  done = 1;
	       }
	  }
#endif
	if (!done)
	  {
             struct _Edje_Program_Data  data;

             data.ed = ed;
             data.source = src;
             data.signal = sig;
#ifdef EDJE_PROGRAM_CACHE
	     data.matched = 0;
	     data.matches = NULL;
#endif
             if (ed->collection->programs)
               {
		  const Eina_List *match;
		  const Eina_List *l;
		  Edje_Program *pr;

		  if (ed->patterns.programs.globing)
		    if (edje_match_programs_exec(ed->patterns.programs.signals_patterns,
						 ed->patterns.programs.sources_patterns,
						 sig,
						 src,
						 ed->patterns.programs.globing,
						 _edje_glob_callback,
						 &data) == 0)
		      goto break_prog;

		  match = edje_match_signal_source_hash_get(sig, src,
							    ed->patterns.programs.exact_match);
		  EINA_LIST_FOREACH(match, l, pr)
		    _edje_glob_callback(pr, &data);
               }

#ifdef EDJE_PROGRAM_CACHE
	     if (tmps)
	       {
		  if (data.matched == 0)
		    {
		      if (!ec->prog_cache.no_matches)
			ec->prog_cache.no_matches = eina_hash_string_superfast_new(NULL);
		      eina_hash_add(ec->prog_cache.no_matches, tmps, ed);
		    }
		  else
		    {
		      if (!ec->prog_cache.matches)
			ec->prog_cache.matches = eina_hash_string_superfast_new(NULL);
		      eina_hash_add(ec->prog_cache.matches, tmps, data.matches);
		    }
	       }
#endif
	  }
	_edje_emit_cb(ed, sig, src);
	if (_edje_block_break(ed))
	  {
	     goto break_prog;
	  }
     }
   break_prog:
   _edje_thaw(ed);
   _edje_unref(ed);
   _edje_unblock(ed);
}

/* FIXME: what if we delete the evas object??? */
static void
_edje_emit_cb(Edje *ed, const char *sig, const char *src)
{
   Eina_List            *l;

   if (ed->delete_me) return;
   _edje_ref(ed);
   _edje_freeze(ed);
   _edje_block(ed);

   if (ed->just_added_callbacks)
     _edje_callbacks_patterns_clean(ed);

   ed->walking_callbacks = 1;

   if (ed->callbacks)
     {
	Edje_Signal_Callback *escb;
	const Eina_List *match;
	const Eina_List *l;
        int r = 1;

	_edje_callbacks_patterns_init(ed);
	if (ed->patterns.callbacks.globing)
	  r = edje_match_callback_exec(ed->patterns.callbacks.signals_patterns,
				       ed->patterns.callbacks.sources_patterns,
				       sig,
				       src,
				       ed->patterns.callbacks.globing,
				       ed);

        if (!r)
          goto break_prog;

	match = edje_match_signal_source_hash_get(sig, src,
						  ed->patterns.callbacks.exact_match);
	EINA_LIST_FOREACH(match, l, escb)
	  if ((!escb->just_added) && (!escb->delete_me))
	    {
	       escb->func(escb->data, ed->obj, sig, src);
	       if (_edje_block_break(ed))
		 goto break_prog;
	    }
     }
   break_prog:

   ed->walking_callbacks = 0;
   if ((ed->delete_callbacks) || (ed->just_added_callbacks))
     {
	ed->delete_callbacks = 0;
	ed->just_added_callbacks = 0;
	l = ed->callbacks;
	while (l)
	  {
	     Edje_Signal_Callback *escb = l->data;
	     Eina_List *next_l = l->next;

	     if (escb->just_added)
	       escb->just_added = 0;
	     if (escb->delete_me)
	       {
		  ed->callbacks = eina_list_remove_list(ed->callbacks, l);
		  if (escb->signal) eina_stringshare_del(escb->signal);
		  if (escb->source) eina_stringshare_del(escb->source);
		  free(escb);
	       }
	     l = next_l;
	  }

        _edje_callbacks_patterns_clean(ed);
     }
   _edje_unblock(ed);
   _edje_thaw(ed);
   _edje_unref(ed);
}

static const Edje_External_Param_Info *
_edje_external_param_info_get(const Evas_Object *obj, const char *name)
{
   const Edje_External_Type *type;
   const Edje_External_Param_Info *info;

   type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type) return NULL;
   for (info = type->parameters_info; info->name != NULL; info++)
     if (!strcmp(info->name, name)) return info;

   return NULL;
}

static Edje_External_Param *
_edje_param_external_get(const Evas_Object *obj, const char *name, Edje_External_Param *param)
{
   const Edje_External_Param_Info *info;

   info = _edje_external_param_info_get(obj, name);
   if (!info) return NULL;

   memset(param, 0, sizeof(*param));
   param->name = info->name;
   param->type = info->type;
   if (!_edje_external_param_get(obj, param)) return NULL;
   return param;
}

/* simulate external properties for native objects */
static Edje_External_Param *
_edje_param_native_get(Edje_Real_Part *rp, const char *name, Edje_External_Param *param, void **free_ptr)
{
   *free_ptr = NULL;
   if ((rp->part->type == EDJE_PART_TYPE_TEXT) ||
       (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK))
     {
	if (!strcmp(name, "text"))
	  {
	     param->name = name;
	     param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING;

	     _edje_recalc_do(rp->edje);
	     if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
	       param->s = _edje_entry_text_get(rp);
	     else if (rp->part->type == EDJE_PART_TYPE_TEXT)
		    param->s = rp->text.text;
	     else
	       param->s = evas_object_textblock_text_markup_get(rp->object);
	     return param;
	  }
	if (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
	  {
	     if (!strcmp(name, "text_unescaped"))
	       {
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_STRING;

		  _edje_recalc_do(rp->edje);
		  if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
		    {
		       const char *tmp = _edje_entry_text_get(rp);
		       char *unescaped = _edje_text_unescape(tmp);
		       *free_ptr = unescaped;
		       param->s = unescaped;
		    }
		  else if (rp->part->type == EDJE_PART_TYPE_TEXT)
		    param->s = rp->text.text;
		  else
		    {
		       const char *tmp;
		       char *unescaped;

		       tmp = evas_object_textblock_text_markup_get(rp->object);
		       unescaped = _edje_text_unescape(tmp);
		       *free_ptr = unescaped;
		       param->s = unescaped;
		    }

		  return param;
	       }

	     if ((rp->entry_data) &&
		 (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE) &&
		 (!strcmp(name, "select_allow")))
	       {
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_BOOL;
		  param->i = _edje_entry_select_allow_get(rp);
		  return param;
	       }
	  }
     }

   if ((rp->drag) && (rp->drag->down.count == 0))
     {
	if (!strncmp(name, "drag_", sizeof("drag_") - 1))
	  {
	     const char *sub_name = name + sizeof("drag_") - 1;
	     if (!strcmp(sub_name, "value_x"))
	       {
		  double d;

		  _edje_recalc_do(rp->edje);
		  d = TO_DOUBLE(rp->drag->val.x);
		  if (rp->part->dragable.x < 0) d = 1.0 - d;
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
		  param->d = d;
		  return param;
	       }
	     if (!strcmp(sub_name, "value_y"))
	       {
		  double d;

		  _edje_recalc_do(rp->edje);
		  d = TO_DOUBLE(rp->drag->val.y);
		  if (rp->part->dragable.y < 0) d = 1.0 - d;
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
		  param->d = d;
		  return param;
	       }

	     if (!strcmp(sub_name, "size_w"))
	       {
		  _edje_recalc_do(rp->edje);
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
		  param->d = TO_DOUBLE(rp->drag->size.x);
		  return param;
	       }
	     if (!strcmp(sub_name, "size_h"))
	       {
		  _edje_recalc_do(rp->edje);
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
		  param->d = TO_DOUBLE(rp->drag->size.y);
		  return param;
	       }

	     if (!strcmp(sub_name, "step_x"))
	       {
		  _edje_recalc_do(rp->edje);
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
		  param->d = TO_DOUBLE(rp->drag->step.x);
		  return param;
	       }
	     if (!strcmp(sub_name, "step_y"))
	       {
		  _edje_recalc_do(rp->edje);
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
		  param->d = TO_DOUBLE(rp->drag->step.y);
		  return param;
	       }

	     if (!strcmp(sub_name, "page_x"))
	       {
		  _edje_recalc_do(rp->edje);
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
		  param->d = TO_DOUBLE(rp->drag->page.x);
		  return param;
	       }
	     if (!strcmp(sub_name, "page_y"))
	       {
		  _edje_recalc_do(rp->edje);
		  param->name = name;
		  param->type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
		  param->d = TO_DOUBLE(rp->drag->page.y);
		  return param;
	       }

	     return NULL;
	  }
     }

   return NULL;
}

static Eina_Bool
_edje_param_native_set(Edje_Real_Part *rp, const char *name, const Edje_External_Param *param)
{
   if ((rp->part->type == EDJE_PART_TYPE_TEXT) ||
       (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK))
     {
	if (!strcmp(name, "text"))
	  {
	     if (param->type != EDJE_EXTERNAL_PARAM_TYPE_STRING)
	       return EINA_FALSE;

	     _edje_object_part_text_raw_set
	       (rp->edje->obj, rp, rp->part->name, param->s);
	     return EINA_TRUE;
	  }
	if (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
	  {
	     if (!strcmp(name, "text_unescaped"))
	       {
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_STRING)
		    return EINA_FALSE;

		  if (rp->part->type == EDJE_PART_TYPE_TEXT)
		    _edje_object_part_text_raw_set
		      (rp->edje->obj, rp, rp->part->name, param->s);
		  else
		    {
		       char *escaped = _edje_text_escape(param->s);
		      _edje_object_part_text_raw_set
			 (rp->edje->obj, rp, rp->part->name, escaped);
		       free(escaped);
		    }

		  return EINA_TRUE;
	       }

	     if ((rp->entry_data) &&
		 (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE) &&
		 (!strcmp(name, "select_allow")))
	       {
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_BOOL)
		    return EINA_FALSE;
		  _edje_entry_select_allow_set(rp, param->i);
		  return EINA_TRUE;
	       }
	  }
     }

   if ((rp->drag) && (rp->drag->down.count == 0))
     {
	if (!strncmp(name, "drag_", sizeof("drag_") - 1))
	  {
	     const char *sub_name = name + sizeof("drag_") - 1;
	     if (!strcmp(sub_name, "value_x"))
	       {
		  double d;
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
		    return EINA_FALSE;
		  d = param->d;
		  if (rp->part->dragable.confine_id != -1)
		    d = CLAMP(d, 0.0, 1.0);
		  if (rp->part->dragable.x < 0) d = 1.0 - d;
		  if (rp->drag->val.x == FROM_DOUBLE(d)) return EINA_TRUE;
		  rp->drag->val.x = FROM_DOUBLE(d);
#ifdef EDJE_CALC_CACHE
		  rp->invalidate = 1;
#endif
		  _edje_dragable_pos_set
		    (rp->edje, rp, rp->drag->val.x, rp->drag->val.y);
		  _edje_emit(rp->edje, "drag,set", rp->part->name);
		  return EINA_TRUE;
	       }
	     if (!strcmp(sub_name, "value_y"))
	       {
		  double d;
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
		    return EINA_FALSE;
		  d = param->d;
		  if (rp->part->dragable.confine_id != -1)
		    d = CLAMP(d, 0.0, 1.0);
		  if (rp->part->dragable.y < 0) d = 1.0 - d;
		  if (rp->drag->val.y == FROM_DOUBLE(d)) return EINA_TRUE;
		  rp->drag->val.y = FROM_DOUBLE(d);
#ifdef EDJE_CALC_CACHE
		  rp->invalidate = 1;
#endif
		  _edje_dragable_pos_set
		    (rp->edje, rp, rp->drag->val.x, rp->drag->val.y);
		  _edje_emit(rp->edje, "drag,set", rp->part->name);
		  return EINA_TRUE;
	       }

	     if (!strcmp(sub_name, "size_w"))
	       {
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
		    return EINA_FALSE;
		  rp->drag->size.x = FROM_DOUBLE(CLAMP(param->d, 0.0, 1.0));
		  rp->edje->dirty = 1;
#ifdef EDJE_CALC_CACHE
		  rp->invalidate = 1;
#endif
		  _edje_recalc(rp->edje);
		  return EINA_TRUE;
	       }
	     if (!strcmp(sub_name, "size_h"))
	       {
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
		    return EINA_FALSE;
		  rp->drag->size.y = FROM_DOUBLE(CLAMP(param->d, 0.0, 1.0));
		  rp->edje->dirty = 1;
#ifdef EDJE_CALC_CACHE
		  rp->invalidate = 1;
#endif
		  _edje_recalc(rp->edje);
		  return EINA_TRUE;
	       }

	     if (!strcmp(sub_name, "step_x"))
	       {
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
		    return EINA_FALSE;
		  rp->drag->step.x = FROM_DOUBLE(CLAMP(param->d, 0.0, 1.0));
#ifdef EDJE_CALC_CACHE
		  rp->invalidate = 1;
#endif
		  return EINA_TRUE;
	       }
	     if (!strcmp(sub_name, "step_y"))
	       {
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
		    return EINA_FALSE;
		  rp->drag->step.y = FROM_DOUBLE(CLAMP(param->d, 0.0, 1.0));
#ifdef EDJE_CALC_CACHE
		  rp->invalidate = 1;
#endif
		  return EINA_TRUE;
	       }

	     if (!strcmp(sub_name, "page_x"))
	       {
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
		    return EINA_FALSE;
		  rp->drag->page.x = FROM_DOUBLE(CLAMP(param->d, 0.0, 1.0));
#ifdef EDJE_CALC_CACHE
		  rp->invalidate = 1;
#endif
		  return EINA_TRUE;
	       }
	     if (!strcmp(sub_name, "page_y"))
	       {
		  if (param->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
		    return EINA_FALSE;
		  rp->drag->page.y = FROM_DOUBLE(CLAMP(param->d, 0.0, 1.0));
#ifdef EDJE_CALC_CACHE
		  rp->invalidate = 1;
#endif
		  return EINA_TRUE;
	       }

	     return EINA_FALSE;
	  }
     }

   return EINA_FALSE;
}

static const Edje_External_Param_Info *
_edje_native_param_info_get(const Edje_Real_Part *rp, const char *name)
{
   if ((rp->part->type == EDJE_PART_TYPE_TEXT) ||
       (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK))
     {
	if (!strcmp(name, "text"))
	  {
	     static const Edje_External_Param_Info pi =
	       EDJE_EXTERNAL_PARAM_INFO_STRING("text");
	     return &pi;
	  }
	if (rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
	  {
	     if (!strcmp(name, "text_unescaped"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_STRING("text_unescaped");
		  return &pi;
	       }
	     if (!strcmp(name, "select_allow"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_BOOL("text_unescaped");
		  return &pi;
	       }
	  }
     }

   if ((rp->drag) && (rp->drag->down.count == 0))
     {
	if (!strncmp(name, "drag_", sizeof("drag_") - 1))
	  {
	     name += sizeof("drag_") - 1;
	     if (!strcmp(name, "value_x"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("drag_value_x");
		  return &pi;
	       }
	     if (!strcmp(name, "value_y"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("drag_value_y");
		  return &pi;
	       }
	     if (!strcmp(name, "size_w"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("drag_size_w");
		  return &pi;
	       }
	     if (!strcmp(name, "size_h"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("drag_size_h");
		  return &pi;
	       }
	     if (!strcmp(name, "step_x"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("drag_step_x");
		  return &pi;
	       }
	     if (!strcmp(name, "step_y"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("drag_step_y");
		  return &pi;
	       }
	     if (!strcmp(name, "page_x"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("drag_page_x");
		  return &pi;
	       }
	     if (!strcmp(name, "page_y"))
	       {
		  static const Edje_External_Param_Info pi =
		    EDJE_EXTERNAL_PARAM_INFO_DOUBLE("drag_page_y");
		  return &pi;
	       }

	     return NULL;
	  }
     }

   return NULL;
}

static Edje_External_Param *
_edje_param_convert(Edje_External_Param *param, const Edje_External_Param_Info *dst_info)
{
   if (param->type == dst_info->type) return param;

   switch (dst_info->type)
     {
      case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
      case EDJE_EXTERNAL_PARAM_TYPE_INT:
	{
	   int i;
	   switch (param->type)
	     {
	      case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
		 i = (int)param->d;
		 break;
	      case EDJE_EXTERNAL_PARAM_TYPE_STRING:
	      case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
		 i = (param->s) ? atoi(param->s) : 0;
		 break;
	      case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
	      case EDJE_EXTERNAL_PARAM_TYPE_INT:
		 i = param->i;
	      default:
		 return NULL;
	     }
	   if (dst_info->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	     i = !!i;
	   param->type = dst_info->type;
	   param->i = i;
	   return param;
	}

      case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
	{
	   double d;
	   switch (param->type)
	     {
	      case EDJE_EXTERNAL_PARAM_TYPE_INT:
		 d = (double)param->i;
		 break;
	      case EDJE_EXTERNAL_PARAM_TYPE_STRING:
	      case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
		 d = (param->s) ? atof(param->s) : 0.0;
		 break;
	      case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
		 d = (double)param->i;
	      default:
		 return NULL;
	     }
	   param->type = dst_info->type;
	   param->d = d;
	   return param;
	}

      case EDJE_EXTERNAL_PARAM_TYPE_STRING:
	{
	   static char s[64];
	   switch (param->type)
	     {
	      case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
	      case EDJE_EXTERNAL_PARAM_TYPE_INT:
		 if (!snprintf(s, sizeof(s), "%i", param->i)) return NULL;
		 break;
	      case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
		 if (!snprintf(s, sizeof(s), "%f", param->d)) return NULL;
		 break;
	      case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
		 param->type = dst_info->type;
		 return param;
	      default:
		 return NULL;
	     }
	   param->type = dst_info->type;
	   param->s = s;
	   return param;
	}

      case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
	{
	   static char s[64];
	   const char *val;
	   switch (param->type)
	     {
	      case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
	      case EDJE_EXTERNAL_PARAM_TYPE_INT:
		 if (!snprintf(s, sizeof(s), "%i", param->i)) return NULL;
		 val = s;
		 break;
	      case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
		 if (!snprintf(s, sizeof(s), "%f", param->d)) return NULL;
		 val = s;
		 break;
	      case EDJE_EXTERNAL_PARAM_TYPE_STRING:
		 val = param->s;
		 break;
	      default:
		 return NULL;
	     }

	   param->type = dst_info->type;
	   if (param->s != val) param->s = val;
	   return param;
	}

      default: return NULL;
     }
}

static Eina_Bool
_edje_param_validate(const Edje_External_Param *param, const Edje_External_Param_Info *info)
{
   switch (info->type)
     {
      case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
	 return ((param->i == 0) || (param->i == 1));

      case EDJE_EXTERNAL_PARAM_TYPE_INT:
	 if ((info->info.i.min != EDJE_EXTERNAL_INT_UNSET) &&
	     (info->info.i.min > param->i))
	   return EINA_FALSE;

	 if ((info->info.i.max != EDJE_EXTERNAL_INT_UNSET) &&
	     (info->info.i.max < param->i))
	   return EINA_FALSE;

	 return EINA_TRUE;

      case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
	 if ((info->info.d.min != EDJE_EXTERNAL_DOUBLE_UNSET) &&
	     (info->info.d.min > param->d))
	   return EINA_FALSE;

	 if ((info->info.d.max != EDJE_EXTERNAL_DOUBLE_UNSET) &&
	     (info->info.d.max < param->d))
	   return EINA_FALSE;

	 return EINA_TRUE;

      case EDJE_EXTERNAL_PARAM_TYPE_STRING:
	 if (!param->s) return EINA_FALSE;
	 if (info->info.s.accept_fmt)
	   INF("string 'accept_fmt' validation not implemented.");
	 if (info->info.s.deny_fmt)
	   INF("string 'deny_fmt' validation not implemented.");
	 return EINA_TRUE;

      case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
	{
	   const char **itr = info->info.c.choices;
	   if (!itr) return EINA_FALSE;
	   for (; *itr != NULL; itr++)
	     if (!strcmp(*itr, param->s))
	       return EINA_TRUE;
	   return EINA_FALSE;
	}

      default: return EINA_FALSE;
     }
}

static void
_edje_param_copy(Edje_Real_Part *src_part, const char *src_param, Edje_Real_Part *dst_part, const char *dst_param)
{
   Edje_External_Param val;
   const Edje_External_Param_Info *dst_info;
   void *free_ptr = NULL;

   if ((!src_part) || (!src_param) || (!dst_part) || (!dst_param))
     return;

   if (dst_part->part->type == EDJE_PART_TYPE_EXTERNAL)
     dst_info = _edje_external_param_info_get
       (dst_part->swallowed_object, dst_param);
   else
     dst_info = _edje_native_param_info_get(dst_part, dst_param);

   if (!dst_info)
     {
	ERR("cannot copy, invalid destination parameter '%s' of part '%s'",
	    dst_param, dst_part->part->name);
	return;
     }

   if (src_part->part->type == EDJE_PART_TYPE_EXTERNAL)
     {
	if (!_edje_param_external_get
	    (src_part->swallowed_object, src_param, &val))
	  {
	     ERR("cannot get parameter '%s' of part '%s'",
		 src_param, src_part->part->name);
	     return;
	  }
     }
   else
     {
	if (!_edje_param_native_get(src_part, src_param, &val, &free_ptr))
	  {
	     ERR("cannot get parameter '%s' of part '%s'",
		 src_param, src_part->part->name);
	     return;
	  }
     }

   if (!_edje_param_convert(&val, dst_info))
     {
	ERR("cannot convert parameter type %s to requested type %s",
	    edje_external_param_type_str(val.type),
	    edje_external_param_type_str(dst_info->type));
	goto end;
     }

   if (!_edje_param_validate(&val, dst_info))
     {
	ERR("incorrect parameter value failed validation for type %s",
	    edje_external_param_type_str(dst_info->type));
	goto end;
     }

   if (dst_part->part->type == EDJE_PART_TYPE_EXTERNAL)
     {
	val.name = dst_param;
	if (!_edje_external_param_set(dst_part->swallowed_object, &val))
	  {
	     ERR("failed to set parameter '%s' (%s) of part '%s'",
		 dst_param, edje_external_param_type_str(dst_info->type),
		 dst_part->part->name);
	     goto end;
	  }
     }
   else
     {
	if (!_edje_param_native_set(dst_part, dst_param, &val))
	  {
	     ERR("failed to set parameter '%s' (%s) of part '%s'",
		 dst_param, edje_external_param_type_str(dst_info->type),
		 dst_part->part->name);
	     goto end;
	  }
     }

 end:
   free(free_ptr);
}

static void
_edje_param_set(Edje_Real_Part *part, const char *param, const char *value)
{
   Edje_External_Param val;
   const Edje_External_Param_Info *info;

   if ((!part) || (!param) || (!value))
     return;

   if (part->part->type == EDJE_PART_TYPE_EXTERNAL)
     info = _edje_external_param_info_get(part->swallowed_object, param);
   else
     info = _edje_native_param_info_get(part, param);

   if (!info)
     {
	ERR("cannot copy, invalid destination parameter '%s' of part '%s'",
	    param, part->part->name);
	return;
     }

   val.name = "(temp)";
   val.type = EDJE_EXTERNAL_PARAM_TYPE_STRING;
   val.s = value;

   if (!_edje_param_convert(&val, info))
     {
	ERR("cannot convert parameter type STRING to requested type %s",
	    edje_external_param_type_str(info->type));
	return;
     }

   if (!_edje_param_validate(&val, info))
     {
	ERR("incorrect parameter value failed validation for type %s",
	    edje_external_param_type_str(info->type));
	return;
     }

   if (part->part->type == EDJE_PART_TYPE_EXTERNAL)
     {
	val.name = param;
	if (!_edje_external_param_set(part->swallowed_object, &val))
	  {
	     ERR("failed to set parameter '%s' (%s) of part '%s'",
		 param, edje_external_param_type_str(info->type),
		 part->part->name);
	     return;
	  }
     }
   else
     {
	if (!_edje_param_native_set(part, param, &val))
	  {
	     ERR("failed to set parameter '%s' (%s) of part '%s'",
		 param, edje_external_param_type_str(info->type),
		 part->part->name);
	     return;
	  }
     }
}

/**
 *
 * @}
 */
