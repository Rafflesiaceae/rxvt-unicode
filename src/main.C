/*--------------------------------*-C-*---------------------------------*;
 * File:        main.c
 *----------------------------------------------------------------------*
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (c) 1992      John Bovey, University of Kent at Canterbury <jdb@ukc.ac.uk>
 *                              - original version
 * Copyright (c) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 *                              - extensive modifications
 * Copyright (c) 1995      Garrett D'Amore <garrett@netcom.com>
 * Copyright (c) 1997      mj olesen <olesen@me.QueensU.CA>
 *                              - extensive modifications
 * Copyright (c) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 * Copyright (c) 1998-2001 Geoff Wing <gcw@pobox.com>
 *                              - extensive modifications
 * Copyright (c) 2003-2014 Marc Lehmann <schmorp@schmorp.de>
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
 *---------------------------------------------------------------------*/

#include "../config.h"          /* NECESSARY */
#include "rxvt.h"               /* NECESSARY */
#include "init.h"
#include "keyboard.h"
#include "rxvtperl.h"

#include <limits>

#include <assert.h>
#include <signal.h>
#include <string.h>

#include <termios.h>

#ifdef HAVE_XSETLOCALE
# define X_LOCALE
# include <X11/Xlocale.h>
#else
# include <locale.h>
#endif

struct termios rxvt_term::def_tio;
vector<rxvt_term *> rxvt_term::termlist;

// used to tell global functions which terminal instance is "active"
rxvt_t rxvt_current_term;

static char curlocale[128], savelocale[128];

bool
rxvt_set_locale (const char *locale) noexcept
{
  int size = strlen (locale) + 1;

  if (size > sizeof (curlocale))
    rxvt_fatal ("locale string too long, aborting.\n");

  if (!locale || !memcmp (locale, curlocale, size))
    return false;

  memcpy (curlocale, locale, size);
  setlocale (LC_CTYPE, curlocale);
  return true;
}

void
rxvt_push_locale (const char *locale) noexcept
{
  strcpy (savelocale, curlocale);
  rxvt_set_locale (locale);
}

void
rxvt_pop_locale () noexcept
{
  rxvt_set_locale (savelocale);
}

#if ENABLE_COMBINING
class rxvt_composite_vec rxvt_composite;

text_t rxvt_composite_vec::compose (unicode_t c1, unicode_t c2)
{
  compose_char *cc;

  // break compose chains, as stupid readline really likes to duplicate
  // composing characters for some reason, near the end of a line.
  cc = (*this)[c1];
  while (cc)
    {
      if (cc->c2 == c2) return c1;
      cc = (*this)[cc->c1];
    }

  // check to see whether this combination already exists otherwise
  for (cc = v.begin (); cc < v.end (); cc++)
    if (cc->c1 == c1 && cc->c2 == c2)
      return COMPOSE_LO + (cc - v.begin ());

  // allocate a new combination
  if (v.size () == COMPOSE_HI - COMPOSE_LO + 1)
    {
      static int seen;

      if (!seen++)
        rxvt_warn ("too many unrepresentable composite characters, try --enable-unicode3\n");

      return REPLACEMENT_CHAR;
    }

  v.push_back (compose_char (c1, c2));

  return v.size () - 1 + COMPOSE_LO;
}

template<typename T>
int rxvt_composite_vec::expand (unicode_t c, T *r)
{
  compose_char *cc = (*this)[c];

  if (!cc)
    {
      if (r) *r = c;
      return 1;
    }

  int len = expand (cc->c1, r);

  if (r) r += len;

  if (cc->c2 != NOCHAR)
    {
      len++;
      if (r) *r++ = cc->c2;
    }

  return len;
}

template int rxvt_composite_vec::expand<text_t>  (unicode_t c, text_t *);
template int rxvt_composite_vec::expand<wchar_t> (unicode_t c, wchar_t *);
#endif

rxvt_term::rxvt_term ()
    :
    rootwin_ev (this, &rxvt_term::rootwin_cb),
    termwin_ev (this, &rxvt_term::x_cb),
    vt_ev (this, &rxvt_term::x_cb),
#ifdef HAVE_SCROLLBARS
    scrollbar_ev (this, &rxvt_term::x_cb),
#endif
#ifdef MENUBAR
    menubar_ev (this, &rxvt_term::x_cb),
#endif
#ifdef CURSOR_BLINK
  cursor_blink_ev.set     <rxvt_term, &rxvt_term::cursor_blink_cb> (this); cursor_blink_ev.set (0., CURSOR_BLINK_INTERVAL);
#endif
#ifdef TEXT_BLINK
  text_blink_ev.set       <rxvt_term, &rxvt_term::text_blink_cb>   (this); text_blink_ev.set (0., TEXT_BLINK_INTERVAL);
#endif
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
  cont_scroll_ev.set      <rxvt_term, &rxvt_term::cont_scroll_cb>  (this);
#endif
#ifdef SELECTION_SCROLLING
  sel_scroll_ev.set       <rxvt_term, &rxvt_term::sel_scroll_cb>   (this);
#endif
#if defined(MOUSE_WHEEL) && defined(MOUSE_SLIP_WHEELING)
  slip_wheel_ev.set       <rxvt_term, &rxvt_term::slip_wheel_cb>   (this);
#endif
#if ENABLE_PERL
  rootwin_ev.set          <rxvt_term, &rxvt_term::rootwin_cb> (this),
#endif
  scrollbar_ev.set        <rxvt_term, &rxvt_term::x_cb>       (this),
#if USE_XIM
  im_ev.set               <rxvt_term, &rxvt_term::im_cb>      (this),
#endif
#ifdef POINTER_BLANK
  pointer_ev.set          <rxvt_term, &rxvt_term::pointer_cb> (this);
#endif
#ifndef NO_BELL
  bell_ev.set             <rxvt_term, &rxvt_term::bell_cb>    (this);
#endif
  child_ev.set            <rxvt_term, &rxvt_term::child_cb>   (this);
  flush_ev.set            <rxvt_term, &rxvt_term::flush_cb>   (this);
  destroy_ev.set          <rxvt_term, &rxvt_term::destroy_cb> (this);
  pty_ev.set              <rxvt_term, &rxvt_term::pty_cb>     (this);
  termwin_ev.set          <rxvt_term, &rxvt_term::x_cb>       (this);
  vt_ev.set               <rxvt_term, &rxvt_term::x_cb>       (this);

  cmdbuf_ptr = cmdbuf_endp = cmdbuf_base;

  termlist.push_back (this);

#ifdef KEYSYM_RESOURCE
  keyboard = new keyboard_manager;
#endif
}

// clean up the most important stuff, do *not* call x or free mem etc.
// for use before an emergency exit
void
rxvt_term::emergency_cleanup ()
{
  if (cmd_pid)
    kill (-cmd_pid, SIGHUP);

  pty_ev.stop ();
  delete pty; pty = 0;
}

rxvt_term::~rxvt_term ()
{
  termlist.erase (find (termlist.begin (), termlist.end(), this));

  emergency_cleanup ();

#if ENABLE_STYLES
  for (int i = RS_styleCount; --i; )
    if (fontset[i] != fontset[0])
      delete fontset[i];
#endif
  delete fontset[0];

#if HAVE_IMG
  delete bg_img;
#endif

  if (display)
    {
      if (TermWin.parent[0])
        XDestroyWindow (display->display, TermWin.parent[0]);
#if defined(MENUBAR) && (MENUBAR_MAX > 1)
      if (menuBar.win)
        XDestroyWindow (display->display, menuBar.win);
      delete menuBar.drawable;
#endif
    }

#if USE_XIM
      im_destroy ();
#endif
      scrollBar.destroy ();

      if (gc)
        XFreeGC (dpy, gc);

      delete drawable;

      // destroy all windows
      if (parent)
        XDestroyWindow (dpy, parent);

      for (int i = 0; i < TOTAL_COLORS; i++)
        if (ISSET_PIXCOLOR (i))
          {
            pix_colors_focused   [i].free (this);
#if OFF_FOCUS_FADING
            if (rs[Rs_fade])
              pix_colors_unfocused [i].free (this);
#endif
          }

      clear ();

      display->flush (); /* ideally .put should do this */
      displays.put (display);
    }

  scr_release ();

  free (env_windowid);
  free (env_display);
  free (env_term);
  free (env_colorfgbg);
  free (locale);
  free (v_buffer);

  delete selection_req;

  if (env && memcmp (env, envv->begin (), envv->size () * sizeof (char *)))
    rxvt_warn ("env has been modified, probably as a result of a lib calling setenv.\n");

  delete [] env;

  delete envv;
  delete argv;

#ifdef KEYSYM_RESOURCE
  delete keyboard;
#endif
#ifndef NO_RESOURCES
  XrmDestroyDatabase (option_db);
#endif

  SET_R ((rxvt_term *)0);
}

// child has exited, usually destroys
void
rxvt_term::child_cb (ev::child &w, int status)
{
  HOOK_INVOKE ((this, HOOK_CHILD_EXIT, DT_INT, status, DT_END));

  displays.put (display);

  delete envv;
  delete argv;
}

void
rxvt_term::destroy ()
{
  if (destroy_ev.is_active ())
    return;

  HOOK_INVOKE ((this, HOOK_DESTROY, DT_END));

#if ENABLE_OVERLAY
  scr_overlay_off ();
#endif

  if (display)
    {
#if USE_XIM
      im_ev.stop (display);
#endif
      scrollbar_ev.stop (display);
#if ENABLE_PERL
      rootwin_ev.stop (display);
#endif
      termwin_ev.stop (display);
      vt_ev.stop (display);
    }

  flush_ev.stop ();
  pty_ev.stop ();
#ifdef CURSOR_BLINK
  cursor_blink_ev.stop ();
#endif
#ifdef TEXT_BLINK
  text_blink_ev.stop ();
#endif
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
  cont_scroll_ev.stop ();
#endif
#ifdef SELECTION_SCROLLING
  sel_scroll_ev.stop ();
#endif
#ifdef POINTER_BLANK
  pointer_ev.stop ();
#endif

  destroy_ev.start ();
}

void
rxvt_term::destroy_cb (ev::idle &w, int revents)
{
  make_current ();

  delete this;
}

/*----------------------------------------------------------------------*/
/* rxvt_init () */
/* LIBPROTO */
rxvt_t
rxvt_init (int argc, const char *const *argv)
{
  SET_R (new rxvt_term);

  if (!GET_R->init_vars () || !GET_R->init (argc, argv))
    {
      delete GET_R;
      SET_R (0);
    }

  return GET_R;
}

static int (*old_xerror_handler) (Display *dpy, XErrorEvent *event);

void
rxvt_term::set_option (uint8_t opt, bool set) noexcept
{
  /* install exit handler for cleanup */
#if 0
#ifdef HAVE_ATEXIT
  atexit (rxvt_clean_exit);
#else
#endif
#endif

  uint8_t mask = 1 << (opt & 7);
  uint8_t &val = options [opt >> 3];

  sigfillset (&sa.sa_mask);
  sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
  sa.sa_handler = SIG_IGN;           sigaction (SIGHUP , &sa, 0);
  sa.sa_handler = SIG_IGN;           sigaction (SIGPIPE, &sa, 0);
  sa.sa_handler = rxvt_Exit_signal;  sigaction (SIGINT , &sa, 0);
  sa.sa_handler = rxvt_Exit_signal;  sigaction (SIGQUIT, &sa, 0);
  sa.sa_handler = rxvt_Exit_signal;  sigaction (SIGTERM, &sa, 0);
  sa.sa_handler = rxvt_Child_signal; sigaction (SIGCHLD, &sa, 0);

  /* need to trap SIGURG for SVR4 (Unixware) rlogin */
  /* signal (SIGURG, SIG_DFL); */

  old_xerror_handler = XSetErrorHandler ((XErrorHandler) rxvt_xerror_handler);
  //XSetIOErrorHandler ((XErrorHandler) rxvt_xioerror_handler);
}

bool
rxvt_term::init (int argc, const char *const *argv)
{
  /*
   * Save and then give up any super-user privileges
   * If we need privileges in any area then we must specifically request it.
   * We should only need to be root in these cases:
   *  1.  write utmp entries on some systems
   *  2.  chown tty on some systems
   */
  privileges (SAVE);
  privileges (IGNORE);

  init_secondary ();

  const char **cmd_argv = init_resources (argc, argv);

  set_locale ("");

#if MENUBAR_MAX
  menubar_read (rs[Rs_menu]);
#endif
#ifdef HAVE_SCROLLBARS
  if (Options & Opt_scrollBar)
    scrollBar.setIdle ();    /* set existence for size calculations */
#endif

  create_windows (argc, argv);

  init_xlocale ();

  scr_reset ();         /* initialize screen */

#if 0
  XSynchronize (display->display, True);
#endif
#endif

#ifdef HAVE_SCROLLBARS
  if (Options & Opt_scrollBar)
    resize_scrollbar ();      /* create and map scrollbar */
#endif
#if (MENUBAR_MAX)
  if (menubar_visible ())
    XMapWindow (display->display, menuBar.win);
#endif
#ifdef TRANSPARENT
  if (Options & Opt_transparent)
    {
      XSelectInput (display->display, display->root, PropertyChangeMask);
      check_our_parents ();
    }
#endif

  rootwin_ev.start (display, display->root);

  XMapWindow (display->display, TermWin.vt);
  XMapWindow (display->display, TermWin.parent[0]);

  init_command (cmd_argv);

  pty_ev.start (cmd_fd, EVENT_READ);

  check_ev.start ();

  return true;
}

/* ------------------------------------------------------------------------- *
 *                       SIGNAL HANDLING & EXIT HANDLER                      *
 * ------------------------------------------------------------------------- */
/*
 * Catch a SIGCHLD signal and exit if the direct child has died
 */
/* ARGSUSED */
/* EXTPROTO */
RETSIGTYPE
rxvt_Child_signal (int sig __attribute__ ((unused)))
{
  int pid, save_errno = errno;
  while ((pid = waitpid (-1, NULL, WNOHANG)) == -1 && errno == EINTR)
    ;
  errno = save_errno;

#if 0
  if (pid == cmd_pid)
    exit (EXIT_SUCCESS);
#endif
}

/*
 * Catch a fatal signal and tidy up before quitting
 */
/* EXTPROTO */
RETSIGTYPE
rxvt_Exit_signal (int sig)
{
  signal (sig, SIG_DFL);
#ifdef DEBUG_CMD
  rxvt_print_error ("signal %d", sig);
#endif
  rxvt_clean_exit ();
  kill (getpid (), sig);
}

/* INTPROTO */
int
rxvt_xerror_handler (Display *display, XErrorEvent *event)
{
  if (GET_R->allowedxerror == -1)
    GET_R->allowedxerror = event->error_code;
  else
    {
      old_xerror_handler (display, event);
      GET_R->destroy ();
    }

  return 0;
}

/*----------------------------------------------------------------------*/
/*
 * Exit gracefully, clearing the utmp entry and restoring tty attributes
 */
static XErrorHandler old_xerror_handler;

static void
rxvt_emergency_cleanup ()
{
  for (rxvt_term **t = rxvt_term::termlist.begin (); t < rxvt_term::termlist.end (); t++)
    (*t)->emergency_cleanup ();
}

/* ------------------------------------------------------------------------- *
 *                         MEMORY ALLOCATION WRAPPERS                        *
 * ------------------------------------------------------------------------- */
/* EXTPROTO */
void           *
rxvt_malloc (size_t size)
{
  char buffer[BUFSIZ];
  char mesg[BUFSIZ];
  char number[32];

  p = malloc (size);
  if (p)
    return p;

  fprintf (stderr, APL_NAME ": memory allocation failure.  Aborting");
  rxvt_clean_exit ();
  exit (EXIT_FAILURE);
  /* NOTREACHED */
}

/* EXTPROTO */
void           *
rxvt_calloc (size_t number, size_t size)
{
  void           *p;

  p = calloc (number, size);
  if (p)
    return p;

  fprintf (stderr, APL_NAME ": memory allocation failure.  Aborting");
  rxvt_clean_exit ();
  exit (EXIT_FAILURE);
  /* NOTREACHED */
}

/* EXTPROTO */
void           *
rxvt_realloc (void *ptr, size_t size)
{
  void           *p;

  if (ptr)
    p = realloc (ptr, size);
  else
    p = malloc (size);
  if (p)
    return p;

  fprintf (stderr, APL_NAME ": memory allocation failure.  Aborting");
  rxvt_clean_exit ();
  exit (EXIT_FAILURE);
  /* NOTREACHED */
}

/* ------------------------------------------------------------------------- *
 *                            PRIVILEGED OPERATIONS                          *
 * ------------------------------------------------------------------------- */
/* take care of suid/sgid super-user (root) privileges */
void
rxvt_term::privileges (int mode)
{
#if ! defined(__CYGWIN32__)
# if !defined(HAVE_SETEUID) && defined(HAVE_SETREUID)
  /* setreuid () is the poor man's setuid (), seteuid () */
#  define seteuid(a)    setreuid(-1, (a))
#  define setegid(a)    setregid(-1, (a))
#  define HAVE_SETEUID
# endif
# ifdef HAVE_SETEUID
  switch (mode)
    {
      case IGNORE:
        /*
         * change effective uid/gid - not real uid/gid - so we can switch
         * back to root later, as required
         */
        seteuid (getuid ());
        setegid (getgid ());
        break;
      case SAVE:
        euid = geteuid ();
        egid = getegid ();
        break;
      case RESTORE:
        seteuid (euid);
        setegid (egid);
        break;
    }
# else
  switch (mode)
    {
      case IGNORE:
        setuid (getuid ());
        setgid (getgid ());
        /* FALLTHROUGH */
      case SAVE:
        /* FALLTHROUGH */
      case RESTORE:
        break;
    }
# endif
#endif
}

#ifdef UTMP_SUPPORT
void
rxvt_term::privileged_utmp (char action)
{
  D_MAIN ((stderr, "rxvt_privileged_utmp (%c); waiting for: %c (pid: %d)",
          action, next_utmp_action, getpid ()));
  if (next_utmp_action != action || (action != SAVE && action != RESTORE)
      || (Options & Opt_utmpInhibit)
      || ttydev == NULL || *ttydev == '\0')
    return;

  privileges (RESTORE);
  if (action == SAVE)
    {
      next_utmp_action = RESTORE;
      makeutent (ttydev, rs[Rs_display_name]);
    }
  else
    {                    /* action == RESTORE */
      next_utmp_action = IGNORE;
      cleanutent ();
    }
  privileges (IGNORE);
}
#endif

#ifndef NO_SETOWNER_TTYDEV
void
rxvt_term::privileged_ttydev (char action)
{
  D_MAIN ((stderr,
          "privileged_ttydev (%c); waiting for: %c (pid: %d)",
          action, next_tty_action, getpid ()));
  if (next_tty_action != action || (action != SAVE && action != RESTORE)
      || ttydev == NULL || *ttydev == '\0')
    return;

  privileges (RESTORE);

  if (action == SAVE)
    {
      next_tty_action = RESTORE;
# ifndef RESET_TTY_TO_COMMON_DEFAULTS
      /* store original tty status for restoration rxvt_clean_exit () -- rgg 04/12/95 */
      if (lstat (ttydev, &ttyfd_stat) < 0)       /* you lose out */
        next_tty_action = IGNORE;
      else
# endif

      while (nexts)
        {
          chown (ttydev, getuid (), ttygid);      /* fail silently */
          chmod (ttydev, ttymode);
# ifdef HAVE_REVOKE
          revoke (ttydev);
# endif

          if (XQueryExtension (dpy, extname, &major, &first_event, &first_error) && major == event->request_code)
            {
              XGetErrorDatabaseText (dpy, "XlibMessage", "MinorCode", "Request Minor code %d", mesg, BUFSIZ);
              rxvt_warn ("+ (which is extension %s minor code %d)\n", extname, event->minor_code);

              snprintf (buffer, BUFSIZ, "%s.%d", extname, event->minor_code);
              XGetErrorDatabaseText (dpy, "XRequest", buffer, "an unregistered minor code", buffer, BUFSIZ);
              rxvt_warn ("+ (which is %s)\n", buffer);

              break;
            }

          printf ("nextss %d %s\n", nexts, extname);//D
          --nexts;
          ++exts;
        }
#else
      int nexts = 0;
      char **exts = 0;
#endif

      if (!nexts)
        {
          rxvt_warn ("+ (which is an unknown extension)\n", buffer);

          XGetErrorDatabaseText (dpy, "XlibMessage", "MinorCode", "Request Minor code %d", mesg, BUFSIZ);
          snprintf (buffer, BUFSIZ, "+ %s\n", mesg); rxvt_warn (buffer, event->minor_code);

#if 0
          sprintf (number, "RENDER.%d", event->minor_code);
          XGetErrorDatabaseText (dpy, "XRequest", number, "", buffer, BUFSIZ);
          rxvt_warn ("+ (which is %s)\n", buffer);
#endif
        }

      XFreeExtensionList (exts);
    }
  else
    {                    /* action == RESTORE */
      next_tty_action = IGNORE;
# ifndef RESET_TTY_TO_COMMON_DEFAULTS
      chmod (ttydev, ttyfd_stat.st_mode);
      chown (ttydev, ttyfd_stat.st_uid, ttyfd_stat.st_gid);
# else
      chmod (ttydev,
            (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
      chown (ttydev, 0, 0);
# endif

    }

  if (event->error_code == BadWindow
      || event->error_code == BadPixmap
      || event->error_code == BadCursor
      || event->error_code == BadFont
      || event->error_code == BadDrawable
      || event->error_code == BadColor
      || event->error_code == BadGC
      || event->error_code == BadIDChoice
      || event->error_code == BadValue
      || event->error_code == BadAtom)
    {
      if (event->error_code == BadValue)
        XGetErrorDatabaseText (dpy, "XlibMessage", "Value", "Value 0x%x", mesg, BUFSIZ);
      else if (event->error_code == BadAtom)
        XGetErrorDatabaseText (dpy, "XlibMessage", "AtomID", "AtomID 0x%x", mesg, BUFSIZ);
      else
        XGetErrorDatabaseText (dpy, "XlibMessage", "ResourceID", "ResourceID 0x%x", mesg, BUFSIZ);

# ifndef RESET_TTY_TO_COMMON_DEFAULTS
  D_MAIN ((stderr, "%s \"%s\": mode %03o, uid %d, gid %d",
          action == RESTORE ? "Restoring" : (action ==
                                             SAVE ? "Saving" :
                                             "UNKNOWN ERROR for"), ttydev,
          ttyfd_stat.st_mode, ttyfd_stat.st_uid,
          ttyfd_stat.st_gid));
# endif
}
#endif

int ecb_cold
rxvt_xerror_handler (Display *display, XErrorEvent *event)
{
  if (GET_R && GET_R->allowedxerror == -1)
    GET_R->allowedxerror = event->error_code;
  else
    {
      // GET_R is most likely not the terminal which caused the error,
      // so just output the error and continue
#if ENABLE_MINIMAL
      old_xerror_handler (display, event);
#else
      print_x_error (display, event);
#endif
    }

  return 0;
}

int ecb_cold
rxvt_xioerror_handler (Display *display)
{
  rxvt_warn ("X connection to '%s' broken, unable to recover, exiting.\n",
             DisplayString (display));
  rxvt_emergency_cleanup ();
  _exit (EXIT_FAILURE);
}

static struct sig_handlers
{
  ev::sig sw_term, sw_int;

  /*
   * Catch a fatal signal and tidy up before quitting
   */
  void sig_term (ev::sig &w, int revents);

  sig_handlers ()
  {
    sw_term.set<sig_handlers, &sig_handlers::sig_term> (this);
    sw_int .set<sig_handlers, &sig_handlers::sig_term> (this);
  }
} sig_handlers;

void
sig_handlers::sig_term (ev::sig &w, int revents)
{
  rxvt_emergency_cleanup ();
  w.stop ();
  kill (getpid (), w.signum);
}

static void
rxvt_get_ttymode (struct termios *tio)
{
  if (tcgetattr (STDIN_FILENO, tio) < 0)
    memset (tio, 0, sizeof (struct termios));

  for (int i = 0; i < NCCS; i++)
    tio->c_cc[i] = VDISABLE;

  tio->c_cc[VINTR] = CINTR;
  tio->c_cc[VQUIT] = CQUIT;
  tio->c_cc[VERASE] = CERASE;
#ifdef VERASE2
  tio->c_cc[VERASE2] = CERASE2;
#endif
  tio->c_cc[VKILL] = CKILL;
  tio->c_cc[VEOF] = CEOF;
  tio->c_cc[VSTART] = CSTART;
  tio->c_cc[VSTOP] = CSTOP;
  tio->c_cc[VSUSP] = CSUSP;
# ifdef VDSUSP
  tio->c_cc[VDSUSP] = CDSUSP;
# endif
# ifdef VREPRINT
  tio->c_cc[VREPRINT] = CRPRNT;
# endif
# ifdef VDISCRD
  tio->c_cc[VDISCRD] = CFLUSH;
# endif
# ifdef VWERSE
  tio->c_cc[VWERSE] = CWERASE;
# endif
# ifdef VLNEXT
  tio->c_cc[VLNEXT] = CLNEXT;
# endif
# ifdef VSTATUS
  tio->c_cc[VSTATUS] = CSTATUS;
# endif

# if VMIN != VEOF
  tio->c_cc[VMIN] = 1;
# endif
# if VTIME != VEOL
  tio->c_cc[VTIME] = 0;
# endif

  /* input modes */
  tio->c_iflag = (BRKINT | IGNPAR | ICRNL
# ifdef IMAXBEL
                  | IMAXBEL
# endif
                  | IXON);

  /* output modes */
  tio->c_oflag = (OPOST | ONLCR);

  /* control modes */
  tio->c_cflag = (CS8 | CREAD);

  /* local modes */
  tio->c_lflag = (ISIG | ICANON | IEXTEN | ECHO
# if defined (ECHOCTL) && defined (ECHOKE)
                  | ECHOCTL | ECHOKE
# endif
                  | ECHOE | ECHOK);
}

char **rxvt_environ; // startup environment

void
rxvt_init ()
{
  assert (("fontMask must not overlap other RS masks",
           0 == (RS_fontMask & (RS_Sel | RS_baseattrMask | RS_customMask | RS_bgMask | RS_fgMask))));

  rxvt_get_ttymode (&rxvt_term::def_tio);

  // get rid of stdin/stdout as we don't need them, to free resources
  dup2 (STDERR_FILENO, STDIN_FILENO);
  dup2 (STDERR_FILENO, STDOUT_FILENO);

  if (!ev_default_loop ())
    rxvt_fatal ("cannot initialise libev (bad value for LIBEV_METHODS?)\n");

  rxvt_environ = environ;

  signal (SIGHUP,  SIG_IGN);
  signal (SIGPIPE, SIG_IGN);

  sig_handlers.sw_term.start (SIGTERM); ev_unref ();
  sig_handlers.sw_int.start  (SIGINT);  ev_unref ();

  old_xerror_handler = XSetErrorHandler ((XErrorHandler) rxvt_xerror_handler);
  // TODO: handle this with exceptions and tolerate the memory loss
  XSetIOErrorHandler (rxvt_xioerror_handler);

  XrmInitialize ();

#if HAVE_PIXBUF
  #if !GLIB_CHECK_VERSION(2,36,0)
  g_type_init ();
  #endif
#endif
}

/*----------------------------------------------------------------------*/
/*
 * window size/position calculations for XSizeHint and other storage.
 * if width/height are non-zero then override calculated width/height
 */
void
rxvt_term::window_calc (unsigned int newwidth, unsigned int newheight)
{
  short recalc_x, recalc_y;
  int x, y, flags;
  unsigned int w, h;
  unsigned int max_width, max_height;

  D_SIZE ((stderr, "< Cols/Rows: %3d x %3d ; Width/Height: %4d x %4d",
          TermWin.ncol, TermWin.nrow, szHint.width,
          szHint.height));
  szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;
  szHint.win_gravity = NorthWestGravity;
  /* szHint.min_aspect.x = szHint.min_aspect.y = 1; */

  recalc_x = recalc_y = 0;
  flags = 0;

  if (!parsed_geometry)
    {
      parsed_geometry = 1;

      if (rs[Rs_geometry])
        flags = XParseGeometry (rs[Rs_geometry], &x, &y, &w, &h);
      if (flags & WidthValue)
        {
          TermWin.ncol = BOUND_POSITIVE_INT16 (w);
          szHint.flags |= USSize;
        }

      if (flags & HeightValue)
        {
          TermWin.nrow = BOUND_POSITIVE_INT16 (h);
          szHint.flags |= USSize;
        }

      if (flags & XValue)
        {
          szHint.x = x;
          szHint.flags |= USPosition;

          if (flags & XNegative)
            {
              recalc_x = 1;
              szHint.win_gravity = NorthEastGravity;
            }
        }

      if (flags & YValue)
        {
          szHint.y = y;
          szHint.flags |= USPosition;

          if (flags & YNegative)
            {
              recalc_y = 1;

              if (szHint.win_gravity == NorthEastGravity)
                szHint.win_gravity = SouthEastGravity;
              else
                szHint.win_gravity = SouthWestGravity;
            }
        }
    }

  /* TODO: BOUNDS */
  width = ncol * fwidth;
  height = nrow * fheight;
  max_width = MAX_COLS * fwidth;
  max_height = MAX_ROWS * fheight;

  szHint.base_width = szHint.base_height = 2 * int_bwidth;

  window_vt_x = window_vt_y = int_bwidth;

  if (scrollBar.state)
    {
      sb_w = scrollbar_TotalWidth ();
      szHint.base_width += sb_w;
      if (! (Options & Opt_scrollBar_right))
        window_vt_x = sb_w;
    }
  if (menubar_visible ())
    {
      mb_h = menuBar_TotalHeight ();
      szHint.base_height += mb_h;
      window_vt_y = mb_h;
    }
  szHint.width_inc = TermWin.fwidth;
  szHint.height_inc = TermWin.fheight;
  szHint.min_width = szHint.base_width + szHint.width_inc;
  szHint.min_height = szHint.base_height + szHint.height_inc;

  if (newwidth && newwidth - szHint.base_width < max_width)
    {
      szHint.width = newwidth;
      width = newwidth - szHint.base_width;
    }
  else
    {
      MIN_IT (TermWin.width, max_width);
      szHint.width = szHint.base_width + TermWin.width;
    }

  if (newheight && newheight - szHint.base_height < max_height)
    {
      szHint.height = newheight;
      height = newheight - szHint.base_height;
    }
  else
    {
      MIN_IT (TermWin.height, max_height);
      szHint.height = szHint.base_height + TermWin.height;
    }

  if (recalc_x)
    szHint.x += DisplayWidth  (dpy, display->screen) - szHint.width  - 2 * ext_bwidth;

  TermWin.ncol = TermWin.width / TermWin.fwidth;
  TermWin.nrow = TermWin.height / TermWin.fheight;
  D_SIZE ((stderr, "> Cols/Rows: %3d x %3d ; Width/Height: %4d x %4d",
          TermWin.ncol, TermWin.nrow, szHint.width,
          szHint.height));
  return;
}

/*----------------------------------------------------------------------*/
/*
 * Tell the teletype handler what size the window is.
 * Called after a window size change.
 */
void
rxvt_term::tt_winch ()
{
  if (pty->pty < 0)
    return;

  ws.ws_col = TermWin.ncol;
  ws.ws_row = TermWin.nrow;
  ws.ws_xpixel = ws.ws_ypixel = 0;
#ifndef DEBUG_SIZE
  (void)ioctl (cmd_fd, TIOCSWINSZ, &ws);
#else
  if (ioctl (cmd_fd, TIOCSWINSZ, &ws) < 0)
    D_SIZE ((stderr, "Failed to send TIOCSWINSZ to fd %d", fd));
# ifdef SIGWINCH
  else if (cmd_pid)               /* force through to the command */
    kill (cmd_pid, SIGWINCH);
# endif
#endif
}

/*----------------------------------------------------------------------*/
/* rxvt_change_font () - Switch to a new font */
/*
 * init = 1   - initialize
 *
 * fontname == FONT_UP  - switch to bigger font
 * fontname == FONT_DN  - switch to smaller font
 */
void
rxvt_term::change_font (int init, const char *fontname)
{}

void
rxvt_term::set_string_property (Atom prop, const char *str, int len)
{
  XChangeProperty (dpy, parent,
                   prop, XA_STRING, 8, PropModeReplace,
                   (const unsigned char *)str, len >= 0 ? len : strlen (str));
}

void
rxvt_term::set_mbstring_property (Atom prop, const char *str, int len)
{
  XTextProperty ct;

  if (XmbTextListToTextProperty (dpy, (char **)&str, 1, XStdICCTextStyle, &ct) >= 0)
    {
      XSetTextProperty (dpy, parent, &ct, prop);
      XFree (ct.value);
    }
}

void
rxvt_term::set_utf8_property (Atom prop, const char *str, int len)
{
  wchar_t *ws = rxvt_mbstowcs (str, len);
  char *s = rxvt_wcstoutf8 (ws);

  XChangeProperty (dpy, parent,
                   prop, xa[XA_UTF8_STRING], 8, PropModeReplace,
                   (const unsigned char *)s, strlen (s));

  free (s);
  free (ws);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/* xterm sequences - title, iconName, color (exptl) */
void
rxvt_term::set_title (const char *str)
{
#ifndef SMART_WINDOW_TITLE
  XStoreName (display->display, TermWin.parent[0], str);
#else
  char           *name;

  if (XFetchName (display->display, TermWin.parent[0], &name) == 0)
    name = NULL;
  if (name == NULL || STRCMP (name, str))
    XStoreName (display->display, TermWin.parent[0], str);
  if (name)
    XFree (name);
#endif
}

void
rxvt_term::set_icon_name (const char *str)
{
#ifndef SMART_WINDOW_TITLE
  XSetIconName (display->display, TermWin.parent[0], str);
#else
  char           *name;

  if (XGetIconName (display->display, TermWin.parent[0], &name))
    name = NULL;
  if (name == NULL || STRCMP (name, str))
    XSetIconName (display->display, TermWin.parent[0], str);
  if (name)
    XFree (name);
#endif
}

void
rxvt_term::set_window_color (int idx, const char *color)
{
#ifdef XTERM_COLOR_CHANGE
  if (color == NULL || *color == '\0')
    return;

  color = strdup (color);
  allocated.push_back ((void *)color);
  rs[Rs_color + idx] = color;

  /* handle color aliases */
  if (isdigit (*color))
    {
      i = atoi (color);
      if (i >= 8 && i <= 15)
        {        /* bright colors */
          i -= 8;
# ifndef NO_BRIGHTCOLOR
          PixColors[idx] = PixColors[minBrightCOLOR + i];
          SET_PIXCOLOR (idx);
          goto Done;
# endif

        }

      if (i >= 0 && i <= 7)
        { /* normal colors */
          PixColors[idx] = PixColors[minCOLOR + i];
          SET_PIXCOLOR (idx);
          goto Done;
        }
    }

  /*
   * FIXME: should free colors here, but no idea how to do it so instead,
   * so just keep gobbling up the colormap
   */
# if 0
  for (i = Color_Black; i <= Color_White; i++)
    if (PixColors[idx] == PixColors[i])
      break;
  if (i > Color_White)
    {
      /* fprintf (stderr, "XFreeColors: PixColors [%d] = %lu\n", idx, PixColors [idx]); */
      XFreeColors (display->display, XCMAP, (PixColors + idx), 1,
                  DisplayPlanes (display->display, display->screen));
    }
# endif

  PixColors[idx] = xcol;
  SET_PIXCOLOR (idx);

  /* XSetWindowAttributes attr; */
  /* Cursor cursor; */
Done:
  if (idx == Color_bg && ! (Options & Opt_transparent))
    XSetWindowBackground (display->display, TermWin.vt,
                         PixColors[Color_bg]);

  /* handle Color_BD, scrollbar background, etc. */

  set_colorfgbg ();
  recolour_cursor ();
  /* the only reasonable way to enforce a clean update */
  scr_poweron ();
}

void
rxvt_term::recolor_cursor ()
{
  XColor fg, bg;

  xcol[0] = PixColors[Color_pointer];
  xcol[1] = PixColors[Color_bg];
  XQueryColors (display->display, XCMAP, xcol, 2);
  XRecolorCursor (display->display, TermWin_cursor, & (xcol[0]), & (xcol[1]));
#endif
}

/*----------------------------------------------------------------------*/
/*
 * find if fg/bg matches any of the normal (low-intensity) colors
 */
char *
rxvt_term::get_colorfgbg ()
{
  unsigned int    i;
  const char     *xpmb = "\0";
  char            fstr[sizeof ("default") + 1], bstr[sizeof ("default") + 1];

  env_colorfgbg =
    (char *)rxvt_malloc (sizeof ("COLORFGBG=default;default;bg") + 1);
  STRCPY (fstr, "default");
  STRCPY (bstr, "default");
  for (i = Color_Black; i <= Color_White; i++)
    if (pix_colors[Color_fg] == pix_colors[i])
      {
        sprintf (fstr, "%d", (i - Color_Black));
        break;
      }

  for (i = Color_Black; i <= Color_White; i++)
    if (pix_colors[Color_bg] == pix_colors[i])
      {
        sprintf (bstr, "%d", (i - Color_Black));
#ifdef XPM_BACKGROUND
        xpmb = "default;";
#endif
        break;
      }

  sprintf (env_colorfgbg, "COLORFGBG=%s;%s%s", fstr, xpmb, bstr);

#ifndef NO_BRIGHTCOLOR
  colorfgbg = DEFAULT_RSTYLE;
  for (i = minCOLOR; i <= maxCOLOR; i++)
    {
      if (PixColors[Color_fg] == PixColors[i])
        colorfgbg = SET_FGCOLOR (colorfgbg, i);
      if (PixColors[Color_bg] == PixColors[i])
        colorfgbg = SET_BGCOLOR (colorfgbg, i);
    }
#endif
}

/*----------------------------------------------------------------------*/

bool
rxvt_term::set_color (rxvt_color &color, const char *name)
{
  if (!screen_in_out->set (display, colour))
    {
      rxvt_print_error ("can't allocate colour: %s", colour);
      return false;
    }

  rxvt_warn ("can't get colour '%s', continuing without.\n", name);
  return false;
}

void
rxvt_term::alias_color (int dst, int src)
{
  pix_colors[dst].free (this);
  pix_colors[dst].set (this, rs[Rs_color + dst] = rs[Rs_color + src]);
}

#ifdef SMART_RESIZE
static unsigned int
get_parent_bw (Display *dpy, Window w)
{
  int idummy;
  unsigned int udummy;
  Window wdummy, parent;
  Window *children;
  unsigned int nchildren, border_width;

  XQueryTree (dpy, w, &wdummy, &parent, &children, &nchildren);
  XFree (children);
  XGetGeometry (dpy, parent, &wdummy, &idummy, &idummy,
                &udummy, &udummy, &border_width, &udummy);

  return border_width;
}
#endif

/* -------------------------------------------------------------------- *
 * -                         WINDOW RESIZING                          - *
 * -------------------------------------------------------------------- */
void
rxvt_term::resize_all_windows (unsigned int newwidth, unsigned int newheight, int ignoreparent)
{
  int fix_screen;
  int old_width  = szHint.width;
  int old_height = szHint.height;

  window_calc (newwidth, newheight);

  bool set_hint = !HOOK_INVOKE ((this, HOOK_RESIZE_ALL_WINDOWS, DT_INT, newwidth, DT_INT, newheight, DT_END));

  // to avoid races between us and the wm, we clear the incremental size hints around the xresizewindow
  if (set_hint)
    {
      szHint.flags &= ~(PBaseSize | PResizeInc);
      XSetWMNormalHints (dpy, parent, &szHint);
      szHint.flags |= PBaseSize | PResizeInc;
    }

  if (!ignoreparent)
    {
#ifdef SMART_RESIZE
      /*
       * resize by Marius Gedminas <marius.gedminas@uosis.mif.vu.lt>
       * reposition window on resize depending on placement on screen
       */
      int x, y, x1, y1;
      int dx, dy;
      unsigned int unused_w1, unused_h1, unused_b1, unused_d1;
      Window unused_cr;

      XTranslateCoordinates (dpy, parent, display->root,
                             0, 0, &x, &y, &unused_cr);
      XGetGeometry (dpy, parent, &unused_cr, &x1, &y1,
                    &unused_w1, &unused_h1, &unused_b1, &unused_d1);
      /*
       * if display->root isn't the parent window, a WM will probably have offset
       * our position for handles and decorations.  Counter it
       */
      if (x1 != x || y1 != y)
        {
          unsigned int border_width = get_parent_bw (dpy, parent);

          x -= x1 + border_width;
          y -= y1 + border_width;
        }

      x1 = (DisplayWidth  (dpy, display->screen) - old_width ) / 2;
      y1 = (DisplayHeight (dpy, display->screen) - old_height) / 2;
      dx = old_width  - szHint.width;
      dy = old_height - szHint.height;

      /* Check position of the center of the window */
      if (x < x1)             /* left half */
        dx = 0;
      else if (x == x1)       /* exact center */
        dx /= 2;
      if (y < y1)             /* top half */
        dy = 0;
      else if (y == y1)       /* exact center */
        dy /= 2;

      XMoveResizeWindow (dpy, parent, x + dx, y + dy,
                         szHint.width, szHint.height);
#else
      XResizeWindow (dpy, parent, szHint.width, szHint.height);
#endif
    }

  if (set_hint)
    XSetWMNormalHints (dpy, parent, &szHint);

  fix_screen = ncol != prev_ncol || nrow != prev_nrow;

  if (fix_screen || newwidth != old_width || newheight != old_height)
    {
      if (scrollBar.state)
        scrollBar.resize ();

      if (menubar_visible ())
        XMoveResizeWindow (display->display, menuBar.win, window_vt_x,
                           0, TermWin_TotalWidth (), menuBar_TotalHeight ());

      XMoveResizeWindow (display->display, TermWin.vt, window_vt_x,
                         window_vt_y, TermWin_TotalWidth (),
                         TermWin_TotalHeight ());
      scr_clear ();
#ifdef XPM_BACKGROUND
      resize_pixmap ();
#endif

      HOOK_INVOKE ((this, HOOK_SIZE_CHANGE, DT_INT, newwidth, DT_INT, newheight, DT_END));
    }

  if (fix_screen || old_height == 0)
    scr_reset ();

      /* scr_reset only works on the primary screen */
      if (old_height)      /* this is not the first time through */
        {
          unsigned int ncol = TermWin.ncol;
          TermWin.ncol = prev_ncol; // save b/c scr_blank_screen_mem uses this
          curr_screen = scr_change_screen (PRIMARY);
          TermWin.ncol = ncol;
        }

      scr_reset ();

      if (curr_screen >= 0) /* this is not the first time through */
        {
          scr_change_screen (curr_screen);
          selection_check (old_ncol != TermWin.ncol ? 4 : 0);
        }
    }

  old_width = szHint.width;
  old_height = szHint.height;

#ifdef USE_XIM
  IMSetStatusPosition ();
#endif
}

/*
 * Set the width/height of the vt window in characters.  Units are pixels.
 * good for toggling 80/132 columns
 */
void
rxvt_term::set_widthheight (unsigned int newwidth, unsigned int newheight)
{
  XWindowAttributes wattr;

  if (newwidth == 0 || newheight == 0)
    {
      XGetWindowAttributes (display->display, display->root, &wattr);
      if (width == 0)
        width = wattr.width - szHint.base_width;
      if (height == 0)
        height = wattr.height - szHint.base_height;
    }

  if (newwidth != vt_width || newheight != vt_height)
    {
      newwidth += szHint.base_width;
      newheight += szHint.base_height;
      resize_all_windows (newwidth, newheight, 0);
    }
}

/* -------------------------------------------------------------------- *
 * -                      X INPUT METHOD ROUTINES                     - *
 * -------------------------------------------------------------------- */
#if USE_XIM

void
rxvt_term::im_set_color (unsigned long &fg, unsigned long &bg)
{
  size->x = TermWin.int_bwidth;
  size->y = TermWin.int_bwidth;
  size->width = Width2Pixel (TermWin.ncol);
  size->height = Height2Pixel (TermWin.nrow);
}

void
rxvt_term::im_set_size (XRectangle &size)
{
  // the int_bwidth terms make no sense to me
  size.x      = int_bwidth;
  size.y      = int_bwidth;
  size.width  = Width2Pixel (ncol) + int_bwidth;
  size.height = Height2Pixel (nrow) + int_bwidth;
}

void
rxvt_term::im_set_preedit_area (XRectangle &preedit_rect,
                                XRectangle &status_rect,
                                const XRectangle &needed_rect)
{
  preedit_rect.x      = needed_rect.width;
  preedit_rect.y      = 0;
  preedit_rect.width  = Width2Pixel (ncol) - needed_rect.width + 1;
  preedit_rect.height = fheight;

  status_rect.x       = 0;
  status_rect.y       = 0;
  status_rect.width   = needed_rect.width ? needed_rect.width : Width2Pixel (ncol) + 1;
  status_rect.height  = fheight;
}

/* Checking whether input method is running. */
bool
rxvt_term::im_is_running ()
{
  Atom atom;
  Window win;
  char server[IMBUFSIZ];

  /* get current locale modifier */
  if ((p = XSetLocaleModifiers (NULL)) != NULL)
    {
      STRCPY (server, "@server=");
      STRNCAT (server, & (p[4]), IMBUFSIZ - 9); /* skip "@im=" */
      if ((p = STRCHR (server + 1, '@')) != NULL)      /* first one only */
        *p = '\0';

      atom = XInternAtom (display->display, server, False);
      win = XGetSelectionOwner (display->display, atom);
      if (win != None)
        return true;
    }

  return false;
}

void
rxvt_term::im_send_spot ()
{
  XPoint nspot;
  XVaNestedList preedit_attr;

  if (Input_Context == NULL
      || !TermWin.focus || ! (input_style & XIMPreeditPosition)
      || ! (event_type == KeyPress
           || event_type == Expose
           || event_type == NoExpose
           || event_type == SelectionNotify
           || event_type == ButtonRelease || event_type == FocusIn)
      || !IMisRunning ())
    return;

  im_set_position (nspot);

  preedit_attr = XVaCreateNestedList (0, XNSpotLocation, &spot, NULL);
  XSetICValues (Input_Context, XNPreeditAttributes, preedit_attr, NULL);
  XFree (preedit_attr);
}

  spot = nspot;

  if (scrollbar_visible () && ! (Options & Opt_scrollBar_right))
    vtx = scrollbar_TotalWidth ();

  mbh = menubar_visible () ? menuBar_TotalHeight () : 0;
  mbh -= TermWin.lineSpace;

  preedit_rect->x = needed_rect->width + vtx;
  preedit_rect->y = Height2Pixel (TermWin.nrow - 1) + mbh;

  preedit_rect->width = Width2Pixel (TermWin.ncol + 1) - needed_rect->width + vtx;
  preedit_rect->height = Height2Pixel (1);

  status_rect->x = vtx;
  status_rect->y = Height2Pixel (TermWin.nrow - 1) + mbh;

  status_rect->width = needed_rect->width ? needed_rect->width : Width2Pixel (TermWin.ncol + 1);
  status_rect->height = Height2Pixel (1);
}

void
rxvt_term::im_destroy ()
{
  if (input_method)
    {
      if (Input_Context && input_method->xim)
        XDestroyIC (Input_Context);

      display->put_xim (input_method);
      input_method = 0;
    }

  Input_Context = 0;
}

#ifdef ENABLE_XIM_ONTHESPOT

static void
xim_preedit_start (XIC ic, XPointer client_data, XPointer call_data)
{
  ((rxvt_term *)client_data)->make_current ();
  HOOK_INVOKE (((rxvt_term *)client_data, HOOK_XIM_PREEDIT_START, DT_END));
}

static void
xim_preedit_done (XIC ic, XPointer client_data, XPointer call_data)
{
  ((rxvt_term *)client_data)->make_current ();
  HOOK_INVOKE (((rxvt_term *)client_data, HOOK_XIM_PREEDIT_DONE, DT_END));
}

static void
xim_preedit_draw (XIC ic, XPointer client_data, XIMPreeditDrawCallbackStruct *call_data)
{
  rxvt_term *term = (rxvt_term *)client_data;
  XIMText *text = call_data->text;

  term->make_current ();

  if (text)
    {
      wchar_t *str;

      if (!text->encoding_is_wchar && text->string.multi_byte)
        {
          // of course, X makes it ugly again
          if (term->rs[Rs_imLocale])
            SET_LOCALE (term->rs[Rs_imLocale]);

          str = rxvt_temp_buf<wchar_t> (text->length + 1);
          mbstowcs (str, text->string.multi_byte, text->length + 1);

          if (term->rs[Rs_imLocale])
            SET_LOCALE (term->locale);
        }
      else
        str = text->string.wide_char;

      HOOK_INVOKE ((term, HOOK_XIM_PREEDIT_DRAW,
                    DT_INT, call_data->caret,
                    DT_INT, call_data->chg_first,
                    DT_INT, call_data->chg_length,
                    DT_LCS_LEN, (void *)text->feedback, text->feedback ? (int)text->length : 0,
                    DT_WCS_LEN, str, str ? (int)text->length : 0,
                    DT_END));
    }
  else
    HOOK_INVOKE ((term, HOOK_XIM_PREEDIT_DRAW,
                  DT_INT, call_data->caret,
                  DT_INT, call_data->chg_first,
                  DT_INT, call_data->chg_length,
                  DT_END));
}

#if 0
static void
xim_preedit_caret (XIC ic, XPointer client_data, XIMPreeditCaretCallbackStruct *call_data)
{
  ((rxvt_term *)client_data)->make_current ();
  HOOK_INVOKE (((rxvt_term *)client_data, HOOK_XIM_PREEDIT_CARET,
                DT_INT, call_data->position,
                DT_INT, call_data->direction,
                DT_INT, call_data->style,
                DT_END));
}
#endif

#endif

/*
 * Try to open a XIM with the current modifiers, then see if we can
 * open a suitable preedit type
 */
bool
rxvt_term::im_get_ic (const char *modifiers)
{
  int i, j, found;
  XIM xim;
  XPoint spot;
  XRectangle rect, status_rect, needed_rect;
  unsigned long fg, bg;
  const char *p;
  char **s;
  XIMStyles *xim_styles;

  set_environ (env);

  if (! ((p = XSetLocaleModifiers (modifiers)) && *p))
    return false;

  D_MAIN ((stderr, "rxvt_IM_get_IC ()"));
  input_method = display->get_xim (locale, modifiers);
  if (input_method == NULL)
    return false;

  xim = input_method->xim;
  spot.x = spot.y = -1;

  xim_styles = NULL;
  if (XGetIMValues (xim, XNQueryInputStyle, &xim_styles, NULL)
      || !xim_styles || !xim_styles->count_styles)
    {
      im_destroy ();
      return false;
    }

  p = rs[Rs_preeditType] ? rs[Rs_preeditType] : "OverTheSpot,OffTheSpot,Root";
  s = rxvt_splitcommastring (p);
  for (i = found = 0; !found && s[i]; i++)
    {
      if (!STRCMP (s[i], "OverTheSpot"))
        input_style = (XIMPreeditPosition | XIMStatusNothing);
      else if (!STRCMP (s[i], "OffTheSpot"))
        input_style = (XIMPreeditArea | XIMStatusArea);
      else if (!STRCMP (s[i], "Root"))
        input_style = (XIMPreeditNothing | XIMStatusNothing);

  for (int pi = 0; pi < 2; pi++)
    {
      p = pet[pi];

      if (!p)
        continue;

      s = rxvt_strsplit (',', p);

      for (i = found = 0; !found && s[i]; i++)
        {
          if (!strcmp (s[i], "OverTheSpot"))
            input_style = XIMPreeditPosition | XIMStatusNothing;
          else if (!strcmp (s[i], "OffTheSpot"))
            input_style = XIMPreeditArea | XIMStatusArea;
          else if (!strcmp (s[i], "Root"))
            input_style = XIMPreeditNothing | XIMStatusNothing;
          else if (!strcmp (s[i], "None"))
            input_style = XIMPreeditNone | XIMStatusNone;
#ifdef ENABLE_XIM_ONTHESPOT
          else if (SHOULD_INVOKE (HOOK_XIM_PREEDIT_START) && !strcmp (s[i], "OnTheSpot"))
            input_style = XIMPreeditCallbacks | XIMStatusNothing;
#endif
          else
            input_style = XIMPreeditNothing | XIMStatusNothing;

          for (j = 0; j < xim_styles->count_styles; j++)
            if (input_style == xim_styles->supported_styles[j])
              {
                rxvt_free_strsplit (s);

                found = 1;
                goto foundpet;
              }

        }

      rxvt_free_strsplit (s);
    }

foundpet:

  XFree (xim_styles);

  if (!found)
    {
      im_destroy ();
      return false;
    }

  XFontSet fs = 0;
  XVaNestedList preedit_attr = 0, status_attr = 0;

  if (input_style & (XIMPreeditPosition | XIMPreeditArea))
    {
      // fake us a font-set, please
      char **missing_charset_list;
      int missing_charset_count;
      char *def_string;
      char pat[512];

      sprintf (pat,
               "-*-*-*-R-*-*-%d-*-*-*-*-*-*,"
               "-*-*-*-R-*-*-%d-*-*-*-*-*-*,"
               "-*-*-*-R-*-*-%d-*-*-*-*-*-*,"
               "-*-*-*-R-*-*-%d-*-*-*-*-*-*,"
               "-*-*-*-R-*-*-%d-*-*-*-*-*-*,"
               "*",
               fheight,
               fheight + 1, fheight - 1,
               fheight - 2, fheight + 2);

      fs = XCreateFontSet (dpy, rs[Rs_imFont] ? rs[Rs_imFont] : pat,
                           &missing_charset_list, &missing_charset_count, &def_string);

      if (missing_charset_list)
        XFreeStringList (missing_charset_list);

      if (!fs)
        {
          input_style &= ~(XIMPreeditPosition | XIMPreeditArea);
          rxvt_warn ("unable to create fontset for input method, try \"-pt Root\". Continuing.\n");
        }
    }

  if (input_style & XIMPreeditPosition)
    {
      im_set_size (rect);
      im_set_position (spot);
      im_set_color (fg, bg);

      preedit_attr = XVaCreateNestedList (0, XNArea, &rect,
                                         XNSpotLocation, &spot,
                                         XNForeground, fg, XNBackground, bg,
                                         //XNFontSet, TermWin.fontset,
                                         NULL);
    }
  else if (input_style & XIMPreeditArea)
    {
      im_set_color (fg, bg);

      /*
       * The necessary width of preedit area is unknown
       * until create input context.
       */
      needed_rect.width = 0;
      im_set_preedit_area (rect, status_rect, needed_rect);

      im_set_preedit_area (&rect, &status_rect, &needed_rect);

      preedit_attr = XVaCreateNestedList (0, XNArea, &rect,
                                         XNForeground, fg, XNBackground, bg,
                                         //XNFontSet, TermWin.fontset,
                                         NULL);
      status_attr = XVaCreateNestedList (0, XNArea, &status_rect,
                                        XNForeground, fg, XNBackground, bg,
                                        //XNFontSet, TermWin.fontset,
                                        NULL);
    }
#if ENABLE_XIM_ONTHESPOT
  else if (input_style & XIMPreeditCallbacks)
    {
      XIMCallback xcb[4];

  Input_Context = XCreateIC (xim, XNInputStyle, input_style,
                            XNClientWindow, TermWin.parent[0],
                            XNFocusWindow, TermWin.parent[0],
                            preedit_attr ? XNPreeditAttributes : NULL,
                            preedit_attr,
                            status_attr ? XNStatusAttributes : NULL,
                            status_attr, NULL);
  if (preedit_attr) XFree (preedit_attr);
  if (status_attr) XFree (status_attr);

  if (Input_Context == NULL)
    {
      rxvt_print_error ("failed to create input context");
      display->put_xim (input_method);
      return false;
    }

#if 0
  // unfortunately, only the focus window is used by XIM, hard to fix
  if (!XGetICValues (Input_Context, XNFilterEvents, &vt_emask_xim, NULL))
    vt_select_input ();
#endif

  im_set_position ();

  D_MAIN ((stderr, "rxvt_IM_get_IC () - successful connection"));
  return true;
}

void
rxvt_term::im_cb ()
{
  int i;
  const char *p;
  char **s;
  char buf[IMBUFSIZ];

  make_current ();

  im_destroy ();

  D_MAIN ((stderr, "rxvt_IMInstantiateCallback ()"));
  if (Input_Context)
    return;

  if (rs[Rs_imLocale])
    rxvt_set_locale (rs[Rs_imLocale]);
#endif

  p = rs[Rs_inputMethod];
  if (p && *p)
    {
      bool found = false;

      s = rxvt_strsplit (',', p);

      for (i = 0; s[i]; i++)
        {
          if (*s[i])
            {
              strcpy (buf, "@im=");
              strncat (buf, s[i], IMBUFSIZ - 5);
              if (im_get_ic (buf))
                {
                  found = true;
                  break;
                }
            }
        }
      for (i = 0; s[i]; i++)
        free (s[i]);
      free (s);

      if (found)
        goto done;
    }

  /* try with XMODIFIERS env. var. */
  if (im_get_ic (""))
    goto done;

  /* try with no modifiers base IF the user didn't specify an IM */
  if (im_get_ic ("@im=none"))
    goto done;

done:
  if (rs[Rs_imLocale])
    rxvt_set_locale (locale);
#endif
}

void
rxvt_term::im_set_position ()
{
  XRectangle preedit_rect, status_rect, *needed_rect;
  XVaNestedList preedit_attr, status_attr;

  if (Input_Context == NULL
      || !TermWin.focus || ! (input_style & XIMPreeditArea)
      || !IMisRunning ())
    return;

  /* Getting the necessary width of preedit area */
  status_attr = XVaCreateNestedList (0, XNAreaNeeded, &needed_rect, NULL);
  XGetICValues (Input_Context, XNStatusAttributes, status_attr, NULL);
  XFree (status_attr);

      XSetICValues (Input_Context,
                    XNPreeditAttributes, preedit_attr, NULL);
    }
  else
    {
      /* Getting the necessary width of preedit area */
      status_attr = XVaCreateNestedList (0, XNAreaNeeded, &needed_rect, NULL);
      XGetICValues (Input_Context, XNStatusAttributes, status_attr, NULL);
      XFree (status_attr);

  preedit_attr = XVaCreateNestedList (0, XNArea, &preedit_rect, NULL);
  status_attr = XVaCreateNestedList (0, XNArea, &status_rect, NULL);

  XSetICValues (Input_Context,
               XNPreeditAttributes, preedit_attr,
               XNStatusAttributes, status_attr, NULL);

  XFree (preedit_attr);
  XFree (status_attr);
}
#endif /* USE_XIM */

void
rxvt_term::get_window_origin (int &x, int &y)
{
  Window cr;
  XTranslateCoordinates (dpy, parent, display->root, 0, 0, &x, &y, &cr);
}

/*----------------------- end-of-file (C source) -----------------------*/
