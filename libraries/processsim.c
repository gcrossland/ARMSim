/* -------------------------------------------------------------------
   ProcessSim Library V1.06                            C COMPONENTS
   (c) Geoffrey Crossland 2000, 2002
   Program by Geoffrey Crossland
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "../header.h"



/* -------------------------------------------------------------------
   void processsim_init

   Initialises the ProcessSim library.
------------------------------------------------------------------- */
struct processsim_fu_table_element *processsim_fu_table=0;
struct processsim_pipe_table_element *processsim_pipe_table=0;
struct processsim_pipe_table_element **processsim_gpipe_table=0;
i32 processsim_main_clocks_count=0;
char *processsim_main_output_path;
i32 processsim_main_output_step=1;
clock_t processsim_main_secs_start;



void processsim_init (i32 gpipe_count,char *output_path,
                      i32 output_step)
{
  processsim_gpipe_table=
  mem_alloc(sizeof(struct processsim_pipe_table_element*)*
            gpipe_count);
  processsim_main_output_path=output_path;
  processsim_main_output_step=output_step;
}



/* -------------------------------------------------------------------
   void processsim_lock

   Declares that the user has finished adding Functional Units and
   Pipes to the system. This must be called before the simulation
   is started.
------------------------------------------------------------------- */
i32 processsim_output_x_max,processsim_output_x_min,
    processsim_output_x_size,processsim_output_y_max,
    processsim_output_y_min,processsim_output_y_size;
char **processsim_output_buffers,*processsim_output_html;
ui8 processsim_output_outputting;



void processsim_lock (void)
{
  struct processsim_fu_table_element *element=processsim_fu_table;
  i32 x_max=0,x_min=0x7FFFFFFF,y_max=0,y_min=0x7FFFFFFF,
      html_size=0;
  char **output_buffers,**output_buffers_internal;

  while (element!=(struct processsim_fu_table_element*) 0)
  {
    if ((*element).output_buffer!=0)
    {
      if ((*element).output_x>x_max)
      {
        x_max=(*element).output_x;
      }

      if ((*element).output_x<x_min)
      {
        x_min=(*element).output_x;
      }

      if ((*element).output_y>y_max)
      {
        y_max=(*element).output_y;
      }

      if ((*element).output_y<y_min)
      {
        y_min=(*element).output_y;
      }
    }

    element=(*element).next;
  }

  /* We define the min values to be inclusive and the max values
     to be exclusive.
  */
  x_max++;
  y_max++;


  output_buffers=mem_alloc(sizeof(char*)*(x_max-x_min)*
                           (y_max-y_min));
  output_buffers_internal=output_buffers+(x_max-x_min)*(y_max-y_min);
  do
  {
    *(--output_buffers_internal)=0;
  }
  while (output_buffers_internal>output_buffers);

  element=processsim_fu_table;

  while (element!=(struct processsim_fu_table_element*) 0)
  {
    if ((*element).output_buffer!=0)
    {
      *(output_buffers+(((*element).output_y-y_min)*(x_max-x_min))+
        ((*element).output_x-x_min))=(*element).output_buffer;
      html_size+=(*element).output_buffer_size;
    }

    element=(*element).next;
  }

  /* Give us enough space to create the output file filenames at
     the end.
  */
  html_size+=strlen(processsim_main_output_path)+19;

  processsim_output_x_max=x_max;
  processsim_output_x_min=x_min;
  processsim_output_x_size=100/(x_max-x_min);
  processsim_output_y_max=y_max;
  processsim_output_y_min=y_min;
  processsim_output_y_size=100/(y_max-y_min);
  processsim_output_buffers=output_buffers;
  processsim_output_html=mem_alloc(html_size);

  processsim_main_secs_start=clock();
}



/* -------------------------------------------------------------------
   i32 processsim_main

   Performs one cycle of the simulation universe.
------------------------------------------------------------------- */
extern void processsim_main_print (void);

void processsim_main_print (void)
{
  {
    struct processsim_fu_table_element *next;
    struct processsim_pipe_table_element **slots;

    printd(50,"\n\nEnumerating the Functional Unit table:\n\n",0);

    next=processsim_fu_table;

    while (next!=NULL)
    {
      printd(50,"\nElement &%X:\n",next);
      printd(50,"Executor at &%X\n",(*next).executor);

      slots=(*next).pipes;
      printd(50,"Pipes at &%X:\n",slots);
      while (*slots!=NULL)
      {
        printd(50,"  &%X\n",*slots);
        slots++; 
      }

      next=(*next).next;
    }
  }

  {
    struct processsim_pipe_table_element *next;

    printd(50,"\n\nEnumerating the Pipe table:\n\n",0);

    next=processsim_pipe_table;

    while (next!=NULL)
    {
      printd(50,"\nElement &%X:\n",next);
      printd(50,"Data is &%X\n",(*next).data);
      printd(50,"Goes between &%X and &",(*next).fus[0]);
      printdc(50,"%X\n",(*next).fus[1]);
      next=(*next).next;
    }
  }

  printd(50,"\n\n",0);
}



i32 processsim_main (ui8 output)
{
  struct processsim_fu_table_element *element;

  processsim_output_outputting=output;

  element=processsim_fu_table;

  while (element!=(struct processsim_fu_table_element*) 0)
  {
    (*((*element).executor))((void*) element);
    element=(*element).next;
  }

  if (output==true)
  {
    processsim_main_output();
  }

  processsim_main_clocks_count++;

  return true;
}



/* -------------------------------------------------------------------
   i32 processsim_main_clocks

   Returns the number of completed cycles.
------------------------------------------------------------------- */
i32 processsim_main_clocks (void)
{
  return processsim_main_clocks_count;
}



/* -------------------------------------------------------------------
   f32 processsim_main_secs

   Returns the number of seconds since the simulation was locked.
------------------------------------------------------------------- */
f32 processsim_main_secs (void)
{
  return (((f32) (clock()-processsim_main_secs_start))/
          ((f32) CLOCKS_PER_SEC));
}



/* -------------------------------------------------------------------
   ui8 processsim_main_outputting

   Returns whether an output file is to be created for the current
   cycle or not.
------------------------------------------------------------------- */
ui8 processsim_main_outputting (void)
{
  return processsim_output_outputting;
}



/* -------------------------------------------------------------------
   void processsim_main_output

   Generates files of the current state of the simulation, based
   on the output buffers of the Functional Units.
------------------------------------------------------------------- */
#define strupd(T,S)                                                  \
{                                                                    \
  char *source=(S);                                                  \
                                                                     \
  while ((*((T)++)=*(source++))!=0);                                 \
  (T)--;                                                             \
}



void processsim_main_output (void)
{
  i32 x_loop,y_loop,cycle,output_size;
  char **output_buffers,**output_buffers_internal,*html_internal;

  html_internal=processsim_output_html;
  output_buffers_internal=processsim_output_buffers;
  cycle=processsim_main_clocks();

  html_internal+=sprintf(html_internal,
                         "<TABLE border=0 width=100%% height=85%%>"
                         "<TR><TH align=center valign=middle colspan=%d>"
                         "After Cycle %d:"
                         "</TH></TR>",processsim_output_x_max-
                                      processsim_output_x_min,cycle);

  for (y_loop=processsim_output_y_min;y_loop<processsim_output_y_max;
       y_loop++)
  {
    strupd(html_internal,"<TR>");

    for (x_loop=processsim_output_x_min;
         x_loop<processsim_output_x_max;x_loop++)
    {
      html_internal+=
      sprintf(html_internal,
              "<TD align=center valign=middle width=%d%% height=%d%%>",
              processsim_output_x_size,processsim_output_y_size);

      if (*output_buffers_internal==0)
      {
        strupd(html_internal,"&nbsp;");
      }
      else
      {
        strupd(html_internal,*output_buffers_internal);
      }

      strupd(html_internal,"</TD>");

      output_buffers_internal++;
    }

    strupd(html_internal,"</TR>");
  }

  strupd(html_internal,"</TABLE></DIV></BODY></HTML>");
  output_size=html_internal-processsim_output_html;
  html_internal++;

  sprintf(html_internal,"%s/d%07X.htm",
          processsim_main_output_path,cycle);
  processsim_main_output_file (html_internal,true,cycle,
                               processsim_output_html,output_size);

  sprintf(html_internal,"%s/s%07X.htm",
          processsim_main_output_path,cycle);
  processsim_main_output_file (html_internal,false,cycle,
                               processsim_output_html,output_size);
}



/* -------------------------------------------------------------------
   void processsim_main_output_file

   Generates the individual output files
------------------------------------------------------------------- */
void processsim_main_output_file (char *filename,ui8 dynamic,
                                  i32 cycle,char *output,
                                  i32 output_size)
{
  FILE *file_handle;
  int result;
  char buffer[256],*error;

  do
  {
    if ((file_handle=fopen(filename,"w"))!=NULL)
    {
      if (dynamic)
      {
        result=fprintf(file_handle,
                       "<HTML>"
                       "<HEAD><TITLE>ARMSim Results</TITLE>"
                       "<META http-equiv=\"refresh\" content=\"%d; url=d%07X.htm\"></HEAD>"
                       "<BODY>"
                       "<DIV align=center>"
                       "<TABLE border=0 width=100%% height=10%%><TR>"
                       "<TD align=center valign=middle width=100%%>"
                       "<FORM action=\"s%07X.htm\" method=get>"
                       "<INPUT type=submit accesskey=s value=\"Stop\">"
                       "</FORM>"
                       "</TD>"
                       "</TR></TABLE>",processsim_main_output_step,
                       cycle+1,cycle);
      }
      else
      {
        result=fprintf(file_handle,
                       "<HTML>"
                       "<HEAD><TITLE>ARMSim Results</TITLE></HEAD>"
                       "<BODY>"
                       "<DIV align=center>"
                       "<TABLE border=0 width=100%% height=10%%><TR>"
                       "<TD align=center valign=middle width=33%%>"
                       "<FORM action=\"s%07X.htm\" method=get>"
                       "<INPUT type=submit accesskey=b value=\"Back\">"
                       "</FORM>"
                       "</TD>"
                       "<TD align=center valign=middle width=34%%>"
                       "<FORM action=\"d%07X.htm\" method=get>"
                       "<INPUT type=submit accesskey=s value=\"Step\">"
                       "</FORM>"
                       "</TD>"
                       "<TD align=center valign=middle width=33%%>"
                       "<FORM action=\"s%07X.htm\" method=get>"
                       "<INPUT type=submit accesskey=f value=\"Forward\">"
                       "</FORM>"
                       "</TD>"
                       "</TR></TABLE>",cycle-1,cycle+1,cycle+1);
      }

      if (result>=0)
      {
/*        if (fprintf(file_handle,"%s",output)>=0)*/
        if (fwrite(output,sizeof(char),output_size,file_handle)==
            output_size)
        {
          if (fclose(file_handle)==0)
          {
            return;
          }
          else
          {
            error="unable to complete writing to and closing of file";
          }
        }
        else
        {
          fclose(file_handle);
          error="unable to create file body";
        }
      }
      else
      {
        fclose(file_handle);
        error="unable to create file header";
      }
    }
    else
    {
      error="unable to open file";
    }

    sprintf(buffer,"Unable to complete creation of output file %s (%s). Retry?",filename,error);

    if (error_nonfatal(buffer,ERROR_STYLE_ok | ERROR_STYLE_cancel)==
        ERROR_RESULT_cancel)
    {
      error_fatal("Unable to create output file",(dynamic ? 7 : 8));
    }
  }
  while (true);
}



/* -------------------------------------------------------------------
   void *processsim_fu_gen

   Adds a Functional Unit to the current list, returning its
   handle.
------------------------------------------------------------------- */
void *processsim_fu_gen (void (*executor)(void*),i32 pipe_count,
                         i32 output_buffer_size,i32 output_x,
                         i32 output_y)
{
  struct processsim_fu_table_element *next;
  struct processsim_pipe_table_element **pipes;

  next=processsim_fu_table;
  processsim_fu_table=mem_alloc(sizeof(struct
                                       processsim_fu_table_element));
  (*processsim_fu_table).executor=executor;
  (*processsim_fu_table).workspace=(void*) 0;
  pipes=mem_alloc(sizeof(struct processsim_pipe_table_element*)*
                  pipe_count);
  (*processsim_fu_table).pipes=pipes;
  (*processsim_fu_table).output_buffer_size=output_buffer_size;
  if (output_buffer_size!=0)
  {
    (*processsim_fu_table).output_x=output_x;
    (*processsim_fu_table).output_y=output_y;
    *((*processsim_fu_table).output_buffer=
      mem_alloc(output_buffer_size))=0;
  }
  else
  {
    (*processsim_fu_table).output_buffer=0;
  }
  (*processsim_fu_table).next=next;

  return ((void*) processsim_fu_table);
}



/* -------------------------------------------------------------------
   void *processsim_pipe_gen

   Adds a Pipe to the current list and associates it with two
   Functional Units, returning its handle. If both Functional Units
   are PROCESSSIM_FU_NULL and both Pipe values are equal, the Pipe
   is assumed to be a Global Pipe.
------------------------------------------------------------------- */
void *processsim_pipe_gen (void *fu0,i32 fu0_pipe,void *fu1,
                           i32 fu1_pipe)
{
  struct processsim_pipe_table_element *next;

  printd(50,"\n\nNew pipe:\n",0);

  next=processsim_pipe_table;
  processsim_pipe_table=
  mem_alloc(sizeof(struct processsim_pipe_table_element));
  (*processsim_pipe_table).data=(void*) 0;
  (*processsim_pipe_table).fus[0]=(struct
                                   processsim_fu_table_element*) fu0;
  (*processsim_pipe_table).fus[1]=(struct
                                   processsim_fu_table_element*) fu1;
  (*processsim_pipe_table).next=next;

  if (fu0==PROCESSSIM_FU_NULL && fu1==PROCESSSIM_FU_NULL &&
      fu0_pipe==fu1_pipe)
  {
    *(processsim_gpipe_table+fu0_pipe)=processsim_pipe_table;
  }
  else
  {
    *(((*((struct processsim_fu_table_element*) fu0)).pipes)+
      fu0_pipe)=
    *(((*((struct processsim_fu_table_element*) fu1)).pipes)+
      fu1_pipe)=
    processsim_pipe_table;
  }



/*  struct processsim_pipe_table_element *next,**pipes_old,**pipes_new,
                                **pipes_old_internal,**pipes_ptr;
  i32 pipes_loop,pipes_count,pipe;

  pipes_ptr=&((*((struct processsim_fu_table_element*) fu0)).pipes);
  for (pipes_loop=2;pipes_loop>0;pipes_loop--)
  {
    printd(50,"Old pipes value: &%X\n",*pipes_ptr);

    pipes_old_internal=pipes_old=pipes_new=*pipes_ptr;
    pipes_count=1;
    do
    {
      pipes_count++;
    }
    while (*(pipes_new++)!=(struct processsim_pipe_table_element*) 0);

    *pipes_ptr=pipes_new=mem_alloc(pipes_count*
                                   sizeof(struct
                                          processsim_pipe_table_element*));

    printd(50,"New pipes value: &%X\n",*pipes_ptr);

    while ((*(pipes_new++)=*(pipes_old_internal++))!=
           (struct processsim_pipe_table_element*) 0);

    *(pipes_new-1)=processsim_pipe_table;
    *pipes_new=(struct processsim_pipe_table_element*) 0;
    mem_free(pipes_old);

    pipes_ptr=&((*((struct processsim_fu_table_element*) fu1)).pipes);
  }*/


  return ((void*) processsim_pipe_table);
}



/* -------------------------------------------------------------------
   void *processsim_pipe_read

   Reads the current value of a Pipe associated with a Functional
   Unit. If the Functional Unit is PROCESSSIM_FU_NULL, the Pipe is
   assumed to be a Global Pipe.
------------------------------------------------------------------- */
void *processsim_pipe_read (void *fu,i32 pipe)
{
  if (fu==PROCESSSIM_FU_NULL)
  {
    return (*(*(processsim_gpipe_table+pipe))).data;
  }
  else
  {
    return (*(*(((*((struct processsim_fu_table_element*) fu)).pipes)+
                pipe))).data;
  }
}



/* -------------------------------------------------------------------
   void processsim_pipe_write

   Writes the current value of a Pipe associated with a Functional
   Unit. If the Functional Unit is PROCESSSIM_FU_NULL, the Pipe is
   assumed to be a Global Pipe.
------------------------------------------------------------------- */
void processsim_pipe_write (void *fu,i32 pipe,void *value)
{
  if (fu==PROCESSSIM_FU_NULL)
  {
    (*(*(processsim_gpipe_table+pipe))).data=value;
  }
  else
  {
    (*(*(((*((struct processsim_fu_table_element*) fu)).pipes)+
         pipe))).data=value;
  }
}



/* -------------------------------------------------------------------
   void *processsim_workspace_read

   Returns the workspace pointer associated with a Functional Unit.
------------------------------------------------------------------- */
void *processsim_workspace_read (void *fu)
{
  return (*((struct processsim_fu_table_element*) fu)).workspace;
}



/* -------------------------------------------------------------------
   void processsim_workspace_write

   Writes the workspace pointer associated with a Functional Unit.
------------------------------------------------------------------- */
void processsim_workspace_write (void *fu,void *value)
{
  (*((struct processsim_fu_table_element*) fu)).workspace=value;
}



/* -------------------------------------------------------------------
   char *processsim_output_read

   Returns the output buffer pointer associated with a Functional
   Unit.
------------------------------------------------------------------- */
char *processsim_output_read (void *fu)
{
  return (*((struct processsim_fu_table_element*) fu)).output_buffer;
}
