/*---------------------------------------------------------------------------*
 * File:        screen.C
 *---------------------------------------------------------------------------*
 *
 * Copyright (c) 1997-2001 Geoff Wing <gcw@pobox.com>
 * Copyright (c) 2003-2007 Marc Lehmann <schmorp@schmorp.de>
 * Copyright (c) 2015      Emanuele Giaquinta <e.giaquinta@glauco.it>
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
 *--------------------------------------------------------------------------*/

/*
 * This file handles _all_ screen updates and selections
 */

#include "../config.h"          /* NECESSARY */
#include "rxvt.h"               /* NECESSARY */
#include "rxvtperl.h"           /* NECESSARY */

#include <inttypes.h>

static inline void
fill_text (text_t *start, text_t value, int len)
{
  while (len--)
    *start++ = value;
}

/* ------------------------------------------------------------------------- */
#define TABSIZE                 8       /* default tab size */

/* ------------------------------------------------------------------------- *
 *             GENERAL SCREEN AND SELECTION UPDATE ROUTINES                  *
 * ------------------------------------------------------------------------- */
#define ZERO_SCROLLBACK()                                              \
    if (option (Opt_scrollTtyOutput))                                  \
        view_start = 0
#define CLEAR_SELECTION()                                              \
    selection.beg.row = selection.beg.col                              \
        = selection.end.row = selection.end.col = 0
#define CLEAR_ALL_SELECTION()                                          \
    selection.beg.row = selection.beg.col                              \
        = selection.mark.row = selection.mark.col                      \
        = selection.end.row = selection.end.col = 0

#define ROW_AND_COL_IS_AFTER(A, B, C, D)                               \
    (((A) > (C)) || (((A) == (C)) && ((B) > (D))))
#define ROW_AND_COL_IS_BEFORE(A, B, C, D)                              \
    (((A) < (C)) || (((A) == (C)) && ((B) < (D))))
#define ROW_AND_COL_IN_ROW_AFTER(A, B, C, D)                           \
    (((A) == (C)) && ((B) > (D)))
#define ROW_AND_COL_IN_ROW_AT_OR_AFTER(A, B, C, D)                     \
    (((A) == (C)) && ((B) >= (D)))
#define ROW_AND_COL_IN_ROW_BEFORE(A, B, C, D)                          \
    (((A) == (C)) && ((B) < (D)))
#define ROW_AND_COL_IN_ROW_AT_OR_BEFORE(A, B, C, D)                    \
    (((A) == (C)) && ((B) <= (D)))

/* these must be row_col_t */
#define ROWCOL_IS_AFTER(X, Y)                                           \
    ROW_AND_COL_IS_AFTER ((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IS_BEFORE(X, Y)                                          \
    ROW_AND_COL_IS_BEFORE ((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_AFTER(X, Y)                                       \
    ROW_AND_COL_IN_ROW_AFTER ((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_BEFORE(X, Y)                                      \
    ROW_AND_COL_IN_ROW_BEFORE ((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_AT_OR_AFTER(X, Y)                                 \
    ROW_AND_COL_IN_ROW_AT_OR_AFTER ((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_AT_OR_BEFORE(X, Y)                                \
    ROW_AND_COL_IN_ROW_AT_OR_BEFORE ((X).row, (X).col, (Y).row, (Y).col)

/*
 * CLEAR_CHARS: clear <num> chars starting from pixel position <x,y>
 */
#define drawBuffer      TermWin.vt

#define CLEAR_ROWS(row, num)                                            \
    if (TermWin.mapped)                                              \
        XClearArea (display->display, drawBuffer, TermWin.int_bwidth,      \
                    Row2Pixel (row), (unsigned int)TermWin.width,      \
                    (unsigned int)Height2Pixel (num), False)

#define CLEAR_CHARS(x, y, num)                                          \
    if (TermWin.mapped)                                              \
        XClearArea (display->display, drawBuffer, x, y,                       \
                    (unsigned int)Width2Pixel (num),                      \
                    (unsigned int)Height2Pixel (1), False)

#define ERASE_ROWS(row, num)                                            \
    XFillRectangle (display->display, drawBuffer, TermWin.gc,              \
                    TermWin.int_bwidth, Row2Pixel (row),               \
                    (unsigned int)TermWin.width,                      \
                    (unsigned int)Height2Pixel (num))

/* ------------------------------------------------------------------------- *
 *                        SCREEN `COMMON' ROUTINES                           *
 * ------------------------------------------------------------------------- */

/* Fill part/all of a line with blanks. */
void
rxvt_term::scr_blank_line (line_t &l, unsigned int col, unsigned int width, rend_t efs) const noexcept
{
  if (!l.valid ())
    {
      l.alloc ();
      col = 0;
      width = ncol;
    }

  l.touch ();

  efs &= ~RS_baseattrMask; // remove italic etc. fontstyles
  efs = SET_FONT (efs, FONTSET (efs)->find_space_font ());

  text_t *et = l.t + col;
  rend_t *er = l.r + col;

  while (width--)
    {
      *et++ = ' ';
      *er++ = efs;
    }
}

/* ------------------------------------------------------------------------- */
/* Fill a full line with blanks - make sure it is allocated first */
void
rxvt_term::scr_blank_screen_mem (line_t &l, rend_t efs) const noexcept
{
#ifdef DEBUG_STRICT
  assert ((tp[row] && rp[row]) || (tp[row] == NULL && rp[row] == NULL));
#endif
  if (tp[row] == NULL)
    {
      tp[row] = (text_t *)talloc->alloc ();
      rp[row] = (rend_t *)ralloc->alloc ();
    }

  l.l = 0;
  l.f = 0;
}

// nuke a single wide character at the given column
void
rxvt_term::scr_kill_char (line_t &l, int col) const noexcept
{
  // find begin
  while (col > 0 && l.t[col] == NOCHAR)
    col--;

  rend_t rend = l.r[col] & ~RS_baseattrMask;
  rend = SET_FONT (rend, FONTSET (rend)->find_space_font ());

  l.touch ();

  // found start, nuke
  do {
    l.t[col] = ' ';
    l.r[col] = rend;
    col++;
  } while (col < ncol && l.t[col] == NOCHAR);
}

// set the rendition of a single wide character beginning at the given column
void
rxvt_term::scr_set_char_rend (line_t &l, int col, rend_t rend)
{
  do {
    l.r[col] = rend;
    col++;
  } while (col < ncol && l.t[col] == NOCHAR);
}

/* ------------------------------------------------------------------------- *
 *                          SCREEN INITIALISATION                            *
 * ------------------------------------------------------------------------- */

void
rxvt_term::scr_alloc () noexcept
{
  int tsize = sizeof (text_t) * ncol;
  int rsize = sizeof (rend_t) * ncol;

  // we assume that rend_t size is a sufficient alignment
  // factor for text_t and line_t values, and we only
  // need to adjust tsize.
  tsize = (tsize + sizeof (rend_t) - 1);
  tsize -= tsize % sizeof (rend_t);

  int all_rows = total_rows + nrow + nrow;

  chunk_size = (sizeof (line_t) + rsize + tsize) * all_rows;
  chunk = chunk_alloc (chunk_size, 0);

  char *base = (char *)chunk + sizeof (line_t) * all_rows;

  for (int row = 0; row < all_rows; ++row)
    {
      line_t &l = ((line_t *)chunk) [row];

      l.t = (text_t *)base; base += tsize;
      l.r = (rend_t *)base; base += rsize;
      l.l = -1;
      l.f = 0;
    }

  drawn_buf = (line_t *)chunk;
  swap_buf  = drawn_buf + nrow;
  row_buf   = swap_buf  + nrow;
}

void
rxvt_term::copy_line (line_t &dst, line_t &src)
{
  scr_blank_screen_mem (dst, DEFAULT_RSTYLE);
  dst.l = min (src.l, ncol);
  memcpy (dst.t, src.t, sizeof (text_t) * dst.l);
  memcpy (dst.r, src.r, sizeof (rend_t) * dst.l);
  dst.f = src.f;
}

void ecb_cold
rxvt_term::scr_reset ()
{
#if ENABLE_OVERLAY
  scr_overlay_off ();
#endif

  D_SCREEN ((stderr, "rxvt_scr_reset ()"));

  TermWin.view_start = 0;
  num_scr = 0;

  if (ncol == 0)
    ncol = 80;

  if (nrow == 0)
    nrow = 24;

  if (ncol == prev_ncol && nrow == prev_nrow)
    return;

  // we need at least two lines for wrapping to work correctly
  while (nrow + saveLines < 2)
    {
      //TODO//FIXME
      saveLines++;
      prev_nrow--;
      top_row--;
    }

  want_refresh = 1;

  int prev_total_rows = prev_nrow + saveLines;
  total_rows = nrow + saveLines;

  screen.tscroll = 0;
  screen.bscroll = nrow - 1;

  void *prev_chunk = chunk;
  size_t prev_chunk_size = chunk_size;
  line_t *prev_drawn_buf = drawn_buf;
  line_t *prev_swap_buf  = swap_buf;
  line_t *prev_row_buf   = row_buf;

  scr_alloc ();

  if (!prev_row_buf)
    {
      top_row    = 0;
      term_start = 0;

  if (prev_nrow == 0)
    {
      /*
       * first time called so just malloc everything : don't rely on realloc
       * Note: this is still needed so that all the scrollback lines are NULL
       */
      screen.text = (text_t **)rxvt_calloc (total_rows, sizeof (text_t *));
      buf_text = (text_t **)rxvt_calloc (total_rows, sizeof (text_t *));
      drawn_text = (text_t **)rxvt_calloc (nrow, sizeof (text_t *));
      swap.text = (text_t **)rxvt_calloc (nrow, sizeof (text_t *));

      screen.tlen = (int16_t *)rxvt_calloc (total_rows, sizeof (int16_t));
      swap.tlen = (int16_t *)rxvt_calloc (nrow, sizeof (int16_t));

      screen.rend = (rend_t **)rxvt_calloc (total_rows, sizeof (rend_t *));
      buf_rend = (rend_t **)rxvt_calloc (total_rows, sizeof (rend_t *));
      drawn_rend = (rend_t **)rxvt_calloc (nrow, sizeof (rend_t *));
      swap.rend = (rend_t **)rxvt_calloc (nrow, sizeof (rend_t *));

      for (p = 0; p < nrow; p++)
        {
          q = p + TermWin.saveLines;
          scr_blank_screen_mem (screen.text, screen.rend, q, DEFAULT_RSTYLE);
          scr_blank_screen_mem (swap.text, swap.rend, p, DEFAULT_RSTYLE);
          screen.tlen[q] = swap.tlen[p] = 0;
          scr_blank_screen_mem (drawn_text, drawn_rend, p, DEFAULT_RSTYLE);
        }

      MEMSET (charsets, 'B', sizeof (charsets));
      TermWin.nscrolled = 0;       /* no saved lines */
      rstyle = DEFAULT_RSTYLE;
      screen.flags = Screen_DefaultFlags;
      screen.cur.row = screen.cur.col = 0;
      screen.charset = 0;
      current_screen = PRIMARY;
      scr_cursor (SAVE);

#if NSCREENS
      swap.flags = Screen_DefaultFlags;
      swap.cur.row = swap.cur.col = 0;
      swap.charset = 0;
      current_screen = SECONDARY;
      scr_cursor (SAVE);
      current_screen = PRIMARY;
#endif

      selection.text = NULL;
      selection.len = 0;
      selection.op = SELECTION_CLEAR;
      selection.screen = PRIMARY;
      selection.clicks = 0;
      selection.clip_text = NULL;
      selection.clip_len = 0;
    }
  else
    {
      /*
       * add or delete rows as appropriate
       */

      int common_col = min (prev_ncol, ncol);

      // resize swap_buf, blank drawn_buf
      for (int row = min (nrow, prev_nrow); row--; )
        {
          scr_blank_screen_mem (drawn_buf [row], DEFAULT_RSTYLE);
          memcpy (drawn_buf [row].t, prev_drawn_buf [row].t, sizeof (text_t) * common_col);
          memcpy (drawn_buf [row].r, prev_drawn_buf [row].r, sizeof (rend_t) * common_col);

          for (p = nrow; p < prev_nrow; p++)
            {
              q = p + TermWin.saveLines;
              if (screen.text[q])
                {
#ifdef DEBUG_STRICT
                  assert (screen.rend[q]);
#endif
                  talloc->free (screen.text[q]);
                  ralloc->free (screen.rend[q]);
                }
              if (swap.text[p])
                {
#ifdef DEBUG_STRICT
                  assert (swap.rend[p]);
#endif
                  talloc->free (swap.text[p]);
                  ralloc->free (swap.rend[p]);
                }
#ifdef DEBUG_STRICT
              assert (drawn_text[p] && drawn_rend[p]);
#endif
              talloc->free (drawn_text[p]);
              ralloc->free (drawn_rend[p]);
            }

          /* we have fewer rows so fix up cursor position */
          MIN_IT (screen.cur.row, (int32_t)nrow - 1);
          MIN_IT (swap.cur.row, (int32_t)nrow - 1);

          scr_reset_realloc (); /* realloc _last_ */
        }
      else if (nrow > prev_nrow)
        {
          /* add rows */
          scr_reset_realloc (); /* realloc _first_ */

          TermWin.ncol = prev_ncol; // save b/c scr_blank_screen_mem uses this

          k = min (TermWin.nscrolled, nrow - prev_nrow);

          for (p = prev_total_rows; p < total_rows; p++)
            {
              screen.tlen[p] = 0;
              screen.text[p] = NULL;
              screen.rend[p] = NULL;
            }

          for (p = prev_total_rows; p < total_rows - k; p++)
            scr_blank_screen_mem (screen.text, screen.rend, p, setrstyle);

          for (p = prev_nrow; p < nrow; p++)
            {
              swap.tlen[p] = 0;
              swap.text[p] = NULL;
              swap.rend[p] = NULL;
              drawn_text[p] = NULL;
              drawn_rend[p] = NULL;
              scr_blank_screen_mem (swap.text, swap.rend, p, setrstyle);
              scr_blank_screen_mem (drawn_text, drawn_rend, p, setrstyle);
            }

          if (k > 0)
            {
              scr_scroll_text (0, (int)nrow - 1, -k, 1);
              screen.cur.row += k;
              screen.s_cur.row += k;
              TermWin.nscrolled -= k;
            }
#ifdef DEBUG_STRICT
          assert (screen.cur.row < TermWin.nrow);
          assert (swap.cur.row < TermWin.nrow);
#else                           /* drive with your eyes closed */

          MIN_IT (screen.cur.row, nrow - 1);
          MIN_IT (swap.cur.row, nrow - 1);
#endif
          TermWin.ncol =  ncol; // save b/c scr_blank_screen_mem uses this
        }

      int p    = MOD (term_start + prev_nrow, prev_total_rows);  // previous row
      int pend = MOD (term_start + top_row  , prev_total_rows);
      int q    = total_rows; // rewrapped row

#if ENABLE_FRILLS
      if ((rewrap_always || top_row) && !rewrap_never)
        {
          // Re-wrap lines. This is rather ugly, possibly because I am too dumb
          // to come up with a lean and mean algorithm.
          // TODO: maybe optimise when width didn't change

          row_col_t ocur = screen.cur;
          ocur.row = MOD (term_start + ocur.row, prev_total_rows);

          do
            {
              p = MOD (p - 1, prev_total_rows);
              assert (prev_row_buf [MOD (p, prev_total_rows)].t);
              int plines = 1;
              int llen = prev_row_buf [MOD (p, prev_total_rows)].l;

              while (p != pend && prev_row_buf [MOD (p - 1, prev_total_rows)].is_longer ())
                {
                  p = MOD (p - 1, prev_total_rows);

                  MIN_IT (screen.tlen[p], (int16_t)ncol);

                  if (ncol > prev_ncol)
                    scr_blank_line (& (screen.text[p][prev_ncol]),
                                    & (screen.rend[p][prev_ncol]),
                                    ncol - prev_ncol,
                                    setrstyle);
                }
            }

          for (p = 0; p < nrow; p++)
            {
              text_t *t = (text_t *)ta->alloc (); memcpy (t, drawn_text[p], common * sizeof (text_t)); drawn_text[p] = t;
              rend_t *r = (rend_t *)ra->alloc (); memcpy (r, drawn_rend[p], common * sizeof (rend_t)); drawn_rend[p] = r;

              if (ncol > prev_ncol)
                scr_blank_line (& (drawn_text[p][prev_ncol]),
                                & (drawn_rend[p][prev_ncol]),
                                ncol - prev_ncol, setrstyle);

              if (swap.text[p])
                {
                  text_t *t = (text_t *)ta->alloc (); memcpy (t, swap.text[p], common * sizeof (text_t)); swap.text[p] = t;
                  rend_t *r = (rend_t *)ra->alloc (); memcpy (r, swap.rend[p], common * sizeof (rend_t)); swap.rend[p] = r;

                  MIN_IT (swap.tlen[p], (int16_t)ncol);

                  if (ncol > prev_ncol)
                    scr_blank_line (& (swap.text[p][prev_ncol]),
                                    & (swap.rend[p][prev_ncol]),
                                    ncol - prev_ncol, setrstyle);
                }

              int qlines = max (0, (llen - 1) / ncol) + 1;

              // drop partial lines completely
              if (q < qlines)
                break;

              q -= qlines;

              int lofs = 0;
              line_t *qline;

              // re-assemble the full line by destination lines
              for (int qrow = q; qlines--; qrow++)
                {
                  qline = row_buf + qrow;
                  qline->alloc (); // redundant with next line
                  qline->l = ncol;
                  qline->is_longer (1);

                  int qcol = 0;

                  // see below for cursor adjustment rationale
                  if (p == ocur.row)
                    screen.cur.row = q - (total_rows - nrow);

                  // fill a single destination line
                  while (lofs < llen && qcol < ncol)
                    {
                      int prow = lofs / prev_ncol;
                      int pcol = lofs % prev_ncol;

                      prow = MOD (p + prow, prev_total_rows);

                      // we only adjust the cursor _row_ and put it into
                      // the topmost line of "long line" it was in, as
                      // this seems to upset applications/shells/readline
                      // least.
                      if (prow == ocur.row)
                        screen.cur.row = q - (total_rows - nrow);

                      line_t &pline = prev_row_buf [prow];

                      int len = min (min (prev_ncol - pcol, ncol - qcol), llen - lofs);

                      memcpy (qline->t + qcol, pline.t + pcol, len * sizeof (text_t));
                      memcpy (qline->r + qcol, pline.r + pcol, len * sizeof (rend_t));

                      lofs += len;
                      qcol += len;
                    }
                }

              qline->l = llen ? MOD (llen - 1, ncol) + 1 : 0;
              qline->is_longer (0);
              scr_blank_line (*qline, qline->l, ncol - qline->l, DEFAULT_RSTYLE);
            }
          while (p != pend && q > 0);
        }
      else
#endif
        {
          // wing, instead of wrap
          screen.cur.row += nrow - prev_nrow;

          do
            {
              p = MOD (p - 1, prev_total_rows);
              q--;

              copy_line (row_buf [q], prev_row_buf [p]);
            }
          while (p != pend && q > 0);
        }

      if (tabs)
        free (tabs);
    }

  // ensure drawn_buf, swap_buf and terminal rows are all initialized
  for (int row = nrow; row--; )
    {
      if (!ROW       (row).valid ()) scr_blank_screen_mem (ROW       (row), DEFAULT_RSTYLE);
      if (!swap_buf  [row].valid ()) scr_blank_screen_mem (swap_buf  [row], DEFAULT_RSTYLE);
      if (!drawn_buf [row].valid ()) scr_blank_screen_mem (drawn_buf [row], DEFAULT_RSTYLE);
    }

  chunk_free (prev_chunk, prev_chunk_size);

  free (tabs);
  tabs = (char *)rxvt_malloc (ncol);

  for (int col = ncol; col--; )
    tabs [col] = col % TABSIZE == 0;

  CLEAR_ALL_SELECTION ();

  prev_nrow = nrow;
  prev_ncol = ncol;

  tabs = (char *)rxvt_malloc (ncol * sizeof (char));

  for (p = 0; p < ncol; p++)
    tabs[p] = (p % TABSIZE == 0) ? 1 : 0;

  tt_winch ();

  HOOK_INVOKE ((this, HOOK_RESET, DT_END));
}

void
rxvt_term::scr_reset_realloc ()
{
  chunk_free (chunk, chunk_size);
  chunk = 0;
  row_buf = 0;

  nrow = TermWin.nrow;
  total_rows = nrow + TermWin.saveLines;
  /* *INDENT-OFF* */
  screen.text = (text_t **)rxvt_realloc (screen.text, total_rows * sizeof (text_t *));
  buf_text    = (text_t **)rxvt_realloc (buf_text   , total_rows * sizeof (text_t *));
  drawn_text  = (text_t **)rxvt_realloc (drawn_text , nrow       * sizeof (text_t *));
  swap.text   = (text_t **)rxvt_realloc (swap.text  , nrow       * sizeof (text_t *));

  screen.tlen = (int16_t *)rxvt_realloc (screen.tlen, total_rows * sizeof (int16_t));
  swap.tlen   = (int16_t *)rxvt_realloc (swap.tlen  , total_rows * sizeof (int16_t));

  screen.rend = (rend_t **)rxvt_realloc (screen.rend, total_rows * sizeof (rend_t *));
  buf_rend    = (rend_t **)rxvt_realloc (buf_rend   , total_rows * sizeof (rend_t *));
  drawn_rend  = (rend_t **)rxvt_realloc (drawn_rend , nrow       * sizeof (rend_t *));
  swap.rend   = (rend_t **)rxvt_realloc (swap.rend  , nrow       * sizeof (rend_t *));
  /* *INDENT-ON* */
}

/* ------------------------------------------------------------------------- */
/*
 * Free everything.  That way malloc debugging can find leakage.
 */
void
rxvt_term::scr_release ()
{
  uint16_t total_rows;
  int i;

  total_rows = TermWin.nrow + TermWin.saveLines;

#ifdef DEBUG_STRICT
  for (i = 0; i < total_rows; i++)
    {
      if (screen.text[i])
        /* then so is screen.rend[i] */
        assert (screen.rend[i]);
    }
#endif

  delete talloc; talloc = 0;
  delete ralloc; ralloc = 0;

  free (screen.text);
  free (screen.tlen);
  free (screen.rend);
  free (drawn_text);
  free (drawn_rend);
  free (swap.text);
  free (swap.tlen);
  free (swap.rend);
  free (buf_text);
  free (buf_rend);
  free (tabs);
  tabs = 0;
}

/* ------------------------------------------------------------------------- */
/*
 * Hard/Soft reset
 */
void ecb_cold
rxvt_term::scr_poweron ()
{
  D_SCREEN ((stderr, "rxvt_scr_poweron ()"));

  scr_release ();

  prev_nrow = prev_ncol = 0;
  rvideo_mode = false;
  scr_soft_reset ();
  scr_reset ();

  scr_clear ();
  scr_refresh (SLOW_REFRESH);
}

/* ------------------------------------------------------------------------- *
 *                         PROCESS SCREEN COMMANDS                           *
 * ------------------------------------------------------------------------- */
/*
 * Save and Restore cursor
 * XTERM_SEQ: Save cursor   : ESC 7
 * XTERM_SEQ: Restore cursor: ESC 8
 */
void
rxvt_term::scr_cursor (cursor_mode mode) noexcept
{
  screen_t       *s;

  D_SCREEN ((stderr, "rxvt_scr_cursor (%c)", mode));

#if NSCREENS && !defined(NO_SECONDARY_SCREEN_CURSOR)
  if (current_screen == SECONDARY)
    s = & (swap);
  else
#endif
    s = & (screen);
  switch (mode)
    {
      case SAVE:
        s->s_cur.row = screen.cur.row;
        s->s_cur.col = screen.cur.col;
        s->s_rstyle = rstyle;
        s->s_charset = screen.charset;
        s->s_charset_char = charsets[screen.charset];
        break;

      case RESTORE:
        want_refresh = 1;
        screen.cur.row = s->s_cur.row;
        screen.cur.col = s->s_cur.col;
        screen.flags &= ~Screen_WrapNext;
        rstyle = s->s_rstyle;
        screen.charset = s->s_charset;
        charsets[screen.charset] = s->s_charset_char;
        set_font_style ();
        break;
    }

  /* boundary check in case screen size changed between SAVE and RESTORE */
  MIN_IT (s->cur.row, TermWin.nrow - 1);
  MIN_IT (s->cur.col, TermWin.ncol - 1);
#ifdef DEBUG_STRICT
  assert (s->cur.row >= 0);
  assert (s->cur.col >= 0);
#else                           /* drive with your eyes closed */
  MAX_IT (s->cur.row, 0);
  MAX_IT (s->cur.col, 0);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Swap between primary and secondary screens
 * XTERM_SEQ: Primary screen  : ESC [ ? 4 7 h
 * XTERM_SEQ: Secondary screen: ESC [ ? 4 7 l
 */
void
rxvt_term::scr_change_screen (int scrn)
{
  if (scrn == current_screen)
    return;

  want_refresh = 1;
  view_start = 0;

  D_SCREEN ((stderr, "rxvt_scr_change_screen (%d)", scrn));

  current_screen = scrn;

  if (current_screen == scrn)
    return current_screen;

  selection_check (2);        /* check for boundary cross */

  SWAP_IT (current_screen, scrn, int);
#if NSCREENS
  if (option (Opt_secondaryScreen))
    {
      SWAP_IT (screen.text[i + offset], swap.text[i], text_t *);
      SWAP_IT (screen.tlen[i + offset], swap.tlen[i], int16_t);
      SWAP_IT (screen.rend[i + offset], swap.rend[i], rend_t *);
    }
  SWAP_IT (screen.cur.row, swap.cur.row, int16_t);
  SWAP_IT (screen.cur.col, swap.cur.col, int16_t);
# ifdef DEBUG_STRICT
  assert ((screen.cur.row >= 0) && (screen.cur.row < prev_nrow));
  assert ((screen.cur.col >= 0) && (screen.cur.col < prev_ncol));
# else                          /* drive with your eyes closed */
  MAX_IT (screen.cur.row, 0);
  MIN_IT (screen.cur.row, (int32_t)prev_nrow - 1);
  MAX_IT (screen.cur.col, 0);
  MIN_IT (screen.cur.col, (int32_t)prev_ncol - 1);
# endif
  SWAP_IT (screen.charset, swap.charset, int16_t);
  SWAP_IT (screen.flags, swap.flags, int);
  screen.flags |= Screen_VisibleCursor;
  swap.flags |= Screen_VisibleCursor;
#else
# ifdef SCROLL_ON_NO_SECONDARY
  if (current_screen == PRIMARY)
    scr_scroll_text (0, (prev_nrow - 1), prev_nrow, 0);
# endif
#endif
    if (option (Opt_secondaryScroll))
      scr_scroll_text (0, prev_nrow - 1, prev_nrow);
}

// clear WrapNext indicator, solidifying position on next line
void
rxvt_term::scr_do_wrap () noexcept
{
  if (!(screen.flags & Screen_WrapNext))
    return;

  screen.flags &= ~Screen_WrapNext;

  screen.cur.col = 0;

  if (screen.cur.row == screen.bscroll)
    scr_scroll_text (screen.tscroll, screen.bscroll, 1);
  else if (screen.cur.row < nrow - 1)
    screen.cur.row++;
}

/* ------------------------------------------------------------------------- */
/*
 * Change the colour for following text
 */
void
rxvt_term::scr_color (unsigned int color, int fgbg) noexcept
{
  if (!IN_RANGE_INC (color, minCOLOR, maxTermCOLOR24))
    color = fgbg;

  if (fgbg == Color_fg)
    rstyle = SET_FGCOLOR (rstyle, color);
  else
    rstyle = SET_BGCOLOR (rstyle, color);
}

/* ------------------------------------------------------------------------- */
/*
 * Change the rendition style for following text
 */
void
rxvt_term::scr_rendition (int set, int style) noexcept
{
  if (set)
    rstyle |= style;
  else if (style == ~RS_None)
    rstyle = DEFAULT_RSTYLE;
  else
    rstyle &= ~style;
}

/* ------------------------------------------------------------------------- */
/*
 * Scroll text between <row1> and <row2> inclusive, by <count> lines
 * count positive ==> scroll up
 * count negative ==> scroll down
 */
int ecb_hot
rxvt_term::scr_scroll_text (int row1, int row2, int count) noexcept
{
  if (count == 0 || (row1 > row2))
    return 0;

  want_refresh = 1;
  D_SCREEN ((stderr, "rxvt_scroll_text (%d,%d,%d,%d): %s", row1, row2, count, spec, (current_screen == PRIMARY) ? "Primary" : "Secondary"));

  if ((count > 0) && (row1 == 0) && (current_screen == PRIMARY))
    {
      nscrolled = (long)TermWin.nscrolled + (long)count;

      if (nscrolled > (long)TermWin.saveLines)
        TermWin.nscrolled = TermWin.saveLines;
      else
        TermWin.nscrolled = (uint16_t)nscrolled;

      if ((Options & Opt_scrollWithBuffer)
          && TermWin.view_start != 0
          && TermWin.view_start != TermWin.saveLines)
        scr_page (UP, count);
    }
  else if (!spec)
    row1 += TermWin.saveLines;

  row2 += TermWin.saveLines;

  if (selection.op && current_screen == selection.screen)
    {
      i = selection.beg.row + TermWin.saveLines;
      j = selection.end.row + TermWin.saveLines;
      if ((i < row1 && j > row1)
          || (i < row2 && j > row2)
          || (i - count < row1 && i >= row1)
          || (i - count > row2 && i <= row2)
          || (j - count < row1 && j >= row1)
          || (j - count > row2 && j <= row2))
        {
          CLEAR_ALL_SELECTION ();
          selection.op = SELECTION_CLEAR;  /* XXX: too aggressive? */
        }
      else if (j >= row1 && j <= row2)
        {
          /* move selected region too */
          selection.beg.row -= count;
          selection.end.row -= count;
          selection.mark.row -= count;
        }
    }

  selection_check (0);        /* _after_ TermWin.nscrolled update */

  num_scr += count;

  if (count < 0)
    count = -count;

  i = row2 - row1 + 1;
  MIN_IT (count, i);

  if (j > 0)
    {
      min_it (count, total_rows - (nrow - (row2 + 1)));

      top_row = max (top_row - count, -saveLines);

      // sever bottommost line
      {
        line_t &l = ROW(row2);
        l.is_longer (0);
        l.touch ();
      }

      // scroll everything up 'count' lines
      term_start = (term_start + count) % total_rows;

      // now copy lines below the scroll region bottom to the
      // bottom of the screen again, so they look as if they
      // hadn't moved.
      for (int i = nrow; --i > row2; )
        {
          line_t &l1 = ROW(i - count);
          line_t &l2 = ROW(i);

          ::swap (l1, l2);
          l2.touch ();
        }

      // erase newly scrolled-in lines
      for (int i = count; i--; )
        {
          line_t &l = ROW(row2 - i);

          // optimise if already cleared, can be significant on slow machines
          // could be rolled into scr_blank_screen_mem
          if (l.r && l.l < ncol - 1 && !((l.r[l.l + 1] ^ rstyle) & (RS_fgMask | RS_bgMask)))
            {
              scr_blank_line (l, 0, l.l, rstyle);
              l.l = 0;
              l.f = 0;
            }
          else
            scr_blank_screen_mem (l, rstyle);
        }

      // move and/or clear selection, if any
      if (selection.op && current_screen == selection.screen
          && selection.beg.row <= row2)
        {
          selection.beg.row  -= count;
          selection.end.row  -= count;
          selection.mark.row -= count;

          selection_check (0);
        }

      // finally move the view window, if desired
      if (option (Opt_scrollWithBuffer)
          && view_start != 0
          && view_start != -saveLines)
        scr_page (count);

      if (SHOULD_INVOKE (HOOK_SCROLL_BACK))
        HOOK_INVOKE ((this, HOOK_SCROLL_BACK, DT_INT, count, DT_INT, top_row, DT_END));
    }
  else
    {
      if (selection.op && current_screen == selection.screen)
        {
          if ((selection.beg.row < row1 && selection.end.row > row1)
              || (selection.beg.row < row2 && selection.end.row > row2)
              || (selection.beg.row - count < row1 && selection.beg.row >= row1)
              || (selection.beg.row - count > row2 && selection.beg.row <= row2)
              || (selection.end.row - count < row1 && selection.end.row >= row1)
              || (selection.end.row - count > row2 && selection.end.row <= row2))
            {
              CLEAR_ALL_SELECTION ();
              selection.op = SELECTION_CLEAR;
            }
          else if (selection.end.row >= row1 && selection.end.row <= row2)
            {
              /* move selected region too */
              selection.beg.row  -= count;
              selection.end.row  -= count;
              selection.mark.row -= count;

              selection_check (0);
            }
        }

      // use a simple and robust scrolling algorithm, this
      // part of scr_scroll_text is not time-critical.

      // sever line above scroll region
      if (row1)
        {
          line_t &l = ROW(row1 - 1);
          l.is_longer (0);
          l.touch ();
        }

      int rows = row2 - row1 + 1;

      min_it (count, rows);

      line_t *temp_buf = rxvt_temp_buf<line_t> (rows);

      for (int row = 0; row < rows; row++)
        {
          temp_buf [row] = ROW(row1 + (row + count + rows) % rows);

          if (!IN_RANGE_EXC (row + count, 0, rows))
            scr_blank_screen_mem (temp_buf [row], rstyle);
        }

      for (int row = 0; row < rows; row++)
        ROW(row1 + row) = temp_buf [row];

      // sever bottommost line
      {
        line_t &l = ROW(row2);
        l.is_longer (0);
        l.touch ();
      }
    }

  /* C: Resurrect lines */
  for (; i--; j++)
    {
      screen.tlen[j] = 0;
      screen.text[j] = buf_text[i];
      screen.rend[j] = buf_rend[i];

      if (!spec)              /* line length may not equal TermWin.ncol */
        scr_blank_screen_mem (screen.text, screen.rend,
                              (unsigned int)j, rstyle);
    }

  return count;
}

/* ------------------------------------------------------------------------- */
/*
 * Add text given in <str> of length <len> to screen struct
 */
void ecb_hot
rxvt_term::scr_add_lines (const wchar_t *str, int len, int minlines) noexcept
{
  if (len <= 0)               /* sanity */
    return;

  bool checksel;
  unicode_t c;
  int ncol = this->ncol;
  const wchar_t *strend = str + len;

  D_SCREEN ((stderr, "rxvt_scr_add_lines (%d,%d)", nlines, len));
  ZERO_SCROLLBACK ();
  if (nlines > 0)
    {
      minlines += screen.cur.row - screen.bscroll;
      min_it (minlines, screen.cur.row - top_row);

      if (minlines > 0
          && screen.tscroll == 0
          && screen.bscroll == nrow - 1)
        {
          /* _at least_ this many lines need to be scrolled */
          scr_scroll_text (screen.tscroll, screen.bscroll, nlines, 0);
          screen.cur.row -= nlines;
        }
    }
#ifdef DEBUG_STRICT
  assert (screen.cur.col < last_col);
  assert ((screen.cur.row < TermWin.nrow)
         && (screen.cur.row >= - (int32_t)TermWin.nscrolled));
#else                           /* drive with your eyes closed */
  MIN_IT (screen.cur.col, last_col - 1);
  MIN_IT (screen.cur.row, (int32_t)TermWin.nrow - 1);
  MAX_IT (screen.cur.row, - (int32_t)TermWin.nscrolled);
#endif
  row = screen.cur.row + TermWin.saveLines;

  assert (screen.cur.col < ncol);
  assert (screen.cur.row < nrow
          && screen.cur.row >= top_row);
  int row = screen.cur.row;

  checksel = selection.op && current_screen == selection.screen ? 1 : 0;

  line_t *line = &ROW(row);

  while (str < strend)
    {
      c = str[i++];
      switch (c)
        {
          case '\t':
            scr_tab (1);
            continue;
          case '\n':
            if (screen.tlen[row] != -1)      /* XXX: think about this */
              MAX_IT (screen.tlen[row], screen.cur.col);
            screen.flags &= ~Screen_WrapNext;

            if (screen.cur.row == screen.bscroll)
              scr_scroll_text (screen.tscroll, screen.bscroll, 1);
            else if (screen.cur.row < (nrow - 1))
              row = ++screen.cur.row;

            line = &ROW(row);  /* _must_ refresh */
            continue;
          case '\r':
            if (screen.tlen[row] != -1)      /* XXX: think about this */
              MAX_IT (screen.tlen[row], screen.cur.col);
            screen.flags &= ~Screen_WrapNext;
            screen.cur.col = 0;
            continue;
          }
        else if (c == C0_HT)
          {
            scr_tab (1, true);
            continue;
          }

      if (checksel            /* see if we're writing within selection */
          && !ROWCOL_IS_BEFORE (screen.cur, selection.beg)
          && ROWCOL_IS_BEFORE (screen.cur, selection.end))
        {
          checksel = 0;
          /*
           * If we wrote anywhere in the selected area, kill the selection
           * XXX: should we kill the mark too?  Possibly, but maybe that
           *      should be a similar check.
           */
          CLEAR_SELECTION ();
        }

      if (ecb_unlikely (screen.flags & Screen_WrapNext))
        {
          screen.tlen[row] = -1;
          if (screen.cur.row == screen.bscroll)
            scr_scroll_text (screen.tscroll, screen.bscroll, 1, 0);
          else if (screen.cur.row < (TermWin.nrow - 1))
            row = (++screen.cur.row) + TermWin.saveLines;
          stp = screen.text[row];  /* _must_ refresh */
          srp = screen.rend[row];  /* _must_ refresh */
          screen.cur.col = 0;
          screen.flags &= ~Screen_WrapNext;
        }

      // some utf-8 decoders "decode" surrogate characters: let's fix this.
      if (ecb_unlikely (IN_RANGE_INC (c, 0xd800, 0xdfff)))
        c = 0xfffd;

      // rely on wcwidth to tell us the character width, do wcwidth before
      // further replacements, as wcwidth might return -1 for the line
      // drawing characters below as they might be invalid in the current
      // locale.
      int width = WCWIDTH (c);

      if (ecb_unlikely (charsets [screen.charset] == '0')) // DEC SPECIAL
        {
          // vt100 special graphics and line drawing
          // 5f-7e standard vt100
          // 40-5e rxvt extension for extra curses acs chars
          static uint16_t vt100_0[62] = { // 41 .. 7e
                    0x2191, 0x2193, 0x2192, 0x2190, 0x2588, 0x259a, 0x2603, // 41-47 hi mr. snowman!
                 0,      0,      0,      0,      0,      0,      0,      0, // 48-4f
                 0,      0,      0,      0,      0,      0,      0,      0, // 50-57
                 0,      0,      0,      0,      0,      0,      0, 0x0020, // 58-5f
            0x25c6, 0x2592, 0x2409, 0x240c, 0x240d, 0x240a, 0x00b0, 0x00b1, // 60-67
            0x2424, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x23ba, // 68-6f
            0x23bb, 0x2500, 0x23bc, 0x23bd, 0x251c, 0x2524, 0x2534, 0x252c, // 70-77
            0x2502, 0x2264, 0x2265, 0x03c0, 0x2260, 0x00a3, 0x00b7,         // 78-7e
          };

          if (c >= 0x41 && c <= 0x7e && vt100_0[c - 0x41])
            {
              c = vt100_0[c - 0x41];
              width = 1; // vt100 line drawing characters are always single-width
            }
        }

      if (ecb_unlikely (screen.flags & Screen_Insert))
        scr_insdel_chars (width, INSERT);

      if (width != 0)
        {
#if !UNICODE_3
          // trim characters we can't store directly :(
          if (c >= 0x10000)
# if ENABLE_COMBINING
            c = rxvt_composite.compose (c); // map to lower 16 bits
# else
            c = 0xfffd;
# endif
#endif

          rend_t rend = SET_FONT (rstyle, FONTSET (rstyle)->find_font (c));

          // if the character doesn't fit into the remaining columns...
          if (ecb_unlikely (screen.cur.col > ncol - width && ncol >= width))
            {
              if (screen.flags & Screen_Autowrap)
                {
                  // ... artificially enlargen the previous one
                  c = NOCHAR;
                  // and try the same character next loop iteration
                  --str;
                }
              else
                screen.cur.col = ncol - width;
            }

          // nuke the character at this position, if required
          // due to wonderful coincidences everywhere else in this loop
          // we never have to check for overwriting a wide char itself,
          // only its tail.
          if (ecb_unlikely (line->t[screen.cur.col] == NOCHAR))
            scr_kill_char (*line, screen.cur.col);

          line->touch ();

          do
            {
              line->t[screen.cur.col] = c;
              line->r[screen.cur.col] = rend;

              if (ecb_likely (screen.cur.col < ncol - 1))
                screen.cur.col++;
              else
                {
                  line->l = ncol;
                  if (screen.flags & Screen_Autowrap)
                    screen.flags |= Screen_WrapNext;

                  goto end_of_line;
                }

              c = NOCHAR;
            }
          while (ecb_unlikely (--width > 0));

          // pad with spaces when overwriting wide character with smaller one
          for (int c = screen.cur.col; ecb_unlikely (c < ncol && line->t[c] == NOCHAR); c++)
            {
              line->t[c] = ' ';
              line->r[c] = rend;
            }

end_of_line:
          ;
        }
#if ENABLE_COMBINING
      else // width == 0
        {
          if (c != 0xfeff) // ignore BOM
            {
              // handle combining characters
              // we just tag the accent on the previous on-screen character.
              // this is arguably not correct, but also arguably not wrong.
              // we don't handle double-width characters nicely yet.
              line_t *linep;
              text_t *tp;
              rend_t *rp;

              if (screen.cur.col > 0)
                {
                  linep = line;
                  tp = line->t + screen.cur.col - 1;
                  rp = line->r + screen.cur.col - 1;
                }
              else if (screen.cur.row > 0
                       && ROW(screen.cur.row - 1).is_longer ())
                {
                  linep = &ROW(screen.cur.row - 1);
                  tp = linep->t + ncol - 1;
                  rp = linep->r + ncol - 1;
                }
              else
                continue;

              linep->touch ();

              while (*tp == NOCHAR && tp > linep->t)
                tp--, rp--;

              // first try to find a precomposed character
              unicode_t n = rxvt_compose (*tp, c);
              if (n == NOCHAR)
                n = rxvt_composite.compose (*tp, c);

              *tp = n;
              *rp = SET_FONT (*rp, FONTSET (*rp)->find_font (*tp));
            }
        }
#endif
    }

  if (screen.tlen[row] != -1)      /* XXX: think about this */
    MAX_IT (screen.tlen[row], screen.cur.col);

  /*
   * If we wrote anywhere in the selected area, kill the selection
   * XXX: should we kill the mark too?  Possibly, but maybe that
   *      should be a similar check.
   */
  if (clearsel)
    CLEAR_SELECTION ();

#ifdef DEBUG_STRICT
  assert (screen.cur.row >= 0);
#else                           /* drive with your eyes closed */
  MAX_IT (screen.cur.row, 0);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Process Backspace.  Move back the cursor back a position, wrap if have to
 * XTERM_SEQ: CTRL-H
 */
void
rxvt_term::scr_backspace () noexcept
{
  if (screen.cur.col == 0)
    {
      if (screen.cur.row > 0)
        {
#ifdef TERMCAP_HAS_BW
          screen.cur.col = ncol - 1;
          --screen.cur.row;

          want_refresh = 1;
#endif
        }
    }
  else if (screen.flags & Screen_WrapNext)
    screen.flags &= ~Screen_WrapNext;
  else
    scr_gotorc (0, -1, RELATIVE);
}

/* ------------------------------------------------------------------------- */
/*
 * Process Horizontal Tab
 * count: +ve = forward; -ve = backwards
 * XTERM_SEQ: CTRL-I
 */
void
rxvt_term::scr_tab (int count, bool ht) noexcept
{
  int i, x;

  D_SCREEN ((stderr, "rxvt_scr_tab (%d)", count));
  want_refresh = 1;
  i = x = screen.cur.col;

  if (count == 0)
    return;
  else if (count > 0)
    {
      line_t &l = ROW(screen.cur.row);
      rend_t base_rend = l.r[i];
      ht &= l.t[i] == ' ';

      for (; ++i < ncol; )
        if (tabs[i])
          {
            x = i;

            if (!--count)
              break;
          }
        else
          ht &= l.t[i] == ' '
                && RS_SAME (l.r[i], base_rend);

      if (count)
        x = ncol - 1;

      // store horizontal tab commands as characters inside the text
      // buffer so they can be selected and pasted.
      if (ht && option (Opt_pastableTabs))
        {
          base_rend = SET_FONT (base_rend, 0);

          l.touch (x);

          i = screen.cur.col;

          l.t[i] = '\t';
          l.r[i] = base_rend;

          while (++i < x)
            {
              l.t[i] = NOCHAR;
              l.r[i] = base_rend;
            }
        }
    }
  else /* if (count < 0) */
    {
      for (; --i >= 0; )
        if (tabs[i])
          {
            x = i;
            if (!++count)
              break;
          }

      if (count)
        x = 0;
    }

  if (x != screen.cur.col)
    scr_gotorc (0, x, R_RELATIVE);
}

/* ------------------------------------------------------------------------- */
/*
 * Process DEC Back Index
 * XTERM_SEQ: ESC 6
 * Move cursor left in row.  If we're at the left boundary, shift everything
 * in that row right.  Clear left column.
 */
#if !ENABLE_MINIMAL
void ecb_cold
rxvt_term::scr_backindex () noexcept
{
  if (screen.cur.col > 0)
    scr_gotorc (0, -1, R_RELATIVE | C_RELATIVE);
  else
    scr_insdel_chars (1, INSERT);
}
#endif
/* ------------------------------------------------------------------------- */
/*
 * Process DEC Forward Index
 * XTERM_SEQ: ESC 9
 * Move cursor right in row.  If we're at the right boundary, shift everything
 * in that row left.  Clear right column.
 */
#if !ENABLE_MINIMAL
void ecb_cold
rxvt_term::scr_forwardindex () noexcept
{
  if (screen.cur.col < ncol - 1)
    scr_gotorc (0, 1, R_RELATIVE | C_RELATIVE);
  else
    {
      line_t &l = ROW(screen.cur.row);

      l.touch ();
      l.is_longer (0);

      scr_gotorc (0, 0, R_RELATIVE);
      scr_insdel_chars (1, DELETE);
      scr_gotorc (0, ncol - 1, R_RELATIVE);
    }
}
#endif

/* ------------------------------------------------------------------------- */
/*
 * Goto Row/Column
 */
void
rxvt_term::scr_gotorc (int row, int col, int relative) noexcept
{
  want_refresh = 1;
  ZERO_SCROLLBACK ();

  D_SCREEN ((stderr, "rxvt_scr_gotorc (r:%s%d,c:%s%d): from (r:%d,c:%d)", (relative & R_RELATIVE ? "+" : ""), row, (relative & C_RELATIVE ? "+" : ""), col, screen.cur.row, screen.cur.col));

  screen.cur.col = ((relative & C_RELATIVE) ? (screen.cur.col + col)
                    : col);
  MAX_IT (screen.cur.col, 0);
  MIN_IT (screen.cur.col, (int32_t)TermWin.ncol - 1);

  screen.flags &= ~Screen_WrapNext;

  if (relative & R_RELATIVE)
    {
      if (row > 0)
        {
          if (screen.cur.row <= screen.bscroll
              && (screen.cur.row + row) > screen.bscroll)
            screen.cur.row = screen.bscroll;
          else
            screen.cur.row += row;
        }
      else if (row < 0)
        {
          if (screen.cur.row >= screen.tscroll
              && (screen.cur.row + row) < screen.tscroll)
            screen.cur.row = screen.tscroll;
          else
            screen.cur.row += row;
        }
    }
  else
    {
      if (screen.flags & Screen_Relative)
        {
          /* relative origin mode */
          screen.cur.row = row + screen.tscroll;
          MIN_IT (screen.cur.row, screen.bscroll);
        }
      else
        screen.cur.row = row;
    }
  MAX_IT (screen.cur.row, 0);
  MIN_IT (screen.cur.row, (int32_t)TermWin.nrow - 1);
}

/* ------------------------------------------------------------------------- */
/*
 * direction should be UP or DN
 */
void
rxvt_term::scr_index (enum page_dirn direction) noexcept
{
  want_refresh = 1;
  dirn = ((direction == UP) ? 1 : -1);
  D_SCREEN ((stderr, "rxvt_scr_index (%d)", dirn));

  ZERO_SCROLLBACK ();

  screen.flags &= ~Screen_WrapNext;

  if ((screen.cur.row == screen.bscroll && direction == UP)
      || (screen.cur.row == screen.tscroll && direction == DN))
    scr_scroll_text (screen.tscroll, screen.bscroll, dirn, 0);
  else
    screen.cur.row += dirn;
  MAX_IT (screen.cur.row, 0);
  MIN_IT (screen.cur.row, (int32_t)TermWin.nrow - 1);
  selection_check (0);
}

/* ------------------------------------------------------------------------- */
/*
 * Erase part or whole of a line
 * XTERM_SEQ: Clear line to right: ESC [ 0 K
 * XTERM_SEQ: Clear line to left : ESC [ 1 K
 * XTERM_SEQ: Clear whole line   : ESC [ 2 K
 * extension: clear to right unless wrapped: ESC [ 3 K
 */
void
rxvt_term::scr_erase_line (int mode) noexcept
{
  unsigned int col, num;

  want_refresh = 1;
  D_SCREEN ((stderr, "rxvt_scr_erase_line (%d) at screen row: %d", mode, screen.cur.row));
  ZERO_SCROLLBACK ();

  selection_check (1);

  line_t &line = ROW(screen.cur.row);

  line.touch ();
  line.is_longer (0);

  switch (mode)
    {
      case 3:
        if (screen.flags & Screen_WrapNext)
          return;

        /* fall through */

      case 0:                     /* erase to end of line */
        col = screen.cur.col;
        num = TermWin.ncol - col;
        MIN_IT (screen.tlen[row], (int16_t)col);
        if (ROWCOL_IN_ROW_AT_OR_AFTER (selection.beg, screen.cur)
            || ROWCOL_IN_ROW_AT_OR_AFTER (selection.end, screen.cur))
          CLEAR_SELECTION ();
        break;

      case 1:                     /* erase to beginning of line */
        col = 0;
        num = screen.cur.col + 1;
        if (ROWCOL_IN_ROW_AT_OR_BEFORE (selection.beg, screen.cur)
            || ROWCOL_IN_ROW_AT_OR_BEFORE (selection.end, screen.cur))
          CLEAR_SELECTION ();
        break;

      case 2:                     /* erase whole line */
        col = 0;
        num = ncol;
        line.l = 0;
        if (selection.beg.row <= screen.cur.row
            && selection.end.row >= screen.cur.row)
          CLEAR_SELECTION ();
        break;
      default:
        return;
    }

  if (screen.text[row])
    scr_blank_line (& (screen.text[row][col]),
                    & (screen.rend[row][col]), num, rstyle);
  else
    scr_blank_screen_mem (screen.text, screen.rend, row, rstyle);
}

/* ------------------------------------------------------------------------- */
/*
 * Erase part or whole of the screen
 * XTERM_SEQ: Clear screen after cursor : ESC [ 0 J
 * XTERM_SEQ: Clear screen before cursor: ESC [ 1 J
 * XTERM_SEQ: Clear whole screen        : ESC [ 2 J
 */
void
rxvt_term::scr_erase_screen (int mode) noexcept
{
  int num;
  int32_t row;
  rend_t ren;
  XGCValues gcvalue;

  want_refresh = 1;
  D_SCREEN ((stderr, "rxvt_scr_erase_screen (%d) at screen row: %d", mode, screen.cur.row));
  ZERO_SCROLLBACK ();

  switch (mode)
    {
      case 0:                     /* erase to end of screen */
        scr_erase_line (0);
        row = screen.cur.row + 1;    /* possible OOB */
        num = nrow - row;
        break;
      case 1:                     /* erase to beginning of screen */
        scr_erase_line (1);
        row = 0;
        num = screen.cur.row;
        break;
      case 2:                     /* erase whole screen */
        selection_check (3);
        row = 0;
        num = nrow;
        break;
      default:
        return;
    }

  if (selection.op && current_screen == selection.screen
      && ((selection.beg.row >= row && selection.beg.row <= row + num)
          || (selection.end.row >= row
              && selection.end.row <= row + num)))
    CLEAR_SELECTION ();

  if (row >= nrow) /* Out Of Bounds */
    return;
  MIN_IT (num, (TermWin.nrow - row));
  if (rstyle & (RS_RVid | RS_Uline))
    ren = (rend_t) ~RS_None;
  else if (GET_BASEBG (rstyle) == Color_bg)
    {
      ren = DEFAULT_RSTYLE;
      CLEAR_ROWS (row, num);
    }
  else
    {
      ren = (rstyle & (RS_fgMask | RS_bgMask));
      gcvalue.foreground = PixColors[GET_BGCOLOR (rstyle)];
      XChangeGC (display->display, TermWin.gc, GCForeground, &gcvalue);
      ERASE_ROWS (row, num);
      gcvalue.foreground = PixColors[Color_fg];
      XChangeGC (display->display, TermWin.gc, GCForeground, &gcvalue);
    }

  for (; num--; row++)
    {
      scr_blank_screen_mem (screen.text, screen.rend,
                            (unsigned int) (row + row_offset), rstyle);
      screen.tlen[row + row_offset] = 0;
      scr_blank_line (drawn_text[row], drawn_rend[row],
                      (unsigned int)TermWin.ncol, ren);
    }
}

#if !ENABLE_MINIMAL
void
rxvt_term::scr_erase_savelines () noexcept
{
  want_refresh = 1;
  ZERO_SCROLLBACK ();

  top_row = 0;
}
#endif

/* ------------------------------------------------------------------------- */
/*
 * Fill the screen with `E's
 * XTERM_SEQ: Screen Alignment Test: ESC # 8
 */
void ecb_cold
rxvt_term::scr_E () noexcept
{
  rend_t fs;

  want_refresh = 1;
  num_scr_allow = 0;
  ZERO_SCROLLBACK ();
  selection_check (3);

  num_scr_allow = 0;

  row_col_t pos;
  pos.row = pos.col = 0;
  if (ROWCOL_IS_AFTER (selection.end, pos))
    CLEAR_SELECTION ();

  fs = SET_FONT (rstyle, FONTSET (rstyle)->find_font ('E'));
  for (int row = nrow; row--; )
    {
      line_t &line = ROW(row);

      fill_text (line.t, 'E', ncol);
      rend_t *r1 = line.r;

      for (int j = ncol; j--; )
        *r1++ = fs;

      line.is_longer (0);
      line.touch (ncol);
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Insert/Delete <count> lines
 */
void
rxvt_term::scr_insdel_lines (int count, int insdel) noexcept
{
  int end;

  ZERO_SCROLLBACK ();

  selection_check (1);

  if (screen.cur.row > screen.bscroll)
    return;

  end = screen.bscroll - screen.cur.row + 1;
  if (count > end)
    {
      if (insdel == DELETE)
        return;
      else if (insdel == INSERT)
        count = end;
    }

  scr_scroll_text (screen.cur.row, screen.bscroll, insdel * count, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Insert/Delete <count> characters from the current position
 */
void
rxvt_term::scr_insdel_chars (int count, int insdel) noexcept
{
  want_refresh = 1;
  ZERO_SCROLLBACK ();

  if (count <= 0)
    return;

  scr_do_wrap ();

  selection_check (1);
  MIN_IT (count, (TermWin.ncol - screen.cur.col));

  int row = screen.cur.row;

  line_t *line = &ROW(row);

  line->touch ();
  line->is_longer (0);

  // nuke wide spanning the start
  if (line->t[screen.cur.col] == NOCHAR)
    scr_kill_char (*line, screen.cur.col);

  stp = screen.text[row];
  srp = screen.rend[row];
  slp = & (screen.tlen[row]);
  switch (insdel)
    {
      case INSERT:
        line->l = min (line->l + count, ncol);

        if (line->t[screen.cur.col] == NOCHAR)
          scr_kill_char (*line, screen.cur.col);

        for (int col = ncol - 1; (col - count) >= screen.cur.col; col--)
          {
            stp[col] = stp[col - count];
            srp[col] = srp[col - count];
          }
        if (*slp != -1)
          {
            *slp += count;
            MIN_IT (*slp, TermWin.ncol);
          }

        if (selection.op && current_screen == selection.screen
            && ROWCOL_IN_ROW_AT_OR_AFTER (selection.beg, screen.cur))
          {
            if (selection.end.row != screen.cur.row
                || (selection.end.col + count >= TermWin.ncol))
              CLEAR_SELECTION ();
            else
              {              /* shift selection */
                selection.beg.col += count;
                selection.mark.col += count; /* XXX: yes? */
                selection.end.col += count;
              }
          }
        scr_blank_line (& (stp[screen.cur.col]), & (srp[screen.cur.col]),
                        (unsigned int)count, rstyle);
        break;
      case ERASE:
        screen.cur.col += count;     /* don't worry if > TermWin.ncol */
        selection_check (1);
        screen.cur.col -= count;
        scr_blank_line (& (stp[screen.cur.col]), & (srp[screen.cur.col]),
                        (unsigned int)count, rstyle);
        break;
      case DELETE:
        tr = srp[TermWin.ncol - 1]
             & (RS_fgMask | RS_bgMask | RS_baseattrMask);
        for (col = screen.cur.col; (col + count) < TermWin.ncol; col++)
          {
            stp[col] = stp[col + count];
            srp[col] = srp[col + count];
          }
        scr_blank_line (& (stp[TermWin.ncol - count]),
                        & (srp[TermWin.ncol - count]),
                        (unsigned int)count, tr);
        if (*slp == -1) /* break line continuation */
          *slp = TermWin.ncol;
        *slp -= count;
        MAX_IT (*slp, 0);
        if (selection.op && current_screen == selection.screen
            && ROWCOL_IN_ROW_AT_OR_AFTER (selection.beg, screen.cur))
          {
            if (selection.end.row != screen.cur.row
                || (screen.cur.col >= selection.beg.col - count)
                || selection.end.col >= TermWin.ncol)
              CLEAR_SELECTION ();
            else
              {
                /* shift selection */
                selection.beg.col  += count;
                selection.mark.col += count; /* XXX: yes? */
                selection.end.col  += count;
              }
          }

        scr_blank_line (*line, screen.cur.col, count, rstyle);
        break;

      case ERASE:
        screen.cur.col += count;     /* don't worry if > ncol */
        selection_check (1);
        screen.cur.col -= count;

        // nuke wide char after the end
        if (screen.cur.col + count < ncol && line->t[screen.cur.col + count] == NOCHAR)
          scr_kill_char (*line, screen.cur.col + count);

        scr_blank_line (*line, screen.cur.col, count, rstyle);
        break;

      case DELETE:
        line->l = max (line->l - count, 0);

        // nuke wide char spanning the end
        if (screen.cur.col + count < ncol && line->t[screen.cur.col + count] == NOCHAR)
          scr_kill_char (*line, screen.cur.col + count);

        for (int col = screen.cur.col; (col + count) < ncol; col++)
          {
            line->t[col] = line->t[col + count];
            line->r[col] = line->r[col + count];
          }

        scr_blank_line (*line, ncol - count, count, rstyle);

        if (selection.op && current_screen == selection.screen
            && ROWCOL_IN_ROW_AT_OR_AFTER (selection.beg, screen.cur))
          {
            if (selection.end.row != screen.cur.row
                || (screen.cur.col >= selection.beg.col - count)
                || selection.end.col >= ncol)
              CLEAR_SELECTION ();
            else
              {
                /* shift selection */
                selection.beg.col  -= count;
                selection.mark.col -= count; /* XXX: yes? */
                selection.end.col  -= count;
              }
          }

        break;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Set the scrolling region
 * XTERM_SEQ: Set region <top> - <bot> inclusive: ESC [ <top> ; <bot> r
 */
void ecb_cold
rxvt_term::scr_scroll_region (int top, int bot) noexcept
{
  MAX_IT (top, 0);
  MIN_IT (bot, (int)TermWin.nrow - 1);
  if (top > bot)
    return;

  screen.tscroll = top;
  screen.bscroll = bot;
  scr_gotorc (0, 0, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Make the cursor visible/invisible
 * XTERM_SEQ: Make cursor visible  : ESC [ ? 25 h
 * XTERM_SEQ: Make cursor invisible: ESC [ ? 25 l
 */
void ecb_cold
rxvt_term::scr_cursor_visible (int mode) noexcept
{
  want_refresh = 1;

  if (mode)
    screen.flags |= Screen_VisibleCursor;
  else
    screen.flags &= ~Screen_VisibleCursor;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/unset automatic wrapping
 * XTERM_SEQ: Set Wraparound  : ESC [ ? 7 h
 * XTERM_SEQ: Unset Wraparound: ESC [ ? 7 l
 */
void ecb_cold
rxvt_term::scr_autowrap (int mode) noexcept
{
  if (mode)
    screen.flags |= Screen_Autowrap;
  else
    screen.flags &= ~ (Screen_Autowrap | Screen_WrapNext);
}

/* ------------------------------------------------------------------------- */
/*
 * Set/unset margin origin mode
 * Absolute mode: line numbers are counted relative to top margin of screen
 *      and the cursor can be moved outside the scrolling region.
 * Relative mode: line numbers are relative to top margin of scrolling region
 *      and the cursor cannot be moved outside.
 * XTERM_SEQ: Set Absolute: ESC [ ? 6 h
 * XTERM_SEQ: Set Relative: ESC [ ? 6 l
 */
void ecb_cold
rxvt_term::scr_relative_origin (int mode) noexcept
{
  if (mode)
    screen.flags |= Screen_Relative;
  else
    screen.flags &= ~Screen_Relative;

  scr_gotorc (0, 0, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Set insert/replace mode
 * XTERM_SEQ: Set Insert mode : ESC [ ? 4 h
 * XTERM_SEQ: Set Replace mode: ESC [ ? 4 l
 */
void ecb_cold
rxvt_term::scr_insert_mode (int mode) noexcept
{
  if (mode)
    screen.flags |= Screen_Insert;
  else
    screen.flags &= ~Screen_Insert;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/Unset tabs
 * XTERM_SEQ: Set tab at current column  : ESC H
 * XTERM_SEQ: Clear tab at current column: ESC [ 0 g
 * XTERM_SEQ: Clear all tabs             : ESC [ 3 g
 */
void ecb_cold
rxvt_term::scr_set_tab (int mode) noexcept
{
  if (mode < 0)
    MEMSET (tabs, 0, TermWin.ncol * sizeof (char));
  else if (screen.cur.col < TermWin.ncol)
    tabs[screen.cur.col] = (mode ? 1 : 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Set reverse/normal video
 * XTERM_SEQ: Reverse video: ESC [ ? 5 h
 * XTERM_SEQ: Normal video : ESC [ ? 5 l
 */
void
rxvt_term::scr_rvideo_mode (bool on) noexcept
{
  rvideo_mode = on;

#ifndef NO_BELL
  on ^= rvideo_bell;
#endif

  if (rvideo_state != on)
    {
      rvideo = mode;
      SWAP_IT (PixColors[Color_fg], PixColors[Color_bg], rxvt_color);
#if defined(XPM_BACKGROUND)
      if (bgPixmap.pixmap == None)
#endif
#if defined(TRANSPARENT)
        if (! (Options & Opt_transparent) || am_transparent == 0)
#endif
          XSetWindowBackground (display->display, TermWin.vt,
                               PixColors[Color_bg]);

      gcvalue.foreground = PixColors[Color_fg];
      gcvalue.background = PixColors[Color_bg];
      XChangeGC (display->display, TermWin.gc, GCBackground | GCForeground,
                &gcvalue);
      scr_clear ();
      scr_touch (true);
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Report current cursor position
 * XTERM_SEQ: Report position: ESC [ 6 n
 */
void ecb_cold
rxvt_term::scr_report_position () noexcept
{
  tt_printf ("\033[%d;%dR", screen.cur.row + 1, screen.cur.col + 1);
}

/* ------------------------------------------------------------------------- *
 *                                  FONTS                                    *
 * ------------------------------------------------------------------------- */

/*
 * Set font style
 */
void ecb_cold
rxvt_term::set_font_style () noexcept
{
#if 0
  switch (charsets [screen.charset])
    {
      case '0':                   /* DEC Special Character & Line Drawing Set */
        break;
      case 'A':                   /* United Kingdom (UK) */
        break;
      case 'B':                   /* United States (USASCII) */
        break;
      case '<':                   /* Multinational character set */
        break;
      case '5':                   /* Finnish character set */
        break;
      case 'C':                   /* Finnish character set */
        break;
      case 'K':                   /* German character set */
        break;
    }
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Choose a font
 * XTERM_SEQ: Invoke G0 character set: CTRL-O
 * XTERM_SEQ: Invoke G1 character set: CTRL-N
 * XTERM_SEQ: Invoke G2 character set: ESC N
 * XTERM_SEQ: Invoke G3 character set: ESC O
 */
void ecb_cold
rxvt_term::scr_charset_choose (int set) noexcept
{
  screen.charset = set;
  set_font_style ();
}

/* ------------------------------------------------------------------------- */
/*
 * Set a font
 * XTERM_SEQ: Set G0 character set: ESC ( <C>
 * XTERM_SEQ: Set G1 character set: ESC ) <C>
 * XTERM_SEQ: Set G2 character set: ESC * <C>
 * XTERM_SEQ: Set G3 character set: ESC + <C>
 * See set_font_style for possible values for <C>
 */
void
rxvt_term::scr_charset_set (int set, unsigned int ch) noexcept
{
  charsets[set] = (unsigned char)ch;
  set_font_style ();
}


/* ------------------------------------------------------------------------- *
 *                        MAJOR SCREEN MANIPULATION                          *
 * ------------------------------------------------------------------------- */

/*
 * refresh matching text.
 */
bool ecb_cold
rxvt_term::scr_refresh_rend (rend_t mask, rend_t value) noexcept
{
  bool found = false;

  for (int i = 0; i < nrow; i++)
    {
      rend_t *drp = drawn_buf[i].r;

      for (int col = 0; col < ncol; col++, drp++)
        if ((*drp & mask) == value)
          {
            found = true;
            *drp = ~value;
          }
    }

  return found;
}

/*
 * Refresh an area
 */
enum {
  PART_BEG = 0,
  PART_END,
  RC_COUNT
};

void ecb_hot
rxvt_term::scr_expose (int x, int y, int ewidth, int eheight, bool refresh) noexcept
{
  int i;
  row_col_t rc[RC_COUNT];

  if (!drawn_buf)  /* sanity check */
    return;

#ifdef DEBUG_STRICT
  x = max (x, (int)TermWin.int_bwidth);
  x = min (x, (int)TermWin.width);
  y = max (y, (int)TermWin.int_bwidth);
  y = min (y, (int)TermWin.height);
#endif

  /* round down */
  rc[PART_BEG].col = Pixel2Col (x);
  rc[PART_BEG].row = Pixel2Row (y);
  /* round up */
  rc[PART_END].col = Pixel2Width (x + width + TermWin.fwidth - 1);
  rc[PART_END].row = Pixel2Row (y + height + TermWin.fheight - 1);

  /* sanity checks */
  for (i = PART_BEG; i < RC_COUNT; i++)
    {
      MIN_IT (rc[i].col, TermWin.ncol - 1);
      MIN_IT (rc[i].row, TermWin.nrow - 1);
    }

  D_SCREEN ((stderr, "rxvt_scr_expose (x:%d, y:%d, w:%d, h:%d) area (c:%d,r:%d)- (c:%d,r:%d)", x, y, width, height, rc[PART_BEG].col, rc[PART_BEG].row, rc[PART_END].col, rc[PART_END].row));

  for (i = rc[PART_BEG].row; i <= rc[PART_END].row; i++)
    fill_text (& (drawn_text[i][rc[PART_BEG].col]), 0,
               (rc[PART_END].col - rc[PART_BEG].col + 1));

  if (refresh)
    scr_refresh ();
}

/* ------------------------------------------------------------------------- */
/*
 * Refresh the entire screen
 */
void
rxvt_term::scr_touch (bool refresh) noexcept
{
  scr_expose (0, 0, vt_width, vt_height, refresh);
}

/* ------------------------------------------------------------------------- */
/*
 * Move the display so that the line represented by scrollbar value Y is at
 * the top of the screen
 */
void
rxvt_term::scr_move_to (int y, int len) noexcept
{
  long p = 0;
  uint16_t oldviewstart;

  oldviewstart = TermWin.view_start;
  if (y < len)
    {
      p = (TermWin.nrow + TermWin.nscrolled) * (len - y) / len;
      p -= (long) (TermWin.nrow - 1);
      p = max (p, 0);
    }
  TermWin.view_start = (uint16_t)min (p, TermWin.nscrolled);
  D_SCREEN ((stderr, "rxvt_scr_move_to (%d, %d) view_start:%d", y, len, TermWin.view_start));

  return scr_changeview (oldviewstart);
}

/* ------------------------------------------------------------------------- */
/*
 * Page the screen up/down nlines
 * direction should be UP or DN
 */
bool
rxvt_term::scr_page (int nlines) noexcept
{
  int             n;
  uint16_t       oldviewstart;

  D_SCREEN ((stderr, "rxvt_scr_page (%s, %d) view_start:%d", ((direction == UP) ? "UP" : "DN"), nlines, TermWin.view_start));
#ifdef DEBUG_STRICT
  assert ((nlines >= 0) && (nlines <= TermWin.nrow));
#endif
  oldviewstart = TermWin.view_start;
  if (direction == UP)
    {
      n = TermWin.view_start + nlines;
      TermWin.view_start = min (n, TermWin.nscrolled);
    }
  else
    {
      n = TermWin.view_start - nlines;
      TermWin.view_start = max (n, 0);
    }
  return scr_changeview (oldviewstart);
}

bool
rxvt_term::scr_changeview (int new_view_start) noexcept
{
  if (TermWin.view_start != oldviewstart)
    {
      want_refresh = 1;
      num_scr -= (TermWin.view_start - oldviewstart);
    }
  return (int) (TermWin.view_start - oldviewstart);
}

#ifndef NO_BELL
void
rxvt_term::bell_cb (ev::timer &w, int revents)
{
  rvideo_bell = false;
  scr_rvideo_mode (rvideo_mode);
  refresh_check ();
}
#endif

/* ------------------------------------------------------------------------- */
void
rxvt_term::scr_bell () noexcept
{
#ifndef NO_BELL

# ifndef NO_MAPALERT
#  ifdef MAPALERT_OPTION
  if (option (Opt_mapAlert))
#  endif
    XMapWindow (display->display, TermWin.parent[0]);
# endif

# if ENABLE_FRILLS
  if (option (Opt_urgentOnBell))
    set_urgency (1);
# endif

  if (option (Opt_visualBell))
    {
      rvideo_bell = true;
      scr_rvideo_mode (rvideo_mode);
      flush ();

      bell_ev.start (VISUAL_BELL_DURATION);
    }
  else
    XBell (display->display, 0);
#endif
}

/* ------------------------------------------------------------------------- */
void ecb_cold
rxvt_term::scr_printscreen (int fullhist) noexcept
{
#ifdef PRINTPIPE
  int nrows, row_start;
  FILE *fd = popen_printer ();

  if (!fd)
    return;

  if (fullhist)
    {
      nrows = nrow - top_row;
      row_start = top_row;
    }
  else
    {
      nrows = nrow;
      row_start = view_start;
    }

  wctomb (0, 0);

  for (int r1 = row_start; r1 < row_start + nrows; r1++)
    {
      t = screen.text[r1 + row_offset];
      for (i = TermWin.ncol - 1; i >= 0; i--)
        if (!isspace (t[i]))
          break;
      fprintf (fd, "%.*s\n", (i + 1), t);
    }

  pclose_printer (fd);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Refresh the screen
 * drawn_text/drawn_rend contain the screen information before the update.
 * screen.text/screen.rend contain what the screen will change to.
 */

#define FONT_WIDTH(X, Y)                                                \
    (X)->per_char[ (Y) - (X)->min_char_or_byte2].width
#define FONT_RBEAR(X, Y)                                                \
    (X)->per_char[ (Y) - (X)->min_char_or_byte2].rbearing
#define FONT_LBEAR(X, Y)                                                \
    (X)->per_char[ (Y) - (X)->min_char_or_byte2].lbearing
#define IS_FONT_CHAR(X, Y)                                              \
    ((Y) >= (X)->min_char_or_byte2 && (Y) <= (X)->max_char_or_byte2)

void
rxvt_term::scr_refresh (unsigned char refresh_type)
{
  unsigned char   clearfirst, /* first character writes before cell        */
  clearlast,  /* last character writes beyond cell         */
  must_clear, /* use draw_string not draw_image_string     */
  rvid,       /* reverse video this position               */
  showcursor; /* show the cursor                           */
  int16_t         col, row,   /* column/row we're processing               */
  ocrow;      /* old cursor row                            */
  int             cursorwidth;
  int             i,          /* tmp                                       */
  row_offset; /* basic offset in screen structure          */
#ifndef NO_CURSORCOLOR
  rend_t          cc1;        /* store colours at cursor position (s)       */
#endif
  rend_t         *drp, *srp;  /* drawn-rend-pointer, screen-rend-pointer   */
  text_t         *dtp, *stp;  /* drawn-text-pointer, screen-text-pointer   */

  want_refresh = 0;        /* screen is current */

  if (refresh_type == NO_REFRESH || !mapped)
    return;

  /*
   * A: set up vars
   */
  refresh_count = 0;

  unsigned int old_screen_flags = screen.flags;
  bool have_bg = 0;
#ifdef HAVE_IMG
  have_bg = bg_img != 0;
#endif
  ocrow = oldcursor.row; /* is there an old outline cursor on screen? */

  /*
   * B: reverse any characters which are selected
   */
  scr_reverse_selection ();

  HOOK_INVOKE ((this, HOOK_REFRESH_BEGIN, DT_END));
#if ENABLE_OVERLAY
  scr_swap_overlay ();
#endif

  bool showcursor = screen.flags & Screen_VisibleCursor;

  /*
   * C: set the cursor character (s)
   */
  {
#ifdef CURSOR_BLINK
    if (hidden_cursor)
      showcursor = 0;
#endif

    if (showcursor)
      {
        int col = screen.cur.col;

        srp = & (screen.rend[screen.cur.row + TermWin.saveLines]
                [screen.cur.col]);

#ifndef NO_CURSORCOLOR
            cc1 = *srp & (RS_fgMask | RS_bgMask);
            if (ISSET_PIXCOLOR (Color_cursor))
              ccol1 = Color_cursor;
            else
#ifdef CURSOR_COLOR_IS_RENDITION_COLOR
              ccol1 = GET_FGCOLOR (rstyle);
#else
              ccol1 = Color_fg;
#endif
#ifdef CURSOR_COLOR_IS_RENDITION_COLOR
              ccol2 = GET_BGCOLOR (rstyle);
#else
              ccol2 = Color_bg;
#endif
            *srp = SET_FGCOLOR (*srp, ccol1);
            *srp = SET_BGCOLOR (*srp, ccol2);
#endif

#ifndef NO_CURSORCOLOR
        if (ISSET_PIXCOLOR (Color_cursor2))
          ccol2 = Color_cursor2;
        else
#endif
#ifdef CURSOR_COLOR_IS_RENDITION_COLOR
          ccol2 = bgcolor_of (rstyle);
#else
          ccol2 = Color_bg;
#endif

        if (focus && cursor_type == 0)
          {
            rend_t rend = cur_rend;

            rend ^= RS_RVid;
            rend = SET_FGCOLOR (rend, ccol1);
            rend = SET_BGCOLOR (rend, ccol2);

            scr_set_char_rend (ROW(screen.cur.row), cur_col, rend);
          }
      }

    /* make sure no outline cursor is left around */
    if (ocrow != -1 && ocrow < nrow && oldcursor.col < ncol)
      drawn_buf[ocrow].r[oldcursor.col] ^= (RS_RVid | RS_Uline);

    // save the current cursor coordinates if the cursor is visible
    // and either the window is unfocused or the cursor style is
    // underline or vertical bar, so as to clear the outline cursor in
    // the next refresh if the cursor moves or becomes invisible
    if (showcursor && (!focus || cursor_type != 0) && screen.cur.row - view_start < nrow)
      {
        oldcursor.row = screen.cur.row - view_start;
        oldcursor.col = screen.cur.col;
      }
    else
      oldcursor.row = -1;
  }

#ifndef NO_SLOW_LINK_SUPPORT
  /*
   * D: CopyArea pass - very useful for slower links
   *    This has been deliberately kept simple.
   */
  i = num_scr;
  if (refresh_type == FAST_REFRESH && num_scr_allow && i
      && abs (i) < TermWin.nrow && !must_clear)
    {
      int16_t nits;
      int i = num_scr;
      int j;
      int len, wlen;

      j = nrow;
      wlen = len = -1;
      row = i > 0 ? 0 : j - 1;

      for (; j-- >= 0; row += (i > 0 ? 1 : -1))
        {
          if (row + i >= 0 && row + i < nrow && row + i != ocrow)
            {
              line_t s  = ROW(view_start + row);
              line_t d  = drawn_buf[row];
              line_t d2 = drawn_buf[row + i];

              for (nits = 0, col = ncol; col--; )
                if (s.t[col] != d2.t[col] || s.r[col] != d2.r[col])
                  nits--;
                else if (s.t[col] != d.t[col] || s.r[col] != d.r[col])
                  nits++;

              if (nits > 8) /* XXX: arbitrary choice */
                {
                  for (col = ncol; col--; )
                    {
                      *d.t++ = *d2.t++;
                      *d.r++ = *d2.r++;
                    }

                  if (len == -1)
                    len = row;

                  wlen = row;
                  continue;
                }
            }

          if (len >= 0)
            {
              /* also comes here at end if needed because of >= above */
              if (wlen < len)
                SWAP_IT (wlen, len, int);

              D_SCREEN ((stderr, "rxvt_scr_refresh (): XCopyArea: %d -> %d (height: %d)", len + i, len, wlen - len + 1));
              XCopyArea (display->display, TermWin.vt, TermWin.vt,
                         TermWin.gc, 0, Row2Pixel (len + i),
                         (unsigned int)TermWin_TotalWidth (),
                         (unsigned int)Height2Pixel (wlen - len + 1),
                         0, Row2Pixel (len));
              gcv.graphics_exposures = 0; XChangeGC (dpy, gc, GCGraphicsExposures, &gcv);

              len = -1;
            }
        }
    }
#endif

  /*
   * E: main pass across every character
   */
  for (row = 0; row < nrow; row++)
    {
      text_t *stp = ROW(view_start + row).t;
      rend_t *srp = ROW(view_start + row).r;
      text_t *dtp = drawn_buf[row].t;
      rend_t *drp = drawn_buf[row].r;

      /*
       * E2: OK, now the real pass
       */
      int ypixel = (int)Row2Pixel (row);

      for (col = 0; col < ncol; col++)
        {
          /* compare new text with old - if exactly the same then continue */
          rend_t rend = srp[col];     /* screen rendition (target rendtion) */

          if (stp[col] == dtp[col]    /* Must match characters to skip. */
              && (rend == drp[col]    /* Either rendition the same or   */
                  || (stp[col] == ' ' /* space w/ no background change  */
                      && GET_BGATTR (rend) == GET_BGATTR (drp[col]))))
            continue;

          // redraw one or more characters

          // seek to the beginning of wide characters
          while (ecb_unlikely (stp[col] == NOCHAR && col > 0))
            --col;

          rend_t rend = srp[col];     /* screen rendition (target rendition) */
          text_t *text = stp + col;
          int count = 1;

          dtp[col] = stp[col];
          drp[col] = rend;

          int xpixel = Col2Pixel (col);

          int xpixel = Col2Pixel (col);

          // this loop looks very messy, it can probably be optimized
          // and cleaned a bit by you?
          for (i = 0; ++col < TermWin.ncol; )
            {
              if (stp[col] == NOCHAR)
                {
                  dtp[col] = stp[col];
                  drp[col] = srp[col];

                  count++;
                  i++;

                  continue;
                }

              if (!RS_SAME (rend, srp[col]))
                break;

              count++;

              if (stp[col] != dtp[col]
                  || !RS_SAME (srp[col], drp[col]))
                {
                  if (have_bg && (i++ > count / 2))
                    break;

                  dtp[col] = stp[col];
                  drp[col] = rend;
                  i = 0;
                }
              else if (have_bg || (stp[col] != ' ' && ++i >= 16))
                break;
            }

          col--;      /* went one too far.  move back */
          count -= i; /* dump any matching trailing chars */

          // sometimes we optimize away the trailing NOCHAR's, add them back
          while (ecb_unlikely (i && text[count] == NOCHAR))
            count++, i--;

          /*
           * Determine the attributes for the string
           */
          int fore = fgcolor_of (rend); // desired foreground
          int back = bgcolor_of (rend); // desired background

          rend = GET_ATTR (rend);

          rvid = !! (rend & RS_RVid);
          if (rvid)
            {
              SWAP_IT (fore, back, int);

#ifndef NO_BOLD_UNDERLINE_REVERSE
              if (rend & RS_Bold && fore == Color_fg)
                {
                  if (ISSET_PIXCOLOR (Color_BD))
                    fore = Color_BD;
# if !ENABLE_STYLES
                  else
                    invert = !invert;
# endif
                }

              if (rend & RS_Italic && fore == Color_fg)
                {
                  if (ISSET_PIXCOLOR (Color_IT))
                    fore = Color_IT;
# if !ENABLE_STYLES
                  else
                    invert = !invert;
# endif
                }

              if (rend & RS_Uline && fore == Color_fg && ISSET_PIXCOLOR (Color_UL))
                fore = Color_UL;
#endif

#ifdef OPTION_HC
              if (rend & RS_Sel)
                {
                  /* invert the column if no highlightColor is set or it is the
                   * current cursor column */
                  if (!(showcursor && row == screen.cur.row && text - stp == screen.cur.col)
                      && ISSET_PIXCOLOR (Color_HC))
                    {
                      if (ISSET_PIXCOLOR (Color_HTC))
                        fore = Color_HTC;
                      // if invert is 0 reverse video is set so we use bg color as fg color
                      else if (!invert)
                        fore = back;

                      back = Color_HC;
                      invert = 0;
                    }
                }
#endif

              if (invert)
                {
                    ::swap (fore, back);

#ifndef NO_BOLD_UNDERLINE_REVERSE
                  if (fore == back)
                    {
                      fore = Color_bg;
                      back = Color_fg;
                    }
#endif
                }

#ifdef TEXT_BLINK
              if (rend & RS_Blink && (back == Color_bg || fore == Color_bg))
                {
                  if (!text_blink_ev.is_active ())
                    {
                      text_blink_ev.again ();
                      hidden_text = 0;
                    }
                  else if (hidden_text)
                    fore = back;
                }
#endif

#if ENABLE_STYLES
              // "careful" (too wide) character handling

              // include previous careful character(s) if possible, looks nicer (best effort...)
              while (text > stp
                  && srp[text - stp - 1] & RS_Careful
                  && RS_SAME (rend, srp[text - stp - 1]))
                text--, count++, xpixel -= fwidth;

              // force redraw after "careful" characters to avoid pixel droppings
              for (int i = 0; srp[col + i] & RS_Careful && col + i < ncol - 1; i++)
                drp[col + i + 1] = srp[col + i + 1] ^ RS_redraw;

              // force redraw before "careful" characters to avoid pixel droppings
              for (int i = 0; srp[text - stp - i] & RS_Careful && text - i > stp; i++)
                drp[text - stp - i - 1] = srp[text - stp - i - 1] ^ RS_redraw;
#endif
            }

          /*
           * Actually do the drawing of the string here
           */
          rxvt_font *font = (*fontset[GET_STYLE (rend)])[GET_FONT (rend)];

          if (ecb_likely (have_bg && back == Color_bg))
            {
              // this is very ugly, maybe push it into ->draw?

              for (i = 0; i < count; i++) /* don't draw empty strings */
                if (text[i] != ' ')
                  {
                    font->draw (*drawable, xpixel, ypixel, text, count, fore, Color_transparent);
                    goto did_clear;
                  }

              CLEAR_CHARS (xpixel, ypixel, count);
              did_clear: ;
            }
          else
            font->draw (*drawable, xpixel, ypixel, text, count, fore, back);

          if ((rend & RS_Uline) && (font->descent > 1))
            XDrawLine (display->display, drawBuffer, TermWin.gc,
                      xpixel, ypixel + font->ascent + 1,
                      xpixel + Width2Pixel (count) - 1, ypixel + font->ascent + 1);
        }                     /* for (col....) */
    }                         /* for (row....) */

  /*
   * G: cleanup cursor and display outline cursor if necessary
   */
  if (showcursor)
    {
      if (focus)
        {
          srp = & (screen.rend[screen.cur.row + TermWin.saveLines]
                  [screen.cur.col]);
          *srp ^= RS_RVid;
#ifndef NO_CURSORCOLOR
          *srp = (*srp & ~ (RS_fgMask | RS_bgMask)) | cc1;
#endif

        }
      else if (oldcursor.row >= 0)
        {
#ifndef NO_CURSORCOLOR
          if (ISSET_PIXCOLOR (Color_cursor))
            XSetForeground (display->display, TermWin.gc, PixColors[Color_cursor]);
#endif
          XDrawRectangle (display->display, drawBuffer, TermWin.gc,
                         Col2Pixel (oldcursor.col),
                         Row2Pixel (oldcursor.row),
                         (unsigned int) (Width2Pixel (cursorwidth) - 1),
                         (unsigned int) (Height2Pixel (1) - TermWin.lineSpace - 1));
        }
    }

  /*
   * H: cleanup selection
   */
#if ENABLE_OVERLAY
  scr_swap_overlay ();
#endif
  HOOK_INVOKE ((this, HOOK_REFRESH_END, DT_END));

  scr_reverse_selection ();

  /*
   * I: other general cleanup
   */
  if (clearfirst && TermWin.int_bwidth)
    /*
     * clear the whole screen height, note that width == 0 is treated
     * specially by XClearArea
     */
    XClearArea (display->display, TermWin.vt, 0, 0,
               (unsigned int)TermWin.int_bwidth,
               (unsigned int)TermWin_TotalHeight (), False);
  if (clearlast && TermWin.int_bwidth)
    /*
     * clear the whole screen height, note that width == 0 is treated
     * specially by XClearArea
     */
    XClearArea (display->display, TermWin.vt,
               TermWin.width + TermWin.int_bwidth, 0,
               (unsigned int)TermWin.int_bwidth,
               (unsigned int)TermWin_TotalHeight (), False);
  if (refresh_type & SMOOTH_REFRESH)
    XSync (display->display, False);

  num_scr = 0;
  num_scr_allow = 1;
}

void ecb_cold
rxvt_term::scr_remap_chars (line_t &l) noexcept
{
  if (!l.valid ())
    return;

  l.touch (); // maybe a bit of an overkill, but it's not performance-relevant

  for (int i = ncol; i--; )
    l.r[i] = SET_FONT (l.r[i], FONTSET (l.r[i])->find_font (l.t[i]));
}

void ecb_cold
rxvt_term::scr_remap_chars () noexcept
{
  for (int i = total_rows; i--; )
    scr_remap_chars (row_buf [i]);

  for (int i = nrow; i--; )
    {
      scr_remap_chars (drawn_buf [i]);
      scr_remap_chars (swap_buf [i]);
    }
}

void ecb_cold
rxvt_term::scr_recolor (bool refresh) noexcept
{
  bool transparent = false;

#ifdef HAVE_IMG
  if (bg_img != 0)
    {
# if ENABLE_TRANSPARENCY
      if (bg_flags & BG_IS_TRANSPARENT)
        {
          XSetWindowBackgroundPixmap (dpy, parent, bg_img->pm);
          XSetWindowBackgroundPixmap (dpy, vt, ParentRelative);

          transparent = true;
        }
      else
# endif
        {
          XSetWindowBackground (dpy, parent, pix_colors[Color_border]);
          XSetWindowBackgroundPixmap (dpy, vt, bg_img->pm);
        }
    }
  else
#endif
    {
      XSetWindowBackground (dpy, parent, pix_colors[Color_border]);
      XSetWindowBackground (dpy, vt, pix_colors[Color_bg]);
    }

  XClearWindow (dpy, parent);

  if (scrollBar.state && scrollBar.win)
    {
      if (transparent)
        XSetWindowBackgroundPixmap (dpy, scrollBar.win, ParentRelative);
      else
        XSetWindowBackground (dpy, scrollBar.win, pix_colors[scrollBar.color ()]);
      scrollBar.state = SB_STATE_IDLE;
      scrollBar.show (0);
    }

  if (refresh)
    {
      scr_clear ();
      scr_touch (true);
    }
  want_refresh = 1;
}

/* ------------------------------------------------------------------------- */
void
rxvt_term::scr_clear ()
{
  if (!mapped)
    return;

  num_scr_allow = 0;
  want_refresh = 1;

  if (really)
    XClearWindow (dpy, vt);
}

void
rxvt_term::scr_xor_rect (int beg_row, int beg_col, int end_row, int end_col, rend_t rstyle1, rend_t rstyle2) noexcept
{
  int view_end = view_start + nrow;
  int row, col;

  for (row = max (beg_row, view_start); row <= min (end_row, view_end); row++)
    {
      text_t *stp = ROW(row).t;
      rend_t *srp = ROW(row).r;

      if (! (Options & Opt_transparent_all))
        i = 0;
      else
        i = (int) (sizeof (TermWin.parent) / sizeof (Window));

      while (i--)
        if (TermWin.parent[i] != None)
          XClearWindow (display->display, TermWin.parent[i]);
    }
}

void
rxvt_term::scr_xor_span (int beg_row, int beg_col, int end_row, int end_col, rend_t rstyle) noexcept
{
  int view_end = view_start + nrow;
  int row, col;

  if (beg_row >= view_start)
    {
      col = beg_col;
      row = beg_row;
    }
  else
    {
      col = 0;
      row = view_start;
    }

  for (; row < min (end_row, view_end); row++, col = 0)
    for (rend_t *srp = ROW(row).r; col < ncol; col++)
      srp[col] ^= rstyle;

  if (row == end_row)
    for (rend_t *srp = ROW(row).r; col < end_col; col++)
      srp[col] ^= rstyle;
}

/* ------------------------------------------------------------------------- */
void ecb_hot
rxvt_term::scr_reverse_selection () noexcept
{
  if (selection.op
      && current_screen == selection.screen
      && selection.end.row >= view_start)
    {
#if !ENABLE_MINIMAL
      if (selection.rect)
        scr_xor_rect (selection.beg.row, selection.beg.col,
                      selection.end.row, selection.end.col,
                      RS_Sel | RS_RVid, RS_Sel | RS_RVid | RS_Uline);
      else
#endif
        scr_xor_span (selection.beg.row, selection.beg.col,
                      selection.end.row, selection.end.col,
                      RS_Sel | RS_RVid);
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Dump the whole scrollback and screen to the passed filedescriptor.  The
 * invoking routine must close the fd.
 */
#if 0
void
rxvt_term::scr_dump (int fd) noexcept
{
  int             row, wrote;
  unsigned int    width, towrite;
  char            r1[] = "\n";

  for (row = TermWin.saveLines - TermWin.nscrolled;
       row < TermWin.saveLines + TermWin.nrow - 1; row++)
    {
      width = screen.tlen[row] >= 0 ? screen.tlen[row]
              : TermWin.ncol;
      for (towrite = width; towrite; towrite -= wrote)
        {
          wrote = write (fd, & (screen.text[row][width - towrite]),
                        towrite);
          if (wrote < 0)
            return;         /* XXX: death, no report */
        }
      if (screen.tlen[row] >= 0)
        if (write (fd, r1, 1) <= 0)
          return; /* XXX: death, no report */
    }
}
#endif

/* ------------------------------------------------------------------------- *
 *                           CHARACTER SELECTION                             *
 * ------------------------------------------------------------------------- */
void
rxvt_term::selection_check (int check_more) noexcept
{
  if (!selection.op)
    return;

  pos.row = pos.col = 0;
  if ((selection.beg.row < - (int32_t)TermWin.nscrolled)
      || (selection.beg.row >= TermWin.nrow)
      || (selection.mark.row < - (int32_t)TermWin.nscrolled)
      || (selection.mark.row >= TermWin.nrow)
      || (selection.end.row < - (int32_t)TermWin.nscrolled)
      || (selection.end.row >= TermWin.nrow)
      || (check_more == 1
          && current_screen == selection.screen
          && !ROWCOL_IS_BEFORE (screen.cur, selection.beg)
          && ROWCOL_IS_BEFORE (screen.cur, selection.end))
      || (check_more == 2
          && ROWCOL_IS_BEFORE (selection.beg, pos)
          && ROWCOL_IS_AFTER (selection.end, pos))
      || (check_more == 3
          && ROWCOL_IS_AFTER (selection.end, pos))
      || (check_more == 4     /* screen width change */
          && (selection.beg.row != selection.end.row
              || selection.end.col > TermWin.ncol)))
    CLEAR_SELECTION ();
}

/* ------------------------------------------------------------------------- */
/*
 * Paste a selection direct to the command fd
 */
void
rxvt_term::tt_paste (char *data, unsigned int len) noexcept
{
  /* convert normal newline chars into common keyboard Return key sequence */
  for (unsigned int i = 0; i < len; i++)
    if (data[i] == C0_LF)
      data[i] = C0_CR;

  #if ENABLE_FRILLS
  if ((priv_modes & PrivMode_BracketPaste) && !option (Opt_disablePasteBrackets))
    tt_printf ("\x1b[200~");
  #endif

  tt_write (data, len);

  free (ds);
}

/* ------------------------------------------------------------------------- */
/*
 * Respond to a notification that a primary selection has been sent
 * EXT: SelectionNotify
 */
int
rxvt_term::selection_paste (Window win, Atom prop, bool delete_prop)
{
  long nread = 0;
  unsigned long bytes_after;
  XTextProperty ct;

  D_SELECT ((stderr, "rxvt_selection_paste (%08lx, %lu, %d), wait=%2x", win, (unsigned long)prop, (int)delete_prop, selection_wait));

  if (prop == None)         /* check for failed XConvertSelection */
    {
      if ((selection_type & Sel_CompoundText))
        {
          int selnum = selection_type & Sel_whereMask;

          selection_type = 0;
          if (selnum != Sel_direct)
            selection_request_other (XA_STRING, selnum);
        }

      if ((selection_type & Sel_UTF8String))
        {
          int selnum = selection_type & Sel_whereMask;

          selection_type = Sel_CompoundText;
          if (selnum != Sel_direct)
            selection_request_other (xa[XA_COMPOUND_TEXT], selnum);
          else
            selection_type = 0;
        }

      return 0;
    }

  for (;;)
    {
      if (XGetWindowProperty (display->display, win, prop, (long) (nread / 4),
                             (long) (PROP_SIZE / 4), delete_prop,
                             AnyPropertyType, &ct.encoding, &ct.format,
                             &ct.nitems, &bytes_after,
                             &ct.value) != Success)
        break;

      if (ct.encoding == 0)
        {
          D_SELECT ((stderr, "rxvt_selection_paste: property didn't exist!"));
          break;
        }

      if (ct.value == NULL)
        {
          D_SELECT ((stderr, "rxvt_selection_paste: property shooting blanks!"));
          continue;
        }

      if (ct.nitems == 0)
        {
          D_SELECT ((stderr, "rxvt_selection_paste: property empty - also INCR end"));
          if (selection_wait == Sel_normal && nread == 0)
            {
              /*
               * pass through again trying CUT_BUFFER0 if we've come from
               * XConvertSelection () but nothing was presented
               */
              D_SELECT ((stderr, "rxvt_selection_request: pasting CUT_BUFFER0"));
              selection_paste (display->root, XA_CUT_BUFFER0, False);
            }

          nread = -1;         /* discount any previous stuff */
          break;
        }

      nread += ct.nitems;

      char **cl;
      int cr;
      if (XmbTextPropertyToTextList (display->display, &ct, &cl,
                                     &cr) >= 0 && cl)
        {
          for (int i = 0; i < cr; i++)
            paste ((unsigned char *)cl[i], STRLEN (cl[i]));

          XFreeStringList (cl);
        }
      else
        paste (ct.value, ct.nitems);

      if (bytes_after == 0)
        break;

      XFree (ct.value);
    }

  if (ct.value)
    XFree (ct.value);

  if (selection_wait == Sel_normal)
    selection_wait = Sel_none;

  D_SELECT ((stderr, "rxvt_selection_paste: bytes written: %ld", nread));
  return (int)nread;
}

void
rxvt_term::paste (char *data, unsigned int len) noexcept
{
  if (HOOK_INVOKE ((this, HOOK_TT_PASTE, DT_STR_LEN, data, len, DT_END)))
    return;
  D_SELECT ((stderr, "rxvt_selection_property (%08lx, %lu)", win, (unsigned long)prop));
  if (selection_wait == Sel_normal)
    {
      int             a, afmt;
      Atom            atype;
      unsigned long   bytes_after, nitems;
      unsigned char  *s = NULL;

      a = XGetWindowProperty (display->display, win, prop, 0L, 1L, False,
                             xa[XA_INCR], &atype, &afmt, &nitems,
                             &bytes_after, &s);
      if (s)
        XFree (s);
      if (a != Success)
        return;
#ifndef __CYGWIN32__
      if (atype == xa[XA_INCR])
        {  /* start an INCR transfer */
          D_SELECT ((stderr, "rxvt_selection_property: INCR: starting transfer"));
          XDeleteProperty (display->display, win, prop);
          XFlush (display->display);
          reget_time = 1;
          selection_wait = Sel_incr;
        }
#endif

    }
  else if (selection_wait == Sel_incr)
    {
      reget_time = 1;
      if (selection_paste (win, prop, True) == -1)
        {
          D_SELECT ((stderr, "rxvt_selection_property: INCR: clean end"));
          selection_wait = Sel_none;
          incr_ev.stop ();
        }
    }
  if (reget_time) /* received more data so reget time */
    incr_ev.start (NOW + 10);
}

/* ------------------------------------------------------------------------- */
/*
 * Request the current selection: 
 * Order: > internal selection if available
 *        > PRIMARY, SECONDARY, CLIPBOARD if ownership is claimed (+)
 *        > CUT_BUFFER0
 * (+) if ownership is claimed but property is empty, rxvt_selection_paste ()
 *     will auto fallback to CUT_BUFFER0
 * EXT: button 2 release
 */
void
rxvt_term::selection_request (Time tm, int selnum) noexcept
{
  D_SELECT ((stderr, "rxvt_selection_request (%lu, %d, %d)", tm, x, y));
  if (x < 0 || x >= TermWin.width || y < 0 || y >= TermWin.height)
    return;                 /* outside window */

  if (selection.text != NULL)
    {    /* internal selection */
      D_SELECT ((stderr, "rxvt_selection_request: pasting internal"));
      paste (selection.text, selection.len);
      return;
    }
  else
    {
      selection_req = new rxvt_selection (display, selnum, tm, vt, xa[XA_VT_SELECTION], this);
      selection_req->run ();
    }
  selection_wait = Sel_none;       /* don't loop in rxvt_selection_paste () */
  D_SELECT ((stderr, "rxvt_selection_request: pasting CUT_BUFFER0"));
  selection_paste (display->root, XA_CUT_BUFFER0, False);
}

int
rxvt_term::selection_request_other (Atom target, int selnum)
{
  Atom            sel;
#ifdef DEBUG_SELECT
  char           *debug_xa_names[] = { "PRIMARY", "SECONDARY", "CLIPBOARD" };
#endif

  selection_type |= selnum;
  if (selnum == Sel_Primary)
    sel = XA_PRIMARY;
  else if (selnum == Sel_Secondary)
    sel = XA_SECONDARY;
  else
    sel = xa[XA_CLIPBOARD];
  if (XGetSelectionOwner (display->display, sel) != None)
    {
      D_SELECT ((stderr, "rxvt_selection_request_other: pasting %s", debug_xa_names[selnum]));
      XConvertSelection (display->display, sel, target, xa[XA_VT_SELECTION],
                        TermWin.vt, selection_request_time);
      return 1;
    }
  return 0;
}

/* ------------------------------------------------------------------------- */
/*
 * Clear all selected text
 * EXT: SelectionClear
 */
void
rxvt_term::selection_clear (bool clipboard) noexcept
{
  D_SELECT ((stderr, "rxvt_selection_clear ()"));

  want_refresh = 1;
  if (selection.text)
    free (selection.text);
  selection.text = NULL;
  selection.len = 0;
  CLEAR_SELECTION ();
}

/* ------------------------------------------------------------------------- */
/*
 * Copy a selection into the cut buffer
 * EXT: button 1 or 3 release
 */
void
rxvt_term::selection_make (Time tm)
{
  int size;
  wchar_t *new_selection_text;
  text_t *t;

  D_SELECT ((stderr, "rxvt_selection_make (): selection.op=%d, selection.clicks=%d", selection.op, selection.clicks));
  switch (selection.op)
    {
      case SELECTION_CONT:
        break;
      case SELECTION_INIT:
        CLEAR_SELECTION ();
        /* FALLTHROUGH */
      case SELECTION_BEGIN:
        selection.op = SELECTION_DONE;
        /* FALLTHROUGH */
      default:
        return;
    }

  selection.op = SELECTION_DONE;

  if (selection.clicks == 4)
    return;                 /* nothing selected, go away */

  i = (selection.end.row - selection.beg.row + 1) * (TermWin.ncol + 1) + 1;
  str = (char *)rxvt_malloc (i * MB_CUR_MAX + 1);

  size = (selection.end.row - selection.beg.row + 1) * (ncol + 1);
  new_selection_text = (wchar_t *)rxvt_malloc ((size + 4) * sizeof (wchar_t));

  col = selection.beg.col;
  MAX_IT (col, 0);
  row = selection.beg.row + TermWin.saveLines;
  end_row = selection.end.row + TermWin.saveLines;

  int col = selection.beg.col;
  int row = selection.beg.row;

  int end_col;

  for (; row <= selection.end.row; row++, col = 0)
    {
      t = & (screen.text[row][col]);

      col = max (col, 0);

      if (row == selection.end.row)
        min_it (end_col, selection.end.col);

      t = ROW(row).t + col;

      for (; col < end_col; col++)
        {
          if (*t == NOCHAR)
            t++;
#if ENABLE_COMBINING
          else if (IS_COMPOSE (*t))
            {
              int len = rxvt_composite.expand (*t);

              extra -= (len - 1);

              if (extra < 0)
                {
                  extra += size;
                  size += size;
                  new_selection_text = (wchar_t *)rxvt_realloc (new_selection_text, (size + 4) * sizeof (wchar_t));
                }

              ofs += rxvt_composite.expand (*t++, new_selection_text + ofs);
            }
#endif
          else
            new_selection_text[ofs++] = *t++;
        }

#if !ENABLE_MINIMAL
      if (selection.rect)
        {
          while (ofs
                 && new_selection_text[ofs - 1] != C0_LF
                 && unicode::is_space (new_selection_text[ofs - 1]))
            --ofs;

          new_selection_text[ofs++] = C0_LF;
        }
      else
#endif
        if (!ROW(row).is_longer ()
            && (row != selection.end.row || end_col != selection.end.col)
            && (row != selection.beg.row || selection.beg.col < ncol))
          new_selection_text[ofs++] = C0_LF;
    }

  new_selection_text[ofs] = 0;

  if (ofs == 0)
    {
      free (new_selection_text);
      return;
    }

  free (selection.text);

  // we usually allocate much more than necessary, so realloc it smaller again
  selection.len = ofs;
  selection.text = (wchar_t *)rxvt_realloc (new_selection_text, (ofs + 1) * sizeof (wchar_t));

  if (HOOK_INVOKE ((this, HOOK_SEL_GRAB, DT_LONG, (long)tm, DT_END)))
    return;

  selection_grab (tm);
}

bool
rxvt_term::selection_grab (Time tm, bool clipboard) noexcept
{
  Atom sel;

  if (!clipboard)
    {
      selection_time = tm;
      sel = XA_PRIMARY;
    }
  else
    rxvt_print_error ("can't get primary selection");

  XSetSelectionOwner (dpy, sel, vt, tm);
  if (XGetSelectionOwner (dpy, sel) == vt)
    {
      display->set_selection_owner (this, clipboard);
      return true;
    }
  else
    {
      selection_clear (clipboard);
      return false;
    }

    if (XmbTextListToTextProperty (display->display, &cl, 1, XStringStyle, &ct) >= 0)
      {
        XChangeProperty (display->display, display->root, XA_CUT_BUFFER0, XA_STRING, 8,
                        PropModeReplace, ct.value, ct.nitems);
        XFree (ct.value);
      }
    else
      XChangeProperty (display->display, display->root, XA_CUT_BUFFER0, XA_STRING, 8,
                      PropModeReplace, selection.text, (int)selection.len);
  }

  selection_time = tm;
  D_SELECT ((stderr, "rxvt_selection_make (): selection.len=%d", selection.len));
}

/* ------------------------------------------------------------------------- */
/*
 * Mark or select text based upon number of clicks: 1, 2, or 3
 * EXT: button 1 press
 */
void ecb_cold
rxvt_term::selection_click (int clicks, int x, int y) noexcept
{
  D_SELECT ((stderr, "rxvt_selection_click (%d, %d, %d)", clicks, x, y));

  clicks = ((clicks - 1) % 3) + 1;
  selection.clicks = clicks;       /* save clicks so extend will work */

  selection_start_colrow (Pixel2Col (x), Pixel2Row (y));

  if (clicks == 2 || clicks == 3)
    selection_extend_colrow (selection.mark.col,
                             selection.mark.row - view_start,
                             0, /* button 3     */
                             1, /* button press */
                             0);        /* click change */
}

/* ------------------------------------------------------------------------- */
/*
 * Mark a selection at the specified col/row
 */
void ecb_cold
rxvt_term::selection_start_colrow (int col, int row) noexcept
{
  want_refresh = 1;

  selection.mark.row = row + view_start;
  selection.mark.col = col;
  selection.mark.row = row - TermWin.view_start;
  MAX_IT (selection.mark.row, - (int32_t)TermWin.nscrolled);
  MIN_IT (selection.mark.row, (int32_t)TermWin.nrow - 1);
  MAX_IT (selection.mark.col, 0);
  MIN_IT (selection.mark.col, (int32_t)TermWin.ncol - 1);

  if (selection.op)
    {
      /* clear the old selection */
      selection.beg.row = selection.end.row = selection.mark.row;
      selection.beg.col = selection.end.col = selection.mark.col;
    }

  selection.op = SELECTION_INIT;
  selection.screen = current_screen;
}

/* ------------------------------------------------------------------------- */
/*
 * Word select: select text for 2 clicks
 * We now only find out the boundary in one direction
 */

/* what do we want: spaces/tabs are delimiters or cutchars or non-cutchars */
#define DELIMIT_TEXT(x) \
    (((x) == ' ' || (x) == '\t') ? 2 : (STRCHR (rs[Rs_cutchars], (x)) != NULL))
#define DELIMIT_REND(x)        1

void ecb_cold
rxvt_term::selection_delimit_word (enum page_dirn dirn, const row_col_t *mark, row_col_t *ret) noexcept
{
  int col, row, dirnadd, tcol, trow, w1, w2;
  row_col_t bound;
  text_t *stp;
  rend_t *srp;

  if (dirn == UP)
    {
      bound.row = top_row - 1;
      bound.col = 0;
      dirnadd = -1;
    }
  else
    {
      bound.row = nrow;
      bound.col = ncol - 1;
      dirnadd = 1;
    }
  row = mark->row + TermWin.saveLines;
  col = mark->col;
  MAX_IT (col, 0);
  /* find the edge of a word */
  stp = & (screen.text[row][col]);
  w1 = DELIMIT_TEXT (*stp);

  if (selection_style != NEW_SELECT)
    {
      if (w1 == 1)
        {
          stp += dirnadd;
          if (DELIMIT_TEXT (*stp) == 1)
            goto Old_Word_Selection_You_Die;
          col += dirnadd;
        }
      w1 = 0;
    }
  srp = (&screen.rend[row][col]);
  w2 = DELIMIT_REND (*srp);

  for (;;)
    {
      for (; col != bound.col; col += dirnadd)
        {
          stp += dirnadd;
          if (DELIMIT_TEXT (*stp) != w1)
            break;
          srp += dirnadd;
          if (DELIMIT_REND (*srp) != w2)
            break;
        }

      if ((col == bound.col) && (row != bound.row))
        {
          if (screen.tlen[ (row - (dirn == UP ? 1 : 0))] == -1)
            {
              trow = row + dirnadd;
              tcol = dirn == UP ? ncol - 1 : 0;

              if (!ROW(trow).t)
                break;
              stp = & (screen.text[trow][tcol]);
              srp = & (screen.rend[trow][tcol]);
              if (DELIMIT_TEXT (*stp) != w1 || DELIMIT_REND (*srp) != w2)
                break;

              row = trow;
              col = tcol;
              continue;
            }
        }
      break;
    }
Old_Word_Selection_You_Die:
  D_SELECT ((stderr, "rxvt_selection_delimit_word (%s,...) @ (r:%3d, c:%3d) has boundary (r:%3d, c:%3d)", (dirn == UP ? "up     " : "down"), mark->row, mark->col, row - TermWin.saveLines, col));

  if (dirn == DN)
    col++;                  /* put us on one past the end */

  /* Poke the values back in */
  ret->row = row;
  ret->col = col;
}

/* ------------------------------------------------------------------------- */
/*
 * Extend the selection to the specified x/y pixel location
 * EXT: button 3 press; button 1 or 3 drag
 * flag == 0 ==> button 1
 * flag == 1 ==> button 3 press
 * flag == 2 ==> button 3 motion
 */
void ecb_cold
rxvt_term::selection_extend (int x, int y, int flag) noexcept
{
  int col = clamp (Pixel2Col (x), 0, ncol);
  int row = clamp (Pixel2Row (y), 0, nrow - 1);

  col = Pixel2Col (x);
  row = Pixel2Row (y);
  MAX_IT (row, 0);
  MIN_IT (row, (int)TermWin.nrow - 1);
  MAX_IT (col, 0);
  MIN_IT (col, (int)TermWin.ncol);

#ifndef NO_NEW_SELECTION
  /*
  * If we're selecting characters (single click) then we must check first
  * if we are at the same place as the original mark.  If we are then
  * select nothing.  Otherwise, if we're to the right of the mark, you have to
  * be _past_ a character for it to be selected.
  */
  if (((selection.clicks % 3) == 1) && !flag
      && (col == selection.mark.col
          && (row == selection.mark.row - view_start)))
    {
      if (((selection.clicks % 3) == 1) && !flag
          && (col == selection.mark.col
              && (row == selection.mark.row + TermWin.view_start)))
        {
          /* select nothing */
          selection.beg.row = selection.end.row = 0;
          selection.beg.col = selection.end.col = 0;
          selection.clicks = 4;
          want_refresh = 1;
          D_SELECT ((stderr, "rxvt_selection_extend () selection.clicks = 4"));
          return;
        }
    }

  if (selection.clicks == 4)
    selection.clicks = 1;

  selection_extend_colrow (col, row, !!flag,  /* ? button 3      */
                           flag == 1 ? 1 : 0,     /* ? button press  */
                           0);    /* no click change */
}

/* ------------------------------------------------------------------------- */
/*
 * Extend the selection to the specified col/row
 */
void ecb_cold
rxvt_term::selection_extend_colrow (int32_t col, int32_t row, int button3, int buttonpress, int clickchange) noexcept
{
  row_col_t pos;
  enum {
    LEFT, RIGHT
  } closeto = RIGHT;

  D_SELECT ((stderr, "rxvt_selection_extend_colrow (c:%d, r:%d, %d, %d) clicks:%d, op:%d", col, row, button3, buttonpress, selection.clicks, selection.op));
  D_SELECT ((stderr, "rxvt_selection_extend_colrow () ENT  b: (r:%d,c:%d) m: (r:%d,c:%d), e: (r:%d,c:%d)", selection.beg.row, selection.beg.col, selection.mark.row, selection.mark.col, selection.end.row, selection.end.col));

  want_refresh = 1;
  switch (selection.op)
    {
      case SELECTION_INIT:
        CLEAR_SELECTION ();
        selection.op = SELECTION_BEGIN;
        /* FALLTHROUGH */
      case SELECTION_BEGIN:
        if (row != selection.mark.row || col != selection.mark.col
            || (!button3 && buttonpress))
          selection.op = SELECTION_CONT;
        break;
      case SELECTION_DONE:
        selection.op = SELECTION_CONT;
        /* FALLTHROUGH */
      case SELECTION_CONT:
        break;
      case SELECTION_CLEAR:
        selection_start_colrow (col, row);
        /* FALLTHROUGH */
      default:
        return;
    }

  if (selection.beg.col == selection.end.col
      && selection.beg.col != selection.mark.col
      && selection.beg.row == selection.end.row
      && selection.beg.row != selection.mark.row)
    {
      selection.beg.col = selection.end.col = selection.mark.col;
      selection.beg.row = selection.end.row = selection.mark.row;
      D_SELECT ((stderr, "rxvt_selection_extend_colrow () ENT2 b: (r:%d,c:%d) m: (r:%d,c:%d), e: (r:%d,c:%d)", selection.beg.row, selection.beg.col, selection.mark.row, selection.mark.col, selection.end.row, selection.end.col));
    }

  pos.col = col;
  pos.row = view_start + row;

  pos.row -= TermWin.view_start;   /* adjust for scroll */

#ifndef NO_OLD_SELECTION
  /*
   * This mimics some of the selection behaviour of version 2.20 and before.
   * There are no ``selection modes'', button3 is always character extension.
   * Note: button3 drag is always available, c.f. v2.20
   * Selection always terminates (left or right as appropriate) at the mark.
   */
  if (selection_style == OLD_SELECT)
    {
      if (selection.clicks == 1 || button3)
        {
          if (hate_those_clicks)
            {
              hate_those_clicks = 0;
              if (selection.clicks == 1)
                {
                  selection.beg.row = selection.mark.row;
                  selection.beg.col = selection.mark.col;
                }
              else
                {
                  selection.mark.row = selection.beg.row;
                  selection.mark.col = selection.beg.col;
                }
            }
          if (ROWCOL_IS_BEFORE (pos, selection.mark))
            {
              selection.end.row = selection.mark.row;
              selection.end.col = selection.mark.col + 1;
              selection.beg.row = pos.row;
              selection.beg.col = pos.col;
            }
          else
            {
              selection.beg.row = selection.mark.row;
              selection.beg.col = selection.mark.col;
              selection.end.row = pos.row;
              selection.end.col = pos.col + 1;
            }
        }
      else if (selection.clicks == 2)
        {
          selection_delimit_word (UP, & (selection.mark),
                                  & (selection.beg));
          selection_delimit_word (DN, & (selection.mark),
                                  & (selection.end));
          hate_those_clicks = 1;
        }
      else if (selection.clicks == 3)
        {
          selection.beg.row = selection.end.row = selection.mark.row;
          selection.beg.col = 0;
          selection.end.col = ncol;
          hate_those_clicks = 1;
        }
      D_SELECT ((stderr, "rxvt_selection_extend_colrow () EXIT b: (r:%d,c:%d) m: (r:%d,c:%d), e: (r:%d,c:%d)", selection.beg.row, selection.beg.col, selection.mark.row, selection.mark.col, selection.end.row, selection.end.col));
      return;
    }
#endif                          /* ! NO_OLD_SELECTION */
#ifndef NO_NEW_SELECTION
  /* selection_style must not be OLD_SELECT to get here */
  /*
   * This is mainly xterm style selection with a couple of differences, mainly
   * in the way button3 drag extension works.
   * We're either doing: button1 drag; button3 press; or button3 drag
   *  a) button1 drag : select around a midpoint/word/line - that point/word/line
   *     is always at the left/right edge of the selection.
   *  b) button3 press: extend/contract character/word/line at whichever edge of
   *     the selection we are closest to.
   *  c) button3 drag : extend/contract character/word/line - we select around
   *     a point/word/line which is either the start or end of the selection
   *     and it was decided by whichever point/word/line was `fixed' at the
   *     time of the most recent button3 press
   */
  if (button3 && buttonpress)
    {
      /* button3 press */
      /*
       * first determine which edge of the selection we are closest to
       */
      if (ROWCOL_IS_BEFORE (pos, selection.beg)
          || (!ROWCOL_IS_AFTER (pos, selection.end)
              && (((pos.col - selection.beg.col)
                   + ((pos.row - selection.beg.row) * ncol))
                  < ((selection.end.col - pos.col)
                     + ((selection.end.row - pos.row) * ncol)))))
        closeto = LEFT;

      if (closeto == LEFT)
        {
          selection.beg.row = pos.row;
          selection.beg.col = pos.col;
          selection.mark.row = selection.end.row;
          selection.mark.col = selection.end.col - (selection.clicks == 2);
        }
      else
        {
          selection.end.row = pos.row;
          selection.end.col = pos.col;
          selection.mark.row = selection.beg.row;
          selection.mark.col = selection.beg.col;
        }
    }
  else
    {                    /* button1 drag or button3 drag */
      if (ROWCOL_IS_AFTER (selection.mark, pos))
        {
          if (selection.mark.row == selection.end.row
              && selection.mark.col == selection.end.col
              && clickchange
              && selection.clicks == 2)
            selection.mark.col--;

          selection.beg.row = pos.row;
          selection.beg.col = pos.col;
          selection.end.row = selection.mark.row;
          selection.end.col = selection.mark.col + (selection.clicks == 2);
        }
      else
        {
          selection.beg.row = selection.mark.row;
          selection.beg.col = selection.mark.col;
          selection.end.row = pos.row;
          selection.end.col = pos.col;
        }
    }

  if (selection.clicks == 1)
    {
      if (selection.beg.col > ROW(selection.beg.row).l //TODO//FIXME//LEN
          && !ROW(selection.beg.row).is_longer ()
#if !ENABLE_MINIMAL
          && !selection.rect
#endif
         )
        selection.beg.col = ncol;

      if (
          selection.end.col > ROW(selection.end.row).l //TODO//FIXME//LEN
          && !ROW(selection.end.row).is_longer ()
#if !ENABLE_MINIMAL
          && !selection.rect
#endif
         )
        selection.end.col = ncol;
    }
  else if (selection.clicks == 2)
    {
      if (ROWCOL_IS_AFTER (selection.end, selection.beg))
        selection.end.col--;
      selection_delimit_word (UP, & (selection.beg),
                              & (selection.beg));
      selection_delimit_word (DN, & (selection.end),
                              & (selection.end));
    }
  else if (selection.clicks == 3)
    {
#if ENABLE_FRILLS
      if (option (Opt_tripleclickwords))
        {
          selection_delimit_word (UP, &selection.beg, &selection.beg);

          selection_delimit_word (UP, & (selection.beg),
                                  & (selection.beg));
          end_row = screen.tlen[selection.mark.row
                                + TermWin.saveLines];
          for (end_row = selection.mark.row; end_row < TermWin.nrow;
               end_row++)
            {
              if (!ROW(end_row).is_longer ())
                {
                  selection.end.row = end_row;
                  selection.end.col = ROW(end_row).l;
# if !ENABLE_MINIMAL
                  selection_remove_trailing_spaces ();
# endif
                  break;
                }
            }
        }
      else
#endif
        {
          if (ROWCOL_IS_AFTER (selection.mark, selection.beg))
            selection.mark.col++;

          selection.beg.col = 0;
          selection.end.col = ncol;

          // select a complete logical line
          while (selection.beg.row > -saveLines
                 && ROW(selection.beg.row - 1).is_longer ())
            selection.beg.row--;

          while (selection.end.row < nrow
                 && ROW(selection.end.row).is_longer ())
            selection.end.row++;
        }
    }

  if (button3 && buttonpress)
    {
      /* mark may need to be changed */
      if (closeto == LEFT)
        {
          selection.mark.row = selection.end.row;
          selection.mark.col = selection.end.col - (selection.clicks == 2);
        }
      else
        {
          selection.mark.row = selection.beg.row;
          selection.mark.col = selection.beg.col;
        }
    }
  D_SELECT ((stderr, "rxvt_selection_extend_colrow () EXIT b: (r:%d,c:%d) m: (r:%d,c:%d), e: (r:%d,c:%d)", selection.beg.row, selection.beg.col, selection.mark.row, selection.mark.col, selection.end.row, selection.end.col));
#endif                          /* ! NO_NEW_SELECTION */
}

#if !ENABLE_MINIMAL
void ecb_cold
rxvt_term::selection_remove_trailing_spaces () noexcept
{
  int32_t end_col, end_row;
  text_t *stp;

  end_col = selection.end.col;
  end_row = selection.end.row;

  for (; end_row >= selection.beg.row; )
    {
      stp = ROW(end_row).t;

      while (--end_col >= 0)
        {
          if (stp[end_col] != NOCHAR
              && !unicode::is_space (stp[end_col]))
            break;
        }

      if (end_col >= 0
          || !ROW(end_row - 1).is_longer ())
        {
          selection.end.col = end_col + 1;
          selection.end.row = end_row;
          break;
        }

      end_row--;
      end_col = ncol;
    }

  if (selection.mark.row > selection.end.row)
    {
      selection.mark.row = selection.end.row;
      selection.mark.col = selection.end.col;
    }
  else if (selection.mark.row == selection.end.row
           && selection.mark.col > selection.end.col)
    selection.mark.col = selection.end.col;
}
#endif

/* ------------------------------------------------------------------------- */
/*
 * Double click on button 3 when already selected
 * EXT: button 3 double click
 */
void ecb_cold
rxvt_term::selection_rotate (int x, int y) noexcept
{
  selection.clicks = selection.clicks % 3 + 1;
  selection_extend_colrow (Pixel2Col (x), Pixel2Row (y), 1, 0, 1);
}

/* ------------------------------------------------------------------------- */
/*
 * Respond to a request for our current selection
 * EXT: SelectionRequest
 */
void ecb_cold
rxvt_term::selection_send (const XSelectionRequestEvent &rq) noexcept
{
  Atom property = rq.property == None ? rq.target : rq.property;
  XSelectionEvent ev;

  ev.type = SelectionNotify;
  ev.property = None;
  ev.display = rq.display;
  ev.requestor = rq.requestor;
  ev.selection = rq.selection;
  ev.target = rq.target;
  ev.time = rq.time;

  if (rq.target == xa[XA_TARGETS])
    {
      Atom target_list[6];
      Atom *target = target_list;

      *target++ = xa[XA_TARGETS];
      *target++ = xa[XA_TIMESTAMP];
      *target++ = XA_STRING;
      *target++ = xa[XA_TEXT];
      *target++ = xa[XA_COMPOUND_TEXT];
#if X_HAVE_UTF8_STRING
      *target++ = xa[XA_UTF8_STRING];
#endif
      XChangeProperty (display->display, rq.requestor, rq.property, XA_ATOM,
                      (8 * sizeof (target_list[0])), PropModeReplace,
                      (unsigned char *)target_list,
                      target - target_list);
      ev.property = rq.property;
    }
#if TODO // TODO
  else if (rq.target == xa[XA_MULTIPLE])
    {
      /* TODO: Handle MULTIPLE */
    }
#endif
  else if (rq.target == xa[XA_TIMESTAMP] && rq.selection == XA_PRIMARY && selection.text)
    {
      XChangeProperty (display->display, rq.requestor, rq.property, XA_INTEGER,
                      (8 * sizeof (Time)), PropModeReplace,
                      (unsigned char *)&selection_time, 1);
      ev.property = rq.property;
    }
  else if (rq.target == XA_STRING
           || rq.target == xa[XA_TEXT]
           || rq.target == xa[XA_COMPOUND_TEXT]
           || rq.target == xa[XA_UTF8_STRING]
          )
    {
      XTextProperty ct;
      Atom target = rq.target;
      short freect = 0;
      int selectlen;
      wchar_t *cl;
      enum {
        enc_string        = XStringStyle,
        enc_text          = XStdICCTextStyle,
        enc_compound_text = XCompoundTextStyle,
#ifdef X_HAVE_UTF8_STRING
        enc_utf8          = XUTF8StringStyle,
#else
        enc_utf8          = -1,
#endif
      } style;

      if (target == XA_STRING)
        // we actually don't do XA_STRING, but who cares, as i18n clients
        // will ask for another format anyways.
        style = enc_string;
      else if (target == xa[XA_TEXT])
        style = enc_text;
      else if (target == xa[XA_COMPOUND_TEXT])
        style = enc_compound_text;
#if !ENABLE_MINIMAL
      else if (target == xa[XA_UTF8_STRING])
        style = enc_utf8;
#endif
      else
        {
          target = xa[XA_COMPOUND_TEXT];
          style = enc_compound_text;
        }

      if (rq.selection == XA_PRIMARY && selection.text)
        {
          cl = selection.text;
          selectlen = selection.len;
        }
      else if (rq.selection == xa[XA_CLIPBOARD] && selection.clip_text)
        {
          cl = selection.clip_text;
          selectlen = selection.clip_len;
        }
      else
        {
          cl = (wchar_t *)L"";
          selectlen = 0;
        }

      if (XmbTextListToTextProperty (display->display, &cl, 1, style, &ct) >= 0)
        freect = 1;
      else
        {
          /* if we failed to convert then send it raw */
          ct.value = (unsigned char *)cl;
          ct.nitems = selectlen;
          ct.encoding = target;
        }

      XChangeProperty (dpy, rq.requestor, property,
                       ct.encoding, 8, PropModeReplace,
                       ct.value, (int)ct.nitems);
      ev.property = property;

      if (freect)
        XFree (ct.value);
    }

/*
 * return col/row values corresponding to x/y pixel values
 */
void
rxvt_term::pixel_position (int *x, int *y)
{
  *x = Pixel2Col (*x);
  /* MAX_IT (*x, 0); MIN_IT (*x, (int)TermWin.ncol - 1); */
  *y = Pixel2Row (*y);
  /* MAX_IT (*y, 0); MIN_IT (*y, (int)TermWin.nrow - 1); */
}

/* ------------------------------------------------------------------------- */
#if USE_XIM
void ecb_cold
rxvt_term::im_set_position (XPoint &pos) noexcept
{
  XWindowAttributes xwa;

  XGetWindowAttributes (dpy, vt, &xwa);

  pos.x = xwa.x + Col2Pixel    (screen.cur.col);
  pos.y = xwa.y + Height2Pixel (screen.cur.row) + fbase;
}
#endif

#if ENABLE_OVERLAY
void
rxvt_term::scr_overlay_new (int x, int y, int w, int h) noexcept
{
  if (nrow < 1 || ncol < 1)
    return;

  want_refresh = 1;

  scr_overlay_off ();

  if (x < 0) x = ncol - w;
  if (y < 0) y = nrow - h;

  // make space for border
  w += 2; min_it (w, ncol);
  h += 2; min_it (h, nrow);

  x -= 1; clamp_it (x, 0, ncol - w);
  y -= 1; clamp_it (y, 0, nrow - h);

  ov.x = x; ov.y = y;
  ov.w = w; ov.h = h;

  ov.text = new text_t *[h];
  ov.rend = new rend_t *[h];

  for (y = 0; y < h; y++)
    {
      text_t *tp = ov.text[y] = new text_t[w];
      rend_t *rp = ov.rend[y] = new rend_t[w];

      text_t t0, t1, t2;
      rend_t r = OVERLAY_RSTYLE;

      if (y == 0)
        t0 = 0x2554, t1 = 0x2550, t2 = 0x2557;
      else if (y < h - 1)
        t0 = 0x2551, t1 = 0x0020, t2 = 0x2551;
      else
        t0 = 0x255a, t1 = 0x2550, t2 = 0x255d;

      *tp++ = t0;
      *rp++ = r;

      for (x = w - 2; x > 0; --x)
        {
          *tp++ = t1;
          *rp++ = r;
        }

      *tp = t2;
      *rp = r;
    }
}

void
rxvt_term::scr_overlay_off () noexcept
{
  if (!ov.text)
    return;

  want_refresh = 1;

  for (int y = 0; y < ov.h; y++)
    {
      delete [] ov.text[y];
      delete [] ov.rend[y];
    }

  delete [] ov.text; ov.text = 0;
  delete [] ov.rend; ov.rend = 0;
}

void
rxvt_term::scr_overlay_set (int x, int y, text_t text, rend_t rend) noexcept
{
  if (!ov.text || x >= ov.w - 2 || y >= ov.h - 2)
    return;

  x++, y++;

  ov.text[y][x] = text;
  ov.rend[y][x] = rend;
}

void
rxvt_term::scr_overlay_set (int x, int y, const char *s) noexcept
{
  while (*s)
    scr_overlay_set (x++, y, *s++);
}

void
rxvt_term::scr_overlay_set (int x, int y, const wchar_t *s) noexcept
{
  while (*s)
    {
      text_t t = *s++;
      int width = WCWIDTH (t);

      while (width--)
        {
          scr_overlay_set (x++, y, t);
          t = NOCHAR;
        }
    }
}

void
rxvt_term::scr_swap_overlay () noexcept
{
  if (!ov.text)
    return;

  // hide cursor if it is within the overlay area
  if (IN_RANGE_EXC (screen.cur.col - ov.x, 0, ov.w)
      && IN_RANGE_EXC (screen.cur.row - ov.y, 0, ov.h))
    screen.flags &= ~Screen_VisibleCursor;

  // swap screen mem with overlay
  for (int y = ov.h; y--; )
    {
      text_t *t1 = ov.text[y];
      rend_t *r1 = ov.rend[y];

      text_t *t2 = ROW(y + ov.y + view_start).t + ov.x;
      rend_t *r2 = ROW(y + ov.y + view_start).r + ov.x;

      for (int x = ov.w; x--; )
        {
          text_t t = *t1; *t1++ = *t2; *t2++ = t;
          rend_t r = *r1; *r1++ = *r2; *r2++ = SET_FONT (r, FONTSET (r)->find_font (t));
        }
    }
}

#endif
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 *                              DEBUG ROUTINES                               *
 * ------------------------------------------------------------------------- */
#if 0
void
rxvt_debug_colors (void)
{
  int             color;
  const char     *name[] =
    {
      "fg", "bg",
      "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"
    };

  fprintf (stderr, "Color ( ");
  if (rstyle & RS_RVid)
    fprintf (stderr, "rvid ");
  if (rstyle & RS_Bold)
    fprintf (stderr, "bold ");
  if (rstyle & RS_Blink)
    fprintf (stderr, "blink ");
  if (rstyle & RS_Uline)
    fprintf (stderr, "uline ");
  fprintf (stderr, "): ");

  color = GET_FGCOLOR (rstyle);
#ifndef NO_BRIGHTCOLOR
  if (color >= minBrightCOLOR && color <= maxBrightCOLOR)
    {
      color -= (minBrightCOLOR - minCOLOR);
      fprintf (stderr, "bright ");
    }
#endif
  fprintf (stderr, "%s on ", name[color]);

  color = GET_BGCOLOR (rstyle);
#ifndef NO_BRIGHTCOLOR
  if (color >= minBrightCOLOR && color <= maxBrightCOLOR)
    {
      color -= (minBrightCOLOR - minCOLOR);
      fprintf (stderr, "bright ");
    }
#endif
  fprintf (stderr, "%s\n", name[color]);
}
#endif
