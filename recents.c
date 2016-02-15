/**
*************************************************************
* @file recents.c
* @brief basic recent files management from terminal
* This tiny program allows you to clean recent files from
* terminal. Useful for scripting.
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 30 ene 2016
*
* Changelog
*   20160215: Initial release
*
* Changelog:
*  - Make Makefile
*  - Documentation
*  - Recent file removal
*  - Recent file list
*
* Compile:
* $ gcc -g -o recents recents.c $(pkg-config --libs --cflags gtk+-3.0)
*************************************************************/

/*
    recents - basic recent files management from terminal
    Copyright (C) 2016 Gaspar Fernandez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/* Compile with: 
   gcc -g -o recents recents.c $(pkg-config --libs --cflags gtk+-3.0)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <gtk-3.0/gtk/gtk.h>
#include <gio/gio.h>
#include <stdio_ext.h>
#include <sys/types.h>
#include <utime.h>
#include <errno.h>

#define VERSION 0.1
#define UNSTABLE 1

#define DEFAULT_MIME_TYPE "application/octet-stream"

typedef struct recent_file_options_t
{
  GSList* fileNames;
  uint8_t touchFile;
  uint8_t force;
  uint8_t quiet;
  int result;
} recent_file_options_t;

typedef enum recents_main_action_t
{
  RMA_NONE,
  RMA_INCLUDE,
  RMA_CLEAR,
  RMA_HELP
} recents_main_action_t;

int8_t file_exists(const char *filename)
{
  FILE* f = fopen(filename, "r");
  if (f == NULL)
    return 0;

  fclose(f);
  return 1;
}

char* default_mime_type()
{
  char *res;
  res = g_malloc(sizeof(char) * (strlen(DEFAULT_MIME_TYPE)+1));
  strcpy(res, DEFAULT_MIME_TYPE);
  return res;
}

char* get_mime(const char* fileName)
{
  char *res;
  char *content_type = g_content_type_guess (fileName, NULL, 0, NULL);
  if (content_type == NULL)
    return default_mime_type();
  char *mime_type = g_content_type_get_mime_type (content_type);
  g_free(content_type);
  if (mime_type == NULL)
    return default_mime_type();

  res = g_malloc(sizeof(char) * (strlen(mime_type)+1));
  strcpy(res, mime_type);

  g_free(mime_type);
  return res;
}

gboolean main_include_task(gpointer _options)
{
  GtkRecentManager *grm = gtk_recent_manager_get_default();
  GtkRecentData *data;
  GSList* iterator = NULL;
  recent_file_options_t* options = _options;
  unsigned added = 0;		/* Total files added */

  static gchar* groups[] = {
    NULL
  };
  
  for (iterator = options->fileNames; iterator; iterator = iterator->next) 
    {
      char* _fileName = (char*)iterator->data;
      if (!file_exists(_fileName))
	{
	  if (!options->quiet)
	    fprintf (stderr, "Error '%s' does not exist!\n", _fileName);
	  continue;
	}

      char* fileName = realpath(_fileName, NULL);
      if (fileName==NULL)
	{
	  if (!options->quiet)
	    fprintf (stderr, "Error getting '%s' path!\n", _fileName);
	  continue;
	}
      data = g_slice_new(GtkRecentData);
      data->display_name=g_strdup(fileName);
      data->description = NULL;
      data->mime_type=get_mime(fileName);
      data->app_name = (gchar*) g_get_application_name();
      data->app_exec = g_strdup("recents");
      data->groups = groups;
      data->is_private = FALSE;
      gchar *uri = g_filename_to_uri(fileName, NULL, NULL);
      if (gtk_recent_manager_add_full(grm, uri, data)) 
	{
	  if (!options->quiet)
	    printf("File '%s' added successfully\n", fileName);
	  ++added;
	}

      if (options->touchFile)
	{
	  struct utimbuf utb;
	  time_t now = time(NULL);
	  utb.actime = now;
	  utb.modtime = now;
	  if ( (utime (fileName, &utb)<0) && (!options->quiet) )
	    {
	      fprintf (stderr, "Could not touch '%s' (errno: %d, %s)\n", fileName, errno, strerror(errno));
	    }
	}
      free(fileName);
    }

  options->result = (added == g_slist_length(options->fileNames))?0:100;
  gtk_main_quit();
}

gboolean main_clear_task(gpointer _options)
{
  GtkRecentManager *grm = gtk_recent_manager_get_default();
  recent_file_options_t* options = _options;

  if (!options->quiet)
    printf ("No Silencioso\n");
  else
    printf ("Silencioso\n");

  gtk_recent_manager_purge_items(grm, NULL);
  options->result = 34;
  gtk_main_quit();
}

void initialize_options(recent_file_options_t* options)
{
  options->fileNames = NULL;
  options->touchFile= FALSE;
  options->force=0;
  options->quiet=0;
  options->result=0;
}

void help()
{
  FILE* output = stdout;

  fprintf(output, "recents comes with ABSOLUTELY NO WARRANTY.  This is free software, and you\n"
	  "are welcome to redistribute it under certain conditions.  See the GNU\n"
	  "General Public Licence for details.\n\n");
  fprintf(output, "recents is a simple recents file manager allowing you to add and clear\n"
	  "your recent files special folder in a GTK+ based environment.\n\n");

  fprintf(output, "Usage: recents [-qat] FILE [FILE2] ... [FILEn]\n"
	  "  or   recents [-qfc]\n"
	  "  or   recents [-h]\n\n");
  fprintf(output, "Options\n"
	  " -q, --quiet\t\tQuiet mode. No unnecesary output. Useful for scripting\n"
	  " -a, --add\t\tAdd files to recent files. Files must be specified as\n"
	  "\t\t\targuments after the options\n"
	  " -t, --touch\t\tWhen adding files to recent files, these files are touched\n"
	  "\t\t\tto update their modification date. You can now order recent files\n"
          "\t\t\tby date and see them at the top.\n"
	  " -c, --clear\t\tClear recent files.\n"
	  " -f, --force\t\tForce clean option. Doesn't prompt for confirmation.\n"
	  " -h, --help\t\tShows this help.\n\n");
  fprintf(output, "If you want more information about this software, report bugs, suggestions or\n"
	  "any comment, please go to http://gaspar.totaki.com/\n\n");
}

void fatal(char* error, int usage)
{
  fprintf (stderr, "There was an unexpected error: \n");
  fprintf (stderr, "\t%s\n\n", error);
  if (usage)
    help();
  exit(1);
}

void check_config (recent_file_options_t* options, recents_main_action_t action, int optind, int argc)
{
  if (action == RMA_INCLUDE)
    {
      if (options->force)
	fprintf (stderr, "Force option (-f) will be ignored as we are just adding files.\n");
      if (optind==argc)
	fatal ("No files specified\n", 0);
    }
  else if (action == RMA_CLEAR)
    {
      if (options->touchFile)
	fprintf (stderr, "Touch option (-t) will be ignores as we are clearing recent files.\n");
    }
}

gboolean confirm_delete(recent_file_options_t* options)
{
  char option;
  static char yes = 'y';
  static char no = 'n';

  if (options->force)
    return TRUE;

  if (!options->quiet)
    printf ("Are you sure you want to clear recent files (%c/%c) ", yes, no);

  option = tolower(getchar());
  __fpurge(stdin);
  while ( (option != yes) && (option != no) )
    {
      if (!options->quiet)
	printf ("Please press %c or %c\n", yes, no);
      while (getchar()!='\n');
      option = tolower(getchar());
      __fpurge(stdin);
    }
  return (option == yes);
}

int main(int argc, char*argv[])
{
  extern char* optarg;
  extern int optind;
  extern int optopt;
  extern int opterr;
  int c;
  int i;
  int oindex;
  opterr = 0;
  recent_file_options_t options;
  recents_main_action_t action = RMA_NONE;
  char bad_option[2]="\0\0";

  initialize_options(&options);

  gtk_init(&argc, &argv);	/* Initialize GTK+ */
  struct option cli_options[] = {
    {"quiet", 0, NULL, 'q'},
    {"force", 0, NULL, 'f'},
    {"add", 0, NULL, 'a'},
    {"touch", 0, NULL, 't'},
    {"clear", 0, NULL, 'c'},
    {"help", 0, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  while ((c = getopt_long (argc, argv, "qfatch", cli_options, &oindex)) != -1)
    {
      switch (c)
	{
	case 0:
	  fatal (g_strconcat("Option ", optarg, " not recognized", NULL), 1);
	  break;		/* Not neccessary */
	case '?':
	  bad_option[0]=optopt;
	  fatal (g_strconcat("Option -", bad_option, " not recognized", NULL), 1);
	  break;		/* Not neccesary */
	case 'q':
	  options.quiet = 1;
	  break;
	case 'f':
	  options.force = 1;
	  break;
	case 'a':
	  if (action != RMA_NONE)
	    fatal ("Only one action can be performed", 1);

	  action = RMA_INCLUDE;
	  break;
	case 't':
	  options.touchFile = 1;
	  break;
	case 'c':
	  if (action != RMA_NONE)
	    fatal ("Only one action can be performed", 1);

	  action = RMA_CLEAR;
	  break;
	case 'h':
	  action = RMA_HELP;	/* Help will be shown without error */
	  break;
	default:
	  fatal("Bad usage. Please read help below", 1);
	}
    }

  if (options.quiet)
    fprintf (stdout, "recents - basic recent files management from terminal\n"
	     "by Gaspar Fernandez (2016) http://gaspar.totaki.com/\n\n");

  /* This will exit on error */
  check_config (&options, action, optind, argc);

  if (action == RMA_HELP)
    {
      help();
      return 0;
    }
  else if (action == RMA_INCLUDE)
    {
      for (i=optind; i<argc; ++i)
	{
	  options.fileNames = g_slist_append(options.fileNames, argv[i]);
	}
      g_idle_add(main_include_task, &options);
    }
  else if (action == RMA_CLEAR)
    {
      if (confirm_delete(&options))
	g_idle_add(main_clear_task, &options);
      else
	return 2;
    }
  else
    exit(2);
  gtk_main();

  return options.result;
}
