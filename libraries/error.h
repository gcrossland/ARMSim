/* -------------------------------------------------------------------
   Error Report Library V1.15                           HEADER FILE
   (c) Geoffrey Crossland 1999, 2000, 2002

   V1.00 : Displays errors and ends the task.
   V1.05 : Provides options for the error handler to display
           additional information.
   V1.10 : Now supports non-fatal errors.
   V1.15 : Supports both an ANSI C-based and a Win32
           implementation. Provides a routine to terminate the
           task.
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   General macro, variable and structure definitions
------------------------------------------------------------------- */
typedef i32 error_int;


#define ERROR_NUMBER    0x0000FFFF
#define ERROR_mem       (1<<16)
#define ERROR_justtext  (2<<16)

#define ERROR_STYLE_ok      (1<<0)
#define ERROR_STYLE_cancel  (1<<1)
#define ERROR_STYLE_yes     (1<<2)
#define ERROR_STYLE_no      (1<<3)

#ifdef WIN32A
#define ERROR_RESULT_ok      IDOK
#define ERROR_RESULT_cancel  IDCANCEL
#define ERROR_RESULT_yes     IDYES
#define ERROR_RESULT_no      IDNO
#else
#define ERROR_RESULT_ok      ERROR_STYLE_ok
#define ERROR_RESULT_cancel  ERROR_STYLE_cancel
#define ERROR_RESULT_yes     ERROR_STYLE_yes
#define ERROR_RESULT_no      ERROR_STYLE_no
#endif



/* -------------------------------------------------------------------
   External variables and routines
------------------------------------------------------------------- */
extern void error_exit (void);
extern void error_fatal (chr *string,error_int number);
extern error_int error_nonfatal (chr *string,error_int style);
