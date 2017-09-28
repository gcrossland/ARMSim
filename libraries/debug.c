/* -------------------------------------------------------------------
   Debug Stream Library V1.20                          C COMPONENTS
   (c) Geoffrey Crossland 1999, 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "../header.h"
#ifdef DEBUG_MAXSTREAMS


debug_int debug_streams[DEBUG_MAXSTREAMS];
debug_int debug_indent[DEBUG_MAXSTREAMS];
chr debug_space[DEBUG_MAXSTREAMS];


void debug_init (void)
{
  int loop;

  for (loop=0;loop<DEBUG_MAXSTREAMS;loop++)
  {
    debug_streams[loop]=0;
    debug_indent[loop]=DEBUG_INDENT_TYPE_unallocated;
    debug_space[loop]=' ';
  }
}



void debug_newstream (int stream,char *filename)
{
  if (filename==0)
  {
    #ifdef WIN32A
    AllocConsole();
    debug_streams[stream]=(int) GetStdHandle(STD_OUTPUT_HANDLE);
    #else
    debug_streams[stream]=(int) stdout;
    setvbuf((FILE*) stdout,0,_IONBF,0);
    #endif

    debug_indent[stream]=DEBUG_INDENT_TYPE_interactive;
  }
  else
  {
    debug_streams[stream]=(int) fopen(filename,"w");
    debug_indent[stream]=DEBUG_INDENT_TYPE_file;
    setvbuf((FILE*) debug_streams[stream],0,_IONBF,0);
  }
}



void debug_closestream (int stream)
{
  if ((debug_indent[stream] & DEBUG_INDENT_TYPE)!=
      DEBUG_INDENT_TYPE_unallocated)
  {
    if ((debug_indent[stream] & DEBUG_INDENT_TYPE)==
        DEBUG_INDENT_TYPE_interactive)
    {
      #ifdef WIN32A
      FreeConsole();
      #endif
    }
    else
    {
      fclose((FILE*) debug_streams[stream]);
    }
  }

  debug_indent[stream]=DEBUG_INDENT_TYPE_unallocated;
}



void debug_fin (void)
{
  int loop;

  for (loop=0;loop<DEBUG_MAXSTREAMS;loop++)
  {
    debug_closestream(loop);
  }
}
#endif
