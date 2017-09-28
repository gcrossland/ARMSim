/* -------------------------------------------------------------------
   Error Report Library V1.15                          C COMPONENTS
   (c) Geoffrey Crossland 1999, 2000, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "../header.h"



/* -------------------------------------------------------------------
   void error_exit

   Terminates the process.
------------------------------------------------------------------- */
void error_exit (void)
{
  #ifdef WIN32A
  DWORD win32_exit_code;
  #endif

  debug_fin();

  #ifdef WIN32A
  GetExitCodeProcess(OpenProcess(0,FALSE,GetCurrentProcessId()),
                     &win32_exit_code);
  ExitProcess((UINT) win32_exit_code);
  #else
  exit(1);
  #endif
}



/* -------------------------------------------------------------------
   void error_fatal

   Displays the text of the error and aborts.

   Each error is given number, between 0 and 65535. The top 16 bits
   of the error carry type flags, which direct the error handler to
   alter the given text in some way or clarify the situation.
------------------------------------------------------------------- */
void error_fatal (chr *string,error_int number)
{
  chr buffer[512];

  /* 0- used so far */

  switch (number & ~ERROR_NUMBER)
  {
  case ERROR_mem:
    sprintf(buffer,"The system is probably out of memory. %s (%d)",
            string,(number & ERROR_NUMBER));
    break;
  case ERROR_justtext:
    sprintf(buffer,"%s",string);
    break;
  default:
    #ifdef WIN32A
    sprintf(buffer,"%s (internal error %d, last Win32 error %d)",
            string,(number & ERROR_NUMBER),GetLastError());
    #else
    sprintf(buffer,"%s (internal error %d)",string,(number &
                                                    ERROR_NUMBER));
    #endif
  }

  #ifdef WIN32A
  MessageBox(NULL,buffer,"Error",
             (UINT) (MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK));
  #else
  printf("Error: %s\nPress any key to continue",buffer);
  getchar();
  printf("\n");
  #endif

  error_exit();
}



/* -------------------------------------------------------------------
   error_int error_nonfatal

   Displays the text of the error. Returns a result code or zero,
   on failure
------------------------------------------------------------------- */
error_int error_nonfatal (chr *string,error_int style)
{
  #ifdef WIN32A
  UINT win32_uint;

  if (style==ERROR_STYLE_ok)
  {
    win32_uint=MB_TASKMODAL | MB_OK;
  }
  else
  {
    if (style==(ERROR_STYLE_ok | ERROR_STYLE_cancel))
    {
      win32_uint=MB_TASKMODAL | MB_OKCANCEL;
    }
    else
    {
      if (style==(ERROR_STYLE_yes | ERROR_STYLE_no))
      {
        win32_uint=MB_TASKMODAL | MB_YESNO;
      }
      else
      {
        if (style==(ERROR_STYLE_yes | ERROR_STYLE_no |
                    ERROR_STYLE_cancel))
        {
          win32_uint=MB_TASKMODAL | MB_YESNOCANCEL;
        }
        else
        {
          win32_uint=MB_TASKMODAL | MB_OK;
        }
      }
    }
  }

  return ((error_int) MessageBox(NULL,(LPCTSTR) string,
                                 "Error",win32_uint));
  #else
  int input;

  printf("Error: %s\n",string);

  if (style!=0)
  {
    ui8 first=true;

    if ((style & ERROR_STYLE_ok)!=0)
    {
      printf("%s(O)k",(first ? first=false,"" : " "));
    }

    if ((style & ERROR_STYLE_cancel)!=0)
    {
      printf("%s(C)ancel",(first ? first=false,"" : " "));
    }

    if ((style & ERROR_STYLE_yes)!=0)
    {
      printf("%s(Y)es",(first ? first=false,"" : " "));
    }

    if ((style & ERROR_STYLE_no)!=0)
    {
      printf("%s(N)o",(first ? first=false,"" : " "));
    }

    printf(" ?");
  }
  else
  {
    printf("Press any key to continue");
  }

  while (true)
  {
    input=getchar();

    if (style==0)
    {
      printf("\n");
      return ERROR_RESULT_ok;
    }
    else
    {
      if ((input=='O' || input=='o') && (style & ERROR_STYLE_ok)!=0)
      {
        printf(" Ok\n");
        return ERROR_RESULT_ok;
      }

      if ((input=='C' || input=='c') && (style &
                                         ERROR_STYLE_cancel)!=0)
      {
        printf(" Cancel\n");
        return ERROR_RESULT_cancel;
      }

      if ((input=='Y' || input=='y') && (style & ERROR_STYLE_yes)!=0)
      {
        printf(" Yes\n");
        return ERROR_RESULT_yes;
      }

      if ((input=='N' || input=='n') && (style & ERROR_STYLE_no)!=0)
      {
        printf(" No\n");
        return ERROR_RESULT_no;
      }
    }
  }
  #endif
}
