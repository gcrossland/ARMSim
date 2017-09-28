/* -------------------------------------------------------------------
   Debug Stream Library V1.20                                HEADER
   (c) Geoffrey Crossland 1999, 2000, 2001

   V1.00 : Permits streaming of text to a file, for debug purposes.
   V1.05 : New interactive mode, which displays text dynamically in
           another window (console or task window)
   V1.10 : Implements generic cbannel model, providing multiple
           streams simultaneously (this provides a thread-safe
           design model).
   V1.11 : If an attempt is made to use an unallocated stream,
           it is ignored.
   V1.12 : Finally, I've broken and optimised the output routines
           for early-abort on unallocated streams.
   V1.20 : Brought back into play an ANSI C implementation.
           Introduced the debug_int type, so that general types of
           streams could be held. debug_renter and debug_rexit have
           been removed and replaced with debug_enter and
           debug_exit, which use GCC variables to access functions'
           names. printd and printdc now accept a variable number
           of arguments.
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   General macro, variable and structure definitions
------------------------------------------------------------------- */
typedef int debug_int;


#define true   1
#define false  0


#define DEBUG_MAXSTREAMS 256
#define DEBUG_MAXLINELEN 1024


/* Parts of debug_indent also act as a semaphore for whether a
   stream is in use or not.
*/
#define DEBUG_INDENT_DEPTH             0x0FFFFFFF
#define DEBUG_INDENT_TYPE              0xF0000000
#define DEBUG_INDENT_TYPE_unallocated  0x00000000
#define DEBUG_INDENT_TYPE_file         0x10000000
#define DEBUG_INDENT_TYPE_interactive  0x20000000


#define printd(S,args...) debug_print((S),true,##args)


#define printdc(S,args...) debug_print((S),false,##args)


#ifdef WIN32A
#define debug_printindent(S)                                         \
{                                                                    \
  if ((debug_indent[(S)] & DEBUG_INDENT_TYPE)!=                      \
      DEBUG_INDENT_TYPE_unallocated)                                 \
  {                                                                  \
    debug_space[debug_indent[(S)] & DEBUG_INDENT_DEPTH]=0;           \
                                                                     \
    if ((debug_indent[(S)] & DEBUG_INDENT_TYPE)==                    \
        DEBUG_INDENT_TYPE_interactive)                               \
    {                                                                \
      DWORD debug_outtemp;                                           \
                                                                     \
      WriteFile((HANDLE) debug_streams[(S)],debug_space,             \
                (debug_indent[(S)] & DEBUG_INDENT_DEPTH),            \
                &debug_outtemp,NULL);                                \
    }                                                                \
    else                                                             \
    {                                                                \
      fprintf((FILE*) debug_streams[(S)],debug_space);               \
      fflush(NULL);                                                  \
    }                                                                \
                                                                     \
    debug_space[debug_indent[(S)] & DEBUG_INDENT_DEPTH]=' ';         \
  }                                                                  \
}
#else
#define debug_printindent(S)                                         \
{                                                                    \
  if ((debug_indent[(S)] & DEBUG_INDENT_TYPE)!=                      \
      DEBUG_INDENT_TYPE_unallocated)                                 \
  {                                                                  \
    debug_space[debug_indent[(S)] & DEBUG_INDENT_DEPTH]=0;           \
    fprintf((FILE*) debug_streams[(S)],debug_space);                 \
    fflush(NULL);                                                    \
    debug_space[debug_indent[(S)] & DEBUG_INDENT_DEPTH]=' ';         \
  }                                                                  \
}
#endif


#ifdef WIN32A
#define debug_print(S,I,args...)                                     \
{                                                                    \
  char debug_str[DEBUG_MAXLINELEN];                                  \
                                                                     \
  if ((debug_indent[(S)] & DEBUG_INDENT_TYPE)!=                      \
      DEBUG_INDENT_TYPE_unallocated)                                 \
  {                                                                  \
    if ((I))                                                         \
    {                                                                \
      debug_printindent((S));                                        \
    }                                                                \
                                                                     \
    if ((debug_indent[(S)] & DEBUG_INDENT_TYPE)==                    \
        DEBUG_INDENT_TYPE_interactive)                               \
    {                                                                \
      DWORD debug_outtemp;                                           \
                                                                     \
      WriteFile((HANDLE) debug_streams[(S)],debug_str,               \
                sprintf(debug_str,##args),&debug_outtemp,NULL);      \
    }                                                                \
    else                                                             \
    {                                                                \
      fprintf((FILE*) debug_streams[(S)],##args);                    \
      fflush(NULL);                                                  \
    }                                                                \
  }                                                                  \
}
#else
#define debug_print(S,I,args...)                                     \
{                                                                    \
  char debug_str[DEBUG_MAXLINELEN];                                  \
                                                                     \
  if ((debug_indent[(S)] & DEBUG_INDENT_TYPE)!=                      \
      DEBUG_INDENT_TYPE_unallocated)                                 \
  {                                                                  \
    if ((I))                                                         \
    {                                                                \
      debug_printindent((S));                                        \
    }                                                                \
                                                                     \
    fprintf((FILE*) debug_streams[(S)],##args);                      \
    fflush(NULL);                                                    \
  }                                                                  \
}
#endif


#define debug_enter(S)                                               \
{                                                                    \
  if ((debug_indent[(S)] & DEBUG_INDENT_TYPE)!=                      \
      DEBUG_INDENT_TYPE_unallocated)                                 \
  {                                                                  \
    printd((S),"Entering function %s:\n",__FUNCTION__);              \
                                                                     \
    debug_indent[(S)]=(debug_indent[(S)] & DEBUG_INDENT_TYPE) |      \
                      ((debug_indent[(S)]+2) & DEBUG_INDENT_DEPTH);  \
  }                                                                  \
}


#define debug_exit(S)                                                \
{                                                                    \
  if ((debug_indent[(S)] & DEBUG_INDENT_TYPE)!=                      \
      DEBUG_INDENT_TYPE_unallocated)                                 \
  {                                                                  \
    debug_indent[(S)]=(debug_indent[(S)] & DEBUG_INDENT_TYPE) |      \
                      ((debug_indent[(S)]-2) & DEBUG_INDENT_DEPTH);  \
                                                                     \
    printd((S),"Exiting function %s:\n",__FUNCTION__);               \
  }                                                                  \
}



/* -------------------------------------------------------------------
   External variables and routines
------------------------------------------------------------------- */
extern debug_int debug_streams[DEBUG_MAXSTREAMS];
extern debug_int debug_indent[DEBUG_MAXSTREAMS];
extern chr debug_space[DEBUG_MAXSTREAMS];


extern void debug_init (void);
extern void debug_newstream (int stream,chr *filename);
extern void debug_closestream (int stream);
extern void debug_fin (void);
