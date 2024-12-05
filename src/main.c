#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "dhcp.h"
#include "format.h"
#include "port_utils.h"
#include "server.h"

static bool get_args (int, char **);

bool debug = false;
bool call_server = false;
bool thread = false;

int
main (int argc, char **argv)
{
  get_args (argc, argv);
  if (debug)
    fprintf (stderr, "Shutting down\n");

  if (call_server)
    {
      echo_server ();
    }
  return EXIT_SUCCESS;
}

static bool
get_args (int argc, char **argv)
{
  int ch = 0;
  while ((ch = getopt (argc, argv, "dhst:")) != -1)
    {
      switch (ch)
        {
        case 'd':
          debug = true;
          break;
        case 's':
          call_server = true;
          break;
        case 't':
          thread = true;
          break;
        default:
          return false;
        }
    }
  return true;
}
