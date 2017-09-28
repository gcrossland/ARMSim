/* -------------------------------------------------------------------
   File Handling Library V1.11                          HEADER FILE
   (c) Geoffrey Crossland 1999, 2000, 2001, 2002

   V1.00 : Provides access to files, reading and writing data of
           various sizes.
   V1.05 : Provides incremental enhancements.
   V1.10 : Has facilities for maintaining data structures in a
           format convenient for both memory use and export to
           file.
   V1.11 : Reinstated ANSI C-based file handling code.
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   General macro, variable and structure definitions
------------------------------------------------------------------- */
typedef i32 file_int;



/* -------------------------------------------------------------------
   External variables and routines
------------------------------------------------------------------- */
extern file_int file_getsize (chr *filename);
extern void *file_load (chr *filename,file_int filesize,void *block);
#ifdef WIN32A
extern void *file_load_bmp (chr *filename,
                            LPBITMAPINFO *win32_bitmapinfo_ret);
#endif
extern file_int file_save (chr *filename,file_int filesize,
                           void *block);
extern fileh file_open_new (chr *filename);
extern fileh file_open_old (chr *filename);
extern file_int file_close (fileh filehandle);
extern file_int file_move (fileh filehandle,file_int offset);
extern ui8 file_get_ui8 (fileh filehandle);
extern file_int file_put_ui8 (fileh filehandle,ui8 value);
extern i32 file_get_i32 (fileh filehandle);
extern file_int file_put_i32 (fileh filehandle,i32 value);
