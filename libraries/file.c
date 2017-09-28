/* -------------------------------------------------------------------
   File Handling Library V1.11                         C COMPONENTS
   (c) Geoffrey Crossland 1999, 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "../header.h"



/* -------------------------------------------------------------------
   file_int file_getsize

   Given a filename, returns the number of bytes in the file, or
   0, for an error (or an empty file).
------------------------------------------------------------------- */
file_int file_getsize (chr *filename)
{
  FILE *filehandle;
  long int filesize;

/*  printd(1,"Opening the file...\n",0);*/

  if ((filehandle=fopen(filename,"rb"))==NULL)
  {
    return 0;
  }

/*  printd(1,"Seeking the end...\n",0);*/

  if (fseek(filehandle,(long int) 0,SEEK_END)!=0)
  {
    fclose(filehandle);
    return 0;
  }

/*  printd(1,"Getting the position...\n",0);*/

  if ((filesize=ftell(filehandle))==-1)
  {
    fclose(filehandle);
    return 0;
  }

/*  printd(1,"Returning...\n",0);*/

  fclose(filehandle);

  return ((file_int) filesize);

/*  i32f error;

  reg[0]=17;
  reg[1]=(i32f) filename;
  error=swi(OS_File,reg);

  if ((error & SWI_VBIT)==SWI_VBIT)
  {
    return 0;
  }
  else
  {
    return ((i32) reg[4]);
  }*/
}



/* -------------------------------------------------------------------
   void *file_load

   Given a filename, the file's size and an appropriately sized
   block of memory, loads the file into it. The returned handle
   is zero on failure
------------------------------------------------------------------- */
void *file_load (chr *filename,file_int filesize,void *block)
{
  FILE *filehandle;

/*  printd(1,".\n",0);*/

  if ((filehandle=fopen(filename,"rb"))==NULL)
  {
    return 0;
  }

/*  printd(1,"..\n",0);*/

  if (fseek(filehandle,(long int) 0,SEEK_SET)!=0)
  {
    fclose(filehandle);
    return 0;
  }

/*  printd(1,"...\n",0);

  printd(1,"There are %d bytes in the file.\n",filesize);
  printd(1,"The addressing target is &%X\n",block);

  {
    int loop;

    for (loop=0;loop<filesize;loop++)
    {
      printd(1,"Byte index %d:\n",loop);
      *(loop+((char*) block))=0;
    }
  }*/


  if (fread(block,(size_t) sizeof(char),(size_t) filesize,
            filehandle)!=((size_t) filesize))
  {
    fclose(filehandle);
    return 0;
  }

/*  printd(1,"....\n",0);*/

  fclose(filehandle);

/*  printd(1,".....\n",0);*/

  return ((void*) block);



/*  i32f error;
  char *file_block;

  file_block=(char*) malloc((size_t) filesize);

  reg[0]=16;
  reg[1]=(i32f) filename;
  reg[2]=(i32f) file_block;
  reg[3]=0;
  swi(OS_File,reg);


  if ((error & SWI_VBIT)==SWI_VBIT)
  {
    free((void*) file_block);
    return 0;
  }
  else
  {
    return file_block;
  }*/
}



/* -------------------------------------------------------------------
   void *file_load_bmp

   Given a filename of a Windows Bitmap file, loads the data and
   creates the bitmap.
------------------------------------------------------------------- */
#ifdef WIN32A
void *file_load_bmp (chr *filename,LPBITMAPINFO *win32_bitmapinfo_ret)
{
  HANDLE win32_handle;
  BITMAPFILEHEADER win32_bitmapfileheader;
  BITMAPINFOHEADER win32_bitmapinfoheader;
  BITMAPINFO *win32_bitmapinfo;
  DWORD win32_dword;
  WORD *win32_pixelarray;

  win32_handle=CreateFile((LPCTSTR) filename,(DWORD) GENERIC_READ,
                          (DWORD) FILE_SHARE_READ,
                          (LPSECURITY_ATTRIBUTES) NULL,
                          (DWORD) OPEN_EXISTING,
                          (DWORD) FILE_ATTRIBUTE_READONLY,
                          (HANDLE) NULL);

  ReadFile(win32_handle,&win32_bitmapfileheader,
           sizeof(BITMAPFILEHEADER),&win32_dword,
           (LPOVERLAPPED) NULL);
  ReadFile(win32_handle,&win32_bitmapinfoheader,
           sizeof(BITMAPINFOHEADER),&win32_dword,
           (LPOVERLAPPED) NULL);

  win32_bitmapinfo=mem_alloc(sizeof(BITMAPINFOHEADER)+
                             ((1<<win32_bitmapinfoheader.biBitCount)*
                              sizeof(RGBQUAD)));

  printd(1,"the address of the bitmapinfo structure is &%X\n",win32_bitmapinfo);

  *win32_bitmapinfo_ret=win32_bitmapinfo;

  ((*win32_bitmapinfo).bmiHeader).biSize=
  win32_bitmapinfoheader.biSize;
  ((*win32_bitmapinfo).bmiHeader).biWidth=
  win32_bitmapinfoheader.biWidth;
  ((*win32_bitmapinfo).bmiHeader).biHeight=
  win32_bitmapinfoheader.biHeight;
  ((*win32_bitmapinfo).bmiHeader).biPlanes=
  win32_bitmapinfoheader.biPlanes;
  ((*win32_bitmapinfo).bmiHeader).biBitCount=
  win32_bitmapinfoheader.biBitCount;
  ((*win32_bitmapinfo).bmiHeader).biCompression=
  win32_bitmapinfoheader.biCompression;
  ((*win32_bitmapinfo).bmiHeader).biSizeImage=
  win32_bitmapinfoheader.biSizeImage;
  ((*win32_bitmapinfo).bmiHeader).biXPelsPerMeter=
  win32_bitmapinfoheader.biXPelsPerMeter;
  ((*win32_bitmapinfo).bmiHeader).biYPelsPerMeter=
  win32_bitmapinfoheader.biYPelsPerMeter;
  ((*win32_bitmapinfo).bmiHeader).biClrUsed=
  win32_bitmapinfoheader.biClrUsed;
  ((*win32_bitmapinfo).bmiHeader).biClrImportant=
  win32_bitmapinfoheader.biClrImportant;
  ReadFile(win32_handle,(*win32_bitmapinfo).bmiColors,
           (1<<win32_bitmapinfoheader.biBitCount)*sizeof(RGBQUAD),
           &win32_dword,(LPOVERLAPPED) NULL);

  win32_pixelarray=mem_alloc(win32_bitmapfileheader.bfSize-
                             win32_bitmapfileheader.bfOffBits);
  ReadFile(win32_handle,win32_pixelarray,
           win32_bitmapfileheader.bfSize-
           win32_bitmapfileheader.bfOffBits,
           &win32_dword,(LPOVERLAPPED) NULL);

  CloseHandle(win32_handle);


  return ((void*) win32_pixelarray);
}
#endif



/* -------------------------------------------------------------------
   file_int file_save

   Given a filename, a pointer to a block and the size of the
   block, creates the stated file and dumps the block's contents
   wholesale to the file. Returns false on failure, true on success.
------------------------------------------------------------------- */
file_int file_save (chr *filename,file_int filesize,void *block)
{
  #ifdef WIN32A
  HANDLE filehandle;
  DWORD scrap;

/*  printd(1,"File about to be created\n",0);*/

  filehandle=CreateFile(filename,(DWORD) (GENERIC_READ |
                                          GENERIC_WRITE),(DWORD) 0,
             (LPSECURITY_ATTRIBUTES) NULL,(DWORD) CREATE_ALWAYS,
             (DWORD) FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);

  if (WriteFile(filehandle,(LPCVOID) block,(DWORD) filesize,&scrap,
	                (LPOVERLAPPED) NULL)==FALSE)
  {
/*    printd(1,"File save failed\nAbout to close file\n",0);*/

    CloseHandle(filehandle);

/*    printd(1,"File closed\nReturning...\n",0);*/

    return false;
  }

/*  printd(1,"File save succeeded\nAbout to close file\n",0);*/

  CloseHandle(filehandle);

  return true;
  #else
  FILE *filehandle;

  if ((filehandle=fopen(filename,"wb"))==NULL)
  {
    return false;
  }

/*  printd(1,"filehandle=&%X\n",filehandle);
  printd(1,"*filehandle=&%X\n",*filehandle);

  printd(1,"File created\nFile about to be saved\n",0);*/

  if (fseek(filehandle,(long int) 0,SEEK_SET)!=0)
  {
    fclose(filehandle);
    return false;
  }

/*  {
    char *ptr,*ptr_end;

    printd(1,"\nEnumerating the memory:\n",0);

    ptr=(char*) block;
    ptr_end=ptr+filesize;

    for (;ptr<ptr_end;ptr++)
    {
      printd(1," %d\n",*ptr);
    }
  }

  printd(1,"Now:\n",0);*/

/*  printd(1,"Just set position\nNow about to test one byte\n",0);*/

/*  fputc((int) 18,filehandle);*/

/*  printd(1,"We're realy going to do it now... honest...\n",0);*/

  if (fwrite(block,(size_t) sizeof(char),(size_t) filesize,
             filehandle)!=((size_t) filesize))
  {
    printd(1,"File save failed\nAbout to close file\n",0);

    fclose(filehandle);

    printd(1,"File closed\nReturning...\n",0);

    return false;
  }

/*  printd(1,"File closed\nReturning...\n",0);*/

    fclose(filehandle);

  return true;
  #endif
}



/* -------------------------------------------------------------------
   fileh file_open_new

   Creates and opens the given file, returning the file handle. If
   the file already exists, it is erased. The routine returns false
   on failure.
------------------------------------------------------------------- */
fileh file_open_new (chr *filename)
{
  FILE *filehandle;

  if ((filehandle=fopen(filename,"wb"))==NULL)
  {
    return ((fileh) false);
  }

  return ((fileh) filehandle);
}



/* -------------------------------------------------------------------
   fileh file_open_old

   Opens the given file, returning the file handle. The file must
   already exist. The routine returns false on failure.
------------------------------------------------------------------- */
fileh file_open_old (chr *filename)
{
  FILE *filehandle;

  if ((filehandle=fopen(filename,"rb+"))==NULL)
  {
    return ((fileh) false);
  }

  return ((fileh) filehandle);
}



/* -------------------------------------------------------------------
   file_int file_close

   Closes the given file, returning false on error and true
   otherwise.
------------------------------------------------------------------- */
file_int file_close (fileh filehandle)
{
  if (fclose((FILE*) filehandle)==EOF)
  {
    return false;
  }

  return true;
}



/* -------------------------------------------------------------------
   file_int file_move

   Moves the file pointer of the given file to the given byte
   position.
------------------------------------------------------------------- */
file_int file_move (fileh filehandle,i32 offset)
{
  if ((fseek((FILE*) filehandle,(long int) offset,(int) SEEK_SET))!=0)
  {
    return false;
  }

  return true;
}



/* -------------------------------------------------------------------
   ui8 file_get_ui8

   Gets the byte in the given file, at its current file pointer,
   and increments the file pointer.
------------------------------------------------------------------- */
ui8 file_get_ui8 (fileh filehandle)
{
  return ((ui8) fgetc((FILE*) filehandle));
}



/* -------------------------------------------------------------------
   file_int file_put_ui8

   Puts the byte into the given file, at its current file pointer,
   and increments the file pointer. False is returned on failure,
   while true is returned on success.
------------------------------------------------------------------- */
file_int file_put_ui8 (fileh filehandle,ui8 value)
{
  if (fputc((int) value,(FILE*) filehandle)==EOF)
  {
    return false;
  }

  return true;
}



/* -------------------------------------------------------------------
   i32 file_get_i32

   Gets the 32-bit word in the given file, at its current file
   pointer, and increments the file pointer by four bytes.
------------------------------------------------------------------- */
i32 file_get_i32 (fileh filehandle)
{
  i32f output;

  output=fgetc((FILE*) filehandle);
  output+=fgetc((FILE*) filehandle)<<8;
  output+=fgetc((FILE*) filehandle)<<16;
  output+=fgetc((FILE*) filehandle)<<24;

  return output;
}



/* -------------------------------------------------------------------
   file_int file_put_i32

   Puts the 32-bit word into the given file, at its current file
   pointer, and increments the file pointer by four bytes. False is
   returned on failure, while true is returned on success.
------------------------------------------------------------------- */
file_int file_put_i32 (fileh filehandle,i32 value)
{
  if (fputc((int) (value & 0xFF),(FILE*) filehandle)==EOF)
  {
    return false;
  }

  if (fputc((int) ((value>>8) & 0xFF),(FILE*) filehandle)==EOF)
  {
    return false;
  }

  if (fputc((int) ((value>>16) & 0xFF),(FILE*) filehandle)==EOF)
  {
    return false;
  }

  if (fputc((int) ((value>>24) & 0xFF),(FILE*) filehandle)==EOF)
  {
    return false;
  }

  return true;
}
