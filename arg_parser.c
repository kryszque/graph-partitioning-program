#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include "arg_parser.h"
const char *argp_program_version =
"G_DIV 1.0";


struct arguments arguments;

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] =
{
  {"input", 'i',"input", 0, "Input file"},
  {"parts",   'p', "parts", 0,
   "Amount of parts"},
  {"margin",   'm', "margin", 0,
   "Margin 10-100%"},
  {"output", 'o', "output", 0, "Output file"},
  {"format",  'f', "format", 0,
   "Output file format <txt|bin>"},
  {0}
};


static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'i':
      arguments->input = arg;
      break;
    case 'p':
      arguments->parts= atof(arg)/100;
      break;
    case 'm':
      arguments->margin = atoi(arg);
      break;
    case 'o':
      arguments->output = arg;
      break;
    case 'f':
      arguments->format = arg;
      break;
        default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/*
   ARGS_DOC. Field 3 in ARGP.
   A description of the non-option command-line arguments
     that we accept.
*/

/*
  DOC.  Field 4 in ARGP.
  Program documentation.
*/
static char doc[] =
"G_DIV -- A graph division program.";

/*
   The ARGP structure itself.
*/
static struct argp argp = {options, parse_opt, doc};


// Function to parse arguments
int parse_arguments(int argc, char **argv)
{
  // Set default values
  arguments.input = NULL;
  arguments.output = NULL;
  arguments.parts = 1;
  arguments.margin = 0.1;
  arguments.format = "txt";

  // Parse arguments
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Validate arguments
  if (arguments.input == NULL) {
    fprintf(stderr, "Error: Input file is required\n");
    return 1;
  }

  return 0;
}

// Getter function for arguments
struct arguments* get_arguments()
{
  return &arguments;
}



