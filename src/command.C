/*--------------------------------*-C-*---------------------------------*
 * File:	command.C
 *----------------------------------------------------------------------*
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (c) 1992      John Bovey, University of Kent at Canterbury <jdb@ukc.ac.uk>
 *				- original version
 * Copyright (c) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 *				- extensive modifications
 * Copyright (c) 1995      Garrett D'Amore <garrett@netcom.com>
 *				- vt100 printing
 * Copyright (c) 1995      Steven Hirsch <hirsch@emba.uvm.edu>
 *				- X11 mouse report mode and support for
 *				  DEC "private mode" save/restore functions.
 * Copyright (c) 1995      Jakub Jelinek <jj@gnu.ai.mit.edu>
 *				- key-related changes to handle Shift+function
 *				  keys properly.
 * Copyright (c) 1997      MJ Olesen <olesen@me.queensu.ca>
 *				- extensive modifications
 * Copyright (c) 1997      Raul Garcia Garcia <rgg@tid.es>
 *				- modification and cleanups for Solaris 2.x
 *				  and Linux 1.2.x
 * Copyright (c) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 * Copyright (c) 1998-2001 Geoff Wing <gcw@pobox.com>
 *				- extensive modifications
 * Copyright (c) 1998      Alfredo K. Kojima <kojima@windowmaker.org>
 * Copyright (c) 2001      Marius Gedminas
 *				- Ctrl/Mod4+Tab works like Meta+Tab (options)
 * Copyright (c) 2003      Rob McMullen <robm@flipturn.org>
 * Copyright (c) 2003-2021 Marc Lehmann <schmorp@schmorp.de>
 * Copyright (c) 2007,2015 Emanuele Giaquinta <e.giaquinta@glauco.it>
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

/*{{{ includes: */
#include "../config.h"
#include "rxvt.h"
#include "rxvtperl.h"
#include "version.h"
#include "command.h"

#ifdef KEYSYM_RESOURCE
# include "keyboard.h"
#endif

#include <signal.h>

#if LINUX_YIELD_HACK
# include <time.h>
#endif

/*----------------------------------------------------------------------*/

#define IS_CONTROL(ch) !((ch) & 0xffffff60UL)

// exception thrown when the command parser runs out of input data
class out_of_input { } out_of_input;

/*{{{ Convert the keypress event into a string */
void
rxvt_term::lookup_key (XKeyEvent &ev)
{
  int ctrl, meta, shft, len;
  KeySym keysym = NoSymbol;
  char rkbuf[KBUFSZ + 1];
  char *kbuf = rkbuf + 1;

#if ISO_14755
  if (iso14755buf & ISO_14755_52)
    return;
#endif

  /*
   * use Num_Lock to toggle Keypad on/off.  If Num_Lock is off, allow an
   * escape sequence to toggle the Keypad.
   *
   * Always permit `shift' to override the current setting
   */
  shft = ev.state & ShiftMask;
  ctrl = ev.state & ControlMask;
  meta = ev.state & ModMetaMask;

  kbuf[0] = 0;

#if USE_XIM
  if (Input_Context)
    {
      Status status_return;

#if 0
#ifdef X_HAVE_UTF8_STRING
      if (enc_utf8 && 0) // currently disabled, doesn't seem to work, nor is useful
        len = Xutf8LookupString (Input_Context, &ev, kbuf,
                                 KBUFSZ, &keysym, &status_return);
      else
#endif
#endif
        {
          wchar_t wkbuf[KBUFSZ + 1];

          // the XOpenIM manpage lies about hardcoding the locale
          // at the point of XOpenIM, so temporarily switch locales
          if (rs[Rs_imLocale])
            SET_LOCALE (rs[Rs_imLocale]);

          // assume wchar_t == unicode or better
          len = XwcLookupString (Input_Context, &ev, wkbuf,
                                 KBUFSZ, &keysym, &status_return);

          if (rs[Rs_imLocale])
            SET_LOCALE (locale);

          if (status_return == XLookupChars
              || status_return == XLookupBoth)
            {
              /* make sure the user can type ctrl-@, i.e. NUL */
              if (len == 1 && *wkbuf == 0)
                {
                  kbuf[0] = 0;
                  len = 1;
                }
              else
                {
                  wkbuf[len] = 0;
                  len = wcstombs ((char *)kbuf, wkbuf, KBUFSZ);
                  if (len < 0)
                    len = 0;
                }
            }
          else
            len = 0;
        }
    }
  else
#endif
    {
      len = XLookupString (&ev, kbuf, KBUFSZ, &keysym, &compose);
    }

  if (keysym != NoSymbol)
    {
      KeySym orig_keysym = keysym;

      /* Shift + F1 - F10 generates F11 - F20 */
      if (shft && keysym >= XK_F1 && keysym <= XK_F10)
        {
          keysym += (XK_F11 - XK_F1);
          shft = 0;	/* turn off Shift */
        }

      if (keysym >= 0xFF00 && keysym <= 0xFFFF)
        {
          bool kp = priv_modes & PrivMode_aplKP ? !shft : shft;
          unsigned int newlen = 1;

          if (ev.state & ModNumLockMask)
            kp = false;

          keysym = translate_keypad (keysym, kp);

          switch (keysym)
            {
#ifndef NO_BACKSPACE_KEY
              case XK_BackSpace:
                if (priv_modes & PrivMode_HaveBackSpace)
                  {
                    kbuf[0] = (!! (priv_modes & PrivMode_BackSpace)
                               ^ !!ctrl) ? '\b' : '\177';
                    kbuf[1] = '\0';
                  }
                else
                  strcpy (kbuf, rs[Rs_backspace_key]);
                break;
#endif
#ifndef NO_DELETE_KEY
              case XK_Delete:
                strcpy (kbuf, rs[Rs_delete_key]);
                break;
#endif
              case XK_Tab:
                if (shft)
                  strcpy (kbuf, "\033[Z");
                else
                  {
#ifdef CTRL_TAB_MAKES_META
                    if (ctrl)
                      meta = 1;
#endif
#ifdef MOD4_TAB_MAKES_META
                    if (ev.state & Mod4Mask)
                      meta = 1;
#endif
                    newlen = 0;
                  }
                break;

              case XK_Up:	/* "\033[A" */
              case XK_Down:	/* "\033[B" */
              case XK_Right:	/* "\033[C" */
              case XK_Left:	/* "\033[D" */
                strcpy (kbuf, "\033[Z");
                kbuf[2] = "DACB"[keysym - XK_Left];
                /* do Shift first */
                if (shft)
                  kbuf[2] = "dacb"[keysym - XK_Left];
                else if (ctrl)
                  {
                    kbuf[1] = 'O';
                    kbuf[2] = "dacb"[keysym - XK_Left];
                  }
                else if (priv_modes & PrivMode_aplCUR)
                  kbuf[1] = 'O';
                break;

              case XK_KP_Enter:
                /* allow shift to override */
                if (kp)
                  {
                    strcpy (kbuf, "\033OM");
                    break;
                  }

                /* FALLTHROUGH */

              case XK_Return:
                if (priv_modes & PrivMode_LFNL)
                  {
                    kbuf[0] = '\015';
                    kbuf[1] = '\012';
                    kbuf[2] = '\0';
                  }
                else
                  {
                    kbuf[0] = '\015';
                    kbuf[1] = '\0';
                  }
                break;

              case XK_KP_F1:	/* "\033OP" */
              case XK_KP_F2:	/* "\033OQ" */
              case XK_KP_F3:	/* "\033OR" */
              case XK_KP_F4:	/* "\033OS" */
                strcpy (kbuf, "\033OP");
                kbuf[2] += (keysym - XK_KP_F1);
                break;

              case XK_KP_Multiply:	/* "\033Oj" : "*" */
              case XK_KP_Add:		/* "\033Ok" : "+" */
              case XK_KP_Separator:	/* "\033Ol" : "," */
              case XK_KP_Subtract:	/* "\033Om" : "-" */
              case XK_KP_Decimal:	/* "\033On" : "." */
              case XK_KP_Divide:	/* "\033Oo" : "/" */
              case XK_KP_0:		/* "\033Op" : "0" */
              case XK_KP_1:		/* "\033Oq" : "1" */
              case XK_KP_2:		/* "\033Or" : "2" */
              case XK_KP_3:		/* "\033Os" : "3" */
              case XK_KP_4:		/* "\033Ot" : "4" */
              case XK_KP_5:		/* "\033Ou" : "5" */
              case XK_KP_6:		/* "\033Ov" : "6" */
              case XK_KP_7:		/* "\033Ow" : "7" */
              case XK_KP_8:		/* "\033Ox" : "8" */
              case XK_KP_9:		/* "\033Oy" : "9" */
                /* allow shift to override */
                if (kp)
                  {
                    strcpy (kbuf, "\033Oj");
                    kbuf[2] += (keysym - XK_KP_Multiply);
                  }
                else
                  {
                    kbuf[0] = ('*' + (keysym - XK_KP_Multiply));
                    kbuf[1] = '\0';
                  }
                break;

              default:
                {
                  int param = map_function_key (keysym);
                  if (param > 0)
                    sprintf (kbuf,"\033[%d~", param);
                  else
                    newlen = 0;
                }
                break;
            }

          if (newlen)
            len = strlen (kbuf);

          if (len > 0)
            {
              /*
               * pass Shift/Control indicators for function keys ending with `~'
               *
               * eg,
               *   Prior = "ESC[5~"
               *   Shift+Prior = "ESC[5$"
               *   Ctrl+Prior = "ESC[5^"
               *   Ctrl+Shift+Prior = "ESC[5@"
               */
              if (kbuf[0] == C0_ESC && kbuf[1] == '[' && kbuf[len - 1] == '~')
                kbuf[len - 1] = (shft ? (ctrl ? '@' : '$') : (ctrl ? '^' : '~'));

              /*
               * Pass meta for all function keys, if 'meta' option set
               */
#ifdef META8_OPTION
              if (meta && (meta_char == 0x80))
                kbuf[len - 1] |= 0x80;
#endif
            }

        }
      else if (ctrl && keysym == XK_minus)
        {
          len = 1;
          kbuf[0] = '\037';	/* Ctrl-Minus generates ^_ (31) */
        }
      else if (keysym == XK_ISO_Left_Tab)
        {
          strcpy (kbuf, "\033[Z");
          len = 3;
        }
      else
        {
#ifdef META8_OPTION
          /* set 8-bit on */
          if (meta && (meta_char == 0x80))
            {
              char *ch;

              for (ch = kbuf; ch < kbuf + len; ch++)
                *ch |= 0x80;
            }
#endif
          /* nil */ ;
        }

      keysym = orig_keysym;
    }

  /* escape prefix */
  if (len && meta
#ifdef META8_OPTION
      && meta_char == C0_ESC
#endif
     )
    {
      *--kbuf = C0_ESC;
      len++;
    }

  if (HOOK_INVOKE ((this, HOOK_KEY_PRESS, DT_XEVENT, &ev, DT_INT, keysym, DT_STR_LEN, kbuf, len, DT_END)))
    return;

  if (keysym != NoSymbol)
    {
#ifdef KEYSYM_RESOURCE
      if (keyboard->dispatch (this, keysym, ev.state, kbuf, len))
        return;
#endif

      if (saveLines)
        {
#ifdef UNSHIFTED_SCROLLKEYS
          if (!ctrl && !meta)
#else
          if (IS_SCROLL_MOD)
#endif
            {
              int lnsppg;

#ifdef PAGING_CONTEXT_LINES
              lnsppg = nrow - PAGING_CONTEXT_LINES;
#else
              lnsppg = nrow * 4 / 5;
#endif
              max_it (lnsppg, 1);

              if (keysym == XK_Prior)
                {
                  scr_page (lnsppg);
                  return;
                }
              else if (keysym == XK_Next)
                {
                  scr_page (-lnsppg);
                  return;
                }
            }
#ifdef SCROLL_ON_UPDOWN_KEYS
          if (IS_SCROLL_MOD)
            {
              if (keysym == XK_Up)
                {
                  scr_page (1);
                  return;
                }
              else if (keysym == XK_Down)
                {
                  scr_page (-1);
                  return;
                }
            }
#endif
#ifdef SCROLL_ON_HOMEEND_KEYS
          if (IS_SCROLL_MOD)
            {
              if (keysym == XK_Home)
                {
                  scr_changeview (top_row);
                  return;
                }
              else if (keysym == XK_End)
                {
                  scr_changeview (0);
                  return;
                }
            }
#endif
        }

      if (shft)
        {
          if (!ctrl && !meta && (priv_modes & PrivMode_ShiftKeys))
            {
              switch (keysym)
                {
                    /* normal XTerm key bindings */
                  case XK_Insert:	/* Shift+Insert = paste mouse selection */
                    selection_request (ev.time);
                    return;
#if TODO
                    /* rxvt extras */
                  case XK_KP_Add:	/* Shift+KP_Add = bigger font */
                    return;
                  case XK_KP_Subtract:	/* Shift+KP_Subtract = smaller font */
                    return;
#endif
                }
            }
        }

      if (ctrl && meta && (keysym == XK_c || keysym == XK_v))
        {
          if (keysym == XK_v)
            selection_request (ev.time, Sel_Clipboard);
          else if (selection.len > 0)
            {
              free (selection.clip_text);
              selection.clip_text = rxvt_wcsdup (selection.text, selection.len);
              selection.clip_len = selection.len;
              selection_grab (CurrentTime, true);
            }

          return;
        }

#if ENABLE_FRILLS || ISO_14755
      // ISO 14755 support
      if (iso14755buf & (ISO_14755_STARTED | ISO_14755_51))
        {
          int hv;

          if (iso14755buf & ISO_14755_51
              && (keysym == XK_space || keysym == XK_KP_Space
                  || keysym == XK_Return || keysym == XK_KP_Enter))
            {
              commit_iso14755 ();
              iso14755buf = ISO_14755_51;
# if ISO_14755
              iso14755_51 (0);
# endif
              return;
            }
          else if (keysym == XK_BackSpace)
            {
              iso14755buf = ((iso14755buf & ISO_14755_MASK) >> 4) | ISO_14755_51;
# if ISO_14755
              iso14755_51 (iso14755buf & ISO_14755_MASK);
# endif
              return;
            }
          else if ((hv = hex_keyval (ev)) >= 0)
            {
              iso14755buf = ((iso14755buf << 4) & ISO_14755_MASK)
                          | hv | ISO_14755_51;
# if ISO_14755
              iso14755_51 (iso14755buf & ISO_14755_MASK);
# endif
              return;
            }
          else
            {
# if ISO_14755
              scr_overlay_off ();
# endif
              iso14755buf = 0;
            }
        }
      else if (option (Opt_iso14755) &&
               ((ctrl && (keysym == XK_Shift_L || keysym == XK_Shift_R))
                || (shft && (keysym == XK_Control_L || keysym == XK_Control_R))))
        if (!(iso14755buf & ISO_14755_STARTED))
          {
            iso14755buf |= ISO_14755_STARTED;
# if ISO_14755
            scr_overlay_new (0, -1, sizeof ("ISO 14755 mode") - 1, 1);
            scr_overlay_set (0, 0, "ISO 14755 mode");
# endif
          }
#endif

#ifdef PRINTPIPE
      if (keysym == XK_Print)
        {
          scr_printscreen (ctrl | shft);
          return;
        }
#endif

      if (keysym >= 0xFF00 && keysym <= 0xFFFF)
        {
#ifdef KEYSYM_RESOURCE
          if (! (shft | ctrl) && Keysym_map[keysym & 0xFF] != NULL)
            {
              unsigned int    l;
              const unsigned char *kbuf0;
              const unsigned char ch = C0_ESC;

              kbuf0 = (Keysym_map[keysym & 0xFF]);
              l = (unsigned int)*kbuf0++;

              /* escape prefix */
              if (meta)
# ifdef META8_OPTION
                if (meta_char == C0_ESC)
# endif
                  tt_write (&ch, 1);
              tt_write (kbuf0, l);
              return;
            }
          else
#endif
            {
              newlen = 1;
              switch (keysym)
                {
#ifndef NO_BACKSPACE_KEY
                  case XK_BackSpace:
                    if (PrivateModes & PrivMode_HaveBackSpace)
                      {
                        kbuf[0] = (!! (PrivateModes & PrivMode_BackSpace)
                                   ^ !!ctrl) ? '\b' : '\177';
                        kbuf[1] = '\0';
                      }
                    else
                      STRCPY (kbuf, key_backspace);
# ifdef MULTICHAR_SET
                    if ((Options & Opt_mc_hack) && screen.cur.col > 0)
                      {
                        int             col, row;

                        newlen = STRLEN (kbuf);
                        col = screen.cur.col - 1;
                        row = screen.cur.row + TermWin.saveLines;
                        if (IS_MULTI2 (screen.rend[row][col]))
                          MEMMOVE (kbuf + newlen, kbuf, newlen + 1);
                      }
# endif
                    break;
#endif
#ifndef NO_DELETE_KEY
                  case XK_Delete:
                    STRCPY (kbuf, key_delete);
# ifdef MULTICHAR_SET
                    if (Options & Opt_mc_hack)
                      {
                        int             col, row;

                        newlen = STRLEN (kbuf);
                        col = screen.cur.col;
                        row = screen.cur.row + TermWin.saveLines;
                        if (IS_MULTI1 (screen.rend[row][col]))
                          MEMMOVE (kbuf + newlen, kbuf, newlen + 1);
                      }
# endif
                    break;
#endif
                  case XK_Tab:
                    if (shft)
                      STRCPY (kbuf, "\033[Z");
                    else
                      {
#ifdef CTRL_TAB_MAKES_META
                        if (ctrl)
                          meta = 1;
#endif
#ifdef MOD4_TAB_MAKES_META
                        if (ev.state & Mod4Mask)
                          meta = 1;
#endif
                        newlen = 0;
                      }
                    break;

#ifdef XK_KP_Left
                  case XK_KP_Up:		/* \033Ox or standard */
                  case XK_KP_Down:	/* \033Or or standard */
                  case XK_KP_Right:	/* \033Ov or standard */
                  case XK_KP_Left:	/* \033Ot or standard */
                    if ((PrivateModes & PrivMode_aplKP) ? !shft : shft)
                      {
                        STRCPY (kbuf, "\033OZ");
                        kbuf[2] = "txvr"[keysym - XK_KP_Left];
                        break;
                      }
                    else
                      /* translate to std. cursor key */
                      keysym = XK_Left + (keysym - XK_KP_Left);
                    /* FALLTHROUGH */
#endif
                  case XK_Up:	/* "\033[A" */
                  case XK_Down:	/* "\033[B" */
                  case XK_Right:	/* "\033[C" */
                  case XK_Left:	/* "\033[D" */
                    STRCPY (kbuf, "\033[Z");
                    kbuf[2] = "DACB"[keysym - XK_Left];
                    /* do Shift first */
                    if (shft)
                      kbuf[2] = "dacb"[keysym - XK_Left];
                    else if (ctrl)
                      {
                        kbuf[1] = 'O';
                        kbuf[2] = "dacb"[keysym - XK_Left];
                      }
                    else if (PrivateModes & PrivMode_aplCUR)
                      kbuf[1] = 'O';
#ifdef MULTICHAR_SET
                    //TODO: ??
                    if (Options & Opt_mc_hack)
                      {
                        int             col, row, m;

                        col = screen.cur.col;
                        row = screen.cur.row + TermWin.saveLines;
                        m = 0;
                        if (keysym == XK_Right
                            && IS_MULTI1 (screen.rend[row][col]))
                          m = 1;
                        else if (keysym == XK_Left)
                          {
                            if (col > 0)
                              {
                                if (IS_MULTI2 (screen.rend[row][col - 1]))
                                  m = 1;
                              }
                            else if (screen.cur.row > 0)
                              {
                                col = screen.tlen[--row];
                                if (col == -1)
                                  col = TermWin.ncol - 1;
                                else
                                  col--;
                                if (col > 0
                                    && IS_MULTI2 (screen.rend[row][col]))
                                  m = 1;
                              }
                          }
                        if (m)
                          MEMMOVE (kbuf + 3, kbuf, 3 + 1);
                      }
#endif
                    break;

#ifndef UNSHIFTED_SCROLLKEYS
# ifdef XK_KP_Prior
                  case XK_KP_Prior:
                    /* allow shift to override */
                    if ((PrivateModes & PrivMode_aplKP) ? !shft : shft)
                      {
                        STRCPY (kbuf, "\033Oy");
                        break;
                      }
                    /* FALLTHROUGH */
# endif
                  case XK_Prior:
                    STRCPY (kbuf, "\033[5~");
                    break;
# ifdef XK_KP_Next
                  case XK_KP_Next:
                    /* allow shift to override */
                    if ((PrivateModes & PrivMode_aplKP) ? !shft : shft)
                      {
                        STRCPY (kbuf, "\033Os");
                        break;
                      }
                    /* FALLTHROUGH */
# endif
                  case XK_Next:
                    STRCPY (kbuf, "\033[6~");
                    break;
#endif
                  case XK_KP_Enter:
                    /* allow shift to override */
                    if ((PrivateModes & PrivMode_aplKP) ? !shft : shft)
                      {
                        STRCPY (kbuf, "\033OM");
                      }
                    else
                      {
                        kbuf[0] = '\r';
                        kbuf[1] = '\0';
                      }
                    break;

#ifdef XK_KP_Begin
                  case XK_KP_Begin:
                    STRCPY (kbuf, "\033Ou");
                    break;

                  case XK_KP_Insert:
                    STRCPY (kbuf, "\033Op");
                    break;

                  case XK_KP_Delete:
                    STRCPY (kbuf, "\033On");
                    break;
#endif
                  case XK_KP_F1:	/* "\033OP" */
                  case XK_KP_F2:	/* "\033OQ" */
                  case XK_KP_F3:	/* "\033OR" */
                  case XK_KP_F4:	/* "\033OS" */
                    STRCPY (kbuf, "\033OP");
                    kbuf[2] += (keysym - XK_KP_F1);
                    break;

                  case XK_KP_Multiply:	/* "\033Oj" : "*" */
                  case XK_KP_Add:	/* "\033Ok" : "+" */
                  case XK_KP_Separator:	/* "\033Ol" : "," */
                  case XK_KP_Subtract:	/* "\033Om" : "-" */
                  case XK_KP_Decimal:	/* "\033On" : "." */
                  case XK_KP_Divide:	/* "\033Oo" : "/" */
                  case XK_KP_0:		/* "\033Op" : "0" */
                  case XK_KP_1:		/* "\033Oq" : "1" */
                  case XK_KP_2:		/* "\033Or" : "2" */
                  case XK_KP_3:		/* "\033Os" : "3" */
                  case XK_KP_4:		/* "\033Ot" : "4" */
                  case XK_KP_5:		/* "\033Ou" : "5" */
                  case XK_KP_6:		/* "\033Ov" : "6" */
                  case XK_KP_7:		/* "\033Ow" : "7" */
                  case XK_KP_8:		/* "\033Ox" : "8" */
                  case XK_KP_9:		/* "\033Oy" : "9" */
                    /* allow shift to override */
                    if ((PrivateModes & PrivMode_aplKP) ? !shft : shft)
                      {
                        STRCPY (kbuf, "\033Oj");
                        kbuf[2] += (keysym - XK_KP_Multiply);
                      }
                    else
                      {
                        kbuf[0] = ('*' + (keysym - XK_KP_Multiply));
                        kbuf[1] = '\0';
                      }
                    break;

                  case XK_Find:
                    STRCPY (kbuf, "\033[1~");
                    break;
                  case XK_Insert:
                    STRCPY (kbuf, "\033[2~");
                    break;
#ifdef DXK_Remove		/* support for DEC remove like key */
                  case DXK_Remove:
                    /* FALLTHROUGH */
#endif
                  case XK_Execute:
                    STRCPY (kbuf, "\033[3~");
                    break;
                  case XK_Select:
                    STRCPY (kbuf, "\033[4~");
                    break;
#ifdef XK_KP_End
                  case XK_KP_End:
                    /* allow shift to override */
                    if ((PrivateModes & PrivMode_aplKP) ? !shft : shft)
                      {
                        STRCPY (kbuf, "\033Oq");
                        break;
                      }
                    /* FALLTHROUGH */
#endif
                  case XK_End:
                    STRCPY (kbuf, KS_END);
                    break;
#ifdef XK_KP_Home
                  case XK_KP_Home:
                    /* allow shift to override */
                    if ((PrivateModes & PrivMode_aplKP) ? !shft : shft)
                      {
                        STRCPY (kbuf, "\033Ow");
                        break;
                      }
                    /* FALLTHROUGH */
#endif
                  case XK_Home:
                    STRCPY (kbuf, KS_HOME);
                    break;

#define FKEY(n, fkey)							\
    sprintf ((char *)kbuf,"\033[%2d~", (int) ((n) + (keysym - fkey)))

                  case XK_F1:	/* "\033[11~" */
                  case XK_F2:	/* "\033[12~" */
                  case XK_F3:	/* "\033[13~" */
                  case XK_F4:	/* "\033[14~" */
                  case XK_F5:	/* "\033[15~" */
                    FKEY (11, XK_F1);
                    break;
                  case XK_F6:	/* "\033[17~" */
                  case XK_F7:	/* "\033[18~" */
                  case XK_F8:	/* "\033[19~" */
                  case XK_F9:	/* "\033[20~" */
                  case XK_F10:	/* "\033[21~" */
                    FKEY (17, XK_F6);
                    break;
                  case XK_F11:	/* "\033[23~" */
                  case XK_F12:	/* "\033[24~" */
                  case XK_F13:	/* "\033[25~" */
                  case XK_F14:	/* "\033[26~" */
                    FKEY (23, XK_F11);
                    break;
                  case XK_F15:	/* "\033[28~" */
                  case XK_F16:	/* "\033[29~" */
                    FKEY (28, XK_F15);
                    break;
                  case XK_Help:	/* "\033[28~" */
                    FKEY (28, XK_Help);
                    break;
                  case XK_Menu:	/* "\033[29~" */
                    FKEY (29, XK_Menu);
                    break;
                  case XK_F17:	/* "\033[31~" */
                  case XK_F18:	/* "\033[32~" */
                  case XK_F19:	/* "\033[33~" */
                  case XK_F20:	/* "\033[34~" */
                  case XK_F21:	/* "\033[35~" */
                  case XK_F22:	/* "\033[36~" */
                  case XK_F23:	/* "\033[37~" */
                  case XK_F24:	/* "\033[38~" */
                  case XK_F25:	/* "\033[39~" */
                  case XK_F26:	/* "\033[40~" */
                  case XK_F27:	/* "\033[41~" */
                  case XK_F28:	/* "\033[42~" */
                  case XK_F29:	/* "\033[43~" */
                  case XK_F30:	/* "\033[44~" */
                  case XK_F31:	/* "\033[45~" */
                  case XK_F32:	/* "\033[46~" */
                  case XK_F33:	/* "\033[47~" */
                  case XK_F34:	/* "\033[48~" */
                  case XK_F35:	/* "\033[49~" */
                    FKEY (31, XK_F17);
                    break;
#undef FKEY
                  default:
                    newlen = 0;
                    break;
                }
              if (newlen)
                len = STRLEN (kbuf);
            }
          /*
           * Pass meta for all function keys, if 'meta' option set
           */
#ifdef META8_OPTION
          if (meta && (meta_char == 0x80) && len > 0)
            kbuf[len - 1] |= 0x80;
#endif

        }
      else if (ctrl && keysym == XK_minus)
        {
          len = 1;
          kbuf[0] = '\037';	/* Ctrl-Minus generates ^_ (31) */
        }
      else
        {
#ifdef META8_OPTION
          /* set 8-bit on */
          if (meta && (meta_char == 0x80))
            {
              unsigned char  *ch;

              for (ch = kbuf; ch < kbuf + len; ch++)
                *ch |= 0x80;
              meta = 0;
            }
#endif
          /* nil */ ;
        }
    }

  if (len <= 0)
    return;			/* not mapped */

  tt_write_user_input (kbuf, (unsigned int)len);
}

void ecb_cold
rxvt_term::key_release (XKeyEvent &ev)
{
  unsigned int n, s;

  n = cmdbuf_ptr - cmdbuf_base;
  s = cmdbuf_base + CBUFSIZ - 1 - cmdbuf_endp;

  if (n > 0 && s < count)
    {
      MEMMOVE (cmdbuf_base, cmdbuf_ptr,
              (unsigned int) (cmdbuf_endp - cmdbuf_ptr));
      cmdbuf_ptr = cmdbuf_base;
      cmdbuf_endp -= n;
      s += n;
    }

  if (count > s)
    {
      rxvt_warn ("data loss: cmd_write too large, continuing.\n");
      count = s;
    }

  for (; count--;)
    *cmdbuf_endp++ = *str++;

  cmd_parse ();

  return 0;
}

void
rxvt_term::flush ()
{
  flush_ev.stop ();

#ifdef HAVE_IMG
  if (bg_flags & BG_NEEDS_REFRESH)
    {
      bg_flags &= ~BG_NEEDS_REFRESH;
      scr_touch (false);
    }
#endif

  if (want_refresh)
    {
      scr_refresh (refresh_type);
      scrollbar_show (1);
#ifdef USE_XIM
      IMSendSpot ();
#endif
    }

  display->flush ();
}

/* checks whether a refresh is requested and starts the refresh timer */
void
rxvt_term::refresh_check ()
{
  if (want_refresh && !flush_ev.is_active ())
    flush_ev.start ();

  display->flush ();
}

void
rxvt_term::flush_cb (ev::timer &w, int revents)
{
  make_current ();

  refresh_count = 0;
  flush ();
}

#ifdef CURSOR_BLINK
void
rxvt_term::cursor_blink_reset ()
{
  if (!focus)
    return;

  if (hidden_cursor)
    {
      hidden_cursor = 0;
      want_refresh = 1;
    }

  if (option (Opt_cursorBlink) || (priv_modes & PrivMode_BlinkingCursor))
    cursor_blink_ev.again ();
  else
    cursor_blink_ev.stop ();
}

void
rxvt_term::cursor_blink_cb (ev::timer &w, int revents)
{
  hidden_cursor = !hidden_cursor;
  want_refresh = 1;
  refresh_check ();
}
#endif

#ifdef TEXT_BLINK
void
rxvt_term::text_blink_cb (ev::timer &w, int revents)
{
  if (scr_refresh_rend (RS_Blink, RS_Blink))
    {
      hidden_text = !hidden_text;
      want_refresh = 1;
      refresh_check ();
    }
  else
    w.stop ();
}
#endif

#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
void
rxvt_term::cont_scroll_cb (ev::timer &w, int revents)
{
  if ((scrollBar.state == SB_STATE_UP || scrollBar.state == SB_STATE_DOWN)
      && scr_page (scrollBar.state == SB_STATE_UP ? UP : DN, 1))
    {
      want_refresh = 1;
      refresh_check ();
    }
  else
    w.stop ();
}
#endif

#ifdef SELECTION_SCROLLING
void
rxvt_term::sel_scroll_cb (ev::timer &w, int revents)
{
  if (scr_page (scroll_selection_lines))
    {
      selection_extend (selection_save_x, selection_save_y, selection_save_state);
      want_refresh = 1;
      refresh_check ();
    }
  else
    w.stop ();
}
#endif

#if defined(MOUSE_WHEEL) && defined(MOUSE_SLIP_WHEELING)
void
rxvt_term::slip_wheel_cb (ev::timer &w, int revents)
{
  if (scr_page (mouse_slip_wheel_speed))
    {
      want_refresh = 1;
      refresh_check ();
    }

  if (view_start == top_row || view_start == 0 || mouse_slip_wheel_speed == 0)
    {
      mouse_slip_wheel_speed = 0;
      w.stop ();
    }
}
#endif

#if LINUX_YIELD_HACK
static struct event_handler
{
  ev::prepare yield_ev;

  void yield_cb (ev::prepare &w, int revents)
  {
    // this should really be sched_yield(), but the linux guys thought
    // that giving a process calling sched_yield () less cpu time than
    // ones with high nice levels is a useful thing to do. It surely is is
    // allowed by the sus... as is returning ENOSYS.
    // since the linux guys additionally thought that breaking the only
    // known workaround against their unusable sched_yield hack is cool,
    // we just nanosleep a bit and hope for the best.

    struct timespec ts = { 0, 1000 };
    nanosleep (&ts, 0);

    w.stop ();
  }

  event_handler ()
  : yield_ev (this, &event_handler::yield_cb)
  {
  }
} event_handler;
#endif

/* make sure all the cmd data is at beginning of cmdbuf */
void
rxvt_term::cmdbuf_reify ()
{
  if (cmdbuf_ptr == cmdbuf_base)
    return;

  ssize_t used = cmdbuf_endp - cmdbuf_ptr;

  memmove (cmdbuf_base, cmdbuf_ptr, used);
  cmdbuf_ptr  = cmdbuf_base;
  cmdbuf_endp = cmdbuf_ptr + used;

}

#if defined (KEYSYM_RESOURCE)
void
rxvt_term::cmdbuf_append (const char *str, size_t count)
{
  cmdbuf_reify ();

  size_t avail = cmdbuf_base + CBUFSIZ - cmdbuf_endp;

  if (count > avail)
    return;

  memcpy (cmdbuf_endp, str, count);
  cmdbuf_endp += count;

  cmd_parse ();
}
#endif

bool
rxvt_term::pty_fill ()
{
  cmdbuf_reify ();

  if (CBUFSIZ == n)
    {
      rxvt_warn ("pty_fill on full buffer, draining input, continuing.\n");
      n = 0;
    }

  memmove (cmdbuf_base, cmdbuf_ptr, n);
  cmdbuf_ptr = cmdbuf_base;
  cmdbuf_endp = cmdbuf_ptr + n;

  n = read (cmd_fd, cmdbuf_endp, CBUFSIZ - n);

  if (n > 0)
    {
      // normally this indicates a "too long" command sequence - just drop the data we have
      cmdbuf_ptr  = cmdbuf_base;
      cmdbuf_endp = cmdbuf_ptr;
      avail       = CBUFSIZ;
    }

  ssize_t r = read (pty->pty, cmdbuf_endp, avail);

  if (r > 0)
    {
      cmdbuf_endp += r;
      return true;
    }
  else if (n < 0 && errno != EAGAIN)
    destroy ();
  
  return false;
}

void
rxvt_term::pty_cb (ev::io &w, int revents)
{
  make_current ();

  if (revents & EVENT_WRITE)
    tt_write (0, 0);
  else if (revents & EVENT_READ)
    // loop, but don't allow a single term to monopolize us
    while (pty_fill ())
      if (cmd_parse ())
        break;
}

#ifdef POINTER_BLANK
void ecb_cold
rxvt_term::pointer_blank ()
{
  if (!option (Opt_pointerBlank))
    return;

  XDefineCursor (dpy, vt, display->blank_cursor);
  XFlush (dpy);

  hidden_pointer = 1;
}

void ecb_cold
rxvt_term::pointer_cb (ev::timer &w, int revents)
{
  make_current ();

  pointer_blank ();
}
#endif

void
rxvt_term::mouse_report (XButtonEvent &ev)
{
  int button_number, key_state = 0;
  int x, y;

  x = 1 + clamp (Pixel2Col (ev.x), 0, ncol - 1);
  y = 1 + clamp (Pixel2Row (ev.y), 0, nrow - 1);

  if (ev.type == MotionNotify)
    {
      if (x == mouse_row && y == mouse_col)
        return;

      mouse_row = x;
      mouse_col = y;
      state += 32;
    }

  if (MEvent.button == AnyButton)
    button_number = 3;
  else
    {
      button_number = MEvent.button - Button1;
      if (button_number > 10)
        return;
      else if (button_number >= 7)
        button_number += 128 - 7;
      else if (button_number >= 3)
        button_number += 64 - 3;
    }

  if (priv_modes & PrivMode_MouseX10)
    {
      /*
       * do not report ButtonRelease
       * no state info allowed
       */
      if (release)
        return;
    }
  else
    {
      /* XTerm mouse reporting needs these values:
       *   4 = Shift
       *   8 = Meta
       *  16 = Control
       * plus will add in our own Double-Click reporting
       *  32 = Double Click
       */
      state += ((MEvent.state & ShiftMask) ? 4 : 0)
               + ((MEvent.state & ModMetaMask) ? 8 : 0)
               + ((MEvent.state & ControlMask) ? 16 : 0);
#ifdef MOUSE_REPORT_DOUBLECLICK
      state += ((MEvent.clicks > 1) ? 32 : 0);
#endif
    }

  int code = 32 + (release ? 3 : button_number) + state;

#if DEBUG_MOUSEREPORT
  fprintf (stderr, "Mouse [");
  if (state & 16)
    fputc ('C', stderr);
  if (state & 4)
    fputc ('S', stderr);
  if (state & 8)
    fputc ('A', stderr);
  if (state & 32)
    fputc ('2', stderr);
  fprintf (stderr, "]: <%d>, %d/%d\n",
          button_number,
          x,
          y);
#endif

#if ENABLE_FRILLS
  if (priv_modes & PrivMode_ExtMouseSGR)
    tt_printf ("\033[<%d;%d;%d%c",
              button_number + state,
              x,
              y,
              release ? 'm' : 'M');
  else if (priv_modes & PrivMode_ExtMouseUrxvt)
    tt_printf ("\033[%d;%d;%dM",
              code,
              x,
              y);
  else if (priv_modes & PrivMode_ExtMouseUTF8)
    if (code < 128)
      tt_printf ("\033[M%c%lc%lc",
                code,
                wint_t (32 + x),
                wint_t (32 + y));
    else
      tt_printf ("\033[M%c%c%lc%lc",
                0xc0 + (code >> 6),
                0x80 + (code & 0x3f),
                wint_t (32 + x),
                wint_t (32 + y));
  else
#endif
    tt_printf ("\033[M%c%c%c",
              code,
              32 + x,
              32 + y);
}

/*{{{ process an X event */
void ecb_hot
rxvt_term::x_cb (XEvent &ev)
{
  make_current ();

  dLocal (Display *, dpy);

  if (ev.xany.window == vt
      && SHOULD_INVOKE (HOOK_X_EVENT)
      && HOOK_INVOKE ((this, HOOK_X_EVENT, DT_XEVENT, &ev, DT_END)))
    return;

  // for XQueryPointer
  Window unused_root, unused_child;
  int unused_root_x, unused_root_y;
  unsigned int unused_mask;

  switch (ev.type)
    {
      case KeyPress:
        key_press (ev.xkey);
        break;

      case KeyRelease:
        key_release (ev.xkey);
        break;

      case ButtonPress:
        button_press (ev.xbutton);
        break;

      case ButtonRelease:
        button_release (ev.xbutton);
        break;

      case ClientMessage:
        if (ev.xclient.format == 32
            && !HOOK_INVOKE ((this, HOOK_CLIENT_MESSAGE, DT_XEVENT, &ev, DT_END)))
          {
            /* Get Dnd data */
            Atom ActualType;
            int ActualFormat;
            unsigned char *data;
            unsigned long Size, RemainingBytes;

            XGetWindowProperty (display->display, display->root,
                               xa[XA_DNDSELECTION],
                               0L, 1000000L,
                               False, AnyPropertyType,
                               &ActualType, &ActualFormat,
                               &Size, &RemainingBytes,
                               &data);
            XChangeProperty (display->display, display->root,
                            XA_CUT_BUFFER0, XA_STRING,
                            8, PropModeReplace,
                            data, STRLEN (data));
            XFree (data);
            selection_paste (display->root, XA_CUT_BUFFER0, True);
            XSetInputFocus (display->display, display->root, RevertToNone, CurrentTime);
          }
        break;

        /*
         * XXX: this is not the _current_ arrangement
         * Here's my conclusion:
         * If the window is completely unobscured, use bitblt's
         * to scroll. Even then, they're only used when doing partial
         * screen scrolling. When partially obscured, we have to fill
         * in the GraphicsExpose parts, which means that after each refresh,
         * we need to wait for the graphics expose or Noexpose events,
         * which ought to make things real slow!
         */
      case VisibilityNotify:
        switch (ev.xvisibility.state)
          {
            case VisibilityUnobscured:
              refresh_type = FAST_REFRESH;
              break;
            case VisibilityPartiallyObscured:
              refresh_type = SLOW_REFRESH;
              break;
            default:
              refresh_type = NO_REFRESH;
              break;
          }
        break;

      case FocusIn:
        if (ev.xfocus.detail != NotifyInferior
            && ev.xfocus.detail != NotifyPointer
            && ev.xfocus.mode != NotifyGrab)
          focus_in ();
        break;

      case FocusOut:
        if (ev.xfocus.detail != NotifyInferior
            && ev.xfocus.detail != NotifyPointer
            && ev.xfocus.mode != NotifyGrab)
          focus_out ();
        break;

      case ConfigureNotify:
        if (ev.xconfigure.window == parent)
          {
            while (XCheckTypedWindowEvent (dpy, ev.xconfigure.window, ConfigureNotify, &ev))
              ;

            bool want_position_change = SHOULD_INVOKE (HOOK_POSITION_CHANGE);

            if (want_position_change)
              {
                int x, y;

                if (ev.xconfigure.send_event)
                  {
                    x = ev.xconfigure.x;
                    y = ev.xconfigure.y;
                  }
                else
                  get_window_origin (x, y);

                if (x != parent_x || y != parent_y)
                  {
                    parent_x = x;
                    parent_y = y;
                    HOOK_INVOKE ((this, HOOK_POSITION_CHANGE, DT_INT, x, DT_INT, y, DT_END));
                  }
              }

            if (szHint.width != ev.xconfigure.width || szHint.height != ev.xconfigure.height)
              {
                seen_resize = 1;
                resize_all_windows (ev.xconfigure.width, ev.xconfigure.height, 1);
              }

            HOOK_INVOKE ((this, HOOK_CONFIGURE_NOTIFY, DT_XEVENT, &ev, DT_END));
          }
        break;

      case SelectionClear:
        selection_clear ();
        break;

      case SelectionClear:
        selection_clear (ev.xselectionclear.selection == xa[XA_CLIPBOARD]);
        break;

      case SelectionRequest:
        selection_send (ev.xselectionrequest);
        break;

      case MapNotify:
        mapped = 1;
#ifdef TEXT_BLINK
        text_blink_ev.start ();
#endif
        HOOK_INVOKE ((this, HOOK_MAP_NOTIFY, DT_XEVENT, &ev, DT_END));
        break;

      case UnmapNotify:
        mapped = 0;
#ifdef TEXT_BLINK
        text_blink_ev.stop ();
#endif
        HOOK_INVOKE ((this, HOOK_UNMAP_NOTIFY, DT_XEVENT, &ev, DT_END));
        break;

      case GraphicsExpose:
      case Expose:
        if (ev.xany.window == vt)
          {
            do
              {
                scr_expose (ev.xexpose.x, ev.xexpose.y,
                            ev.xexpose.width, ev.xexpose.height, false);
              }
            while (XCheckTypedWindowEvent (dpy, vt, ev.xany.type, &ev));

            ev.xany.type = ev.xany.type == Expose ? GraphicsExpose : Expose;

            while (XCheckTypedWindowEvent (dpy, vt, ev.xany.type, &ev))
              {
                scr_expose (ev.xexpose.x, ev.xexpose.y,
                            ev.xexpose.width, ev.xexpose.height, false);
              }

            want_refresh = 1;
          }
        else
          {
            XEvent unused_event;

            while (XCheckTypedWindowEvent (display->display, ev.xany.window,
                                          Expose,
                                          &unused_event)) ;
            while (XCheckTypedWindowEvent (display->display, ev.xany.window,
                                          GraphicsExpose,
                                          &unused_event)) ;
            if (isScrollbarWindow (ev.xany.window))
              {
                scrollBar.setIdle ();
                scrollbar_show (0);
              }
#ifdef MENUBAR
            if (menubar_visible () && isMenuBarWindow (ev.xany.window))
              menubar_expose ();
#endif
          }
        break;

      case MotionNotify:
#ifdef POINTER_BLANK
        if (hidden_pointer)
          pointer_unblank ();
#endif
#if MENUBAR
        if (isMenuBarWindow (ev.xany.window))
          {
            menubar_control (ev.xbutton);
            break;
          }
#endif
        if ((PrivateModes & PrivMode_mouse_report) && ! (bypass_keystate))
          break;

        if (ev.xany.window == vt)
          {
            if (SHOULD_INVOKE (HOOK_MOTION_NOTIFY)
                && HOOK_INVOKE ((this, HOOK_MOTION_NOTIFY, DT_XEVENT, &ev, DT_END)))
              ; // nop
            else if (ev.xbutton.state & (Button1Mask | Button3Mask))
              {
                while (XCheckTypedWindowEvent (dpy, vt, MotionNotify, &ev))
                  ;

                XQueryPointer (dpy, vt,
                               &unused_root, &unused_child,
                               &unused_root_x, &unused_root_y,
                               &ev.xbutton.x, &ev.xbutton.y,
                               &ev.xbutton.state);
#ifdef MOUSE_THRESHOLD
                /* deal with a `jumpy' mouse */
                if (ev.xmotion.time - MEvent.time > MOUSE_THRESHOLD)
#endif
                  {
#if ISO_14755
                    // 5.4
                    if (iso14755buf & (ISO_14755_STARTED | ISO_14755_54))
                      {
                        iso14755_54 (ev.xbutton.x, ev.xbutton.y);
                        break;
                      }
#endif
                    selection_extend (ev.xbutton.x, ev.xbutton.y,
                                      ev.xbutton.state & Button3Mask ? 2 : 0);

#ifdef SELECTION_SCROLLING
                    if (ev.xbutton.y < int_bwidth
                        || Pixel2Row (ev.xbutton.y) > (nrow-1))
                      {
                        page_dirn scroll_selection_dir;
                        int dist;

                        /* don't clobber the current delay if we are
                         * already in the middle of scrolling.
                         */
                        if (!sel_scroll_ev.is_active ())
                          sel_scroll_ev.start (SCROLLBAR_INITIAL_DELAY, SCROLLBAR_CONTINUOUS_DELAY);

                        /* save the event params so we can highlight
                         * the selection in the pending-scroll loop
                         */
                        selection_save_x = ev.xbutton.x;
                        selection_save_y = ev.xbutton.y;
                        selection_save_state = (ev.xbutton.state & Button3Mask) ? 2 : 0;

                        /* calc number of lines to scroll */
                        if (ev.xbutton.y < int_bwidth)
                          {
                            scroll_selection_dir = UP;
                            dist = int_bwidth - ev.xbutton.y;
                          }
                        else
                          {
                            scroll_selection_dir = DN;
                            dist = ev.xbutton.y - (int_bwidth + vt_height);
                          }

                        scroll_selection_lines = Pixel2Height (dist)
                                                 / SELECTION_SCROLL_LINE_SPEEDUP
                                                 + 1;
                        min_it (scroll_selection_lines,
                                SELECTION_SCROLL_MAX_LINES);
                        scroll_selection_lines *= scroll_selection_dir;
                      }
                    else
                      {
                        /* we are within the text window, so we
                         * shouldn't be scrolling
                         */
                        sel_scroll_ev.stop();
                      }
#endif
                  }
              }
          }
        else if (scrollBar.state == SB_STATE_MOTION && ev.xany.window == scrollBar.win)
          {
            while (XCheckTypedWindowEvent (dpy, scrollBar.win,
                                           MotionNotify, &ev))
              ;

            XQueryPointer (dpy, scrollBar.win,
                          &unused_root, &unused_child,
                          &unused_root_x, &unused_root_y,
                          &ev.xbutton.x, &ev.xbutton.y,
                          &unused_mask);
            scr_move_to (scrollBar.position (ev.xbutton.y) - csrO,
                         scrollBar.size ());
            want_refresh = 1;
            scrollBar.show (1);
          }
        break;
    }

#if defined(CURSOR_BLINK)
  if (ev.type == KeyPress)
    cursor_blink_reset ();
#endif

#if defined(POINTER_BLANK)
  if (option (Opt_pointerBlank) && pointerBlankDelay > 0)
    {
      if (ev.type == MotionNotify
          || ev.type == ButtonPress
          || ev.type == ButtonRelease)
        if (hidden_pointer)
          pointer_unblank ();

      if (ev.type == KeyPress && hidden_pointer == 0)
        pointer_blank ();
    }
#endif

  refresh_check ();
}

#if ENABLE_FRILLS
void ecb_cold
rxvt_term::set_urgency (bool enable)
{
  if (enable == urgency_hint)
    return;

  if (XWMHints *h = XGetWMHints (dpy, parent))
    {
      h->flags = h->flags & ~XUrgencyHint | (enable ? XUrgencyHint : 0);
      XSetWMHints (dpy, parent, h);
      urgency_hint = enable;
      XFree (h);
    }
}
#endif

void ecb_cold
rxvt_term::focus_in ()
{
  if (!focus)
    {
      focus = 1;
      want_refresh = 1;

#if USE_XIM
      if (Input_Context != NULL)
        {
          im_set_position ();
          XSetICFocus (Input_Context);
        }
#endif
#if CURSOR_BLINK
      if (option (Opt_cursorBlink))
        cursor_blink_ev.again ();
#endif
#if OFF_FOCUS_FADING
      if (rs[Rs_fade])
        {
          pix_colors = pix_colors_focused;
          scr_recolor ();
        }
#endif
#if ENABLE_FRILLS
      if (option (Opt_urgentOnBell))
        set_urgency (0);

      if (priv_modes & PrivMode_FocusEvent)
        tt_printf ("\x1b[I");
#endif

      HOOK_INVOKE ((this, HOOK_FOCUS_IN, DT_END));
    }
}

void ecb_cold
rxvt_term::focus_out ()
{
  if (focus)
    {
      focus = 0;
      want_refresh = 1;

#if ENABLE_FRILLS
      if (option (Opt_urgentOnBell))
        set_urgency (0);

      if (priv_modes & PrivMode_FocusEvent)
        tt_printf ("\x1b[O");
#endif
#if ENABLE_FRILLS || ISO_14755
      if (iso14755buf)
        {
          iso14755buf = 0;
# if ISO_14755
          scr_overlay_off ();
# endif
        }
#endif
#if USE_XIM
      if (Input_Context != NULL)
        XUnsetICFocus (Input_Context);
#endif
#if CURSOR_BLINK
      if (option (Opt_cursorBlink))
        cursor_blink_ev.stop ();

      hidden_cursor = 0;
#endif
#if OFF_FOCUS_FADING
      if (rs[Rs_fade])
        {
          pix_colors = pix_colors_unfocused;
          scr_recolor ();
        }
#endif

      HOOK_INVOKE ((this, HOOK_FOCUS_OUT, DT_END));
    }
}

void ecb_cold
rxvt_term::update_fade_color (unsigned int idx, bool first_time)
{
#if OFF_FOCUS_FADING
  if (rs[Rs_fade])
    {
      if (!first_time)
        pix_colors_focused [idx].free (this);

      rgba c;
      pix_colors [Color_fade].get (c);
      pix_colors_focused [idx].fade (this, atoi (rs[Rs_fade]), pix_colors_unfocused [idx], c);
    }
#endif
}

#if ENABLE_PERL
void ecb_hot
rxvt_term::rootwin_cb (XEvent &ev)
{
  make_current ();

  if (SHOULD_INVOKE (HOOK_ROOT_EVENT)
      && HOOK_INVOKE ((this, HOOK_ROOT_EVENT, DT_XEVENT, &ev, DT_END)))
    return;

  switch (ev.type)
    {
      case PropertyNotify:
        /*
         * if user used some Esetroot compatible prog to set the root bg,
         * use the property to determine the pixmap.  We use it later on.
         */
        if (ev.xproperty.atom == xa[XA_XROOTPMAP_ID]
            || ev.xproperty.atom == xa[XA_ESETROOT_PMAP_ID])
          {
            HOOK_INVOKE ((this, HOOK_ROOTPMAP_CHANGE, DT_END));
          }

        break;
    }

  refresh_check ();
}
#endif

void
rxvt_term::button_press (XButtonEvent &ev)
{
  int reportmode = 0, clickintime;

  bypass_keystate = ev.state & (ModMetaMask | ShiftMask);

  if (!bypass_keystate)
    reportmode = !! (priv_modes & PrivMode_mouse_report);

  /*
   * VT window processing of button press
   */
  if (ev.window == vt)
    {
      clickintime = ev.time - MEvent.time < MULTICLICK_TIME;
      if (reportmode)
        {
          /* mouse report from vt window */
          /* save the xbutton state (for ButtonRelease) */
          MEvent.state = ev.state;
#ifdef MOUSE_REPORT_DOUBLECLICK
          if (ev.button == MEvent.button && clickintime)
            {
              /* same button, within alloted time */
              MEvent.clicks++;
              if (MEvent.clicks > 1)
                {
                  /* only report double clicks */
                  MEvent.clicks = 2;
                  mouse_report (ev);

                  /* don't report the release */
                  MEvent.clicks = 0;
                  MEvent.button = AnyButton;
                }
            }
          else
            {
              /* different button, or time expired */
              MEvent.clicks = 1;
              MEvent.button = ev.button;
              mouse_report (ev);
            }
#else
          MEvent.button = ev.button;
          mouse_report (ev);
#endif				/* MOUSE_REPORT_DOUBLECLICK */

        }
      else
        {
          if (ev.button != MEvent.button)
            MEvent.clicks = 0;
          switch (ev.button)
            {
              case Button1:
                /* allow shift+left click to extend selection */
                if (ev.state & ShiftMask && ! (PrivateModes & PrivMode_mouse_report))
                  {
                    if (MEvent.button == Button1 && clickintime)
                      selection_rotate (ev.x, ev.y);
                    else
                      selection_extend (ev.x, ev.y, 1);
                  }
                else
                  {
                    if (MEvent.button == Button1 && clickintime)
                      MEvent.clicks++;
                    else
                      MEvent.clicks = 1;

                    selection_click (MEvent.clicks, ev.x, ev.y);
                  }

                MEvent.button = Button1;
                break;

              case Button3:
                if (MEvent.button == Button3 && clickintime)
                  selection_rotate (ev.x, ev.y);
                else
                  selection_extend (ev.x, ev.y, 1);
                MEvent.button = Button3;
                break;
            }
        }
      MEvent.time = ev.time;
      return;
    }

  /*
   * Scrollbar window processing of button press
   */
  if (scrollBar.state && ev.window == scrollBar.win)
    {
      page_dirn direction = NO_DIR;

      if (scrollBar.upButton (ev.y))
        direction = UP; /* up */
      else if (scrollBar.dnButton (ev.y))
        direction = DN;  /* down */

      scrollBar.state = SB_STATE_IDLE;
      /*
       * Rxvt-style scrollbar:
       * move up if mouse is above slider
       * move dn if mouse is below slider
       *
       * XTerm-style scrollbar:
       * Move display proportional to pointer location
       * pointer near top -> scroll one line
       * pointer near bot -> scroll full page
       */
#ifndef NO_SCROLLBAR_REPORT
      if (reportmode)
        {
          /*
           * Mouse report disabled scrollbar:
           * arrow buttons - send up/down
           * click on scrollbar - send pageup/down
           */
          if (direction == UP)
            tt_printf ("\033[A");
          else if (direction == DN)
            tt_printf ("\033[B");
          else
            switch (ev.button)
              {
                case Button2:
                  tt_printf ("\014");
                  break;
                case Button1:
                  tt_printf ("\033[6~");
                  break;
                case Button3:
                  tt_printf ("\033[5~");
                  break;
              }
        }
      else
#endif /* NO_SCROLLBAR_REPORT */
        {
          if (direction != NO_DIR)
            {
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
              if (!cont_scroll_ev.is_active ())
                cont_scroll_ev.start (SCROLLBAR_INITIAL_DELAY, SCROLLBAR_CONTINUOUS_DELAY);
#endif
              if (scr_page (direction, 1))
                {
                  if (direction == UP)
                    scrollBar.state = SB_STATE_UP;
                  else
                    scrollBar.state = SB_STATE_DOWN;
                }
            }
          else
            switch (ev.button)
              {
                case Button2:
                  switch (scrollBar.align)
                    {
                      case SB_ALIGN_TOP:
                        csrO = 0;
                        break;
                      case SB_ALIGN_CENTRE:
                        csrO = (scrollBar.bot - scrollBar.top) / 2;
                        break;
                      case SB_ALIGN_BOTTOM:
                        csrO = scrollBar.bot - scrollBar.top;
                        break;
                    }

                  if (scrollBar.style == SB_STYLE_XTERM
                      || scrollBar.above_slider (ev.y)
                      || scrollBar.below_slider (ev.y))
                    scr_move_to (scrollBar.position (ev.y) - csrO, scrollBar.size ());

                  scrollBar.state = SB_STATE_MOTION;
                  break;

                case Button1:
                  if (scrollBar.align == SB_ALIGN_CENTRE)
                    csrO = ev.y - scrollBar.top;
                  /* FALLTHROUGH */

                case Button3:
                  if (scrollBar.style != SB_STYLE_XTERM)
                    {
                      if (scrollBar.above_slider (ev.y))
# ifdef RXVT_SCROLL_FULL
                        scr_page (UP, nrow - 1);
# else
                        scr_page (UP, nrow / 4);
# endif
                      else if (scrollBar.below_slider (ev.y))
# ifdef RXVT_SCROLL_FULL
                        scr_page (DN, nrow - 1);
# else
                        scr_page (DN, nrow / 4);
# endif
                      else
                        scrollBar.state = SB_STATE_MOTION;
                    }
                  else
                    {
                      scr_page ((ev.button == Button1 ? DN : UP),
                                (nrow
                                 * scrollBar.position (ev.y)
                                 / scrollBar.size ()));
                    }

                  break;
              }
        }

      return;
    }
}

void
rxvt_term::button_release (XButtonEvent &ev)
{
  int reportmode = 0;

  csrO = 0;		/* reset csr Offset */
  if (!bypass_keystate)
    reportmode = !! (priv_modes & PrivMode_mouse_report);

  if (scrollBar.state == SB_STATE_UP || scrollBar.state == SB_STATE_DOWN)
    {
      scrollBar.state = SB_STATE_IDLE;
      scrollBar.show (0);
    }

#ifdef SELECTION_SCROLLING
  pending_scroll_selection=0;
#endif
  if (ev.window == TermWin.vt)
    {
      if (reportmode)
        {
          /* mouse report from vt window */
          /* don't report release of wheel "buttons" */
          if (ev.button >= 4)
            return;
#ifdef MOUSE_REPORT_DOUBLECLICK
          /* only report the release of 'slow' single clicks */
          if (MEvent.button != AnyButton
              && (ev.button != MEvent.button
                  || (ev.time - MEvent.time
                      > MULTICLICK_TIME / 2)))
            {
              MEvent.clicks = 0;
              MEvent.button = AnyButton;
              mouse_report (ev);
            }
#else				/* MOUSE_REPORT_DOUBLECLICK */
          MEvent.button = AnyButton;
          mouse_report (ev);
#endif				/* MOUSE_REPORT_DOUBLECLICK */
          return;
        }
      /*
       * dumb hack to compensate for the failure of click-and-drag
       * when overriding mouse reporting
       */
      if (PrivateModes & PrivMode_mouse_report
          && bypass_keystate
          && ev.button == Button1 && MEvent.clicks <= 1)
        selection_extend (ev.x, ev.y, 0);

      switch (ev.button)
        {
          case Button1:
          case Button3:
            selection_make (ev.time);
            break;
          case Button2:
            selection_request (ev.time, ev.x, ev.y);
            break;
#ifdef MOUSE_WHEEL
          case Button4:
          case Button5:
            {
              int i;
              page_dirn v;

              v = (ev.button == Button4) ? UP : DN;
              if (ev.state & ShiftMask)
                i = 1;
              else if ((Options & Opt_mouseWheelScrollPage))
                i = TermWin.nrow - 1;
              else
                i = 5;
# ifdef MOUSE_SLIP_WHEELING
              if (ev.state & ControlMask)
                {
                  mouse_slip_wheel_speed += (v ? -1 : 1);
                  mouse_slip_wheel_delay = SCROLLBAR_CONTINUOUS_DELAY;
                }
# endif
# ifdef JUMP_MOUSE_WHEEL
              scr_page (v, i);
              scr_refresh (SMOOTH_REFRESH);
              scrollbar_show (1);
# else
              while (i--)
                {
                  scr_page (v, 1);
                  scr_refresh (SMOOTH_REFRESH);
                  scrollbar_show (1);
                }
# endif

            }
            break;
#endif

        }
    }
#ifdef MENUBAR
  else if (isMenuBarWindow (ev.window))
    menubar_control (ev);
#endif
}


#ifdef TRANSPARENT
/*
 * Check our parents are still who we think they are.
 * Do transparency updates if required
 */
int
rxvt_term::check_our_parents ()
{
  int             i, pchanged, aformat, have_pixmap, rootdepth;
  unsigned long   nitems, bytes_after;
  Atom            atype;
  unsigned char   *prop = NULL;
  Window          root, oldp, *list;
  Pixmap          rootpixmap = None;
  XWindowAttributes wattr, wrootattr;

  pchanged = 0;

  if (! (Options & Opt_transparent))
    return pchanged;	/* Don't try any more */

  XGetWindowAttributes (display->display, display->root, &wrootattr);
  rootdepth = wrootattr.depth;

  XGetWindowAttributes (display->display, TermWin.parent[0], &wattr);
  if (rootdepth != wattr.depth)
    {
      if (am_transparent)
        {
          pchanged = 1;
          XSetWindowBackground (display->display, TermWin.vt,
                               PixColors[Color_bg]);
          am_transparent = am_pixmap_trans = 0;
        }
      return pchanged;	/* Don't try any more */
    }

  /* Get all X ops out of the queue so that our information is up-to-date. */
  XSync (display->display, False);

  /*
   * Make the frame window set by the window manager have
   * the root background. Some window managers put multiple nested frame
   * windows for each client, so we have to take care about that.
   */
  i = (xa[XA_XROOTPMAPID] != 0
       && (XGetWindowProperty (display->display, display->root, xa[XA_XROOTPMAPID],
                              0L, 1L, False, XA_PIXMAP, &atype, &aformat,
                              &nitems, &bytes_after, &prop) == Success));
  if (!i || prop == NULL)
    have_pixmap = 0;
  else
    {
      have_pixmap = 1;
      rootpixmap = *(Pixmap *)prop;
      XFree (prop);
    }
  if (have_pixmap)
    {
      /*
       * Copy display->root pixmap transparency
       */
      int             sx, sy, nx, ny;
      unsigned int    nw, nh;
      Window          cr;
      XImage         *image;
      GC              gc;
      XGCValues       gcvalue;

      XTranslateCoordinates (display->display, TermWin.parent[0], display->root,
                            0, 0, &sx, &sy, &cr);
      nw = (unsigned int)szHint.width;
      nh = (unsigned int)szHint.height;
      nx = ny = 0;
      if (sx < 0)
        {
          nw += sx;
          nx = -sx;
          sx = 0;
        }
      if (sy < 0)
        {
          nh += sy;
          ny = -sy;
          sy = 0;
        }
      MIN_IT (nw, (unsigned int) (wrootattr.width - sx));
      MIN_IT (nh, (unsigned int) (wrootattr.height - sy));
      allowedxerror = -1;
      image = XGetImage (display->display, rootpixmap, sx, sy, nw, nh, AllPlanes,
                        ZPixmap);
      /* XXX: handle BadMatch - usually because we're outside the pixmap */
      /* XXX: may need a delay here? */
      allowedxerror = 0;
      if (image == NULL)
        {
          if (am_transparent && am_pixmap_trans)
            {
              pchanged = 1;
              if (TermWin.pixmap != None)
                {
                  mouse_slip_wheel_speed += dirn;
                  clamp_it (mouse_slip_wheel_speed, -nrow, nrow);

                  if (!slip_wheel_ev.is_active ())
                    slip_wheel_ev.start (SCROLLBAR_CONTINUOUS_DELAY, SCROLLBAR_CONTINUOUS_DELAY);
                }
              else
# endif
                {
                  scr_page (dirn, lines);
                  scrollBar.show (1);
                }
            }
            break;
#endif
        }
    }
}

/*}}} */

bool
rxvt_term::cmd_parse ()
{
  bool flag = false;
  unicode_t ch = NOCHAR;
  unsigned char *seq_begin; // remember start of esc-sequence here

  for (;;)
    {
      if (ch == NOCHAR)
        {
          seq_begin = cmdbuf_ptr;
          ch = next_char ();
        }

      if (ch == NOCHAR) // TODO: improve
        break;

      if (!IS_CONTROL (ch) || ch == C0_LF || ch == C0_CR || ch == C0_HT)
        {
          /* Read a text string from the input buffer */
          unicode_t buf[UBUFSIZ];
          bool refreshnow = false;
          int nlines = 0;
          unicode_t *str = buf;

          *str++ = ch;

          for (;;)
            {
              seq_begin = cmdbuf_ptr;
              ch = next_char ();

              if (ch == NOCHAR || (IS_CONTROL (ch) && ch != C0_LF && ch != C0_CR && ch != C0_HT))
                break;

              *str++ = ch;

              if (ch == C0_LF)
                {
                  nlines++;
                  refresh_count++;

                  if (! (Options & Opt_jumpScroll)
                      || (refresh_count >= (refresh_limit * (TermWin.nrow - 1))))
                    {
                      refreshnow = true;
                      flag = true;
                      ch = NOCHAR;
                      break;
                    }

                  // scr_add_lines only works for nlines < TermWin.nrow - 1.
                  if (nlines >= TermWin.nrow - 1)
                    {
                      scr_add_lines (buf, nlines, str - buf);
                      nlines = 0;
                      str = buf;
                    }
                }

              if (str >= buf + UBUFSIZ)
                {
                  ch = NOCHAR;
                  break;
                }
            }

          scr_add_lines (buf, nlines, str - buf);

          /*
           * If there have been a lot of new lines, then update the screen
           * What the heck I'll cheat and only refresh less than every page-full.
           * the number of pages between refreshes is refresh_limit, which
           * is incremented here because we must be doing flat-out scrolling.
           *
           * refreshing should be correct for small scrolls, because of the
           * time-out
           */
          if (refreshnow)
            {
              if ((Options & Opt_jumpScroll) && refresh_limit < REFRESH_PERIOD)
                refresh_limit++;
              else
                scr_refresh (refresh_type);
            }

        }
      else
        {
          try
            {
              process_nonprinting (ch);
            }
          catch (const class out_of_input &o)
            {
              // we ran out of input, retry later
              cmdbuf_ptr = seq_begin;
              break;
            }

          ch = NOCHAR;
        }
    }

  return flag;
}

// read the next character
unicode_t
rxvt_term::next_char ()
{
  while (cmdbuf_ptr < cmdbuf_endp)
    {
      // assume 7-bit to be ascii ALWAYS
      if (*cmdbuf_ptr <= 0x7f && *cmdbuf_ptr != 0x1b)
        return *cmdbuf_ptr++;

      wchar_t wc;
      size_t len = mbrtowc (&wc, (char *)cmdbuf_ptr, cmdbuf_endp - cmdbuf_ptr, mbstate);

      if (len == (size_t)-2)
        {
          // the mbstate stores incomplete sequences. didn't know this :/
          cmdbuf_ptr = cmdbuf_endp;
          break;
        }

      if (len == (size_t)-1)
        return *cmdbuf_ptr++; // the _occasional_ latin1 character is allowed to slip through

      // assume wchar == unicode
      cmdbuf_ptr += len;
      return wc;
    }

  return NOCHAR;
}

/* rxvt_cmd_getc () - Return next input character */
/*
 * Return the next input character after first passing any keyboard input
 * to the command.
 */
unicode_t
rxvt_term::cmd_getc ()
{
  unicode_t c = next_char ();

  if (c == NOCHAR)
    throw out_of_input;

  return c;
}

/*{{{ print pipe */
/*----------------------------------------------------------------------*/
#ifdef PRINTPIPE
FILE * ecb_cold
rxvt_term::popen_printer ()
{
  FILE *stream = popen (rs[Rs_print_pipe] ? rs[Rs_print_pipe] : PRINTPIPE, "w");

  if (stream == NULL)
    rxvt_warn ("can't open printer pipe, not printing.\n");

  return stream;
}

int ecb_cold
rxvt_term::pclose_printer (FILE *stream)
{
  fflush (stream);
  return pclose (stream);
}

/*
 * simulate attached vt100 printer
 */
void ecb_cold
rxvt_term::process_print_pipe ()
{
  int done;
  FILE *fd;

  if (!fd)
    return;

  /*
   * Send all input to the printer until either ESC[4i or ESC[?4i
   * is received.
   */
  for (int done = 0; !done; )
    {
      unsigned char buf[8];
      unicode_t ch;
      unsigned int i, len;

      if ((ch = cmd_getc ()) != C0_ESC)
        {
          if (putc (ch, fd) == EOF)
            break;		/* done = 1 */
        }
      else
        {
          len = 0;
          buf[len++] = ch;

          if ((buf[len++] = cmd_getc ()) == '[')
            {
              if ((ch = cmd_getc ()) == '?')
                {
                  buf[len++] = '?';
                  ch = cmd_getc ();
                }
              if ((buf[len++] = ch) == '4')
                {
                  if ((buf[len++] = cmd_getc ()) == 'i')
                    break;	/* done = 1 */
                }
            }
          
          for (i = 0; i < len; i++)
            if (putc (buf[i], fd) == EOF)
              {
                done = 1;
                break;
              }
        }
    }

  pclose_printer (fd);
}
#endif /* PRINTPIPE */
/*}}} */

enum {
  C1_40 = 0x40,
          C1_41 , C1_BPH, C1_NBH, C1_44 , C1_NEL, C1_SSA, C1_ESA,
  C1_HTS, C1_HTJ, C1_VTS, C1_PLD, C1_PLU, C1_RI , C1_SS2, C1_SS3,
  C1_DCS, C1_PU1, C1_PU2, C1_STS, C1_CCH, C1_MW , C1_SPA, C1_EPA,
  C1_SOS, C1_59 , C1_SCI, C1_CSI, CS_ST , C1_OSC, C1_PM , C1_APC,
};

/*{{{ process non-printing single characters */
void
rxvt_term::process_nonprinting (unicode_t ch)
{
  switch (ch)
    {
      case C0_ESC:
        process_escape_seq ();
        break;
      case C0_ENQ:	/* terminal Status */
        if (rs[Rs_answerbackstring])
          tt_write (rs [Rs_answerbackstring], strlen (rs [Rs_answerbackstring]));
        else
          tt_write (VT100_ANS, strlen (VT100_ANS));
        break;
      case C0_BEL:	/* bell */
        scr_bell ();
        break;
      case C0_BS:		/* backspace */
        scr_backspace ();
        break;
      case C0_HT:		/* tab */
        scr_tab (1);
        break;
      case C0_CR:		/* carriage return */
        scr_gotorc (0, 0, R_RELATIVE);
        break;
      case C0_VT:		/* vertical tab, form feed */
      case C0_FF:
      case C0_LF:		/* line feed */
        scr_index (UP);
        break;
      case C0_SO:		/* shift out - acs */
        scr_charset_choose (1);
        break;
      case C0_SI:		/* shift in - acs */
        scr_charset_choose (0);
        break;

      // 8-bit controls
      case 0x90: 	/* DCS */
        process_dcs_seq ();
        break;
      case 0x9b: 	/* CSI */
        process_csi_seq ();
        break;
      case 0x9d: 	/* CSI */
        process_osc_seq ();
        break;
    }
}
/*}}} */


/*{{{ process VT52 escape sequences */
void
rxvt_term::process_escape_vt52 (unicode_t ch)
{
  int row, col;

  switch (ch)
    {
      case 'A':		/* cursor up */
        scr_gotorc (-1, 0, R_RELATIVE | C_RELATIVE);
        break;
      case 'B':		/* cursor down */
        scr_gotorc (1, 0, R_RELATIVE | C_RELATIVE);
        break;
      case 'C':		/* cursor right */
        scr_gotorc (0, 1, R_RELATIVE | C_RELATIVE);
        break;
      case 'D':		/* cursor left */
        scr_gotorc (0, -1, R_RELATIVE | C_RELATIVE);
        break;
      case 'H':		/* cursor home */
        scr_gotorc (0, 0, 0);
        break;
      case 'I':		/* cursor up and scroll down if needed */
        scr_index (DN);
        break;
      case 'J':		/* erase to end of screen */
        scr_erase_screen (0);
        break;
      case 'K':		/* erase to end of line */
        scr_erase_line (0);
        break;
      case 'Y':         	/* move to specified row and col */
        /* full command is 'ESC Y row col' where row and col
         * are encoded by adding 32 and sending the ascii
         * character.  eg. SPACE = 0, '+' = 13, '0' = 18,
         * etc. */
        row = cmd_getc () - ' ';
        col = cmd_getc () - ' ';
        scr_gotorc (row, col, 0);
        break;
      case 'Z':		/* identify the terminal type */
        tt_printf ("\033/Z");	/* I am a VT100 emulating a VT52 */
        break;
      case '<':		/* turn off VT52 mode */
        priv_modes &= ~PrivMode_vt52;
        break;
      case 'F':     	/* use special graphics character set */
      case 'G':           /* use regular character set */
        /* unimplemented */
        break;
      case '=':     	/* use alternate keypad mode */
      case '>':           /* use regular keypad mode */
        /* unimplemented */
        break;
    }
}
/*}}} */


/*{{{ process escape sequences */
void ecb_hot
rxvt_term::process_escape_seq ()
{
  unicode_t ch = cmd_getc ();

  if (priv_modes & PrivMode_vt52)
    {
      process_escape_vt52 (ch);
      return;
    }

  switch (ch)
    {
      case '#':
        if (cmd_getc () == '8')
          scr_E ();
        break;
      case '(':
        scr_charset_set (0, (unsigned int)cmd_getc ());
        break;
      case ')':
        scr_charset_set (1, (unsigned int)cmd_getc ());
        break;
      case '*':
        scr_charset_set (2, (unsigned int)cmd_getc ());
        break;
      case '+':
        scr_charset_set (3, (unsigned int)cmd_getc ());
        break;
#if !ENABLE_MINIMAL
      case '6':
        scr_backindex ();
        break;
#endif
      case '7':
        scr_cursor (SAVE);
        break;
      case '8':
        scr_cursor (RESTORE);
        break;
#if !ENABLE_MINIMAL
      case '9':
        scr_forwardindex ();
        break;
#endif
      // DECPAM/DECPNM
      case '=':
        priv_modes |= PrivMode_aplKP;
        break;
      case '>':
        priv_modes &= ~PrivMode_aplKP;
        break;

      case C1_40:
        cmd_getc ();
        break;
      case C1_44:
        scr_index (UP);
        break;

        /* 8.3.87: NEXT LINE */
      case C1_NEL:		/* ESC E */
        {
          unicode_t nlcr[] = { C0_LF, C0_CR };
          scr_add_lines (nlcr, 1, 2);
        }
        break;

#if 0 // disabled because embedded newlines can make exploits easier
        /* kidnapped escape sequence: Should be 8.3.48 */
      case C1_ESA:		/* ESC G */
        // used by original rxvt for rob nations own graphics mode
        if (cmd_getc () == 'Q' && option (Opt_insecure))
          tt_printf ("\033G0\012");	/* query graphics - no graphics */
        break;
#endif

        /* 8.3.63: CHARACTER TABULATION SET */
      case C1_HTS:		/* ESC H */
        scr_set_tab (1);
        break;

        /* 8.3.105: REVERSE LINE FEED */
      case C1_RI:			/* ESC M */
        scr_index (DN);
        break;

        /* 8.3.142: SINGLE-SHIFT TWO */
      /* case C1_SS2: break; */

        /* 8.3.143: SINGLE-SHIFT THREE */
      /* case C1_SS3: break; */

        /* 8.3.27: DEVICE CONTROL STRING */
      case C1_DCS:		/* ESC P */
        process_dcs_seq ();
        break;

        /* 8.3.110: SINGLE CHARACTER INTRODUCER */
      case C1_SCI:		/* ESC Z */
        tt_write (ESCZ_ANSWER, sizeof (ESCZ_ANSWER) - 1);
        break;			/* steal obsolete ESC [ c */

        /* 8.3.16: CONTROL SEQUENCE INTRODUCER (CSI) */
      case C1_CSI:		/* ESC [ */
        process_csi_seq ();
        break;

        /* 8.3.90: OPERATING SYSTEM COMMAND (OSC) */
      case C1_OSC:		/* ESC ] */
        process_osc_seq ();
        break;

        /* 8.3.106: RESET TO INITIAL STATE (RIS) */
      case 'c':
        mbstate.reset ();
        scr_poweron ();
        scrollBar.show (1);
        break;

        /* 8.3.79: LOCKING-SHIFT TWO (see ISO2022) */
      case 'n':
        scr_charset_choose (2);
        break;

        /* 8.3.81: LOCKING-SHIFT THREE (see ISO2022) */
      case 'o':
        scr_charset_choose (3);
        break;
    }
}
/*}}} */

/*{{{ process CONTROL SEQUENCE INTRODUCER (CSI) sequences `ESC[' */
enum {
  CSI_ICH = 0x40,
           CSI_CUU, CSI_CUD, CSI_CUF, CSI_CUB, CSI_CNL, CSI_CPL, CSI_CHA,
  CSI_CUP, CSI_CHT, CSI_ED , CSI_EL , CSI_IL , CSI_DL , CSI_EF , CSI_EA ,
  CSI_DCH, CSI_SEE, CSI_CPR, CSI_SU , CSI_SD , CSI_NP , CSI_PP , CSI_CTC,
  CSI_ECH, CSI_CVT, CSI_CBT, CSI_SRS, CSI_PTX, CSI_SDS, CSI_SIMD, CSI_5F,
  CSI_HPA, CSI_HPR, CSI_REP, CSI_DA , CSI_VPA, CSI_VPR, CSI_HVP, CSI_TBC,
  CSI_SM , CSI_MC , CSI_HPB, CSI_VPB, CSI_RM , CSI_SGR, CSI_DSR, CSI_DAQ,
  CSI_70 , CSI_71 , CSI_72 , CSI_73 , CSI_74 , CSI_75 , CSI_76 , CSI_77 ,
  CSI_78 , CSI_79 , CSI_7A , CSI_7B , CSI_7C , CSI_7D , CSI_7E , CSI_7F
};

#define make_byte(b0,b1,b2,b3,b4,b5,b6,b7)			\
    (((b7) << 7) | ((b6) << 6) | ((b5) << 5) | ((b4) << 4)	\
     | ((b3) << 3) | ((b2) << 2) | ((b1) << 1) | (b0))
#define get_byte_array_bit(array, bit)				\
    (!! ((array)[(bit) >> 3] & (1 << ((bit) & 7))))

static const unsigned char csi_defaults[] =
  {
    make_byte (1,1,1,1,1,1,1,1),	/* @, A, B, C, D, E, F, G, */
    make_byte (1,1,0,0,1,1,0,0),	/* H, I, J, K, L, M, N, O, */
    make_byte (1,0,1,1,1,1,1,0),	/* P, Q, R, S, T, U, V, W, */
    make_byte (1,1,1,0,0,0,1,0),	/* X, Y, Z, [, \, ], ^, _, */
    make_byte (1,1,1,0,1,1,1,0),	/* `, a, b, c, d, e, f, g, */
    make_byte (0,0,1,1,0,0,0,0),	/* h, i, j, k, l, m, n, o, */
    make_byte (0,0,0,0,0,0,0,0),	/* p, q, r, s, t, u, v, w, */
    make_byte (0,0,0,0,0,0,0,0),	/* x, y, z, {, |, }, ~,    */
  };

void ecb_hot
rxvt_term::process_csi_seq ()
{
  unicode_t ch, priv, i;
  unsigned int nargs, p;
  int n, ndef;
  int arg[ESC_ARGS];

  nargs = 0;

  priv = 0;
  ch = cmd_getc ();
  if ((ch >= '<' && ch <= '?') || ch == '!')
    {
      /* '<' '=' '>' '?' '!' */
      priv = ch;
      ch = cmd_getc ();
    }

  /* read any numerical arguments */
  for (n = -1; ch < CSI_ICH; )
    {
      if (isdigit (ch))
        {
          if (n < 0)
            n = ch - '0';
          else
            n = n * 10 + ch - '0';
        }
      else if (ch == ';')
        {
          if (nargs < ESC_ARGS)
            arg[nargs++] = n;
          n = -1;
        }
      else if (IS_CONTROL (ch))
        process_nonprinting (ch);

      ch = cmd_getc ();
    }

  if (ch > CSI_7F)
    return;

  if (nargs < ESC_ARGS)
    arg[nargs++] = n;

  i = ch - CSI_ICH;
  ndef = get_byte_array_bit (csi_defaults, i);
  for (p = 0; p < nargs; p++)
    if (arg[p] == -1)
      arg[p] = ndef;

  /*
   * private mode handling
   */
  if (priv)
    {
      switch (priv)
        {
          case '>':
            if (ch == CSI_DA)	/* secondary device attributes */
              {
                // first parameter is normally 0 for vt100, 1 for vt220, 'R' for rxvt,
                // 'U' for rxvt-unicode != 7.[34] (where it was broken).
                //
                // second parameter is xterm patch level for xterm, MMmmpp (e.g. 20703) for rxvt
                // and Mm (e.g. 72 for 7.2) for urxvt <= 7.2, 94 for urxvt <= 8.3, and 95 for later
                // versions.
                //
                tt_printf ("\033[>%d;95;0c", 'U');
              }
            break;

          case '?':
            if (ch == 'h' || ch == 'l' || ch == 'r' || ch == 's' || ch == 't')
              process_terminal_mode (ch, priv, nargs, arg);
            if (prev_ch == '$' && ch == 'p')
              process_terminal_mode (ch, priv, nargs, arg);
            break;

          case '!':
            if (ch == CSI_70)
              {
                /* DECSTR: soft terminal reset, used by our terminfo since 9.06 */
                scr_soft_reset ();

                static const int pm_h[] = { 7, 25 };
                static const int pm_l[] = { 1, 3, 4, 5, 6, 9, 66, 1000, 1001, 1005, 1006, 1015, 1049 };

                process_terminal_mode ('h', 0, ecb_array_length (pm_h), pm_h);
                process_terminal_mode ('l', 0, ecb_array_length (pm_l), pm_l);
              }
          break;
        }

      return;
    }

  switch (ch)
    {
        /*
         * ISO/IEC 6429:1992 (E) CSI sequences (defaults in parentheses)
         */
#ifdef PRINTPIPE
      case CSI_MC:		/* 8.3.83: (0) MEDIA COPY */
        switch (arg[0])
          {
            case 0:			/* initiate transfer to primary aux device */
              scr_printscreen (0);
              break;
            case 5:			/* start relay to primary aux device */
              process_print_pipe ();
              break;
          }
        break;
#endif

      case CSI_CUU:		/* 8.3.22: (1) CURSOR UP */
      case CSI_VPB:		/* 8.3.160: (1) LINE POSITION BACKWARD */
        arg[0] = -arg[0];
        /* FALLTHROUGH */
      case CSI_CUD:		/* 8.3.19: (1) CURSOR DOWN */
      case CSI_VPR:		/* 8.3.161: (1) LINE POSITION FORWARD */
        scr_gotorc (arg[0], 0, RELATIVE);
        break;

      case CSI_CUB:		/* 8.3.18: (1) CURSOR LEFT */
      case CSI_HPB:		/* 8.3.59: (1) CHARACTER POSITION BACKWARD */
#ifdef ISO6429
        arg[0] = -arg[0];
#else				/* emulate common DEC VTs */
        arg[0] = arg[0] ? -arg[0] : -1;
#endif
        /* FALLTHROUGH */
      case CSI_CUF:		/* 8.3.20: (1) CURSOR RIGHT */
      case CSI_HPR:		/* 8.3.60: (1) CHARACTER POSITION FORWARD */
#ifdef ISO6429
        scr_gotorc (0, arg[0], RELATIVE);
#else				/* emulate common DEC VTs */
        scr_gotorc (0, arg[0] ? arg[0] : 1, RELATIVE);
#endif
        break;

      case CSI_CPL:		/* 8.3.13: (1) CURSOR PRECEDING LINE */
        arg[0] = -arg[0];
        /* FALLTHROUGH */
      case CSI_CNL:		/* 8.3.12: (1) CURSOR NEXT LINE */
        scr_gotorc (arg[0], 0, R_RELATIVE);
        break;

      case CSI_CHA:		/* 8.3.9: (1) CURSOR CHARACTER ABSOLUTE */
      case CSI_HPA:		/* 8.3.58: (1) CURSOR POSITION ABSOLUTE */
        scr_gotorc (0, arg[0] - 1, R_RELATIVE);
        break;

      case CSI_VPA:		/* 8.3.159: (1) LINE POSITION ABSOLUTE */
        scr_gotorc (arg[0] - 1, 0, C_RELATIVE);
        break;

      case CSI_CUP:		/* 8.3.21: (1,1) CURSOR POSITION */
      case CSI_HVP:		/* 8.3.64: (1,1) CHARACTER AND LINE POSITION */
        scr_gotorc (arg[0] - 1, nargs < 2 ? 0 : (arg[1] - 1), 0);
        break;

      case CSI_CBT:		/* 8.3.7: (1) CURSOR BACKWARD TABULATION */
        arg[0] = -arg[0];
        /* FALLTHROUGH */
      case CSI_CHT:		/* 8.3.10: (1) CURSOR FORWARD TABULATION */
        scr_tab (arg[0]);
        break;

      case CSI_ED:		/* 8.3.40: (0) ERASE IN PAGE */
        scr_erase_screen (arg[0]);
        break;

      case CSI_EL:		/* 8.3.42: (0) ERASE IN LINE */
        scr_erase_line (arg[0]);
        break;

      case CSI_ICH:		/* 8.3.65: (1) INSERT CHARACTER */
        scr_insdel_chars (arg[0], INSERT);
        break;

      case CSI_IL:		/* 8.3.68: (1) INSERT LINE */
        scr_insdel_lines (arg[0], INSERT);
        break;

      case CSI_DL:		/* 8.3.33: (1) DELETE LINE */
        scr_insdel_lines (arg[0], DELETE);
        break;

      case CSI_ECH:		/* 8.3.39: (1) ERASE CHARACTER */
        scr_insdel_chars (arg[0], ERASE);
        break;

      case CSI_DCH:		/* 8.3.26: (1) DELETE CHARACTER */
        scr_insdel_chars (arg[0], DELETE);
        break;

      case CSI_SD:		/* 8.3.114: (1) SCROLL DOWN */
        arg[0] = -arg[0];
        /* FALLTHROUGH */
      case CSI_SU:		/* 8.3.148: (1) SCROLL UP */
        scr_scroll_text (screen.tscroll, screen.bscroll, arg[0]);
        break;

      case CSI_DA:		/* 8.3.24: (0) DEVICE ATTRIBUTES */
        tt_write (VT100_ANS, sizeof (VT100_ANS) - 1);
        break;

      case CSI_SGR:		/* 8.3.118: (0) SELECT GRAPHIC RENDITION */
        process_sgr_mode (nargs, arg);
        break;

      case CSI_DSR:		/* 8.3.36: (0) DEVICE STATUS REPORT */
        switch (arg[0])
          {
            case 5:			/* DSR requested */
              tt_printf ("\033[0n");
              break;
            case 6:			/* CPR requested */
              scr_report_position ();
              break;
            case 7:			/* unofficial extension */
              if (option (Opt_insecure))
                tt_printf ("%-.250s\012", rs[Rs_display_name]);
              break;
            case 8:			/* unofficial extension */
              {
                string_term st;
                st.v[0] = CHAR_ST;
                st.v[1] = '\0';
                process_xterm_seq (XTerm_title, (char *)RESNAME "-" VERSION, st); // char * cast verified
                break;
              }
          }
        break;

      case CSI_TBC:		/* 8.3.155: (0) TABULATION CLEAR */
        switch (arg[0])
          {
            case 0:			/* char tab stop cleared at active position */
              scr_set_tab (0);
              break;
              /* case 1: */		/* line tab stop cleared in active line */
              /* case 2: */		/* char tab stops cleared in active line */
            case 3:			/* all char tab stops are cleared */
              /* case 4: */		/* all line tab stops are cleared */
            case 5:			/* all tab stops are cleared */
              scr_set_tab (-1);
              break;
          }
        break;

      case CSI_CTC:		/* 8.3.17: (0) CURSOR TABULATION CONTROL */
        switch (arg[0])
          {
            case 0:			/* char tab stop set at active position */
              scr_set_tab (1);
              break;		/* = ESC H */
              /* case 1: */		/* line tab stop set at active line */
            case 2:			/* char tab stop cleared at active position */
              scr_set_tab (0);
              break;		/* = ESC [ 0 g */
              /* case 3: */		/* line tab stop cleared at active line */
              /* case 4: */		/* char tab stops cleared at active line */
            case 5:			/* all char tab stops are cleared */
              scr_set_tab (-1);
              break;		/* = ESC [ 3 g */
              /* case 6: */		/* all line tab stops are cleared */
          }
        break;

      case CSI_RM:		/* 8.3.107: RESET MODE */
        if (arg[0] == 4)
          scr_insert_mode (0);
        else if (arg[0] == 20)
          priv_modes &= ~PrivMode_LFNL;
        break;

      case CSI_SM:		/* 8.3.126: SET MODE */
        if (arg[0] == 4)
          scr_insert_mode (1);
        else if (arg[0] == 20)
          priv_modes |= PrivMode_LFNL;
        break;

      case CSI_71:		// DECSCUSR: set cursor style
        if (prev_ch == ' ')
          set_cursor_style (arg[0]);
        break;

        /*
         * PRIVATE USE beyond this point.  All CSI_7? sequences here
         */
      case CSI_72:		/* DECSTBM: set top and bottom margins */
        if (nargs == 1)
          scr_scroll_region (arg[0] - 1, MAX_ROWS - 1);
        else if (nargs == 0 || arg[0] >= arg[1])
          scr_scroll_region (0, MAX_ROWS - 1);
        else
          scr_scroll_region (arg[0] - 1, arg[1] - 1);
        break;

      case CSI_73:
        scr_cursor (SAVE);
        break;
      case CSI_75:
        scr_cursor (RESTORE);
        break;

#if !ENABLE_MINIMAL
      case CSI_74:
        process_window_ops (arg, nargs);
        break;
#endif

      case CSI_78:		/* DECREQTPARM */
        if (arg[0] == 0 || arg[0] == 1)
          tt_printf ("\033[%d;1;1;128;128;1;0x", arg[0] + 2);
        break;

      default:
        break;
    }
}
/*}}} */

#if !ENABLE_MINIMAL
void
rxvt_term::process_window_ops (const int *args, unsigned int nargs)
{
  int x, y;
  XWindowAttributes wattr;
  Window wdummy;

  if (nargs == 0)
    return;

  switch (args[0])
    {
      /*
       * commands
       */
      case 1:			/* deiconify window */
        XMapWindow (dpy, parent);
        break;
      case 2:			/* iconify window */
        XIconifyWindow (dpy, parent, display->screen);
        break;
      case 3:			/* set position (pixels) */
        XMoveWindow (dpy, parent, args[1], args[2]);
        break;
      case 4:			/* set size (pixels) */
        set_widthheight ((unsigned int)args[2], (unsigned int)args[1]);
        break;
      case 5:			/* raise window */
        XRaiseWindow (dpy, parent);
        break;
      case 6:			/* lower window */
        XLowerWindow (dpy, parent);
        break;
      case 7:			/* refresh window */
        scr_touch (true);
        break;
      case 8:			/* set size (chars) */
        set_widthheight ((unsigned int) (args[2] * fwidth),
                         (unsigned int) (args[1] * fheight));
        break;

      //case 9: NYI, TODO, restore maximized window or maximize window
      default:
        if (args[0] >= 24)	/* set height (chars) */
          set_widthheight ((unsigned int)vt_width,
                           (unsigned int) (args[1] * fheight));
        break;


      /*
       * reports - some output format copied from XTerm
       */
      case 11:			/* report window state */
        XGetWindowAttributes (dpy, parent, &wattr);
        tt_printf ("\033[%dt", wattr.map_state == IsViewable ? 1 : 2);
        break;
      case 13:			/* report window position */
        XGetWindowAttributes (dpy, parent, &wattr);
        XTranslateCoordinates (dpy, parent, wattr.root,
                               -wattr.border_width, -wattr.border_width,
                               &x, &y, &wdummy);
        tt_printf ("\033[3;%d;%dt", x, y);
        break;
      case 14:			/* report window size (pixels) */
        XGetWindowAttributes (dpy, parent, &wattr);
        tt_printf ("\033[4;%d;%dt", wattr.height, wattr.width);
        break;
      case 18:			/* report text area size (chars) */
        tt_printf ("\033[8;%d;%dt", TermWin.nrow, TermWin.ncol);
        break;
      case 19:			/* report window size (chars) */
        tt_printf ("\033[9;%d;%dt", TermWin.nrow, TermWin.ncol);
        break;
#if 0 /* XXX: currently disabled due to security concerns */
      case 20:			/* report icon label */
        {
          char *s;
          XGetIconName (dpy, parent, &s);
          tt_printf ("\033]L%-.250s\234", option (Opt_insecure) && s ? s : "");	/* 8bit ST */
          XFree (s);
        }
        break;
      case 21:			/* report window title */
        {
          char *s;
          XFetchName (dpy, parent, &s);
          tt_printf ("\033]l%-.250s\234", option (Opt_insecure) && s ? s : "");	/* 8bit ST */
          XFree (s);
        }
        break;
#endif
    }
}
#endif

/*----------------------------------------------------------------------*/
/*
 * get input up until STRING TERMINATOR (or BEL)
 * ends_how is terminator used. returned input must be free()'d
 */
unsigned char *
rxvt_term::get_to_st (unicode_t &ends_how)
{
  int seen_esc = 0;	/* seen escape? */
  unsigned int n = 0;
  unsigned char *s;
  unicode_t ch;
  unsigned char string[STRING_MAX];

  while ((ch = cmd_getc ()))
    {
      if (ch == C0_BEL
          || ch == CHAR_ST
          || (ch == 0x5c && seen_esc))	/* 7bit ST */
        break;

      if (ch == C0_ESC)
        {
          if (ch == 0x5c)	/* 7bit ST */
            break;
          else
            return NULL;
        }
      else if (ch == C0_ESC)
        {
          seen_esc = true;
          continue;
        }
      else if (ch == C0_HT)
        ch = ' ';	/* translate '\t' to space */
      else if (ch < 0x20 && (ch != C0_LF && ch != C0_CR))
        return NULL;	/* other control character - exit */

      if (n < sizeof (string) - 1)
        string[n++] = ch;

      seen_esc = 0;
    }

  string[n++] = '\0';

  if ((s = (unsigned char *)rxvt_malloc (n)) == NULL)
    return NULL;

  ends_how = (ch == 0x5c ? C0_ESC : ch);
  STRNCPY (s, string, n);
  return s;
}

/*----------------------------------------------------------------------*/
/*
 * process DEVICE CONTROL STRING `ESC P ... (ST|BEL)' or `0x90 ... (ST|BEL)'
 */
void
rxvt_term::process_dcs_seq ()
{
  unsigned char *s;
  unicode_t eh;

  /*
   * Not handled yet
   */
  s = get_to_st (eh);
  if (s)
    free (s);

  return;
}

/*----------------------------------------------------------------------*/
/*
 * process OPERATING SYSTEM COMMAND sequence `ESC ] Ps ; Pt (ST|BEL)'
 */
void
rxvt_term::process_osc_seq ()
{
  unicode_t ch, eh;
  int arg;

  unicode_t ch = cmd_getc ();
  for (arg = 0; isdigit (ch); ch = cmd_getc ())
    arg = arg * 10 + (ch - '0');

  if (ch == ';')
    {
      unsigned char *s = get_to_st (eh);

      if (s)
        {
          /*
           * rxvt_menubar_dispatch () violates the constness of the string,
           * so do it here
           */
          if (arg == XTerm_Menu)
#if 0 /* XXX: currently disabled due to security concerns */
            menubar_dispatch ((char *)s);
#else
            (void)0;
#endif
          else
            xterm_seq (arg, (char *)s, eh);

          free (s);
        }
    }
}

static unsigned int
colorcube_index (unsigned int idx_r,
                 unsigned int idx_g,
                 unsigned int idx_b)
{
  assert (idx_r < Red_levels);
  assert (idx_g < Green_levels);
  assert (idx_b < Blue_levels);

  return idx_r * Blue_levels * Green_levels +
         idx_g * Blue_levels +
         idx_b;
}

/*
 * Find the nearest color slot in the hidden color cube,
 * adapt its value to the 32bit RGBA color.
 */
unsigned int
rxvt_term::map_rgb24_color (unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
  r &= 0xff;
  g &= 0xff;
  b &= 0xff;
  a &= 0xff;

  uint32_t color = (a << 24) | (r << 16) | (g << 8) | b;

  /* we allow one of the 6 closest neighbouring colours */
  /* to replace the current color, if they not used recently */
  #if 0
  static const unsigned char dxyz[][3] = {
    0, 0, 0,
    0, 0, 4,
    0, 4, 4,
    4, 4, 0,
    4, 0, 4,
    0, 4, 0,
    4, 0, 0,
  };
  #else
  // silly compressed verlapping triplets version of above
  static const unsigned char dxyz[] = {
    0, 0, 0, 4, 4, 0, 4, 0, 0,
  };
  #endif

  static const unsigned char color_level[8][32] = {
    // neighbour index
    {0, 0, 1, 0, 0, 1, 1, 2, 2, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {0, 1, 0, 1, 1, 2, 1, 1, 2, 2, 3, 3, 2, 2, 2, 3, 3, 3, 4, 4, 4, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4},
    {0, 0, 1, 2, 1, 1, 2, 3, 3, 2, 2, 3, 3, 4, 4, 3, 3, 3, 4, 4, 5, 5, 5, 4, 4, 4, 5, 5, 5, 5, 5, 5},
    {0, 0, 2, 1, 2, 3, 2, 2, 3, 4, 4, 3, 3, 4, 4, 5, 5, 4, 4, 5, 5, 5, 6, 6, 5, 5, 5, 6, 6, 6, 6, 6},
    // Red_levels/Green_levels/Blue_levels index
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},
    {0, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5},
    {0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6},
  };

  unsigned int idx;

  for (int n = 0; n < ecb_array_length (dxyz) - 2; ++n)
    {
      unsigned int idx_r = color_level[  Red_levels - dxyz[n + 0]][r / 8];
      unsigned int idx_g = color_level[Green_levels - dxyz[n + 1]][g / 8];
      unsigned int idx_b = color_level[ Blue_levels - dxyz[n + 2]][b / 8];
      unsigned int index = colorcube_index (idx_r, idx_g, idx_b);

      if (n == 0)
        idx = index;

      if (rgb24_color[index] == color)
        {
          rgb24_seqno[index] = ++rgb24_sequence;
          return index + minTermCOLOR24;
        }

      // minor issue: could update index 0 few more times
      if ((rgb24_seqno[index] | rgb24_color[index]) == 0)
        {
          idx = index;
          goto update;
        }

      // like (rgb24_seqno[idx] > rgb24_seqno[index])
      // but also handles wrap around values well enough
      if ((uint16_t)(rgb24_seqno[idx] - rgb24_seqno[index]) < 0x8000)
        idx = index;
    }

update:
  rgb24_color[idx] = color;
  rgb24_seqno[idx] = ++rgb24_sequence;

  idx += minTermCOLOR24;
  pix_colors_focused [idx].free (this);
  pix_colors_focused [idx].set (this, rgba (r * 0x0101, g * 0x0101, b * 0x0101, a * 0x0101));
  update_fade_color (idx, false);

  return idx;
}

void
rxvt_term::process_color_seq (int report, int color, const char *str, string_term &st)
{
  if (str[0] == '?' && !str[1])
    {
      rgba c;
      pix_colors_focused[color].get (c);
      char rgba_str[32];

#if XFT
      if (c.a != rgba::MAX_CC)
        snprintf (rgba_str, sizeof (rgba_str), "rgba:%04x/%04x/%04x/%04x", c.r, c.g, c.b, c.a);
      else
#endif
        snprintf (rgba_str, sizeof (rgba_str), "rgb:%04x/%04x/%04x", c.r, c.g, c.b);

      if (IN_RANGE_INC (color, minCOLOR, maxTermCOLOR))
        tt_printf ("\033]%d;%d;%s%s", report, color - minCOLOR, rgba_str, st.v);
      else
        tt_printf ("\033]%d;%s%s", report, rgba_str, st.v);
    }
  else
    set_window_color (color, str);
}

/*
 * XTerm escape sequences: ESC ] Ps;Pt (ST|BEL)
 */
void
rxvt_term::process_xterm_seq (int op, char *str, string_term &st)
{
  int changed = 0;
  int color;
  char *buf, *name;

  assert (str != NULL);

  if (HOOK_INVOKE ((this, HOOK_OSC_SEQ, DT_INT, op, DT_STR, str, DT_END)))
    return;

  switch (op)
    {
      case XTerm_name:
        set_title (str);
        /* FALLTHROUGH */
      case XTerm_iconName:
        set_icon_name (str);
        break;
      case XTerm_title:
        set_title (str);
        break;
      case XTerm_property:
        if (str[0] == '?')
          {
            Atom prop = display->atom (str + 1);
            Atom actual_type;
            int actual_format;
            unsigned long nitems;
            unsigned long bytes_after;
            unsigned char *value = 0;
            const char *str = "";

            if (prop
                && XGetWindowProperty (dpy, parent,
                                       prop, 0, 1<<16, 0, AnyPropertyType,
                                       &actual_type, &actual_format,
                                       &nitems, &bytes_after, &value) == Success
                && actual_type != None
                && actual_format == 8)
              str = (const char *)(value);

            tt_printf ("\033]%d;%s%s", op, option (Opt_insecure) ? str : "", st.v);

            XFree (value);
          }
        else
          {
            char *eq = strchr (str, '=');

            if (eq)
              {
                *eq = 0;
                set_utf8_property (display->atom (str), eq + 1);
              }
            else
              XDeleteProperty (dpy, parent,
                               display->atom (str));
          }
        break;

      case XTerm_Color:
        for (buf = (char *)str; buf && *buf;)
          {
            if ((name = strchr (buf, ';')) == NULL)
              break;

            *name++ = '\0';
            color = atoi (buf);

            if (color < 0 || color >= TOTAL_COLORS)
              break;

            if ((buf = STRCHR (name, ';')) != NULL)
              *buf++ = '\0';

            set_window_color (color + minCOLOR, name);
          }
        break;
      case Rxvt_restoreFG:
      case XTerm_Color00:
        process_color_seq (op, Color_fg, str, st);
        break;
      case Rxvt_restoreBG:
      case XTerm_Color01:
        process_color_seq (op, Color_bg, str, st);
        break;
#ifndef NO_CURSORCOLOR
      case XTerm_Color_cursor:
        process_color_seq (op, Color_cursor, str, st);
        break;
#endif
      case XTerm_Color_pointer_fg:
        process_color_seq (op, Color_pointer_fg, str, st);
        break;
      case XTerm_Color_pointer_bg:
        process_color_seq (op, Color_pointer_bg, str, st);
        break;
#ifdef OPTION_HC
      case XTerm_Color_HC:
        process_color_seq (op, Color_HC, str, st);
        break;
      case XTerm_Color_HTC:
        process_color_seq (op, Color_HTC, str, st);
        break;
#endif
#ifndef NO_BOLD_UNDERLINE_REVERSE
      case URxvt_Color_BD:
        process_color_seq (op, Color_BD, str, st);
        break;
      case URxvt_Color_UL:
        process_color_seq (op, Color_UL, str, st);
        break;
      case URxvt_Color_IT:
        process_color_seq (op, Color_IT, str, st);
        break;
#endif
      case URxvt_Color_border:
        process_color_seq (op, Color_border, str, st);
        break;

      case XTerm_logfile:
        // TODO, when secure mode?
        break;
      case XTerm_font:
        change_font (str);
        break;
#ifndef NO_FRILLS
      case XTerm_locale:
        if (str[0] == '?' && !str[1])
          tt_printf ("%-.250s\n", locale);
        else
          {
            set_locale (str);
# ifdef USE_XIM
            im_cb ();
# endif
            // TODO: call selection_make with the right values set
            // to re-fresh the selection.
            if (display->selection_owner == this)
              display->set_selection_owner (0);
          }
        break;
#endif
#if 0
      case Rxvt_dumpscreen:	/* no error notices */
        {
          int fd;
          if ((fd = open (str, O_RDWR | O_CREAT | O_EXCL, 0600)) >= 0)
            {
              scr_dump (fd);
              close (fd);
            }
        }
        break;
#endif
    }
}
/*----------------------------------------------------------------------*/

/*{{{ process DEC private mode sequences `ESC [ ? Ps mode' */
/*
 * mode can only have the following values:
 *      'l' = low
 *      'h' = high
 *      's' = save
 *      'r' = restore
 *      't' = toggle
 * so no need for fancy checking
 */
int ecb_cold
rxvt_term::privcases (int mode, unsigned long bit)
{
  int state;

  if (mode == 's')
    {
      if (priv_modes & bit)
        SavedModes |= bit;
      else
        SavedModes &= ~bit;
      return -1;
    }
  else
    {
      if (mode == 'r')
        state = (SavedModes & bit) ? 1 : 0;	/* no overlapping */
      else
        state = (mode == 't') ? ! (priv_modes & bit) : mode;

      if (state)
        priv_modes |= bit;
      else
        priv_modes &= ~bit;
    }

  return state;
}

/* we're not using priv _yet_ */
void
rxvt_term::process_terminal_mode (int mode, int priv ecb_unused, unsigned int nargs, const int *arg)
{
  unsigned int i, j;
  int state;

  static const struct
    {
      const int       argval;
      const unsigned long bit;
    }

  argtopriv[] = {
                  { 1, PrivMode_aplCUR },
                  { 2, PrivMode_vt52 },
                  { 3, PrivMode_132 },
                  { 4, PrivMode_smoothScroll },
                  { 5, PrivMode_rVideo },
                  { 6, PrivMode_relOrigin },
                  { 7, PrivMode_Autowrap },
                  { 9, PrivMode_MouseX10 },
                  { 12, PrivMode_BlinkingCursor },
                 // 18 end FF to printer after print screen
                 // 19 Print screen prints full screen/scroll region
                  { 25, PrivMode_VisibleCursor }, // DECTCEM cnorm/cvvis/civis
#ifdef scrollBar_esc
                  { scrollBar_esc, PrivMode_scrollBar },
#endif
                  { 35, PrivMode_ShiftKeys },   // rxvt extension
                 // 38, tektronix mode          // DECTEK
                  { 40, PrivMode_132OK },
                 // 41 xterm more fixes NYI
                 // 45 margin bell NYI
                 // 46 start logging
                  { 47, PrivMode_Screen },
                  { 66, PrivMode_aplKP },       // DECNKM
#ifndef NO_BACKSPACE_KEY
                  { 67, PrivMode_BackSpace },   // DECBKM
#endif
                  { 1000, PrivMode_MouseX11 },
                  { 1002, PrivMode_MouseBtnEvent },
                  { 1003, PrivMode_MouseAnyEvent },
#if ENABLE_FRILLS
                  { 1004, PrivMode_FocusEvent },
                  { 1005, PrivMode_ExtMouseUTF8 },
                  { 1006, PrivMode_ExtMouseSGR },
#endif
                  { 1010, PrivMode_TtyOutputInh }, // rxvt extension
                  { 1011, PrivMode_Keypress }, // rxvt extension
#if ENABLE_FRILLS
                  { 1015, PrivMode_ExtMouseUrxvt }, // urxvt extension of 1005
#endif
                 // 1035 enable modifiers for alt, numlock NYI
                 // 1036 send ESC for meta keys NYI
                 // 1037 send DEL for keypad delete NYI
                  { 1047, PrivMode_Screen },
                 // 1048 save and restore cursor, implemented in code
                  { 1049, PrivMode_Screen }, /* xterm extension, clear screen on ti rather than te */
                 // 1051, 1052, 1060, 1061 keyboard emulation NYI
#if ENABLE_FRILLS
                  { 2004, PrivMode_BracketPaste },
#endif
                };

  if (nargs == 0)
    return;

  // DECRQM
  if (mode == 'p')
    {
      int status = 0;
      if (nargs != 1)
        return;

      for (j = 0; j < ecb_array_length (argtopriv); j++)
        if (argtopriv[j].argval == arg[0])
          {
            status = (priv_modes & argtopriv[j].bit) ? 1 : 2;
            break;
          }

      tt_printf ("\33[?%d;%d$y", arg[0], status);
      return;
    }

  /* make lo/hi boolean */
  if (mode == 'l')
    mode = 0;		/* reset */
  else if (mode == 'h')
    mode = 1;		/* set */

  for (i = 0; i < nargs; i++)
    {
      state = -1;

      /* basic handling */
      for (j = 0; j < ecb_array_length (argtopriv); j++)
        if (argtopriv[j].argval == arg[i])
          {
            state = privcases (mode, argtopriv[j].bit);
            break;
          }

      /* extra handling for values with state unkept  */
      switch (arg[i])
        {
#if ENABLE_STYLES
          case 1021:
            set_option (Opt_intensityStyles, mode);

            scr_touch (true);
            break;
#endif
          case 1048:		/* alternative cursor save */
            if (option (Opt_secondaryScreen))
              if (mode == 0)
                scr_cursor (RESTORE);
              else if (mode == 1)
                scr_cursor (SAVE);
            break;
        }

      if (state >= 0)
        /* extra handling for values with valid 0 or 1 state */
        switch (arg[i])
          {
              /* case 1:	- application cursor keys */
            case 2:			/* VT52 mode */
              /* oddball mode.  should be set regardless of set/reset
               * parameter.  Return from VT52 mode with an ESC < from
               * within VT52 mode
               */
              priv_modes |= PrivMode_vt52;
              break;
            case 3:			/* 80/132 */
              if (priv_modes & PrivMode_132OK)
                set_widthheight ((state ? 132 : 80) * fwidth, 24 * fheight);
              break;
            case 4:			/* smooth scrolling */
              set_option (Opt_jumpScroll, !state);
              break;
            case 5:			/* reverse video */
              scr_rvideo_mode (state);
              break;
            case 6:			/* relative/absolute origins  */
              scr_relative_origin (state);
              break;
            case 7:			/* autowrap */
              scr_autowrap (state);
              break;
            /* case 8:	- auto repeat, can't do on a per window basis */
            case 9:			/* X10 mouse reporting */
              if (state)		/* orthogonal */
                PrivateModes &= ~PrivMode_MouseX11;
              break;
#ifdef scrollBar_esc
            case scrollBar_esc:
              scrollBar.map (state);
              resize_all_windows (0, 0, 0);
              scr_touch (true);
              break;
#endif
#ifdef CURSOR_BLINK
            case 12:
              cursor_blink_reset ();
              break;
#endif
            case 25:		/* visible/invisible cursor */
              scr_cursor_visible (state);
              break;
            /* case 35:	- shift keys */
            /* case 40:	- 80 <--> 132 mode */
            case 47:		/* secondary screen */
              scr_change_screen (state);
              break;
            /* case 66:	- application key pad */
            /* case 67:	- backspace key */
            case 1000:		/* X11 mouse reporting */
              if (state)		/* orthogonal */
                PrivateModes &= ~PrivMode_MouseX10;
              break;
            case 1002:
            case 1003:
              if (state)
                {
                  priv_modes &= ~(PrivMode_MouseX10|PrivMode_MouseX11);
                  priv_modes &= arg[i] == 1003 ? ~PrivMode_MouseBtnEvent : ~PrivMode_MouseAnyEvent;
                  mouse_row = mouse_col = 0;
                  vt_emask_mouse = PointerMotionMask;
                }
              else
                vt_emask_mouse = NoEventMask;

              vt_select_input ();
              break;
            case 1010:		/* scroll to bottom on TTY output inhibit */
              set_option (Opt_scrollTtyOutput, !state);
              break;
            case 1011:		/* scroll to bottom on key press */
              set_option (Opt_scrollTtyKeypress, state);
              break;
            case 1047:		/* secondary screen w/ clearing */
            case 1049:		/* better secondary screen w/ clearing, but not fully implemented */
              if (current_screen != PRIMARY)
                scr_erase_screen (2);

              scr_change_screen (state);
              break;
            case 1049:		/* secondary screen w/ clearing first */
              if (option (Opt_secondaryScreen))
                if (state)
                  scr_cursor (SAVE);

              scr_change_screen (state);

              if (option (Opt_secondaryScreen))
                if (state)
                  scr_erase_screen (2);
                else
                  scr_cursor (RESTORE);
              break;
            default:
              break;
          }
    }
}
/*}}} */

/*{{{ process sgr sequences */
void ecb_hot
rxvt_term::process_sgr_mode (unsigned int nargs, const int *arg)
{
  unsigned int i;
  short rendset;
  int rendstyle;

  if (nargs == 0)
    {
      scr_rendition (0, ~RS_None);
      return;
    }

  for (i = 0; i < nargs; i++)
    {
      rendset = -1;
      switch (arg[i])
        {
          case 0:
            rendset = 0, rendstyle = ~RS_None;
            break;
          case 1:
            rendset = 1, rendstyle = RS_Bold;
            break;
          //case 2: // low intensity
          case 3:
            rendset = 1, rendstyle = RS_Italic;
            break;
          case 4:
            rendset = 1, rendstyle = RS_Uline;
            break;
          case 5: // slowly blinking
          case 6: // rapidly blinking
            rendset = 1, rendstyle = RS_Blink;
            break;
          //case 6: // scoansi light background
          case 7:
            rendset = 1, rendstyle = RS_RVid;
            break;
          case 8:
            // invisible. NYI
            break;
          //case 10: // scoansi acs off
          //case 11: // scoansi acs on
          //case 12: // scoansi acs on, |0x80
          case 21: // disable bold, blink and invis (some terminals use this)
            rendset = 0, rendstyle = RS_Bold | RS_Blink;
            break;
          case 22:
            rendset = 0, rendstyle = RS_Bold;
            break;
          case 22: // bold off (vt220)
            rendset = 0, rendstyle = RS_Bold;
            break;
          case 23: // disable italic
            rendset = 0, rendstyle = RS_Italic;
            break;
          case 24: // underline off (vt220)
            rendset = 0, rendstyle = RS_Uline;
            break;
          case 25: // blink off (vt220)
            rendset = 0, rendstyle = RS_Blink;
            break;
          case 26: // variable spacing (iso 8613)
            rendset = 0, rendstyle = RS_Blink;
            break;
          case 27: // reverse off (vt220)
            rendset = 0, rendstyle = RS_RVid;
            break;
          //case 28: // visible. NYI
          //case 29: // not crossed-out
        }

      if (rendset != -1)
        {
          scr_rendition (rendset, rendstyle);
          continue;		/* for (;i;) */
        }

      switch (arg[i])
        {
          case 30:
          case 31:		/* set fg color */
          case 32:
          case 33:
          case 34:
          case 35:
          case 36:
          case 37:
            scr_color ((unsigned int) (minCOLOR + (arg[i] - 30)), Color_fg);
            break;
          case 39:		/* default fg */
            scr_color (Color_fg, Color_fg);
            break;

          case 40:
          case 41:		/* set bg color */
          case 42:
          case 43:
          case 44:
          case 45:
          case 46:
          case 47:
            scr_color ((unsigned int) (minCOLOR + (arg[i] - 40)), Color_bg);
            break;
          case 49:		/* default bg */
            scr_color (Color_bg, Color_bg);
            break;

          case 38: // set fg color, ISO 8613-6
          case 48: // set bg color, ISO 8613-6
            {
              unsigned int fgbg = arg[i] == 38 ? Color_fg : Color_bg;
              unsigned int idx;

              if (nargs > i + 2 && arg[i + 1] == 5)
                {
                  idx = minCOLOR + arg[i + 2];
                  i += 2;

                  scr_color (idx, fgbg);
                }
              else if (nargs > i + 4 && arg[i + 1] == 2)
                {
                  unsigned int r = arg[i + 2];
                  unsigned int g = arg[i + 3];
                  unsigned int b = arg[i + 4];
                  unsigned int a = 0xff;

                  idx = map_rgb24_color (r, g, b, a);

                  i += 4;

                  scr_color (idx, fgbg);
                }
            }
            break;

          //case 50: // not variable spacing

#if !ENABLE_MINIMAL
          case 90:
          case 91:		/* set bright fg color */
          case 92:
          case 93:
          case 94:
          case 95:
          case 96:
          case 97:
            scr_color ((unsigned int) (minBrightCOLOR + (arg[i] - 90)), Color_fg);
            break;
          case 100:
          case 101:		/* set bright bg color */
          case 102:
          case 103:
          case 104:
          case 105:
          case 106:
          case 107:
            scr_color ((unsigned int) (minBrightCOLOR + (arg[i] - 100)), Color_bg);
            break;
#endif
        }
    }
}

/*{{{ (do not) process Rob Nation's own graphics mode sequences */
void
rxvt_term::set_cursor_style (int style)
{
  unicode_t ch, cmd = cmd_getc ();

  if (cmd == 'Q')
    {		/* query graphics */
      tt_printf ("\033G0\n");	/* no graphics */
      return;
    }
  /* swallow other graphics sequences until terminating ':' */
  do
    ch = cmd_getc ();
  while (ch != ':');
}
/*}}} */

/* ---------------------------------------------------------------------- */
/* Write data to the pty as typed by the user, pasted with the mouse,
 * or generated by us in response to a query ESC sequence.
 */

/*
 * Send printf () formatted output to the command.
 * Only use for small amounts of data.
 */
void
rxvt_term::tt_printf (const char *fmt,...)
{
  va_list arg_ptr;
  char buf[256];

  va_start (arg_ptr, fmt);
  vsnprintf ((char *)buf, 256, fmt, arg_ptr);
  va_end (arg_ptr);
  tt_write (buf, strlen (buf));
}

/* Write data to the pty as typed by the user. */
void
rxvt_term::tt_write_user_input (const char *data, unsigned int len)
{
  if (HOOK_INVOKE ((this, HOOK_TT_WRITE, DT_STR_LEN, data, len, DT_END)))
    return;

  if (option (Opt_scrollTtyKeypress))
    scr_changeview (0);

  tt_write_ (data, len);
}

void
rxvt_term::tt_write (const char *data, unsigned int len)
{
  if (HOOK_INVOKE ((this, HOOK_TT_WRITE, DT_STR_LEN, data, len, DT_END)))
    return;

  tt_write_ (data, len);
}

static const unsigned int MAX_PTY_WRITE = 255; // minimum MAX_INPUT

void
rxvt_term::tt_write_ (const char *data, unsigned int len)
{
  if (pty->pty < 0)
    return;

  if (v_buflen == 0)
    {
      ssize_t written = write (pty->pty, data, min (len, MAX_PTY_WRITE));

      max_it (written, 0);

      if (written == len)
        return;

      data += written;
      len  -= written;
    }

  v_buffer = (char *)rxvt_realloc (v_buffer, v_buflen + len);

  memcpy (v_buffer + v_buflen, data, len);
  v_buflen += len;

  pty_ev.set (ev::READ | ev::WRITE);
}

void rxvt_term::pty_write ()
{
  int written = write (pty->pty, v_buffer, min (v_buflen, MAX_PTY_WRITE));

  if (written > 0)
    {
      v_buflen -= written;

      if (v_buflen == 0)
        {
          free (v_buffer);
          v_buffer = 0;

          pty_ev.set (ev::READ);
          return;
        }

      memmove (v_buffer, v_buffer + written, v_buflen);
    }
  else if (written != -1 || (errno != EAGAIN && errno != EINTR))
    pty_ev.set (ev::READ);
}

/*----------------------- end-of-file (C source) -----------------------*/

