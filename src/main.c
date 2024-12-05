#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "dhcp.h"
#include "format.h"
#include "port_utils.h"
#include "server.h"

static bool get_args (int, char **, int *);

bool debug = false;
bool call_server = false;
bool thread_flag = false;

int
main (int argc, char **argv)
{
  int timeout = 0;
  get_args (argc, argv, &timeout);
  if (debug)
    fprintf (stderr, "Shutting down\n");

  if (call_server && !thread_flag)
    echo_server (timeout);
  if (call_server && thread_flag)
    echo_server_thread (timeout);
  return EXIT_SUCCESS;
}

static bool
get_args (int argc, char **argv, int *timeout)
{
  int ch = 0;
  while ((ch = getopt (argc, argv, "dht:s:")) != -1)
    {
      switch (ch)
        {
        case 'd':
          debug = true;
          break;
        case 's':
          call_server = true;
          *timeout = strtol (optarg, NULL, 10);
          break;
        case 't':
          thread_flag = true;
          call_server = true;
          *timeout = strtol (optarg, NULL, 10);
          break;
        default:
          return false;
        }
    }
  return true;
}
