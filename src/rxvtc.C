#include "../config.h"
#include "rxvtdaemon.h"

#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <libptytty.h>

#include "rxvtdaemon.h"

#define STATUS_SUCCESS           0
#define STATUS_FAILURE           1
#define STATUS_CONNECTION_FAILED 2

struct client : rxvt_connection
{
  client ();
};

client::client ()
{
  sockaddr_un sa;
  char *sockname = rxvt_connection::unix_sockname ();

  if (strlen (sockname) >= sizeof (sa.sun_path))
    {
      fputs ("socket name too long, aborting.\n", stderr);
      exit (STATUS_FAILURE);
    }

  if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      perror ("unable to create communications socket");
      exit (STATUS_FAILURE);
    }

  sa.sun_family = AF_UNIX;
  strcpy (sa.sun_path, sockname);
  free (sockname);

  if (connect (fd, (sockaddr *)&sa, sizeof (sa)))
    {
      perror ("unable to connect to the rxvt-unicode daemon");
      exit (STATUS_CONNECTION_FAILED);
    }
}

extern char **environ;

int
main (int argc, const char *const *argv)
{
  // instead of getcwd we could opendir (".") and pass the fd for fchdir *g*
  char cwd[PATH_MAX];

  if (!getcwd (cwd, sizeof (cwd)))
    {
      perror ("unable to determine current working directory");
      exit (STATUS_FAILURE);
    }

  client c;

  {
    sigset_t ss;

    sigemptyset (&ss);
    sigaddset (&ss, SIGHUP);
    sigaddset (&ss, SIGPIPE);
    sigprocmask (SIG_BLOCK, &ss, 0);
  }

  if (argc >= 2 && !strcmp (argv[1], "-k"))
    {
      c.send ("QUIT");
      return 0;
    }

  c.send ("NEW");
  // instead of getcwd we could opendir (".") and pass the fd for fchdir *g*
  c.send ("CWD"), c.send (getcwd (buf, sizeof (buf)));

  for (char **var = environ; *var; var++)
    c.send ("ENV"), c.send (*var);

  const char *base = strrchr (argv[0], '/');
  base = base ? base + 1 : argv[0];
  c.send ("ARG"), c.send (strcmp (base, RXVTNAME "c") ? base : RXVTNAME);

  c.send ("ARG"), c.send ("-cd");
  c.send ("ARG"), c.send (cwd);

  for (int i = 1; i < argc; i++)
    c.send ("ARG"), c.send (argv[i]);

  c.send ("END");

  auto_str tok;

  for (;;)
    if (!c.recv (tok))
      {
        fprintf (stderr, "protocol error: unexpected eof from server.\n");
        break;
      }
    else if (!strcmp (tok, "MSG") && c.recv (tok))
      fprintf (stderr, "%s", (const char *)tok);
    else if (!strcmp (tok, "END"))
      {
        int success;
        if (c.recv (success))
          exit (success ? EXIT_SUCCESS : EXIT_FAILURE);
      }
    else
      {
        fprintf (stderr, "protocol error: received illegal token '%s'.\n", (const char *)tok);
        break;
      }

  return EXIT_FAILURE;
}

