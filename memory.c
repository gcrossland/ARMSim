/* -------------------------------------------------------------------
   ARMSim V1.06                                        C COMPONENTS
   (c) Geoffrey Crossland 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "header.h"



/* -------------------------------------------------------------------
   void system_lsu

   The Functional Unit representing the ARM's Load/Store Unit.
------------------------------------------------------------------- */
i32 incadd=0;

extern void system_lsu_testbed (void *fu);

void system_lsu_testbed (void *fu)
{
  struct system_lsu_request *request;

  printd(4,"\nEntered system_lsu_testbed:\n",0);

  if ((request=processsim_pipe_read(fu,0))==0)
  {
    printd(4,"  No active requests\n",0);

    request=mem_alloc(sizeof(struct system_lsu_request));

    printd(4,"  Generating request of block &%X\n",request);

    if (incadd<8)
    {
      printd(4,"    Byte read\n",0);
      (*request).addr=incadd++;
      (*request).word=false;
      (*request).load=true;
    }
    else if (incadd<16)
    {
      printd(4,"    Word read\n",0);
      (*request).addr=incadd-8;
      incadd+=4;
      (*request).word=true;
      (*request).load=true;
    }
    else if (incadd<24)
    {
      printd(4,"    Byte write\n",0);
      (*request).value=incadd-16+'i';
      (*request).addr=(incadd++)-16;
      (*request).word=false;
      (*request).load=false;
    }
    else if (incadd<32)
    {
      printd(4,"    Word write\n",0);
      (*request).value=0xDEADDEAD;
      (*request).addr=incadd-24;
      incadd+=4;
      (*request).word=true;
      (*request).load=false;
    }
    else
    {
      printd(4,"    No request made\n",0)
      mem_free(request);
      (*request).index=0;
      request=0;
    }

    processsim_pipe_write(fu,0,request);
  }

  if (request!=0)  /* request always !=0 after the above code! */
  {
    if ((*request).index!=-1)
    {
      printd(4,"Waiting for the LSU to return with the results\n",0);
    }
    else
    {
      printd(4,"Request has been serviced\n",0);
      printd(4,"  The returned value is %d or &",(int) (*request).value);
      printdc(4,"%X or '",(int) (*request).value);
      printd(4,"%c'\n",(int) (*request).value);

/*      mem_free(request);*/
      processsim_pipe_write(fu,0,0);

      printd(4,"  Pipe cleared\n",0);
    }
  }
}



void system_lsu (void *fu)
{
  struct system_lsu_state *state;
  char *output_buffer;
  struct system_lsu_request *request;
  ui8f *addr;
  i32 pipe_loop,pipe_width,serviced_count;

  state=processsim_workspace_read(fu);
  output_buffer=processsim_output_read(fu);

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=2>Load/Store Unit</TH>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle colspan=2><EM>Request Queue</EM></TD>"
          "</TR>"
          "<TR><TD colspan=2>"
          "<TABLE width=100%% border=1><TR>");

  printd(4,"\nEntered system_lsu:\n",0);

  pipe_width=(i32) (((ieee32) 100)/((ieee32) SYSTEM_LSU_count));

  printd(4,"Display:\n",0);

  for (pipe_loop=0;pipe_loop<SYSTEM_LSU_count;pipe_loop++)
  {
    printd(4,"  Displaying for pipe %d:\n",pipe_loop);

    output_buffer+=
    oprintf(output_buffer,
            "<TD align=left valign=top width=%d%%>",
            (int) pipe_width);

    printd(4,"    About to read pipe:\n",0);

    if ((request=processsim_pipe_read(fu,pipe_loop))==0)
    {
      printd(4,"    Pipe zero:\n",0);

      output_buffer+=oprintf(output_buffer,"Empty");
    }
    else
    {
      printd(4,"    Pipe not zero:\n",0);

      if ((*request).index==0)
      {
        printd(4,"    New request; allocated index %d:\n",(*state).index_alloc);

        (*request).index=(((*state).index_alloc)++);
      }

      output_buffer+=
      oprintf(output_buffer,
              "Index: &%X<BR>Address: &%X<BR>Type: %s %s<BR>Value: ",
              (*request).index,(*request).addr,
              (*request).word==true ? "Word" : "Byte",
              (*request).load==true ? "load" : "store");

      if ((*request).load==true)
      {
        if ((*request).index==-1)
        {
          output_buffer+=
          oprintf(output_buffer,"&%X (complete)",(*request).value);
        }
        else
        {
          output_buffer+=
          oprintf(output_buffer,"(pending)");
        }
      }
      else
      {
        if ((*request).index==-1)
        {
          output_buffer+=
          oprintf(output_buffer,"&%X (complete)",(*request).value);
        }
        else
        {
          output_buffer+=
          oprintf(output_buffer,"&%X (pending)",(*request).value);
        }
      }
    }

    output_buffer+=oprintf(output_buffer,"</TD>");

    printd(4,"  Exiting pipe %d:\n",pipe_loop);
  }


  output_buffer+=
  oprintf(output_buffer,
          "</TR></TABLE></TD></TR>"
          "<TR>"
          "<TD align=left valign=middle colspan=2><EM>Current Request</EM></TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle>Target Index:</TD>"
          "<TD align=left valign=middle>&%X</TD>"
          "</TR>",(*state).index_done);

  for (pipe_loop=0;pipe_loop<SYSTEM_LSU_count;pipe_loop++)
  {
    if ((request=processsim_pipe_read(fu,pipe_loop))!=0 &&
        (*request).index!=-1)
    {
      pipe_loop=SYSTEM_LSU_count+1;
    }
  }

  if (pipe_loop==SYSTEM_LSU_count)
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD colspan=2 valign=middle>No unserviced requests present</TD>"
            "</TR>");
  }
  else
  {
    serviced_count=0;

    printd(4,"Service requests:\n",0);

    for (pipe_loop=0;pipe_loop<SYSTEM_LSU_count;pipe_loop++)
    {
      printd(4,"  Displaying for pipe %d:\n",pipe_loop);

      if ((request=processsim_pipe_read(fu,pipe_loop))!=0 &&
          (*request).index==(*state).index_done)
      {
        (*request).value=system_lsu_access((*state).mem_array,
                                           (*request).addr,
                                           (*request).value,
                                           (*request).word,
                                           (*request).load);

        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle>Index:</TD>"
                "<TD align=left valign=middle>&%X</TD>"
                "</TR>"
                "<TR>"
                "<TD align=left valign=middle>Address:</TD>"
                "<TD align=left valign=middle>&%X</TD>"
                "</TR>"
                "<TR>"
                "<TD align=left valign=middle>Type:</TD>"
                "<TD align=left valign=middle>%s %s</TD>"
                "</TR>"
                "<TR>"
                "<TD align=left valign=middle>Value:</TD>"
                "<TD align=left valign=middle>%d (&%X)</TD>"
                "</TR>",
                (*request).index,(*request).addr,
                (*request).word==true ? "Word" : "Byte",
                (*request).load==true ? "load" : "store",
                (*request).value,(*request).value);

        ((*state).index_done)++;
        (*request).index=-1;

        if ((++serviced_count)>=ARM_LSU_BANDWIDTH)
        {
          pipe_loop=SYSTEM_LSU_count+1;
        }
      }

      printd(4,"  Exiting pipe %d:\n",pipe_loop);
    }

    if (serviced_count==0)
    {
      output_buffer+=
      oprintf(output_buffer,
              "<TR>"
              "<TD valign=middle colspan=2>Request skip</TD>"
              "</TR>");

      ((*state).index_done)++;

      pipe_loop=0;
    }
  }

/* again, spin around all pipes */
/*  for (pipe_loop=0;pipe_loop<SYSTEM_LSU_count;pipe_loop++)
  {
    if ((request=processsim_pipe_read(fu,pipe_loop))!=0 &&
        (*request).index<-1)
    {
      ((*request).index)++;
    }
  }*/


  oprintf(output_buffer,"</TABLE>");
}



/* -------------------------------------------------------------------
   i32f system_lsu_access

   Performs reads from and writes to the ARM's virtual memory
   space. On either a load or a store, the data in question is
   returned.
------------------------------------------------------------------- */
i32f system_lsu_access (ui8f **mem_array,i32f addr,i32f value,
                        ui8 word,ui8 load)
{
  ui8f **mem_array_internal,*mem_addr,*mem_addr_end;

  printd(4,"MEMACCESS: &%X\n",addr);

  printd(4,"  Block at &%X\n",mem_array+(((ui32f) addr)/SYSTEM_LSU_MEMBLOCK));

  if (*(mem_array_internal=mem_array+(((ui32f) addr)/
                                      SYSTEM_LSU_MEMBLOCK))==0)
  {
    printd(4,"    Needs allocating\n",0);

    *mem_array_internal=mem_addr=mem_alloc(SYSTEM_LSU_MEMBLOCK);
    mem_addr_end=mem_addr+SYSTEM_LSU_MEMBLOCK;

    do
    {
      *((i32f*) mem_addr)=0;
      *((i32f*) (mem_addr+4))=0;
      *((i32f*) (mem_addr+8))=0;
      *((i32f*) (mem_addr+12))=0;
      mem_addr+=16;
    }
    while (mem_addr<mem_addr_end);
  }

  printd(4,"  *mem_array_internal=&%X\n",*mem_array_internal);

  mem_addr=*mem_array_internal+(addr & (SYSTEM_LSU_MEMBLOCK-1));

  printd(4,"  Final addr is &%X\n",mem_addr);


/*{
ui8f **xmem_array,**xmem_array_end;

printd(4,"\n\nENUMERATING THE MEMORY ARRAY:\n",0);

xmem_array_end=(xmem_array=mem_array)+((0x10000000/SYSTEM_LSU_MEMBLOCK)*0x10);

do
{
  printd(4,"  addr &%X has value &",xmem_array);
  printdc(4,"%X\n",*xmem_array);
  xmem_array++;
}
while (xmem_array<xmem_array_end);

printd(4,"\n\n",0);

}*/






  if (load==false)
  {
    if (word==false)
    {
      *mem_addr=(ui8f) value;
    }
    else
    {
      *((i32f*) mem_addr)=value;
    }
  }
  else
  {
    if (word==false)
    {
      value=(i32f) *mem_addr;
    }
    else
    {
      value=ROR(*((i32f*) (((size_t) mem_addr) & ~0x3)),
                (((i32f) addr) & 0x3)<<3);
    }
  }

  return value;
}
