/*--------------------------------*-C-*---------------------------------*
 * File:	scrollbar.C
 *----------------------------------------------------------------------*
 *
 * Copyright (c) 1997,1998 mj olesen <olesen@me.QueensU.CA>
 * Copyright (c) 1998      Alfredo K. Kojima <kojima@windowmaker.org>
 *				- N*XTstep like scrollbars
 * Copyright (c) 1999-2001 Geoff Wing <gcw@pobox.com>
 * Copyright (c) 2004      Marc Lehmann <pcg@goof.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

#include "../config.h"		/* NECESSARY */
#include "rxvt.h"		/* NECESSARY */

/*----------------------------------------------------------------------*/

/*
 * Map or unmap a scrollbar.  Returns non-zero upon change of state
 */
void
scrollBar_t::map (int map)
{
  if (map)
    {
      state = SB_STATE_IDLE;

#ifdef HAVE_SCROLLBARS
  if (map && !scrollbar_visible ())
    {
      scrollBar.setIdle ();
      if (!scrollBar.win)
        resize_scrollbar ();
      if (scrollBar.win)
        {
          XMapWindow (display->display, scrollBar.win);
          change = 1;
        }
    }
  else if (!map && scrollbar_visible ())
    {
      scrollBar.state = 0;
      XUnmapWindow (display->display, scrollBar.win);
      change = 1;
    }
}

void
scrollBar_t::resize ()
{
  int delayed_init = 0;
  int window_sb_x = 0;

  if (term->option (Opt_scrollBar_right))
    window_sb_x = term->szHint.width - total_width ();

  update_data ();

  if (!win)
    {
      /* create the scrollbar window */
      scrollBar.win = XCreateSimpleWindow (display->display,
                                          TermWin.parent[0],
                                          window_sb_x, 0,
                                          scrollbar_TotalWidth (),
                                          szHint.height,
                                          0,
                                          PixColors[Color_fg],
                                          PixColors[Color_bg]);
#ifdef DEBUG_X
      XStoreName (display->display, scrollBar.win, "scrollbar");
#endif
      XDefineCursor (display->display, scrollBar.win, leftptr_cursor);

      XSelectInput (display->display, scrollBar.win,
                   ExposureMask | ButtonPressMask | ButtonReleaseMask
                   | Button1MotionMask | Button2MotionMask
                   | Button3MotionMask);
      term->scrollbar_ev.start (term->display, win);

      delayed_init = 1;
    }
  else
    XMoveResizeWindow (term->dpy, win,
                       window_sb_x, 0,
                       total_width (), term->szHint.height);

  show (1);

  if (delayed_init)
    XMapWindow (term->dpy, win);
}

/*
 * Update current scrollbar view w.r.t. slider heights, etc.
 */
int
scrollBar_t::show (int refresh)
{
  int ret;

  if (!scrollbar_visible ())
    return 0;

  if (refresh)
    {
      top = (TermWin.nscrolled - TermWin.view_start);
      bot = top + (TermWin.nrow - 1);
      len = max ((TermWin.nscrolled + (TermWin.nrow - 1)), 1);
      adj = (((bot - top) * scrollbar_size ()) % len) > 0 ? 1 : 0;

      scrollBar.top = (scrollBar.beg + (top * scrollbar_size ()) / len);
      scrollbar_len = ((bot - top) * scrollbar_size ()) / len +
                      scrollbar_minheight () + adj;
      scrollBar.bot = (scrollBar.top + scrollbar_len);
      /* no change */
      if (scrollBar.top == last_top
          && scrollBar.bot == last_bot
          && (scrollBar.state == last_state || !scrollbar_isUpDn ()))
        return 0;
    }

  ret = (this->*update) (refresh);

  last_top = top;
  last_bot = bot;
  last_state = state;

  return ret;
}

void
scrollBar_t::setup (rxvt_term *term)
{
  int i;
  const char *scrollalign, *scrollstyle, *thickness;

  this->term = term;
  scrollalign = term->rs[Rs_scrollBar_align];
  scrollstyle = term->rs[Rs_scrollstyle];
  thickness = term->rs[Rs_scrollBar_thickness];

# if defined(RXVT_SCROLLBAR)
  style = SB_STYLE_RXVT;
# elif defined(XTERM_SCROLLBAR)
  style = SB_STYLE_XTERM;
# elif defined(NEXT_SCROLLBAR)
  style = SB_STYLE_NEXT;
# elif defined(PLAIN_SCROLLBAR)
  style = SB_STYLE_PLAIN;
#else
  style = SB_STYLE_RXVT;
# endif

# if (defined(NEXT_SCROLLBAR) || defined(XTERM_SCROLLBAR) || defined(PLAIN_SCROLLBAR))
  if (scrollstyle)
    {
#  ifdef NEXT_SCROLLBAR
      if (STRNCASECMP (scrollstyle, "next", 4) == 0)
        style = R_SB_NEXT;
#  endif
#  ifdef XTERM_SCROLLBAR
      if (STRNCASECMP (scrollstyle, "xterm", 5) == 0)
        style = R_SB_XTERM;
#  endif
#  ifdef PLAIN_SCROLLBAR
      if (STRNCASECMP (scrollstyle, "plain", 5) == 0)
        style = R_SB_PLAIN;
#  endif

    }
# endif
  if (style == SB_STYLE_NEXT)
    width = SB_WIDTH_NEXT;
  else if (style == SB_STYLE_XTERM)
    width = SB_WIDTH_XTERM;
  else if (style == SB_STYLE_PLAIN)
    width = SB_WIDTH_PLAIN;
  else /* if (style == SB_STYLE_RXVT) */
    width = SB_WIDTH_RXVT;

  if (style != R_SB_NEXT)	/* dishonour request - for now */
    if (thickness && (i = atoi (thickness)) >= SB_WIDTH_MINIMUM)
      width = min (i, SB_WIDTH_MAXIMUM);

# if defined(RXVT_SCROLLBAR)
  if (! (Options & Opt_scrollBar_floating) && style == R_SB_RXVT)
    sb_shadow = SHADOW;
# endif

  /* align = SB_ALIGN_CENTRE; */
  if (scrollalign)
    {
      if (STRNCASECMP (scrollalign, "top", 3) == 0)
        scrollbar_align = R_SB_ALIGN_TOP;
      else if (STRNCASECMP (scrollalign, "bottom", 6) == 0)
        scrollbar_align = R_SB_ALIGN_BOTTOM;
    }
#endif
}

/*----------------------- end-of-file (C source) -----------------------*/

