/* -------------------------------------------------------------------
   ARMSim V1.06                                        C COMPONENTS
   (c) Geoffrey Crossland 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "header.h"


extern void system_lsu_testbed (void *fu);
extern void system_execute_DataProc_testbed (void);

/* three rets */


/* -------------------------------------------------------------------
   void system_fetch

   The Functional Unit representing the ARM's Instruction Fetch
   Unit.
------------------------------------------------------------------- */
void system_fetch (void *fu)
{
  struct system_fetch_state *state;
  char *output_buffer;
  struct system_lsu_request *request;
  struct system_instr *instr;
  #ifdef ARM_SUPERSCALAR
  struct system_instr **fetches_rd_internal,**fetches_wr_internal,
                      **fetches_internal_end;
  i32 fetch_loop,unit_loop,flush_loop;
  ui8 ready_flag;
  #endif
  i32f fetch_word;

  state=processsim_workspace_read(fu);
  output_buffer=processsim_output_read(fu);

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=2>Instruction Fetch Unit</TH>"
          "</TR>");

  printd(4,"\nEntered system_fetch:\n",0);


  if (processsim_pipe_read(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH)!=0)
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=2>"
            "Flush: Control Unit Branch line raised"
            "</TD></TR>");

    #ifdef ARM_SUPERSCALAR
    for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
    {
      if ((request=
           processsim_pipe_read(fu,SYSTEM_FETCH_LSU(unit_loop)))!=0)
      {
        processsim_pipe_write(fu,SYSTEM_FETCH_LSU(unit_loop),0);
        mem_free(request);
      }
    }

    fetches_rd_internal=(*state).fetches;
    fetches_internal_end=(*state).fetches+ARM_SUPERSCALAR_FETCH*2;

    do
    {
      if ((instr=*fetches_rd_internal)!=0)
      {
        mem_free(instr);
        *fetches_rd_internal=0;
      }
    }
    while (++fetches_rd_internal<fetches_internal_end);
    #else
    if ((request=processsim_pipe_read(fu,SYSTEM_FETCH_LSU))!=0)
    {
      processsim_pipe_write(fu,SYSTEM_FETCH_LSU,0);
      mem_free(request);
    }
    #endif

    (*state).addr=arm.r[15];
  }


  #ifdef ARM_SUPERSCALAR
  printd(4,"Checking to see if all words requested are here and if there is space in the buffer for them:\n",0);

  ready_flag=false;

  for (fetch_loop=0;fetch_loop<ARM_SUPERSCALAR_FETCH*2;fetch_loop++)
  {
    if ((*state).fetches[fetch_loop]==0)
    {
      break;
    }
  }

  fetches_rd_internal=(*state).fetches+fetch_loop;
  fetches_internal_end=(*state).fetches+ARM_SUPERSCALAR_FETCH*2;

  if ((ARM_SUPERSCALAR_FETCH*2-fetch_loop)>=ARM_SUPERSCALAR_FETCH)
  {
    ready_flag=true;

    for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
    {
      printd(4,"  Unit %d:\n",unit_loop);

      printd(4,"    About to read pipe %d:\n",SYSTEM_FETCH_LSU(unit_loop));

      if ((request=
           processsim_pipe_read(fu,SYSTEM_FETCH_LSU(unit_loop)))==0 ||
          (*request).index!=-1)
      {
        printd(4,"      Not ready:\n",0);

        ready_flag=false;
        break;
      }
      else
      {
        printd(4,"      Ready:\n",0);
      }
    }
  }
  #endif

  #ifdef ARM_SUPERSCALAR
  if (ready_flag==true)
  #else
  if ((request=
       processsim_pipe_read(fu,SYSTEM_FETCH_LSU))!=0 &&
      (*request).index==-1 &&
      processsim_pipe_read(fu,SYSTEM_FETCH_DECODE)==0)
  #endif
  {
    printd(4,"Ready to cache in fetched words:\n",0);

    output_buffer+=
    oprintf(output_buffer,"<TR>"
                          "<TD align=left valign=middle colspan=2><EM>Instruction Fetch</EM></TD>"
                          "</TR>");

    #ifdef ARM_SUPERSCALAR
    for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
    #endif
    {
      #ifdef ARM_SUPERSCALAR
      printd(4,"\n  Unit %d:\n",unit_loop);

      printd(4,"About to read pipe %d:\n",SYSTEM_FETCH_LSU(unit_loop));
      #endif

      #ifdef ARM_SUPERSCALAR
      request=processsim_pipe_read(fu,SYSTEM_FETCH_LSU(unit_loop));
      #endif

      printd(4,"  Load request complete and decoder ready to accept\n",0);

      output_buffer+=
      oprintf(output_buffer,
              "<TR>"
              "<TD align=left valign=middle>Address:</TD>"
              "<TD align=left valign=middle>&amp;%X</TD>"
              "</TR>"
              "<TR>"
              "<TD align=left valign=middle>Data:</TD>"
              "<TD align=left valign=middle>&amp;%X</TD>"
              "</TR>",(*request).addr,(*request).value);

      instr=mem_alloc(sizeof(struct system_instr));
      printd(128,"instr at &%X\n",instr);
      fetch_word=(*instr).word=(*request).value;
      #ifdef ARM_SUPERSCALAR
      #ifdef ARM_ROB_SPECEXEC_R15READ
      (*instr).pc=(*request).addr;
      #endif
      *(fetches_rd_internal++)=instr;
      #endif

      #ifdef ARM_SUPERSCALAR
      processsim_pipe_write(fu,SYSTEM_FETCH_LSU(unit_loop),0);
      #else
      processsim_pipe_write(fu,SYSTEM_FETCH_LSU,0);
      #endif

      #ifdef ARM_BRANCHPRED
      if ((((fetch_word)>>25) & 0x7)==0x5)                /* Branch */
      {
        i32f target,loop,*branchpred_buffer_addrs_internal;
        ui8 branchpred_buffer_element,*branchpred_buffer_internal;

        printd(4,"    A branch instruction\n",0);

        target=fetch_word & 0xFFFFFF;
        target=target | ~((target & (1<<23))-1);
        target=target<<2;
        target+=8;

        if ((branchpred_buffer_element=
             system_branchpred_read((*request).addr))==0)
        {
          switch (((fetch_word)>>(28-8)) & (0xF<<8))
          {
          case ARM_OP_AL:
            (*instr).branchpred_taken=true;
            break;
          case ARM_OP_NV:
            (*instr).branchpred_taken=false;
            break;
          default:
            (*instr).branchpred_taken=(target>0 ? false : true);
          }
        }
        else
        {
          if (branchpred_buffer_element<
              ARM_BRANCHPRED_DYNAMIC_WEAKEST_TAKEN)
          {
            (*instr).branchpred_taken=false;
          }
          else
          {
            (*instr).branchpred_taken=true;
          }

          (performance.branchpred_dynamic)++;
        }

        if ((*instr).branchpred_taken==true)
        {
          printd(4,"      Predicted to be taken:\n",0);
          printd(4,"      About to flush all following pipes:\n",0);

          (*state).addr=(*request).addr+target;

          printd(4,"        Reset new PC to &X\n",(*state).addr);

          #ifdef ARM_SUPERSCALAR
          /* Free all the subsequent fetches and stall the unit. */

          output_buffer+=
          oprintf(output_buffer,
                  "<TR>"
                  "<TD align=left valign=middle colspan=2>");

          for (flush_loop=unit_loop+1;flush_loop<
                                      ARM_SUPERSCALAR_FETCH;
               flush_loop++)
          {
            printd(4,"        Pipe %d:\n",flush_loop);

            output_buffer+=
            oprintf(output_buffer,"(killing pipe %d) ",flush_loop);


            mem_free(processsim_pipe_read
                     (fu,SYSTEM_FETCH_LSU(flush_loop)));
            processsim_pipe_write(fu,SYSTEM_FETCH_LSU(flush_loop),0);
          }

          output_buffer+=
          oprintf(output_buffer,
                  "</TD>"
                  "</TR>");

          unit_loop=ARM_SUPERSCALAR_FETCH;
          #endif
        }
      }
      #endif

      #ifdef ARM_RETURNPRED
      if ((fetch_word & (0xF<<28))==0xE<<28 &&
          ((fetch_word & ((0x2<<26)+(0xF<<21)+(0xF<<12)))==
           (0x0<<26)+(0xD<<21)+(0xF<<12) ||
          (fetch_word & ((0x2<<26)+(0x1<<20)+(0xF<<12)))==
           (0x1<<26)+(0x1<<20)+(0xF<<12) ||
          (fetch_word & ((0x7<<25)+(0x1<<20)+(0x1<<15)))==
           (0x4<<25)+(0x1<<20)+(0x1<<15)))                /* Return */
      {
        (*instr).returnpred_addr=(*state).addr=
        system_returnpred_pull();

/*
{
char b[256];
sprintf(b,"Cycle is &%X",processsim_main_clocks());
        error_fatal(b,-1);
}*/

        #ifdef ARM_SUPERSCALAR
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle colspan=2>");

        for (flush_loop=unit_loop+1;flush_loop<ARM_SUPERSCALAR_FETCH;
             flush_loop++)
        {
          output_buffer+=
          oprintf(output_buffer,"(killing pipe %d) ",flush_loop);

          mem_free(processsim_pipe_read
                   (fu,SYSTEM_FETCH_LSU(flush_loop)));
          processsim_pipe_write(fu,SYSTEM_FETCH_LSU(flush_loop),0);
        }

        output_buffer+=
        oprintf(output_buffer,
                "</TD>"
                "</TR>");

        unit_loop=ARM_SUPERSCALAR_FETCH;
        #endif
      }
      else
      {
        (*instr).returnpred_addr=-1;
      }
      #endif

      printd(4,"  Completed search for target predicable instrs\n",0);

      mem_free(request);


      #ifndef ARM_SUPERSCALAR
      printd(4,"Ready to pass out:\n",0);

      output_buffer+=
      oprintf(output_buffer,"<TR>"
                            "<TD align=left valign=middle colspan=2><EM>Issue</EM></TD>"
                            "</TR>");

      processsim_pipe_write(fu,SYSTEM_FETCH_DECODE,instr);

      output_buffer+=
      oprintf(output_buffer,
              "<TR>"
              "<TD align=left valign=middle>Data:</TD>"
              "<TD align=left valign=middle>&amp;%X</TD>"
              "</TR>",(*instr).word);
      #endif
    }
  }
  else
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=2>Waiting for instructions to be fetched</TD>"
            "</TR>");
  }


  #ifdef ARM_SUPERSCALAR
  printd(4,"Ready to pass out:\n",0);

  output_buffer+=
  oprintf(output_buffer,"<TR>"
                        "<TD align=left valign=middle colspan=2><EM>Issue</EM></TD>"
                        "</TR>");

  ready_flag=true;

  fetches_rd_internal=(*state).fetches;

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
  {
    if (*(fetches_rd_internal++)==0 ||
        processsim_pipe_read(fu,SYSTEM_FETCH_DECODE(unit_loop))!=0)
    {
      ready_flag=false;
      break;
    }
  }

  if (ready_flag==true)
  {
    fetches_wr_internal=fetches_rd_internal=(*state).fetches;

    for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
    {
      processsim_pipe_write(fu,SYSTEM_FETCH_DECODE(unit_loop),
                            instr=*(fetches_rd_internal++));

      output_buffer+=
      oprintf(output_buffer,
              "<TR>"
              "<TD align=left valign=middle>Data:</TD>"
              "<TD align=left valign=middle>&amp;%X</TD>"
              "</TR>",(*instr).word);
    }

    while (fetches_rd_internal<fetches_internal_end)
    {
      *(fetches_wr_internal++)=*(fetches_rd_internal++);
    }

    while (fetches_wr_internal<fetches_internal_end)
    {
      *(fetches_wr_internal++)=0;
    }
  }
  #endif


  printd(4,"Ready to request next fetches:\n",0);

  #ifdef ARM_SUPERSCALAR
  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
  #endif
  {
    #ifdef ARM_SUPERSCALAR
    if (processsim_pipe_read(fu,SYSTEM_FETCH_LSU(unit_loop))==0)
    #else
    if (processsim_pipe_read(fu,SYSTEM_FETCH_LSU)==0)
    #endif
    {
      printd(4,"  When checking for available fetching slots, no load in progress\n",0);

      request=mem_alloc(sizeof(struct system_lsu_request));
      printd(128,"fetch request at &%X\n",request);

      (*request).addr=(*state).addr;
      (*state).addr+=4;
      (*request).word=true;
      (*request).load=true;
      (*request).index=0;

      printd(4,"  About to request &%X with load request at &",(*request).addr);
      printdc(4,"%X\n",request);

      #ifdef ARM_SUPERSCALAR
      processsim_pipe_write(fu,SYSTEM_FETCH_LSU(unit_loop),request);
      #else
      processsim_pipe_write(fu,SYSTEM_FETCH_LSU,request);
      #endif
    }
  }

  oprintf(output_buffer,"</TABLE>");
}



#ifndef ARM_SUPERSCALAR_SHORT
/* -------------------------------------------------------------------
   void system_decode

   The Functional Unit representing the ARM's Instruction Decode
   Unit.
------------------------------------------------------------------- */
void system_decode (void *fu)
{
  char *output_buffer;
  struct system_instr *instr;
  #ifdef ARM_SUPERSCALAR
  i32 unit_loop;
  #endif

  output_buffer=processsim_output_read(fu);

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle>Decoding Unit</TH>"
          "</TR>");

  printd(4,"\nEntered system_decode:\n",0);


  #ifdef ARM_SUPERSCALAR
  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
  #endif
  {
    #ifdef ARM_SUPERSCALAR
    if ((instr=
         processsim_pipe_read(fu,
                              SYSTEM_DECODE_FETCH(unit_loop)))!=0 &&
        processsim_pipe_read(fu,SYSTEM_DECODE_DECODE2(unit_loop))==0)
    #else
    if ((instr=processsim_pipe_read(fu,SYSTEM_DECODE_FETCH))!=0 &&
        processsim_pipe_read(fu,SYSTEM_DECODE_EXECUTE)==0)
    #endif
    {
      if (processsim_pipe_read(PROCESSSIM_FU_NULL,
                               SYSTEM_CU_BRANCH)!=0)
      {
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle>"
                "Flush: Control Unit Branch line raised"
                "</TD></TR>");

        mem_free(instr);
      }
      else
      {
        printd(4,"  Request in for decoding &%X can be sent\n",instr);

        #ifdef ARM_SUPERSCALAR
        processsim_pipe_write(fu,SYSTEM_DECODE_DECODE2(unit_loop),
                              instr=
                              system_decode_disassemble(instr));
        #else
        processsim_pipe_write(fu,SYSTEM_DECODE_EXECUTE,
                              instr=
                              system_decode_disassemble(instr));
        #endif

        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle>");

        output_buffer+=system_decode_print(output_buffer,instr);

        output_buffer+=
        oprintf(output_buffer,
                "</TD>"
                "</TR>");
      }

      #ifdef ARM_SUPERSCALAR
      processsim_pipe_write(fu,SYSTEM_DECODE_FETCH(unit_loop),0);
      #else
      processsim_pipe_write(fu,SYSTEM_DECODE_FETCH,0);
      #endif
    }
    else
    {
      #ifdef ARM_SUPERSCALAR
      if (processsim_pipe_read(fu,
                               SYSTEM_DECODE_DECODE2(unit_loop))==0)
      #else
      if (processsim_pipe_read(fu,SYSTEM_DECODE_EXECUTE)==0)
      #endif
      {
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle>No decoding in progress</TD>"
                "</TR>");
      }
      else
      {
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle>");

        #ifdef ARM_SUPERSCALAR
        output_buffer+=
        system_decode_print
        (output_buffer,
         processsim_pipe_read(fu,SYSTEM_DECODE_DECODE2(unit_loop)));

        output_buffer+=
        oprintf(output_buffer,
                " (waiting for further deocding) </TD>"
                "</TR>");
        #else
        output_buffer+=
        system_decode_print
        (output_buffer,processsim_pipe_read(fu,
                                            SYSTEM_DECODE_EXECUTE));

        output_buffer+=
        oprintf(output_buffer,
                " (decoded and waiting for execution) </TD>"
                "</TR>");
        #endif
      }
    }
  }

  oprintf(output_buffer,"</TABLE>");
}
#endif



#ifndef ARM_SUPERSCALAR
/* -------------------------------------------------------------------
   void system_execute

   The Functional Unit representing the ARM's Instruction Execution
   Unit.
------------------------------------------------------------------- */
void system_execute (void *fu)
{
  struct system_execute_state *state;
  struct system_instr *instr;
  char *output_buffer;
  i32f op,ops,temp;
  ui8 done;

  state=processsim_workspace_read(fu);
  output_buffer=processsim_output_read(fu);

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=2>Execution Unit</TH>"
          "</TR>");

  printd(4,"\nEntered system_execute:\n",0);

  if ((instr=processsim_pipe_read(fu,SYSTEM_EXECUTE_DECODE))!=0)
  {
    printd(4,"  Instruction ready for execution:\n",0);

    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=2>");

    output_buffer+=system_decode_print(output_buffer,instr);

    op=(*instr).op;
    ops=(*instr).ops;

    ((*state).cycles_loaded)++;

    temp=(op & 0xF<<8) | (arm.cpsr & (ARM_CPSR_N | ARM_CPSR_Z |
                                      ARM_CPSR_C | ARM_CPSR_V));

    if (

(temp & (0xF<<8))==ARM_OP_AL ||

(temp & ((0xF<<8) | ARM_CPSR_Z))==(ARM_OP_EQ | ARM_CPSR_Z) ||
(temp & ((0xF<<8) | ARM_CPSR_Z))==ARM_OP_NE ||
(temp & ((0xF<<8) | ARM_CPSR_C))==(ARM_OP_CS | ARM_CPSR_C) ||
(temp & ((0xF<<8) | ARM_CPSR_C))==ARM_OP_CC ||
(temp & ((0xF<<8) | ARM_CPSR_N))==(ARM_OP_MI | ARM_CPSR_N) ||
(temp & ((0xF<<8) | ARM_CPSR_N))==ARM_OP_PL ||
(temp & ((0xF<<8) | ARM_CPSR_V))==(ARM_OP_VS | ARM_CPSR_V) ||
(temp & ((0xF<<8) | ARM_CPSR_V))==ARM_OP_VC ||

(temp & ((0xF<<8) | ARM_CPSR_C | ARM_CPSR_Z))==(ARM_OP_HI | ARM_CPSR_C) ||

(temp & ((0xF<<8) | ARM_CPSR_C))==ARM_OP_LS ||
(temp & ((0xF<<8) | ARM_CPSR_Z))==(ARM_OP_LS | ARM_CPSR_Z) ||

(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_GE | ARM_CPSR_N | ARM_CPSR_V) ||
(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==ARM_OP_GE ||

(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_LT | ARM_CPSR_N) ||
(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_LT | ARM_CPSR_V) ||

(temp & ((0xF<<8) | ARM_CPSR_Z | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_GT | ARM_CPSR_N | ARM_CPSR_V) ||
(temp & ((0xF<<8) | ARM_CPSR_Z | ARM_CPSR_N | ARM_CPSR_V))==ARM_OP_GT ||

(temp & ((0xF<<8) | ARM_CPSR_Z))==(ARM_OP_LE | ARM_CPSR_Z) ||
(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_LE | ARM_CPSR_N) ||
(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_LE | ARM_CPSR_V)

)

    {
      ((*state).cycles_processing)++;

      if ((op & 0xFF)<0x10)
      {
        done=system_execute_DataProc(instr,fu,&output_buffer);
      }
      else
      {
        switch (op & 0xFF)
        {
        case ARM_OP_MUL:
        case ARM_OP_MLA:
          done=system_execute_Mul(instr,fu,&output_buffer);
          break;
        case ARM_OP_B:
        case ARM_OP_BL:
          done=system_execute_Branch(instr,fu,&output_buffer);
          break;
        case ARM_OP_STR:
        case ARM_OP_LDR:
          done=system_execute_SingleMem(instr,fu,&output_buffer);
          break;
        case ARM_OP_STM:
        case ARM_OP_LDM:
          done=system_execute_MultiMem(instr,fu,&output_buffer);
          break;

        case ARM_OP_MRS:
        case ARM_OP_MSR:
/*          done=system_execute_PSRTrans(instr,fu,&output_buffer);*/
          error_fatal("Need to do MRS/MSR!!",666);
          break;

        default:
          done=true;

        }
      }
    }
    else
    {
      done=true;

      #ifdef ARM_BRANCHPRED
      if ((op & 0xFF)==ARM_OP_B || (op & 0xFF)==ARM_OP_BL)
      {
        ui8 *branchpred_buffer_internal;

        if (ARM_BRANCHPRED!=-1)
        {
          branchpred_buffer_internal=
          system_branchpred_write(arm.r[15]);

          (*branchpred_buffer_internal)--;

          if (*branchpred_buffer_internal<
              ARM_BRANCHPRED_DYNAMIC_STRONGEST_NOTTAKEN)
          {
            *branchpred_buffer_internal=
            ARM_BRANCHPRED_DYNAMIC_STRONGEST_NOTTAKEN;
          }
        }

        (performance.branchpred_count)++;

        if ((*instr).branchpred_taken==true)
        {
          processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,
                                (void*) 1);

          output_buffer+=oprintf(output_buffer," (branch mispredicted)");

          (performance.branchpred_mispredict)++;
        }
        else
        {
          output_buffer+=oprintf(output_buffer," (branch predicted)");
        }
      }
      #endif

      output_buffer+=oprintf(output_buffer," (not executed)");
    }

    if (done==true)
    {
      mem_free(instr);
      arm.r[15]+=4;
      processsim_pipe_write(fu,SYSTEM_EXECUTE_DECODE,0);

      output_buffer+=oprintf(output_buffer," (completed)");

      arm_instrexecuted++;
      (performance.instr_executed)++;
    }
    else
    {
      output_buffer+=oprintf(output_buffer," (not completed)");
    }

    output_buffer+=
    oprintf(output_buffer,
            "</TD>"
            "</TR>");
  }
  else
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=2>No execution in progress</TD>"
            "</TR>");
  }

/*  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle>Time Loaded:</TD>"
          "<TD align=left valign=middle>%d cycles (%.3f%%)</TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle>Time Processing:</TD>"
          "<TD align=left valign=middle>%d cycles (%.3f%%)</TD>"
          "</TR>"
          "</TABLE>",(*state).cycles_loaded,
                     (((double) ((*state).cycles_loaded))*100)/
                     (((double) processsim_main_clocks())+0.0001),
                     (*state).cycles_processing,
                     (((double) ((*state).cycles_processing))*100)/
                     (((double) processsim_main_clocks())+0.0001));*/

  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle>Loaded:</TD>"
          "<TD align=left valign=middle>%.3f%%</TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle>Processing:</TD>"
          "<TD align=left valign=middle>%.3f%%</TD>"
          "</TR>"
          "</TABLE>",(((double) ((*state).cycles_loaded))*100)/
                     (((double) processsim_main_clocks())+0.0001),
                     (((double) ((*state).cycles_processing))*100)/
                     (((double) processsim_main_clocks())+0.0001));
}






void system_execute_DataProc_testbed (void)
{
  char buffx[1024],*buff=buffx;
  struct system_instr i;

/*
i32f t32;
i64 t64;

t32=0x80000000;
t64=(i64) t32;

printd(4,"t32=&%08X\n",t32);
printd(4,"t64=&%08X",(t64>>32) & 0xFFFFFFFF);
printdc(4,"%08X\n",t64 & 0xFFFFFFFF);

t64=t64 & 0xFFFFFFFF;

printd(4,"t64=&%08X",(t64>>32) & 0xFFFFFFFF);
printdc(4,"%08X\n",t64 & 0xFFFFFFFF);

t32=(i32f) t64;

printd(4,"t32=&%08X\n",t32);

return;
*/

  printd(4,"Basics + RRX:\n\n",0);

  arm.r[3]=5;
  i.op=ARM_OP2_REG;
  i.ops=1+(2<<4)+(3<<8);
  system_execute_DataProc(&i,0,&buff);

  i.op=ARM_OP2_CONST;
  i.ops=1+(2<<4)+(0x3F<<8)+(1<<16);
  system_execute_DataProc(&i,0,&buff);

  i.op=ARM_OP2_CONST;
  i.ops=1+(2<<4)+(0x3F<<8)+(3<<16);
  system_execute_DataProc(&i,0,&buff);

  arm.r[10]=0x2;
  i.op=ARM_OP2_REG__RRX;
  i.ops=1+(2<<4)+(10<<8);
  system_execute_DataProc(&i,0,&buff);

  arm.r[8]=0x5;
  i.op=ARM_OP2_REG__RRX;
  i.ops=1+(2<<4)+(8<<8);
  system_execute_DataProc(&i,0,&buff);

  arm.r[10]=0x2;
  i.op=ARM_OP2_REG__RRX;
  i.ops=1+(2<<4)+(10<<8);
  system_execute_DataProc(&i,0,&buff);

  printd(4,"Shift by consts:\n\n",0);

  arm.r[4]=0x5;
  i.op=ARM_OP2_REG__LSL_CONST;
  i.ops=1+(2<<4)+(4<<8)+(1<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[4]=0xFF;
  i.op=ARM_OP2_REG__LSL_CONST;
  i.ops=1+(2<<4)+(4<<8)+(24<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[4]=0xFF;
  i.op=ARM_OP2_REG__LSL_CONST;
  i.ops=1+(2<<4)+(4<<8)+(25<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[4]=0xFF;
  i.op=ARM_OP2_REG__LSL_CONST;
  i.ops=1+(2<<4)+(4<<8)+(26<<12);
  system_execute_DataProc(&i,0,&buff);

  printd(4,"\n",0);

  arm.r[4]=0x5;
  i.op=ARM_OP2_REG__LSR_CONST;
  i.ops=1+(2<<4)+(4<<8)+(2<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[4]=0x5;
  i.op=ARM_OP2_REG__LSR_CONST;
  i.ops=1+(2<<4)+(4<<8)+(2<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[5]=0xFF100000;
  i.op=ARM_OP2_REG__LSR_CONST;
  i.ops=1+(2<<4)+(5<<8)+(0<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[5]=0xFF1;
  i.op=ARM_OP2_REG__LSR_CONST;
  i.ops=1+(2<<4)+(5<<8)+(0<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[4]=0xB;
  i.op=ARM_OP2_REG__LSR_CONST;
  i.ops=1+(2<<4)+(4<<8)+(2<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[4]=0xF000000B;
  i.op=ARM_OP2_REG__LSR_CONST;
  i.ops=1+(2<<4)+(4<<8)+(2<<12);
  system_execute_DataProc(&i,0,&buff);

  printd(4,"\n",0);

  arm.r[4]=0xF000000B;
  i.op=ARM_OP2_REG__ASR_CONST;
  i.ops=1+(2<<4)+(4<<8)+(2<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[4]=0xF000000B;
  i.op=ARM_OP2_REG__ASR_CONST;
  i.ops=1+(2<<4)+(4<<8)+(8<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[4]=0xF000000B;
  i.op=ARM_OP2_REG__ASR_CONST;
  i.ops=1+(2<<4)+(4<<8)+(2<<12);
  system_execute_DataProc(&i,0,&buff);

  printd(4,"\n",0);

  arm.r[7]=0xC0000003;
  i.op=ARM_OP2_REG__ROR_CONST;
  i.ops=1+(2<<4)+(7<<8)+(1<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[7]=0xC0000004;
  i.op=ARM_OP2_REG__ROR_CONST;
  i.ops=1+(2<<4)+(7<<8)+(2<<12);
  system_execute_DataProc(&i,0,&buff);

  printd(4,"Shift by regs:\n\n",0);

  arm.r[0]=0x5;
  arm.r[1]=0x1;
  i.op=ARM_OP2_REG__LSL_REG;
  i.ops=1+(2<<4)+(0<<8)+(1<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[0]=0xC0000000;
  arm.r[1]=31;
  i.op=ARM_OP2_REG__LSL_REG;
  i.ops=1+(2<<4)+(0<<8)+(1<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[0]=0x00000003;
  arm.r[1]=32;
  i.op=ARM_OP2_REG__LSL_REG;
  i.ops=1+(2<<4)+(0<<8)+(1<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[0]=0x0F000000;
  arm.r[1]=33;
  i.op=ARM_OP2_REG__LSL_REG;
  i.ops=1+(2<<4)+(0<<8)+(1<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[0]=0x0F000001;
  arm.r[1]=33;
  i.op=ARM_OP2_REG__LSL_REG;
  i.ops=1+(2<<4)+(0<<8)+(1<<12);
  system_execute_DataProc(&i,0,&buff);

  printd(4,"\n",0);

  arm.r[3]=0x80000000;
  arm.r[6]=32;
  i.op=ARM_OP2_REG__LSR_REG;
  i.ops=1+(2<<4)+(3<<8)+(6<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[3]=0x40000000;
  arm.r[6]=32;
  i.op=ARM_OP2_REG__LSR_REG;
  i.ops=1+(2<<4)+(3<<8)+(6<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[3]=0xF000000B;
  arm.r[6]=3;
  i.op=ARM_OP2_REG__LSR_REG;
  i.ops=1+(2<<4)+(3<<8)+(6<<12);
  system_execute_DataProc(&i,0,&buff);

  printd(4,"\n",0);

  arm.r[3]=0x80000000;
  arm.r[6]=32;
  i.op=ARM_OP2_REG__ASR_REG;
  i.ops=1+(2<<4)+(3<<8)+(6<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[3]=0x70000000;
  arm.r[6]=32;
  i.op=ARM_OP2_REG__ASR_REG;
  i.ops=1+(2<<4)+(3<<8)+(6<<12);
  system_execute_DataProc(&i,0,&buff);

  printd(4,"\n",0);

  arm.r[7]=0xC0000003;
  arm.r[8]=1;
  i.op=ARM_OP2_REG__ROR_REG;
  i.ops=1+(2<<4)+(7<<8)+(8<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[7]=0xC0000004;
  arm.r[8]=2;
  i.op=ARM_OP2_REG__ROR_REG;
  i.ops=1+(2<<4)+(7<<8)+(8<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[7]=0xC0000004;
  arm.r[8]=32;
  i.op=ARM_OP2_REG__ROR_REG;
  i.ops=1+(2<<4)+(7<<8)+(8<<12);
  system_execute_DataProc(&i,0,&buff);

  arm.r[7]=0xC0000004;
  arm.r[8]=32;
  arm.cpsr=arm.cpsr | ARM_CPSR_C;
  i.op=ARM_OP2_REG__ROR_REG;
  i.ops=1+(2<<4)+(7<<8)+(8<<12);
  system_execute_DataProc(&i,0,&buff);
}



ui8 system_execute_DataProc (struct system_instr *instr,void *fu,
                             char **output_buffer)
{
  i64 dest64,dest32;
  i32f op2shift,op2,op1,op,ops,cpsr_in,cpsr_out  /*,temp*/ ;

  op=(*instr).op;
  ops=(*instr).ops;


  /* Prepare for using the flags. */
  cpsr_in=cpsr_out=arm.cpsr;

  printd(4,"The initial state of the carry flag is %d\n",(arm.cpsr & ARM_CPSR_C)>>ARM_CPSR_Cs);


  /* Operand 2: */
  if ((*instr).status==1)
  {
    printd(4,"This is a reg shift's second cycle\n",0);

    op2=(*instr).temp[0];
    cpsr_out=(*instr).temp[1];
  }
  else
  {
    switch (op & (0xF<<16))
    {
    case ARM_OP2_CONST:
      op2=ROR((ops>>8) & 0xFF,(ops>>15) & 0x1E);
      break;
    case ARM_OP2_REG:
      op2=arm.r[(ops>>8) & 0xF];

      printd(4,"The intial op2 is %d (&",op2);
      printdc(4,"%X)\n",op2);

      if (((ops>>8) & 0xF)==15)
      {
        op2+=8;
      }
      break;
    case ARM_OP2_REG__RRX:
      op2=arm.r[(ops>>8) & 0xF];

      printd(4,"The initial op2 is %d (&",op2);
      printdc(4,"%X)\n",op2);

      if (((ops>>8) & 0xF)==15)
      {
        op2+=8;
      }
      cpsr_out=(cpsr_out & ~ARM_CPSR_C) | ((op2 & 0x1)<<ARM_CPSR_Cs);
      op2=(op2>>1) | (cpsr_in & ARM_CPSR_C)<<(31-ARM_CPSR_Cs);
      break;
    default:
      op2=arm.r[(ops>>8) & 0xF];

      printd(4,"The initial op2 is %d (&",op2);
      printdc(4,"%X)\n",op2);

      if ((op & (0xF<<16))<(6<<16))                 /* Shift by reg */
      {
        op2shift=arm.r[(ops>>12) & 0xF] & 0xFF;

        printd(4,"The initial regwise op2shift is %d (&",op2shift);
        printdc(4,"%X)\n",op2shift);

        if (op2shift>31 && (op & ARM_OP2_REG__ROR_REG)!=0)
        {
          op2shift=op2shift & 0x1F;
        }

        if (op2shift>32)
        {
          op2shift=33;
        }

        printd(4,"The final op2shift is %d (&",op2shift);
        printdc(4,"%X)\n",op2shift);

        if (((ops>>8) & 0xF)==15)
        {
          op2+=12;
        }
      }
      else                                        /* Shift by const */
      {
        op2shift=(ops>>12) & 0x1F;

        printd(4,"The initial const op2shift is quoted as %d (&",op2shift);
        printdc(4,"%X)\n",op2shift);

        if (op2shift==0)
        {
          op2shift=32;
        }

        printd(4,"The final op2shift is %d (&",op2shift);
        printdc(4,"%X)\n",op2shift);

        if (((ops>>8) & 0xF)==15)
        {
          op2+=8;
        }
      }

      if (op2shift!=0)
      {
        switch (op & (0xF<<16))
        {
        case ARM_OP2_REG__LSL_CONST:
        case ARM_OP2_REG__LSL_REG:
          if (op2shift==33)
          {
            cpsr_out=cpsr_out & ~ARM_CPSR_C;
            op2=0;
          }
          else
          {
            cpsr_out=(cpsr_out & ~ARM_CPSR_C) |
                     (((op2>>(32-op2shift)) & 0x1)<<ARM_CPSR_Cs);
            op2=op2<<op2shift;
          }
          break;
        case ARM_OP2_REG__LSR_CONST:
        case ARM_OP2_REG__LSR_REG:
          if (op2shift==33)
          {
            cpsr_out=cpsr_out & ~ARM_CPSR_C;
            op2=0;
          }
          else
          {
            cpsr_out=(cpsr_out & ~ARM_CPSR_C) |
                     (((op2>>(op2shift-1)) & 0x1)<<ARM_CPSR_Cs);
            op2=(op2>>op2shift);
            op2=op2 & ((1<<(32-op2shift))-1);
          }
          break;
        case ARM_OP2_REG__ASR_CONST:
        case ARM_OP2_REG__ASR_REG:
          if (op2shift>31)
          {
            cpsr_out=(cpsr_out & ~ARM_CPSR_C) |
                     (((op2>>31) & 0x1)<<ARM_CPSR_Cs);
            op2=~((op2>>31 & 0x1)-1);
          }
          else
          {
            cpsr_out=(cpsr_out & ~ARM_CPSR_C) |
                     (((op2>>(op2shift-1)) & 0x1)<<ARM_CPSR_Cs);

/*            temp=op2>>31 & 0x1;
            op2=op2>>op2shift;
            op2=op2 & ((1<<(32-op2shift))-1);
            op2=op2 | ~((temp<<(32-op2shift))-1);*/

            op2=(op2>>op2shift) |
                ~(((op2>>31 & 0x1) << (32-op2shift))-1);
          }
          break;
        case ARM_OP2_REG__ROR_CONST:
        case ARM_OP2_REG__ROR_REG:
          cpsr_out=(cpsr_out & ~ARM_CPSR_C) |
                   (((op2>>(op2shift-1)) & 0x1)<<ARM_CPSR_Cs);
          op2=((op2>>op2shift) & ((1<<(32-op2shift))-1)) |
              (op2<<(32-op2shift));
        }
      }
    }

    if ((op & (0xF<<16))>(0x1<<16) && (op & (0xF<<16))<(0x6<<16))
    {
      (*instr).status=1;
      (*instr).temp[0]=op2;
      (*instr).temp[1]=cpsr_out;

      *output_buffer+=oprintf(*output_buffer," (regsiter shift)");

      return false;
    }
  }

  printd(4,"The resulting op2 is %d (&",op2);
  printdc(4,"%X)\n",op2);
  printd(4,"The final state of the carry flag is %d\n\n",(cpsr_out & ARM_CPSR_C)>>ARM_CPSR_Cs);


  /* Operand 1: */
  op1=arm.r[(ops>>4) & 0xF];

  printd(4,"The intial op1 is %d (&",op1);
  printdc(4,"%X)\n",op1);

  if (((ops>>4) & 0xF)==15)
  {
    op1+=8+(((*instr).status)<<2);
  }

  printd(4,"The resulting op1 is %d (&",op1);
  printdc(4,"%X)\n",op1);


  /* Operation: */
  switch (op & 0xF)
  {
  case ARM_OP_TST:
  case ARM_OP_AND:
    dest64=((i64) op1) & ((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF) & (((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_TEQ:
  case ARM_OP_EOR:
    dest64=((i64) op1) ^ ((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF) ^ (((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_CMP:
  case ARM_OP_SUB:
/*    dest64=((i64) op1)-((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF)-(((i64) op2) & 0xFFFFFFFF);*/

    dest64=((i64) op1)+(~((i64) op2)+1);
    dest32=(((i64) op1) & 0xFFFFFFFF)+((~((i64) op2)+1) & 0xFFFFFFFF);

    break;
  case ARM_OP_RSB:
/*    dest64=((i64) op2)-((i64) op1);
    dest32=(((i64) op2) & 0xFFFFFFFF)-(((i64) op1) & 0xFFFFFFFF);*/

    dest64=((i64) op2)+(~((i64) op1)+1);
    dest32=(((i64) op2) & 0xFFFFFFFF)+((~((i64) op1)+1) & 0xFFFFFFFF);

    break;
  case ARM_OP_CMN:
  case ARM_OP_ADD:
    dest64=((i64) op1)+((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF)+(((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_ADC:
    dest64=((i64) op1)+((i64) op2)+
           ((i64) ((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs));
    dest32=(((i64) op1) & 0xFFFFFFFF)+(((i64) op2) & 0xFFFFFFFF)+
           (((i64) ((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs)) &
            0xFFFFFFFF);
    break;
  case ARM_OP_SBC:
/*    dest64=((i64) op1)-((i64) op2)+
           ((i64) ((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs))-1;
    dest32=(((i64) op1) & 0xFFFFFFFF)-(((i64) op2) & 0xFFFFFFFF)+
           (((i64) ((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs)) &
            0xFFFFFFFF)-1;*/

    dest64=((i64) op1)+(~((i64) op2)+1)+
           ((i64) (((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs)+(~1+1)));
    dest32=(((i64) op1) & 0xFFFFFFFF)+((~((i64) op2)+1) & 0xFFFFFFFF)+
           (((i64) (((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs)+(~1+1))) &
            0xFFFFFFFF);

    break;
  case ARM_OP_RSC:
/*    dest64=((i64) op2)-((i64) op1)+
         ((i64) ((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs))-1;
    dest32=(((i64) op2) & 0xFFFFFFFF)-(((i64) op1) & 0xFFFFFFFF)+
           (((i64) ((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs)) &
            0xFFFFFFFF)-1;*/

    dest64=((i64) op2)+(~((i64) op1)+1)+
           ((i64) (((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs)+(~1+1)));
    dest32=(((i64) op2) & 0xFFFFFFFF)+((~((i64) op1)+1) & 0xFFFFFFFF)+
           (((i64) (((cpsr_in & ARM_CPSR_C)>>ARM_CPSR_Cs)+(~1+1))) &
            0xFFFFFFFF);


    break;
  case ARM_OP_ORR:
    dest64=((i64) op1) | ((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF) | (((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_MOV:
    dest64=((i64) op2);
    dest32=(((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_BIC:
    dest64=((i64) op1) & ~((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF) & (~((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_MVN:
    dest64=~((i64) op2);
    dest32=(~((i64) op2) & 0xFFFFFFFF);
  }


  /* Writeback of the result and flag handling: */
  if ((op & 0xC)!=0x8)
  {
    arm.r[ops & 0xF]=(i32f) dest64;

    if ((ops & 0xF)==15)
    {
      #ifdef ARM_RETURNPRED
      if ((op & 0xFF)==ARM_OP_MOV)
      {
        system_returnpred_r15dest(instr);
      }
      else
      {
        processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,
                              (void*) 1);
        arm.r[15]-=4;
      }
      #else
      system_returnpred_r15dest(instr);
      #endif
    }
  }

  if ((op & ARM_OP_S)!=0)
  {
    switch (op & 0xFF)
    {
    case ARM_OP_SUB:
    case ARM_OP_RSB:
    case ARM_OP_ADD:
    case ARM_OP_ADC:
    case ARM_OP_SBC:
    case ARM_OP_RSC:
    case ARM_OP_CMP:
    case ARM_OP_CMN:

/*      cpsr_out=(cpsr_out & ~(ARM_CPSR_C | ARM_CPSR_V)) |
               ((op1>((i64) 2147483647) || op1<((i64) -2147483647)
                                               -1) ? ARM_CPSR_V : 0) |
               ((op1>>(32-ARM_CPSR_Cs)) & ARM_CPSR_C);*/


      cpsr_out=(cpsr_out & ~(ARM_CPSR_C | ARM_CPSR_V)) |
               ((dest32>>(32-ARM_CPSR_Cs)) & ARM_CPSR_C);

      switch (op & 0xFF)
      {
      case ARM_OP_ADD:
      case ARM_OP_ADC:
      case ARM_OP_CMN:
        if ((op1>>31 & 0x1)==(op2>>31 & 0x1) &&
            (op1>>31 & 0x1)!=(dest64>>31 & 0x1))
        {
          cpsr_out=cpsr_out | ARM_CPSR_V;
        }
        break;
      case ARM_OP_SUB:
      case ARM_OP_SBC:
      case ARM_OP_CMP:
        if ((op1>>31 & 0x1)!=(op2>>31 & 0x1) &&
            (op1>>31 & 0x1)!=(dest64>>31 & 0x1))
        {
          cpsr_out=cpsr_out | ARM_CPSR_V;
        }
        break;
      case ARM_OP_RSB:
      case ARM_OP_RSC:
        if ((op1>>31 & 0x1)!=(op2>>31 & 0x1) &&
            (op2>>31 & 0x1)!=(dest64>>31 & 0x1))
        {
          cpsr_out=cpsr_out | ARM_CPSR_V;
        }
      }
    case ARM_OP_AND:
    case ARM_OP_EOR:
    case ARM_OP_TST:
    case ARM_OP_TEQ:
    case ARM_OP_ORR:
    case ARM_OP_MOV:
    case ARM_OP_BIC:
    case ARM_OP_MVN:
      cpsr_out=(cpsr_out & ~(ARM_CPSR_N | ARM_CPSR_Z)) |
               ((dest64 & 0xFFFFFFFF)==0 ? ARM_CPSR_Z : 0) |
               ((dest64>>(63-ARM_CPSR_Ns)) & ARM_CPSR_N);
    }

    arm.cpsr=cpsr_out;
  }

  return true;
}



ui8 system_execute_Mul (struct system_instr *instr,void *fu,
                        char **output_buffer)
{
  i64 acc,op2,op1;
  i32f op,ops;

  op=(*instr).op;
  ops=(*instr).ops;


  if ((*instr).status==0)
  {
    /* MULs and MLAs take 1S + nI cycles:
       (unsigned) op2 <= &000000FF   n=1
       (unsigned) op2 <= &0000FFFF   n=2
       else                          n=3
    */

    op2=arm.r[(ops>>8) & 0xF] & 0xFFFFFFFF;

    if (op2<0x100)
    {
      (*instr).status=-3;
    }
    else if (op2<0x10000)
    {
      (*instr).status=-4;
    }
    else
    {
      (*instr).status=-5;
    }
  }


  if ((++((*instr).status))==-1)
  {
    op2=arm.r[(ops>>8) & 0xF];

    printd(4,"The intial op2 is %d (&",op2);
    printdc(4,"%X)\n",op2);

    if (((ops>>8) & 0xF)==15)
    {
      op2+=8;
    }

    op1=arm.r[(ops>>4) & 0xF];

    printd(4,"The intial op1 is %d (&",op1);
    printdc(4,"%X)\n",op1);

    if (((ops>>4) & 0xF)==15)
    {
      op1+=8;
    }

    op1=op1*op2;

    if ((op & 0xFF)==ARM_OP_MLA)
    {
      op2=arm.r[(ops>>12) & 0xF];

      printd(4,"The intial acc is %d (&",op2);
      printdc(4,"%X)\n",op2);

      if (((ops>>12) & 0xF)==15)
      {
        op2+=8;
      }

      op1=op1+op2;
    }

    arm.r[ops & 0xF]=(i32f) op1;

    if ((ops & 0xF)==15)
    {
      arm.r[15]-=4;
      processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,
                            (void*) 1);
    }

    if ((op & ARM_OP_S)!=0)
    {
      arm.cpsr=(arm.cpsr & ~(ARM_CPSR_N | ARM_CPSR_Z)) |
                (op1==0 ? ARM_CPSR_Z : 0) |
                ((((i32f) op1)>>(31-ARM_CPSR_Ns)) & ARM_CPSR_N);
    }

    return true;
  }
  else
  {
    *output_buffer+=oprintf(*output_buffer," (multi-cycle mul)");
  }

  return false;
}



ui8 system_execute_Branch (struct system_instr *instr,void *fu,
                           char **output_buffer)
{
  i32f op,ops;

  op=(*instr).op;
  ops=(*instr).ops;

  #ifdef ARM_BRANCHPRED
  if (ARM_BRANCHPRED!=-1)
  {
    ui8 *branchpred_buffer_internal;

/*    *output_buffer+=oprintf(*output_buffer," (accessing BTB for &amp;%X)",arm.r[15]);*/

    branchpred_buffer_internal=
    system_branchpred_write(arm.r[15]);

    if (*branchpred_buffer_internal==0)
    {
      *branchpred_buffer_internal=ARM_BRANCHPRED_DYNAMIC_WEAKEST_TAKEN;
    }
    else
    {
      (*branchpred_buffer_internal)++;

      if (*branchpred_buffer_internal>ARM_BRANCHPRED_DYNAMIC_STRONGEST_TAKEN)
      {
        *branchpred_buffer_internal=ARM_BRANCHPRED_DYNAMIC_STRONGEST_TAKEN;
      }
    }
  }

  (performance.branchpred_count)++;

  if ((*instr).branchpred_taken==false)
  {
    processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,
                          (void*) 1);

    *output_buffer+=oprintf(*output_buffer," (branch mispredicted)");

    (performance.branchpred_mispredict)++;
  }
  else
  {
    *output_buffer+=oprintf(*output_buffer," (branch predicted)");
  }
  #else
  processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,
                        (void*) 1);
  #endif

  if ((op & 0xFF)==ARM_OP_BL)
  {
    arm.r[14]=arm.r[15]+4;

    #ifdef ARM_RETURNPRED
    system_returnpred_push(arm.r[14]);
    #endif
  }

  arm.r[15]+=ops-4;

  return true;
}



ui8 system_execute_SingleMem (struct system_instr *instr,void *fu,
                              char **output_buffer)
{
  i32f op,ops,op1,op2,op2shift,temp;
  struct system_lsu_request *request;

  op=(*instr).op;
  ops=(*instr).ops;


  if ((*instr).status==0)
  {
    /* temp[0] holds the base; temp[1] holds the base(op)index.
       Which one is used when is dependent on the writeback and
       indexing settings of the instruction.
    */

    switch (op & (0xF<<16))
    {
    case ARM_OP2_CONST:
      op2=(ops>>8) & 0xFFF;
      break;
    case ARM_OP2_REG:
      op2=arm.r[(ops>>8) & 0xF];
      break;
    case ARM_OP2_REG__RRX:
      op2=arm.r[(ops>>8) & 0xF];
      op2=((op2>>1) & 0x7FFFFFFF) |
          (arm.cpsr & ARM_CPSR_C)<<(31-ARM_CPSR_Cs);
      break;
    default:
      op2=arm.r[(ops>>8) & 0xF];

      if ((op & (0xF<<16))<(6<<16))                 /* Shift by reg */
      {
        error_fatal("STR/LDR with an op2 with shift by reg (system_execute_SingleMem)",3);
      }
      else                                        /* Shift by const */
      {
        op2shift=(ops>>12) & 0x1F;

        if (op2shift==0)
        {
          op2shift=32;
        }
      }

      switch (op & (0xF<<16))
      {
      case ARM_OP2_REG__LSL_CONST:
        op2=op2<<op2shift;
        break;
      case ARM_OP2_REG__LSR_CONST:
        op2=(op2>>op2shift);
        op2=op2 & ((1<<(32-op2shift))-1);
        break;
      case ARM_OP2_REG__ASR_CONST:
      case ARM_OP2_REG__ASR_REG:
        temp=op2>>31 & 0x1;
        op2=op2>>op2shift;
        op2=op2 & ((1<<(32-op2shift))-1);
        op2=op2 | ~((temp<<(32-op2shift))-1);
        break;
      case ARM_OP2_REG__ROR_CONST:
      case ARM_OP2_REG__ROR_REG:
        op2=((op2>>op2shift) & ((1<<(32-op2shift))-1)) |
            (op2<<(32-op2shift));
      }
    }

    op1=arm.r[(ops>>4) & 0xF];

    if (((ops>>4) & 0xF)==15)
    {
      op1+=8;
    }

    (*instr).temp[0]=op1;
    if (((ops>>23) & 0x1)!=0)                       /* addr=op1+op2 */
    {
      (*instr).temp[1]=op1+op2;
    }
    else                                            /* addr=op1-op2 */
    {
      (*instr).temp[1]=op1-op2;
    }

    (*instr).status=1;

    *output_buffer+=oprintf(*output_buffer," (address generation)");

    return false;
  }

  if ((*instr).status==1)
  {
    if ((request=processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(0)))!=0)
    {
      if ((*request).index==-1)
      {
        processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(0),0);
        mem_free(request);

        *output_buffer+=oprintf(*output_buffer," (preceding LSU request serviced)");
      }
      else
      {
        *output_buffer+=oprintf(*output_buffer," (stalled on preceding LSU request)");

        return false;
      }
    }

    request=mem_alloc(sizeof(struct system_lsu_request));
    printd(128,"singlemem request at &%X\n",request);


    (*request).addr=(*instr).temp[(ops>>24) & 0x1];
    (*request).word=((ops>>22) & 0x1)==0 ? true : false;
    (*request).index=0;

    if ((op & 0xFF)==ARM_OP_LDR)
    {
      (*request).load=true;
    }
    else
    {
      (*request).load=false;
      temp=arm.r[ops & 0xF];
      (*request).value=(ops & 0xF)==15 ? temp+12 : temp;
    }

    processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(0),request);

    (*instr).status=2;

    *output_buffer+=oprintf(*output_buffer," (requesting %s)",
                            (op & 0xFF)==ARM_OP_LDR ? "load" :
                            "store");

    if (((ops>>21) & 0x1)!=0)
    {
      arm.r[(ops>>4) & 0xF]=(*instr).temp[1];
      *output_buffer+=oprintf(*output_buffer," (writeback)");
    }

    if ((op & 0xFF)==ARM_OP_LDR)
    {
      return false;
    }
    else
    {
      /* If saving, do not bother to wait for the LSU to
         acknowledge the write.
      */
      return true;
    }
  }

  if ((*instr).status==2)
  {
    if ((*(request=processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(0)))).
        index==-1)
    {
      if ((op & 0xFF)==ARM_OP_LDR)
      {
        arm.r[ops & 0xF]=(*request).value;

        if ((ops & 0xF)==15)
        {
          system_returnpred_r15dest(instr);
        }
      }

      processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(0),0);
      mem_free(request);

      *output_buffer+=oprintf(*output_buffer," (LSU request serviced)");
      return true;
    }
    else
    {
      *output_buffer+=oprintf(*output_buffer," (waiting for LSU to respond)");
    }
  }

  return false;
}



ui8 system_execute_MultiMem (struct system_instr *instr,void *fu,
                             char **output_buffer)
{
  i32f op,ops,op1;
  struct system_lsu_request *request;
  i32 temp,unit_loop;
  ui8 ready_flag,request_status;

  op=(*instr).op;
  ops=(*instr).ops;


  if ((*instr).status==0)
  {
    /* temp[0] contains the first address for transfer; temp[1]
       contains the address to be written back to the pointer
       register, if writeback is set.
    */
    temp=((ops>>15) & 0x1)+((ops>>14) & 0x1)+((ops>>13) & 0x1)+
         ((ops>>12) & 0x1)+((ops>>11) & 0x1)+((ops>>10) & 0x1)+
         ((ops>>9) & 0x1)+((ops>>8) & 0x1)+((ops>>7) & 0x1)+
         ((ops>>6) & 0x1)+((ops>>5) & 0x1)+((ops>>4) & 0x1)+
         ((ops>>3) & 0x1)+((ops>>2) & 0x1)+((ops>>1) & 0x1)+
         ((ops>>0) & 0x1);
    temp=temp<<2;

    op1=arm.r[(ops>>16) & 0xF];

    if ((ops & (1<<23))==0)                       /* Decrementation */
    {
      op1-=temp;
      (*instr).temp[1]=op1;
      op1+=4-(ops>>(24-2)) & (0x1<<2);
    }
    else                                          /* Incrementation */
    {
      (*instr).temp[1]=op1+temp;
      op1+=(ops>>(24-2)) & (0x1<<2);
    }

    (*instr).temp[0]=op1;

    (*instr).status=2;

    *output_buffer+=oprintf(*output_buffer," (address generation)");
  }


  if ((*instr).status==2)
  {
    /* The MultiMem pipes are filled from 0 to
       ARM_MULTIMEM_BANDWIDTH-1. Hence, if pipe n is empty, all
       subsequent pipes are empty, too. A collary is that, if pipe
       0 is empty, all pipes are empty.
    */
    if (processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(0))==0)
    {
      (*instr).status=16+16;
    }
    else
    {
      ready_flag=true;

      for (unit_loop=0;unit_loop<ARM_MULTIMEM_BANDWIDTH;unit_loop++)
      {
        if ((request=
             processsim_pipe_read(fu,
                                  SYSTEM_EXECUTE_LSU(unit_loop)))==0)
        {
          break;
        }

        if ((*request).index==-1)
        {
          processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(unit_loop),0);
          mem_free(request);

          *output_buffer+=oprintf(*output_buffer," (preceding LSU request serviced on pipe %d)",unit_loop);
        }
        else
        {
          ready_flag=false;

          *output_buffer+=oprintf(*output_buffer," (stalled on preceding LSU request on pipe %d)",unit_loop);
        }
      }

      if (ready_flag==true)
      {
        (*instr).status=16+16;
      }
    }

    return false;
  }


  if ((*instr).status>16)
  {
    ready_flag=true;

    for (unit_loop=0;unit_loop<ARM_MULTIMEM_BANDWIDTH;unit_loop++)
    {
      if ((request=
           processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(unit_loop)))==0)
      {
        break;
      }

      if ((*request).index!=-1)
      {
        ready_flag=false;
        break;
      }
    }

    if (ready_flag==true)
    {
      /* An old LSU request batch needs to be tidied up or there
         was no previous request batch.
      */
      for (unit_loop=0;unit_loop<ARM_MULTIMEM_BANDWIDTH;unit_loop++)
      {
        if ((request=
             processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(unit_loop)))==
            0)
        {
          break;
        }

        while (((ops>>(32-(*instr).status)) & 0x1)==0 &&
               (*instr).status>16)
        {
          (*instr).status--;
        }

        if ((op & 0xFF)==ARM_OP_LDM)
        {
          arm.r[32-(*instr).status]=(*request).value;
        }

        processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(unit_loop),0);
        mem_free(request);

        *output_buffer+=oprintf(*output_buffer," (completed %s R%d)",
                                (op & 0xFF)==ARM_OP_LDM ? "load to" :
                                "store from",32-(*instr).status);

        (*instr).status--;
      }
    }
    else
    {
      /* Stall until it completes. */
      *output_buffer+=oprintf(*output_buffer," (stalled on %s R%d)",
                              (op & 0xFF)==ARM_OP_LDM ? "load to" :
                              "store from",32-(*instr).status);

      return false;
    }

    request_status=(*instr).status;

/*    *output_buffer+=oprintf(*output_buffer," (bandwidth of %d words)",ARM_MULTIMEM_BANDWIDTH);*/

    for (unit_loop=0;unit_loop<ARM_MULTIMEM_BANDWIDTH;unit_loop++)
    {
/*      *output_buffer+=oprintf(*output_buffer," (fetch loop %d)",unit_loop);*/

      while (((ops>>(32-request_status)) & 0x1)==0 &&
             request_status>16)
      {
        request_status--;
      }

/*      *output_buffer+=oprintf(*output_buffer," (reg %d)",32-request_status);*/

      if (request_status>16)
      {
        request=mem_alloc(sizeof(struct system_lsu_request));
        printd(128,"multimem request at &%X\n",request);

        (*request).addr=(*instr).temp[0];
        (*request).word=true;
        (*request).index=0;

        if ((op & 0xFF)==ARM_OP_LDM)
        {
          (*request).load=true;
        }
        else
        {
          (*request).load=false;
          temp=arm.r[32-request_status];
          (*request).value=request_status==17 ? temp+12 : temp;
        }

        processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(unit_loop),request);

        (*instr).temp[0]+=4;

        *output_buffer+=oprintf(*output_buffer," (requesting %s R%d)",
                                (op & 0xFF)==ARM_OP_LDM ? "load to" :
                                "store from",32-request_status);

        request_status--;
      }
      else
      {
        if (unit_loop==0)
        {
          if (((ops>>15) & 0x1)!=0 && (op & 0xFF)==ARM_OP_LDM)
          {
            system_returnpred_r15dest(instr);
          }

          if (((ops>>21) & 0x1)!=0)
          {
            arm.r[(ops>>16) & 0xF]=(*instr).temp[1];
          }

          *output_buffer+=oprintf(*output_buffer," (writeback)");

          return true;
        }

        break;
      }

/*      *output_buffer+=oprintf(*output_buffer," (out end fetch loop %d)",unit_loop);*/
    }
  }

  return false;
}
#else



/* -------------------------------------------------------------------
   void system_decode2

   The Functional Unit representing the ARM's Second Instruction
   Decode and Register Renaming Unit.
------------------------------------------------------------------- */
void system_decode2 (void *fu)
{
  struct system_decode2_state *state;
  char *output_buffer;
  struct system_instr *instr;
  i32 regs_loop,regs_index,unit_loop;
  i32f reg,*reg_map,*reg_invmap,op,ops;

  state=processsim_workspace_read(fu);
  output_buffer=processsim_output_read(fu);

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=4>Decode 2/Register Renaming Unit</TH>"
          "</TR>");

  printd(4,"\nEntered system_decode2:\n",0);

  reg_map=(*state).reg_map;
  reg_invmap=(*state).reg_invmap;

  if (processsim_pipe_read(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH)!=0)
  {
    /* reload the system regs from the logical set */

    for (unit_loop=0;unit_loop<16;unit_loop++)
    {
      reg_map[unit_loop]=reg_invmap[unit_loop]=unit_loop;
      arm.pr[unit_loop]=arm.r[unit_loop];
    }

    reg_map[16]=ARM_CPSR_Ns;
    reg_invmap[ARM_CPSR_Ns]=16;
    arm.pr[16]=arm.cpsr & ARM_CPSR_N;
    reg_map[17]=ARM_CPSR_Zs;
    reg_invmap[ARM_CPSR_Zs]=17;
    arm.pr[17]=arm.cpsr & ARM_CPSR_Z;
    reg_map[18]=ARM_CPSR_Cs;
    reg_invmap[ARM_CPSR_Cs]=18;
    arm.pr[18]=arm.cpsr & ARM_CPSR_C;
    reg_map[19]=ARM_CPSR_Vs;
    reg_invmap[ARM_CPSR_Vs]=19;
    arm.pr[19]=arm.cpsr & ARM_CPSR_V;

    for (unit_loop=20;unit_loop<ARM_SUPERSCALAR_REGS;unit_loop++)
    {
      reg_map[unit_loop]=-1;
    }
  }


  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
  {
    printd(4,"\n  Pipe %d:\n",unit_loop);

    if ((instr=
         processsim_pipe_read(fu,SYSTEM_DECODE2_DECODE(unit_loop)))!=
        0 && processsim_pipe_read(fu,
                                  SYSTEM_DECODE2_ROB(unit_loop))==0)
    {
      if (processsim_pipe_read(PROCESSSIM_FU_NULL,
                               SYSTEM_CU_BRANCH)!=0)
      {
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle colspan=4>"
                "Flush: Control Unit Branch line raised"
                "</TD></TR>");

        mem_free(instr);
      }
      else
      {
        printd(4,"    Request in for decoding &%X can be sent\n",instr);

        #ifdef ARM_SUPERSCALAR_SHORT
        instr=system_decode_disassemble(instr);
        #endif

        op=(*instr).op;
        ops=(*instr).ops;

        (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_WAITING;
        (*instr).serialising=0;
        (*instr).regs_bitmap_rd=bitmaps_gen(ARM_SUPERSCALAR_REGS);
        (*instr).regs_bitmap_wr=bitmaps_gen(ARM_SUPERSCALAR_REGS);
        for (regs_loop=0;regs_loop<21;regs_loop++)
        {
          (*instr).regs_rd[regs_loop]=-1;
          (*instr).regs_wr[regs_loop]=-1;
          (*instr).regs_val[regs_loop]=0;
        }

/*        printd(4,"    Initially:\n",0);
        printd(4,"      rd bitmap: &",0);
        bp(4,(*instr).regs_bitmap_rd,ARM_SUPERSCALAR_REGS);
        printdc(4,"\n",0);
        printd(4,"      wr bitmap: &",0);
        bp(4,(*instr).regs_bitmap_rd,ARM_SUPERSCALAR_REGS);
        printdc(4,"\n",0);*/

        switch (op & 0xF<<8)
        {
        case ARM_OP_MI:
        case ARM_OP_PL:
        case ARM_OP_GE:
        case ARM_OP_LT:
        case ARM_OP_GT:
        case ARM_OP_LE:
          (*instr).regs_rd[0]=reg_invmap[ARM_CPSR_Ns];
        }

        switch (op & 0xF<<8)
        {
        case ARM_OP_EQ:
        case ARM_OP_NE:
        case ARM_OP_HI:
        case ARM_OP_LS:
        case ARM_OP_GT:
        case ARM_OP_LE:
          (*instr).regs_rd[1]=reg_invmap[ARM_CPSR_Zs];
        }

        switch (op & 0xF<<8)
        {
        case ARM_OP_CS:
        case ARM_OP_CC:
        case ARM_OP_HI:
        case ARM_OP_LS:
          (*instr).regs_rd[2]=reg_invmap[ARM_CPSR_Cs];
        }

        switch (op & 0xF<<8)
        {
        case ARM_OP_VS:
        case ARM_OP_VC:
        case ARM_OP_GE:
        case ARM_OP_LT:
        case ARM_OP_GT:
        case ARM_OP_LE:
          (*instr).regs_rd[3]=reg_invmap[ARM_CPSR_Vs];
        }


        if ((op & 0xFF)<0x10)
        {
          switch (op & 0xF)
          {
          case ARM_OP_ADC:
          case ARM_OP_SBC:
          case ARM_OP_RSC:
            (*instr).regs_rd[2]=reg_invmap[ARM_CPSR_Cs];
          }

          switch (op & 0xF<<16)
          {
          case ARM_OP2_REG__LSL_REG:
          case ARM_OP2_REG__LSR_REG:
          case ARM_OP2_REG__ASR_REG:
          case ARM_OP2_REG__ROR_REG:
            (*instr).regs_rd[6]=reg_invmap[(ops>>12) & 0xF];

            if (((ops>>12) & 0xF)==15)
            {
              (*instr).serialising=SYSTEM_INSTR_SERIALISING_R15READ;
            }
          case ARM_OP2_REG:
          case ARM_OP2_REG__LSL_CONST:
          case ARM_OP2_REG__LSR_CONST:
          case ARM_OP2_REG__ASR_CONST:
          case ARM_OP2_REG__ROR_CONST:
          case ARM_OP2_REG__RRX:
            (*instr).regs_rd[5]=reg_invmap[(ops>>8) & 0xF];

            if (((ops>>8) & 0xF)==15)
            {
              (*instr).serialising=(*instr).serialising |
                                   SYSTEM_INSTR_SERIALISING_R15READ;
            }
          }

          if ((op & 0xFF)!=ARM_OP_MOV &&
              (op & 0xFF)!=ARM_OP_MVN)
          {
            (*instr).regs_rd[4]=reg_invmap[(ops>>4) & 0xF];

            if (((ops>>4) & 0xF)==15)
            {
              (*instr).serialising=(*instr).serialising |
                                   SYSTEM_INSTR_SERIALISING_R15READ;
            }
          }

          if ((op & 0xC)!=0x8)
          {
            (*instr).regs_wr[4]=
            system_decode2_editmap(reg_map,reg_invmap,ops & 0xF,
                                   instr);

            if ((ops & 0xF)==15)
            {
              (*instr).serialising=(*instr).serialising |
                                   SYSTEM_INSTR_SERIALISING_R15WRITE;
            }
          }

          if ((op & ARM_OP_S)!=0)
          {
            switch (op & 0xF)
            {
            case ARM_OP_SUB:
            case ARM_OP_RSB:
            case ARM_OP_ADD:
            case ARM_OP_ADC:
            case ARM_OP_SBC:
            case ARM_OP_RSC:
            case ARM_OP_CMP:
            case ARM_OP_CMN:
              (*instr).regs_wr[3]=
              system_decode2_editmap(reg_map,reg_invmap,ARM_CPSR_Vs,
                                     instr);
            case ARM_OP_AND:
            case ARM_OP_EOR:
            case ARM_OP_TST:
            case ARM_OP_TEQ:
            case ARM_OP_ORR:
            case ARM_OP_MOV:
            case ARM_OP_BIC:
            case ARM_OP_MVN:
              (*instr).regs_wr[0]=
              system_decode2_editmap(reg_map,reg_invmap,ARM_CPSR_Ns,
                                     instr);
              (*instr).regs_wr[1]=
              system_decode2_editmap(reg_map,reg_invmap,ARM_CPSR_Zs,
                                     instr);
              (*instr).regs_wr[2]=
              system_decode2_editmap(reg_map,reg_invmap,ARM_CPSR_Cs,
                                     instr);
            }
          }
        }
        else
        {
          switch (op & 0xFF)
          {
          case ARM_OP_MLA:
            (*instr).regs_rd[6]=reg_invmap[(ops>>12) & 0xF];

            if (((ops>>12) & 0xF)==15)
            {
              (*instr).serialising=SYSTEM_INSTR_SERIALISING_R15READ;
            }
          case ARM_OP_MUL:
            (*instr).regs_rd[5]=reg_invmap[(ops>>8) & 0xF];
            (*instr).regs_rd[4]=reg_invmap[(ops>>4) & 0xF];
            (*instr).regs_wr[4]=
            system_decode2_editmap(reg_map,reg_invmap,ops & 0xF,
                                   instr);

            if ((ops & 0xF)==15)
            {
              (*instr).serialising=(*instr).serialising |
                                   SYSTEM_INSTR_SERIALISING_R15WRITE;
            }

            if (((ops>>8) & 0xF)==15 || ((ops>>4) & 0xF)==15)
            {
              (*instr).serialising=(*instr).serialising |
                                   SYSTEM_INSTR_SERIALISING_R15READ;
            }

            if ((op & ARM_OP_S)!=0)
            {
              (*instr).regs_wr[0]=
              system_decode2_editmap(reg_map,reg_invmap,ARM_CPSR_Ns,
                                     instr);
              (*instr).regs_wr[1]=
              system_decode2_editmap(reg_map,reg_invmap,ARM_CPSR_Zs,
                                     instr);
              (*instr).regs_wr[2]=
              system_decode2_editmap(reg_map,reg_invmap,ARM_CPSR_Cs,
                                     instr);
            }
            break;
          case ARM_OP_BL:
            (*instr).regs_wr[5]=system_decode2_editmap(reg_map,
                                                       reg_invmap,14,
                                                       instr);
          case ARM_OP_B:
            (*instr).regs_rd[4]=reg_invmap[15];
            (*instr).regs_wr[4]=system_decode2_editmap(reg_map,
                                                       reg_invmap,15,
                                                       instr);

            (*instr).serialising=SYSTEM_INSTR_SERIALISING_R15READ |
                                 SYSTEM_INSTR_SERIALISING_R15WRITE;
            break;
          case ARM_OP_STR:
          case ARM_OP_LDR:
            if ((op & 0xF<<16)!=ARM_OP2_CONST)
            {
              (*instr).regs_rd[6]=reg_invmap[(ops>>8) & 0xF];

              if (((ops>>8) & 0xF)==15)
              {
                (*instr).serialising=SYSTEM_INSTR_SERIALISING_R15READ;
              }
            }

            (*instr).regs_rd[5]=reg_invmap[(ops>>4) & 0xF];

            if ((op & 0xFF)==ARM_OP_LDR)
            {
              (*instr).regs_wr[4]=
              system_decode2_editmap(reg_map,reg_invmap,ops & 0xF,
                                     instr);
            }
            else
            {
              (*instr).regs_rd[4]=reg_invmap[ops & 0xF];
            }

            if ((op & 0xFF)==ARM_OP_LDR && (ops & 0xF)==15)
            {
              (*instr).serialising=(*instr).serialising |
                                   SYSTEM_INSTR_SERIALISING_R15WRITE;
            }

            if (((ops>>4) & 0xF)==15)
            {
              (*instr).serialising=(*instr).serialising |
                                   SYSTEM_INSTR_SERIALISING_R15READ;
            }

            if (((ops>>21) & 0x1)==0x1)
            {
              (*instr).regs_wr[5]=
              system_decode2_editmap(reg_map,reg_invmap,(ops>>4) & 
                                                        0xF,instr);
            }
            break;
          case ARM_OP_STM:
          case ARM_OP_LDM:
            (*instr).regs_rd[4]=reg_invmap[(ops>>16) & 0xF];

            if (((ops>>16) & 0xF)==15)
            {
              (*instr).serialising=SYSTEM_INSTR_SERIALISING_R15READ;
            }

            regs_index=5;

            if ((op & 0xFF)==ARM_OP_LDM)
            {
              printd(4,"  Grabbing LDM registers:\n",0);

              if (((ops>>15) & 0x1)!=0)
              {
                (*instr).serialising=
                (*instr).serialising |
                SYSTEM_INSTR_SERIALISING_R15WRITE;
              }

              for (regs_loop=0;regs_loop<16;regs_loop++)
              {
                if (((ops>>regs_loop) & 0x1)==0x1)
                {
                  (*instr).regs_wr[regs_index++]=
                  system_decode2_editmap(reg_map,reg_invmap,regs_loop,
                                         instr);

                  printd(4,"    R%d=>PR",regs_loop);
                  printdc(4,"%d\n",(*instr).regs_wr[regs_index-1]);
                }
              }
            }
            else
            {

printd(4,"\n\nSTM regs:\n",0);
printd(4,"The reg bitmap is &%X\n",ops & 0xFFFF);
printd(4,"",0);

              if (((ops>>15) & 0x1)!=0)
              {
                (*instr).serialising=(*instr).serialising |
                                     SYSTEM_INSTR_SERIALISING_R15READ;
              }

              for (regs_loop=0;regs_loop<16;regs_loop++)
              {
                if (((ops>>regs_loop) & 0x1)==0x1)
                {
                  (*instr).regs_rd[regs_index++]=
                  reg_invmap[regs_loop];

printdc(4,"%d,",(*instr).regs_rd[regs_index-1]);

                }
              }

printd(4,"\n\n\n",0);


            }

            if (((ops>>21) & 0x1)==0x1)
            {
              (*instr).regs_wr[4]=
              system_decode2_editmap(reg_map,reg_invmap,(ops>>16) &
                                                        0xF,instr);
            }
            break;
          case ARM_OP_MRS:
            if ((ops & 0xF<<16)==ARM_OP2_REG)
            {
              (*instr).regs_wr[4]=
              system_decode2_editmap(reg_map,reg_invmap,(ops>>8) &
                                                        0xF,instr);
            }

            (*instr).serialising=SYSTEM_INSTR_SERIALISING_MISC;
            break;
          case ARM_OP_MSR:
            if ((ops & 0xF<<16)==ARM_OP2_REG)
            {
              (*instr).regs_rd[4]=reg_invmap[(ops>>8) & 0xF];
            }

            (*instr).serialising=SYSTEM_INSTR_SERIALISING_MISC;
            break;
          default:
/*            error_fatal("Attempted to converted an undefined instr into a instr",5);*/
            break;
          }
        }

{char ob[512];
 *(ob+system_decode2_print(ob,instr))=0;
        printd(4,"    Microop is '%s'\n",ob);
}

        printd(4,"    Setting up rd/wr masks:\n",0);

        for (regs_loop=0;regs_loop<21;regs_loop++)
        {
          if ((reg=(*instr).regs_rd[regs_loop])!=-1)
          {
            printd(4,"      PR%d read\n",reg);
            bitmaps_set((*instr).regs_bitmap_rd,reg);
          }

          if ((reg=(*instr).regs_wr[regs_loop])!=-1)
          {
            printd(4,"      PR%d written\n",reg);
            bitmaps_set((*instr).regs_bitmap_wr,reg);
          }
        }

/*        printd(4,"    Finally:\n",0);
        printd(4,"      rd bitmap: &",0);
        bp(4,(*instr).regs_bitmap_rd,ARM_SUPERSCALAR_REGS);
        printdc(4,"\n",0);
        printd(4,"      wr bitmap: &",0);
        bp(4,(*instr).regs_bitmap_rd,ARM_SUPERSCALAR_REGS);
        printdc(4,"\n",0);*/


        processsim_pipe_write(fu,SYSTEM_DECODE2_ROB(unit_loop),
                              instr);

        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle colspan=4>");

        output_buffer+=system_decode2_print(output_buffer,instr);

        output_buffer+=
        oprintf(output_buffer,
                "</TD>"
                "</TR>");
      }

      processsim_pipe_write(fu,SYSTEM_DECODE2_DECODE(unit_loop),0);
    }
    else
    {
      if (processsim_pipe_read(fu,SYSTEM_DECODE2_ROB(unit_loop))==0)
      {
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle colspan=4>No decoding in progress</TD>"
                "</TR>");
      }
      else
      {
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle colspan=4>");

        output_buffer+=
        system_decode2_print
        (output_buffer,
         processsim_pipe_read(fu,SYSTEM_DECODE2_ROB(unit_loop)));

        output_buffer+=
        oprintf(output_buffer,
                " (decoded and waiting for a ROB slot) </TD>"
                "</TR>");
      }
    }
  }


  output_buffer+=
  oprintf(output_buffer,"<TR>"
                        "<TD align=left valign=middle colspan=4><EM>Physical to Logical Register Mapping</EM></TD>"
                        "</TR>");

  for (regs_loop=0;regs_loop<(ARM_SUPERSCALAR_REGS>>1);regs_loop++)
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle>PR%d=&amp;%X:</TD>"
            "<TD align=left valign=middle>R%d</TD>"
            "<TD align=left valign=middle>PR%d=&amp;%X:</TD>"
            "<TD align=left valign=middle>R%d</TD>"
            "</TR>",regs_loop,arm.pr[regs_loop],*(reg_map+regs_loop),
            regs_loop+(ARM_SUPERSCALAR_REGS>>1),
            arm.pr[regs_loop+(ARM_SUPERSCALAR_REGS>>1)],
            *(reg_map+regs_loop+(ARM_SUPERSCALAR_REGS>>1)));
  }

  oprintf(output_buffer,"</TABLE>");
}



/* -------------------------------------------------------------------
   i32f system_decode2_editmap

   Allocates the first free logical register to an ARM physical
   register, updating both the register map and its inverse. The
   ARM physical register is returned.
------------------------------------------------------------------- */
#ifdef ARM_REGS_ALLOCATE_CYCLIC
i32f reg_physical_index=0;
#endif


i32f system_decode2_editmap (i32f *reg_map,i32f *reg_invmap,
                             i32f reg_logical,
                             struct system_instr *instr)
{
  i32f *reg_map_internal,reg_physical;

  printd(4,"\nEntering system_decode2_editmap:\n",0);
  printd(4,"  Mapping logical register %d\n",reg_logical);

  if (ARM_SUPERSCALAR_REGS==20 || ((*instr).op & 0xF<<8)!=ARM_OP_AL ||
      reg_logical==15)
  {
    printd(4,reg_logical==15 ? "  Register 15 -> No remapping\n" : "  Conditional -> No remapping\n",0);

    reg_physical=reg_invmap[reg_logical];
  }
  else
  {
    reg_physical=-1;

    #ifdef ARM_REGS_ALLOCATE_FROMTOP
    reg_map_internal=reg_map+ARM_SUPERSCALAR_REGS-1;

    do
    {
      if (*reg_map_internal==-1)
      {
        reg_physical=reg_map_internal-reg_map;
        break;
      }
    }
    while ((reg_map_internal--)>reg_map);
    #endif

    #ifdef ARM_REGS_ALLOCATE_CYCLIC
    reg_map_internal=reg_map+reg_physical_index;

    do
    {
      if (*reg_map_internal==-1)
      {
        reg_physical=reg_map_internal-reg_map;
        reg_physical_index=reg_physical-1;
        break;
      }
    }
    while ((reg_map_internal--)>reg_map);

    if (reg_physical==-1)
    {
      reg_map_internal=reg_map+ARM_SUPERSCALAR_REGS-1;

      do
      {
        if (*reg_map_internal==-1)
        {
          reg_physical=reg_map_internal-reg_map;
          reg_physical_index=reg_physical-1;
          break;
        }
      }
      while ((reg_map_internal--)>reg_map);
    }

    if (reg_physical_index<0)
    {
      reg_physical_index=ARM_SUPERSCALAR_REGS-1;
    }
    #endif

    printd(4,"  Acquired physical register %d\n",reg_physical);

    if (reg_physical==-1)
    {
      error_fatal("No remaining physical registers",4);
    }

    reg_map_internal=reg_map+ARM_SUPERSCALAR_REGS-1;

    do
    {
      if (*reg_map_internal==reg_logical)
      {
        *reg_map_internal=-1;
        break;
      }
    }
    while ((reg_map_internal--)>reg_map);

    *(reg_map+reg_physical)=reg_logical;
    *(reg_invmap+reg_logical)=reg_physical;
  }

  printd(4,"\n\n",0);

  return reg_physical;
}



/* -------------------------------------------------------------------
   void system_rob

   The Functional Unit representing the ARM's Reorder Buffer.
------------------------------------------------------------------- */
void system_rob (void *fu)
{
  struct system_rob_state *state;
  char *output_buffer;
  struct system_instr *instr;
  i32 rob_loop,unit_loop;
  void *regs_bitmap_rd,*regs_bitmap_wr;
  ui8 passed_load,passed_store,passed_serialising;
  struct system_instr **rob_rd_internal,**rob_wr_internal,
                      **rob_internal_end;

  state=processsim_workspace_read(fu);
  output_buffer=processsim_output_read(fu);

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle>Re-order Buffer</TH>"
          "</TR>");

  printd(4,"\nEntered system_rob:\n",0);

  if (processsim_pipe_read(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH)!=0)
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle>"
            "Flush: Control Unit Branch line raised"
            "</TD></TR>");

    for (rob_loop=0;rob_loop<ARM_SUPERSCALAR_ROB;rob_loop++)
    {
      if ((instr=(*state).rob[rob_loop])==0)
      {
        break;
      }

      bitmaps_del((*instr).regs_bitmap_rd);
      bitmaps_del((*instr).regs_bitmap_wr);
      mem_free(instr);
      (*state).rob[rob_loop]=(struct system_instr*) 0;
    }

    for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
    {
      if ((instr=
           processsim_pipe_read(fu,SYSTEM_ROB_DECODE2(unit_loop)))!=0)
      {
        processsim_pipe_write(fu,SYSTEM_ROB_DECODE2(unit_loop),0);

        bitmaps_del((*instr).regs_bitmap_rd);
        bitmaps_del((*instr).regs_bitmap_wr);
        mem_free(instr);
      }
    }
  }
  else
  {
    printd(4,"  Fetching decode instrs:\n",0);

    /* Fetch instrs from the decode2. */
    for (rob_loop=0;rob_loop<ARM_SUPERSCALAR_ROB;rob_loop++)
    {
      if ((*state).rob[rob_loop]==0)
      {
        break;
      }
    }

    printd(4,"    instrs entering ROB at entry %d\n",rob_loop);

    if ((ARM_SUPERSCALAR_ROB-rob_loop)>=ARM_SUPERSCALAR_FETCH)
    {
      for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
      {
        if ((instr=
             processsim_pipe_read(fu,SYSTEM_ROB_DECODE2(unit_loop)))!=0)
        {
          (*state).rob[rob_loop++]=instr;

          processsim_pipe_write(fu,SYSTEM_ROB_DECODE2(unit_loop),0);
        }
      }
    }

    /* Generate rolling read and write masks, check for issuable
       instrs and attempt to issue successful cases.
    */
    printd(4,"  Checking for dependencies in the ROB:\n",0);

    regs_bitmap_rd=(*state).regs_bitmap_rd;
    regs_bitmap_wr=(*state).regs_bitmap_wr;

    bitmaps_wipe(regs_bitmap_rd,ARM_SUPERSCALAR_REGS);
    bitmaps_wipe(regs_bitmap_wr,ARM_SUPERSCALAR_REGS);

/*    printd(4,"      rd bitmap: &",0);
    bp(4,regs_bitmap_rd,ARM_SUPERSCALAR_REGS);
    printdc(4,"\n",0);
    printd(4,"      wr bitmap: &",0);
    bp(4,regs_bitmap_wr,ARM_SUPERSCALAR_REGS);
    printdc(4,"\n",0);*/

    printd(4,"    Checking for executing instrs:\n",0);

    for (rob_loop=0;rob_loop<ARM_SUPERSCALAR_ROB;rob_loop++)
    {
      if ((instr=(*state).rob[rob_loop])==0)
      {
        break;
      }

      if ((*instr).rob_status==SYSTEM_INSTR_ROB_STATUS_ISSUED)
      {
        printd(4,"      Entry %d issued\n",rob_loop);

/*        printd(4,"      instr wr bitmap: &",0);
        bp(4,(*instr).regs_bitmap_wr,ARM_SUPERSCALAR_REGS);
        printdc(4,"\n",0);*/

        bitmaps_or(regs_bitmap_wr,regs_bitmap_wr,
                   (*instr).regs_bitmap_wr,ARM_SUPERSCALAR_REGS);

/*        printd(4,"      wr bitmap: &",0);
        bp(4,regs_bitmap_wr,ARM_SUPERSCALAR_REGS);
        printdc(4,"\n",0);*/
      }
    }

    printd(4,"    Checking for independant instrs:\n",0);

    passed_load=false;
    passed_store=false;
    passed_serialising=false;

    for (rob_loop=0;rob_loop<ARM_SUPERSCALAR_ROB;rob_loop++)
    {
      printd(4,"      Entry %d: &",rob_loop);

      #ifdef ARM_ROB_SPECEXEC
      if ((instr=(*state).rob[rob_loop])==0)
      #else
      if ((instr=(*state).rob[rob_loop])==0 ||
          ((*instr).serialising!=0 && rob_loop!=0))
      #endif
      {
        break;
      }

      printdc(4,"%X:\n",instr);

      if ((*instr).rob_status==SYSTEM_INSTR_ROB_STATUS_WAITING)
      {
        printd(4,"        Waiting to be issued\n",0);

        /* I think that here is an appropriate place to go through
           what the speculative execution options are and how they
           relate to (*instr).serialising.

           Serialising instructions are those which, in some way,
           either require the logical ARM state (i.e. that of the
           retirement unit) to be entirely up to date before being
           issued (SYSTEM_INSTR_SERIALISING_R15READ) or change the
           flow of control, so that instructions coming later in the
           ROB may not actually get executed (the remainder).

           ARM_ROB_SPECEXEC allows the virtual processor to
           recognise that, since logical ARM register writes are
           gated through the retirement unit, any instructions
           writing only to registers (excluding the PC) after a
           serialising instruction can quite happily be allowed to
           execute and write their results to the physical
           registers, which will all be reset if the serialising
           instruction changes flow of control contrary to the
           prediction.

           Since the physical registers do not maintain R15, any
           instruction reading the PC gets its value from the
           logical ARM register set, which means that all preceeding
           instructions must have already retired.
           ARM_ROB_SPECEXEC_R15READ configures the virtual processor
           to have all instructions carry around their own PC with
           them, so PC-reading instructions can be issued as soon as
           all other registers are ready (i.e. they are effectively
           no longer serialising instructions).

           Since writes to the PC also have their flow of control
           effect deferred until retirement,
           ARM_ROB_SPECEXEC_R15WRITE allows the virtual processor to
           issue PC-writing instructions out of order.

           The code below sets, by various predicates, the extent to
           which instructions may be issued out of order (should
           they be in the presence of serialising instructions) and
           also verifies that instructions which commit some or all
           of their results before retirement (e.g. those writing to
           memory) are correctly stalled, where necessary.
        */
        #ifdef ARM_ROB_SPECEXEC
        if (bitmaps_test_and((*instr).regs_bitmap_rd,regs_bitmap_wr,
                             ARM_SUPERSCALAR_REGS)==true &&
            bitmaps_test_and((*instr).regs_bitmap_wr,regs_bitmap_rd,
                             ARM_SUPERSCALAR_REGS)==true &&
            bitmaps_test_and((*instr).regs_bitmap_wr,regs_bitmap_wr,
                             ARM_SUPERSCALAR_REGS)==true &&

        #ifdef ARM_ROB_SPECEXEC_R15READ
        #ifdef ARM_ROB_SPECEXEC_R15WRITE
            (((*instr).serialising &
              SYSTEM_INSTR_SERIALISING_MISC)==0 || rob_loop==0))
        #else
            (((*instr).serialising &
              (SYSTEM_INSTR_SERIALISING_MISC |
               SYSTEM_INSTR_SERIALISING_R15WRITE))==0 || rob_loop==0))
        #endif
        #else
        #ifdef ARM_ROB_SPECEXEC_R15WRITE
            (((*instr).serialising &
              (SYSTEM_INSTR_SERIALISING_MISC |
               SYSTEM_INSTR_SERIALISING_R15READ))==0 || rob_loop==0))
        #else
            (((*instr).serialising &
              (SYSTEM_INSTR_SERIALISING_MISC |
               SYSTEM_INSTR_SERIALISING_R15READ |
               SYSTEM_INSTR_SERIALISING_R15WRITE))==0 || rob_loop==0))
        #endif
        #endif
        #else
        if (bitmaps_test_and((*instr).regs_bitmap_rd,regs_bitmap_wr,
                             ARM_SUPERSCALAR_REGS)==true &&
            bitmaps_test_and((*instr).regs_bitmap_wr,regs_bitmap_rd,
                             ARM_SUPERSCALAR_REGS)==true &&
            bitmaps_test_and((*instr).regs_bitmap_wr,regs_bitmap_wr,
                             ARM_SUPERSCALAR_REGS)==true)
        #endif
        {
          printd(4,"      Entry %d issuable\n",rob_loop);

          /* Structural hazards prevent certain types of instruction
             reordering with respect to others:

             1. Loads may not be reordered with respect to stores.
             2. Stores may not be reordered with respect to stores.

             Furthermore, examination of the ROB must cease on
             serialising instrs, unless they are in the first
             entry of the ROB.

             1. Instructions making use of R15 are serialising,
                unless the engine is configured to disobey this.
             2. Branches not certain to have been correctly
                predicted are serialising, subject to the proviso
                above.
             3. Instructions affecting non-condition flag
                components of the CPSR are serialising.
          */
          if (((*instr).op & 0xFF)<0x10)
          {
            instr=system_rob_issue(fu,instr,
                                   SYSTEM_ROB_DATAPROC(0),
                                   ARM_SUPERSCALAR_DATAPROC);
          }
          else
          {
            switch ((*instr).op & 0xFF)
            {
            case ARM_OP_MUL:
            case ARM_OP_MLA:
              instr=system_rob_issue(fu,instr,
                                     SYSTEM_ROB_MUL(0),
                                     ARM_SUPERSCALAR_MUL);
              break;
            case ARM_OP_B:
            case ARM_OP_BL:
              if (ARM_SUPERSCALAR_BRANCH==0)
              {
                instr=system_rob_issue(fu,instr,
                                       SYSTEM_ROB_DATAPROC(0),1);
              }
              else
              {
                instr=system_rob_issue(fu,instr,
                                       SYSTEM_ROB_BRANCH(0),
                                       ARM_SUPERSCALAR_BRANCH);
              }
              break;
            case ARM_OP_STR:
              if (passed_load==false && passed_store==false &&
                  passed_serialising==false)
              {
                instr=system_rob_issue(fu,instr,
                                       SYSTEM_ROB_SINGLEMEM(0),
                                       ARM_SUPERSCALAR_SINGLEMEM);
              }
              break;
            case ARM_OP_LDR:
              if (passed_store==false)
              {
                instr=system_rob_issue(fu,instr,
                                       SYSTEM_ROB_SINGLEMEM(0),
                                       ARM_SUPERSCALAR_SINGLEMEM);
              }
              break;
            case ARM_OP_STM:
              if (passed_load==false && passed_store==false &&
                  passed_serialising==false)
              {
                instr=system_rob_issue(fu,instr,
                                       SYSTEM_ROB_MULTIMEM(0),
                                       ARM_SUPERSCALAR_MULTIMEM);
              }
              break;
            case ARM_OP_LDM:
              if (passed_store==false)
              {
                instr=system_rob_issue(fu,instr,
                                       SYSTEM_ROB_MULTIMEM(0),
                                       ARM_SUPERSCALAR_MULTIMEM);
              }
              break;
            case ARM_OP_MRS:
            case ARM_OP_MSR:
              error_fatal("Need to do MRS/MSR!!",666);
            }
          }

          if (instr!=0)
          {
            printd(4,"      Entry %d now issued\n",rob_loop);
          }

          instr=(*state).rob[rob_loop];
        }
        else
        {
          printd(4,"      Entry %d non-issuable\n",rob_loop);
        }

        #ifdef ARM_ROB_QUICKMEM
        switch ((*instr).op & 0xFF)
        {
        case ARM_OP_STR:
          passed_store=true;
          break;
        case ARM_OP_LDR:
          passed_load=true;
        }
        #endif

        bitmaps_or(regs_bitmap_rd,regs_bitmap_rd,
                   (*instr).regs_bitmap_rd,ARM_SUPERSCALAR_REGS);
        bitmaps_or(regs_bitmap_wr,regs_bitmap_wr,
                   (*instr).regs_bitmap_wr,ARM_SUPERSCALAR_REGS);
      }

      #ifdef ARM_ROB_QUICKMEM
      switch ((*instr).op & 0xFF)
      {
      case ARM_OP_STM:
        passed_store=true;
        break;
      case ARM_OP_LDM:
        passed_load=true;
      }
      #else
      switch ((*instr).op & 0xFF)
      {
      case ARM_OP_STR:
      case ARM_OP_STM:
        passed_store=true;
        break;
      case ARM_OP_LDR:
      case ARM_OP_LDM:
        passed_load=true;
      }
      #endif

      /* Even if ARM_ROB_SPECEXEC_R15WRITE, since later instructions
         may not be executed, further non-register writing
         operations may not be issued.
      */
      #ifdef ARM_ROB_SPECEXEC
      #ifdef ARM_ROB_SPECEXEC_R15READ
      if (((*instr).serialising &
           (SYSTEM_INSTR_SERIALISING_MISC |
            SYSTEM_INSTR_SERIALISING_R15WRITE))!=0)
      #else
      if (((*instr).serialising &
           (SYSTEM_INSTR_SERIALISING_MISC |
            SYSTEM_INSTR_SERIALISING_R15READ |
            SYSTEM_INSTR_SERIALISING_R15WRITE))!=0)
      #endif
      {
        passed_serialising=true;
      }
      #else
      if ((*instr).serialising==true)
      {
        break;
      }
      #endif
    }


    printd(4,"    Retiring instructions:\n",0);

    rob_loop=0;

    while (rob_loop<ARM_SUPERSCALAR_ROB &&
           rob_loop<ARM_SUPERSCALAR_RETIRE &&
           (instr=(*state).rob[rob_loop])!=0 &&
           (*instr).rob_status>=SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED)
    {
      processsim_pipe_write(fu,SYSTEM_ROB_RETIRE(rob_loop),
                            instr);
      rob_loop++;
    }

    printd(4,"    Shunting up instrs:\n",0);

    if (rob_loop!=0)
    {
      rob_wr_internal=(*state).rob;
      rob_rd_internal=rob_wr_internal+rob_loop;
      rob_internal_end=rob_wr_internal+ARM_SUPERSCALAR_ROB;

      while (rob_rd_internal<rob_internal_end)
      {
        *(rob_wr_internal++)=*(rob_rd_internal++);
      }

      while (rob_wr_internal<rob_internal_end)
      {
        *(rob_wr_internal++)=(struct system_instr*) 0;
      }
    }


    printd(4,"About to cycle through the rob entries",0);

    for (rob_loop=0;rob_loop<ARM_SUPERSCALAR_ROB;rob_loop++)
    {
      output_buffer+=
      oprintf(output_buffer,
              "<TR>"
              "<TD align=left valign=middle>");

      if ((instr=(*state).rob[rob_loop])!=0)
      {
         output_buffer+=system_decode2_print(output_buffer,instr);
      }
      else
      {
        output_buffer+=oprintf(output_buffer,"Empty");
      }

      output_buffer+=
      oprintf(output_buffer,
              "</TD>"
              "</TR>");
    }
  }

  oprintf(output_buffer,"</TABLE>");
}



/* -------------------------------------------------------------------
   struct system_instr *system_rob_issue

   Checks a instr to see if it can be issued and, if so, does so.
   A pointer to the instr is returned if the issue goes ahead; 0
   is returned otherwise.
------------------------------------------------------------------- */
struct system_instr *system_rob_issue (void *fu,
                                       struct system_instr
                                       *instr,
                                       i32 pipe_base,i32 pipe_count)
{
  i32 unit_loop,unit_end,reg_loop,reg;

  unit_end=pipe_base+pipe_count;

{char ob[512];
 *(ob+system_decode2_print(ob,instr))=0;
        printd(4,"Attempting to issuing instr %s:\n",ob);
}

  for (unit_loop=pipe_base;unit_loop<unit_end;unit_loop++)
  {
    if (processsim_pipe_read(fu,unit_loop)==0)
    {
      printd(4,"  Issuing to pipe %d\n",unit_loop);

      processsim_pipe_write(fu,unit_loop,instr);

      printd(4,"  Reading regs:\n",unit_loop);

      for (reg_loop=0;reg_loop<21;reg_loop++)
      {
        if ((reg=(*instr).regs_rd[reg_loop])!=-1)
        {
          #ifdef ARM_ROB_SPECEXEC_R15READ
          if (reg==15)                              /* PR15 <=> R15 */
          {
            (*instr).regs_val[reg_loop]=(*instr).pc;
          }
          else
          #endif
          {
            (*instr).regs_val[reg_loop]=arm.pr[reg];
          }

          printd(4,"    PR%d=",reg);
          printdc(4,"%d\n",arm.pr[reg]);
        }
      }

      (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_ISSUED;

      return instr;
    }
  }

  (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_WAITING;

  return ((struct system_instr*) 0);
}



/* -------------------------------------------------------------------
   void system_execute

   The Functional Unit representing the ARM's Instruction Execution
   Unit.
------------------------------------------------------------------- */
void system_execute (void *fu)
{
  struct system_execute_state *state;
  struct system_instr *instr;
  char *output_buffer;
  i32f op,ops,temp,flags;
  i32 reg_loop;

  state=processsim_workspace_read(fu);
  output_buffer=processsim_output_read(fu);

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=2>Execution Unit</TH>"
          "</TR>");

  printd(4,"\nEntered system_execute:\n",0);

  if (processsim_pipe_read(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH)!=0)
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=2>"
            "Flush: Control Unit Branch line raised"
            "</TD></TR>");

    /* Remove the instr from the incoming pipe, but do not attempt
       to free it, since the ROB will do that for us.
    */
    processsim_pipe_write(fu,SYSTEM_EXECUTE_ROB,0);
  }


  if ((instr=processsim_pipe_read(fu,SYSTEM_EXECUTE_ROB))!=0)
  {
    printd(4,"  Instruction ready for execution:\n",0);

    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=2>");

    output_buffer+=system_decode2_print(output_buffer,instr);

    op=(*instr).op;
    ops=(*instr).ops;

    ((*state).cycles_loaded)++;

    flags=(*instr).regs_val[0] | (*instr).regs_val[1] |
          (*instr).regs_val[2] | (*instr).regs_val[3];

    temp=(op & 0xF<<8) | flags;

    if (

(temp & (0xF<<8))==ARM_OP_AL ||

(temp & ((0xF<<8) | ARM_CPSR_Z))==(ARM_OP_EQ | ARM_CPSR_Z) ||
(temp & ((0xF<<8) | ARM_CPSR_Z))==ARM_OP_NE ||
(temp & ((0xF<<8) | ARM_CPSR_C))==(ARM_OP_CS | ARM_CPSR_C) ||
(temp & ((0xF<<8) | ARM_CPSR_C))==ARM_OP_CC ||
(temp & ((0xF<<8) | ARM_CPSR_N))==(ARM_OP_MI | ARM_CPSR_N) ||
(temp & ((0xF<<8) | ARM_CPSR_N))==ARM_OP_PL ||
(temp & ((0xF<<8) | ARM_CPSR_V))==(ARM_OP_VS | ARM_CPSR_V) ||
(temp & ((0xF<<8) | ARM_CPSR_V))==ARM_OP_VC ||

(temp & ((0xF<<8) | ARM_CPSR_C | ARM_CPSR_Z))==(ARM_OP_HI | ARM_CPSR_C) ||

(temp & ((0xF<<8) | ARM_CPSR_C))==ARM_OP_LS ||
(temp & ((0xF<<8) | ARM_CPSR_Z))==(ARM_OP_LS | ARM_CPSR_Z) ||

(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_GE | ARM_CPSR_N | ARM_CPSR_V) ||
(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==ARM_OP_GE ||

(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_LT | ARM_CPSR_N) ||
(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_LT | ARM_CPSR_V) ||

(temp & ((0xF<<8) | ARM_CPSR_Z | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_GT | ARM_CPSR_N | ARM_CPSR_V) ||
(temp & ((0xF<<8) | ARM_CPSR_Z | ARM_CPSR_N | ARM_CPSR_V))==ARM_OP_GT ||

(temp & ((0xF<<8) | ARM_CPSR_Z))==(ARM_OP_LE | ARM_CPSR_Z) ||
(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_LE | ARM_CPSR_N) ||
(temp & ((0xF<<8) | ARM_CPSR_N | ARM_CPSR_V))==(ARM_OP_LE | ARM_CPSR_V)

)

    {
      ((*state).cycles_processing)++;

      if ((op & 0xFF)<0x10)
      {
        flags=system_execute_DataProc(instr,flags,fu,
                                      &output_buffer);
      }
      else
      {
        switch (op & 0xFF)
        {
        case ARM_OP_MUL:
        case ARM_OP_MLA:
          flags=system_execute_Mul(instr,flags,fu,&output_buffer);
          break;
        case ARM_OP_B:
        case ARM_OP_BL:
          flags=system_execute_Branch(instr,flags,fu,
                                      &output_buffer);
          break;
        case ARM_OP_STR:
        case ARM_OP_LDR:
          flags=system_execute_SingleMem(instr,flags,fu,
                                         &output_buffer);
          break;
        case ARM_OP_STM:
        case ARM_OP_LDM:
          flags=system_execute_MultiMem(instr,flags,fu,
                                        &output_buffer);
          break;

        case ARM_OP_MRS:
        case ARM_OP_MSR:
/*          done=system_execute_PSRTrans(instr,flags,fu,&output_buffer);*/
          error_fatal("Need to do MRS/MSR!!",666);
          break;

        default:
          (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED;
        }
      }

      printd(4,"\nAbout to check to see if instr has finished:\n",0);

      if ((*instr).rob_status==SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED)
      {
        printd(4,"  Finished\n",0);

        processsim_pipe_write(fu,SYSTEM_EXECUTE_ROB,0);

        (*instr).regs_val[0]=flags & ARM_CPSR_N;
        (*instr).regs_val[1]=flags & ARM_CPSR_Z;
        (*instr).regs_val[2]=flags & ARM_CPSR_C;
        (*instr).regs_val[3]=flags & ARM_CPSR_V;

        printd(4,"  Flags written to write block\n",0);

        for (reg_loop=0;reg_loop<21;reg_loop++)
        {
          if ((temp=(*instr).regs_wr[reg_loop])!=-1)
          {
            arm.pr[temp]=(*instr).regs_val[reg_loop];
          }
        }

        output_buffer+=oprintf(output_buffer," (completed)");

        printd(4,"  Physical registers written to\n",0);
      }
      else
      {
        #ifdef ARM_ROB_QUICKMEM
        if ((*instr).rob_status==SYSTEM_INSTR_ROB_STATUS_WAITING)
        {
          processsim_pipe_write(fu,SYSTEM_EXECUTE_ROB,0);
          output_buffer+=oprintf(output_buffer," (unissued)");
        }
        #endif

        output_buffer+=oprintf(output_buffer," (not completed)");
      }
    }
    else
    {
      output_buffer+=oprintf(output_buffer," (not executed)");

      printd(4,"  Not executed\n",0);

      (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_DONE_UNEXECUTED;

      processsim_pipe_write(fu,SYSTEM_EXECUTE_ROB,0);

      output_buffer+=oprintf(output_buffer," (completed)");

      printd(4,"  Physical registers not written to\n",0);
    }


    printd(4,"\nAbout to check to see if instr has finished:\n",0);

    output_buffer+=
    oprintf(output_buffer,
            "</TD>"
            "</TR>");
  }
  else
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=2>No execution in progress</TD>"
            "</TR>");
  }

/*  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle>Time Loaded:</TD>"
          "<TD align=left valign=middle>%d cycles (%.3f%%)</TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle>Time Processing:</TD>"
          "<TD align=left valign=middle>%d cycles (%.3f%%)</TD>"
          "</TR>"
          "</TABLE>",(*state).cycles_loaded,
                     (((double) ((*state).cycles_loaded))*100)/
                     (((double) processsim_main_clocks())+0.0001),
                     (*state).cycles_processing,
                     (((double) ((*state).cycles_processing))*100)/
                     (((double) processsim_main_clocks())+0.0001));*/

  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle>Loaded:</TD>"
          "<TD align=left valign=middle>%.3f%%</TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle>Processing:</TD>"
          "<TD align=left valign=middle>%.3f%%</TD>"
          "</TR>"
          "</TABLE>",(((double) ((*state).cycles_loaded))*100)/
                     (((double) processsim_main_clocks())+0.0001),
                     (((double) ((*state).cycles_processing))*100)/
                     (((double) processsim_main_clocks())+0.0001));
}



i32f system_execute_DataProc (struct system_instr *instr,
                              i32f flags_in,void *fu,
                              char **output_buffer)
{
  i64 dest64,dest32;
  i32f op2shift,op2,op1,op,ops,flags_out;

  op=(*instr).op;
  ops=(*instr).ops;


  /* Prepare for using the flags. */
  flags_out=flags_in;

  printd(4,"The initial state of the carry flag is %d\n",(flags_in & ARM_CPSR_C)>>ARM_CPSR_Cs);


  /* Operand 2: */
  if ((*instr).status==1)
  {
    printd(4,"This is a reg shift's second cycle\n",0);

    op2=(*instr).temp[0];
    flags_out=(*instr).temp[1];
  }
  else
  {
    switch (op & (0xF<<16))
    {
    case ARM_OP2_CONST:
      op2=ROR((ops>>8) & 0xFF,(ops>>15) & 0x1E);
      break;
    case ARM_OP2_REG:
      op2=(*instr).regs_val[5];

      printd(4,"The intial op2 is %d (&",op2);
      printdc(4,"%X)\n",op2);

      if (((ops>>8) & 0xF)==15)
      {
        op2+=8;
      }
      break;
    case ARM_OP2_REG__RRX:
      op2=(*instr).regs_val[5];

      printd(4,"The initial op2 is %d (&",op2);
      printdc(4,"%X)\n",op2);

      if (((ops>>8) & 0xF)==15)
      {
        op2+=8;
      }
      flags_out=(flags_out & ~ARM_CPSR_C) |
                ((op2 & 0x1)<<ARM_CPSR_Cs);
      op2=(op2>>1) | (flags_in & ARM_CPSR_C)<<(31-ARM_CPSR_Cs);
      break;
    default:
      op2=(*instr).regs_val[5];

      printd(4,"The initial op2 is %d (&",op2);
      printdc(4,"%X)\n",op2);

      if ((op & (0xF<<16))<(6<<16))                 /* Shift by reg */
      {
        op2shift=(*instr).regs_val[6];

        printd(4,"The initial regwise op2shift is %d (&",op2shift);
        printdc(4,"%X)\n",op2shift);

        if (op2shift>31 && (op & ARM_OP2_REG__ROR_REG)!=0)
        {
          op2shift=op2shift & 0x1F;
        }

        if (op2shift>32)
        {
          op2shift=33;
        }

        printd(4,"The final op2shift is %d (&",op2shift);
        printdc(4,"%X)\n",op2shift);

        if (((ops>>8) & 0xF)==15)
        {
          op2+=12;
        }
      }
      else                                        /* Shift by const */
      {
        op2shift=(ops>>12) & 0x1F;

        printd(4,"The initial const op2shift is quoted as %d (&",op2shift);
        printdc(4,"%X)\n",op2shift);

        if (op2shift==0)
        {
          op2shift=32;
        }

        printd(4,"The final op2shift is %d (&",op2shift);
        printdc(4,"%X)\n",op2shift);

        if (((ops>>8) & 0xF)==15)
        {
          op2+=8;
        }
      }

      if (op2shift!=0)
      {
        switch (op & (0xF<<16))
        {
        case ARM_OP2_REG__LSL_CONST:
        case ARM_OP2_REG__LSL_REG:
          if (op2shift==33)
          {
            flags_out=flags_out & ~ARM_CPSR_C;
            op2=0;
          }
          else
          {
            flags_out=(flags_out & ~ARM_CPSR_C) |
                      (((op2>>(32-op2shift)) & 0x1)<<ARM_CPSR_Cs);
            op2=op2<<op2shift;
          }
          break;
        case ARM_OP2_REG__LSR_CONST:
        case ARM_OP2_REG__LSR_REG:
          if (op2shift==33)
          {
            flags_out=flags_out & ~ARM_CPSR_C;
            op2=0;
          }
          else
          {
            flags_out=(flags_out & ~ARM_CPSR_C) |
                      (((op2>>(op2shift-1)) & 0x1)<<ARM_CPSR_Cs);
            op2=(op2>>op2shift);
            op2=op2 & ((1<<(32-op2shift))-1);
          }
          break;
        case ARM_OP2_REG__ASR_CONST:
        case ARM_OP2_REG__ASR_REG:
          if (op2shift>31)
          {
            flags_out=(flags_out & ~ARM_CPSR_C) |
                      (((op2>>31) & 0x1)<<ARM_CPSR_Cs);
            op2=~((op2>>31 & 0x1)-1);
          }
          else
          {
            flags_out=(flags_out & ~ARM_CPSR_C) |
                      (((op2>>(op2shift-1)) & 0x1)<<ARM_CPSR_Cs);
            op2=(op2>>op2shift) |
                ~(((op2>>31 & 0x1) << (32-op2shift))-1);
          }
          break;
        case ARM_OP2_REG__ROR_CONST:
        case ARM_OP2_REG__ROR_REG:
          flags_out=(flags_out & ~ARM_CPSR_C) |
                    (((op2>>(op2shift-1)) & 0x1)<<ARM_CPSR_Cs);
          op2=((op2>>op2shift) & ((1<<(32-op2shift))-1)) |
              (op2<<(32-op2shift));
        }
      }
    }

    if ((op & (0xF<<16))>(0x1<<16) && (op & (0xF<<16))<(0x6<<16))
    {
      (*instr).status=1;
      (*instr).temp[0]=op2;
      (*instr).temp[1]=flags_out;

      *output_buffer+=oprintf(*output_buffer," (regsiter shift)");

      return false;
    }
  }

  printd(4,"The resulting op2 is %d (&",op2);
  printdc(4,"%X)\n",op2);
  printd(4,"The final state of the carry flag is %d\n\n",(flags_out & ARM_CPSR_C)>>ARM_CPSR_Cs);


  /* Operand 1: */
  op1=(*instr).regs_val[4];

  printd(4,"The intial op1 is %d (&",op1);
  printdc(4,"%X)\n",op1);

  if (((ops>>4) & 0xF)==15)
  {
    op1+=8+(((*instr).status)<<2);
  }

  printd(4,"The resulting op1 is %d (&",op1);
  printdc(4,"%X)\n",op1);


  /* Operation: */
  switch (op & 0xF)
  {
  case ARM_OP_TST:
  case ARM_OP_AND:
    dest64=((i64) op1) & ((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF) & (((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_TEQ:
  case ARM_OP_EOR:
    dest64=((i64) op1) ^ ((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF) ^ (((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_CMP:
  case ARM_OP_SUB:
    dest64=((i64) op1)+(~((i64) op2)+1);
    dest32=(((i64) op1) & 0xFFFFFFFF)+((~((i64) op2)+1) & 0xFFFFFFFF);
    break;
  case ARM_OP_RSB:
    dest64=((i64) op2)+(~((i64) op1)+1);
    dest32=(((i64) op2) & 0xFFFFFFFF)+((~((i64) op1)+1) & 0xFFFFFFFF);
    break;
  case ARM_OP_CMN:
  case ARM_OP_ADD:
    dest64=((i64) op1)+((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF)+(((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_ADC:
    dest64=((i64) op1)+((i64) op2)+
           ((i64) ((flags_in & ARM_CPSR_C)>>ARM_CPSR_Cs));
    dest32=(((i64) op1) & 0xFFFFFFFF)+(((i64) op2) & 0xFFFFFFFF)+
           (((i64) ((flags_in & ARM_CPSR_C)>>ARM_CPSR_Cs)) &
            0xFFFFFFFF);
    break;
  case ARM_OP_SBC:
    dest64=((i64) op1)+(~((i64) op2)+1)+
           ((i64) (((flags_in & ARM_CPSR_C)>>ARM_CPSR_Cs)+(~1+1)));
    dest32=(((i64) op1) & 0xFFFFFFFF)+((~((i64) op2)+1) & 0xFFFFFFFF)+
           (((i64) (((flags_in & ARM_CPSR_C)>>ARM_CPSR_Cs)+(~1+1))) &
            0xFFFFFFFF);
    break;
  case ARM_OP_RSC:
    dest64=((i64) op2)+(~((i64) op1)+1)+
           ((i64) (((flags_in & ARM_CPSR_C)>>ARM_CPSR_Cs)+(~1+1)));
    dest32=(((i64) op2) & 0xFFFFFFFF)+((~((i64) op1)+1) & 0xFFFFFFFF)+
           (((i64) (((flags_in & ARM_CPSR_C)>>ARM_CPSR_Cs)+(~1+1))) &
            0xFFFFFFFF);
    break;
  case ARM_OP_ORR:
    dest64=((i64) op1) | ((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF) | (((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_MOV:
    dest64=((i64) op2);
    dest32=(((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_BIC:
    dest64=((i64) op1) & ~((i64) op2);
    dest32=(((i64) op1) & 0xFFFFFFFF) & (~((i64) op2) & 0xFFFFFFFF);
    break;
  case ARM_OP_MVN:
    dest64=~((i64) op2);
    dest32=(~((i64) op2) & 0xFFFFFFFF);
  }

  printd(4,"Preparing to write back results\n",0);

  /* Writeback of the result and flag handling: */
  if ((op & 0xC)!=0x8)
  {
    (*instr).regs_val[4]=(i32f) dest64;
  }

  if ((op & ARM_OP_S)!=0)
  {
    switch (op & 0xFF)
    {
    case ARM_OP_SUB:
    case ARM_OP_RSB:
    case ARM_OP_ADD:
    case ARM_OP_ADC:
    case ARM_OP_SBC:
    case ARM_OP_RSC:
    case ARM_OP_CMP:
    case ARM_OP_CMN:
      flags_out=(flags_out & ~(ARM_CPSR_C | ARM_CPSR_V)) |
                ((dest32>>(32-ARM_CPSR_Cs)) & ARM_CPSR_C);

      switch (op & 0xFF)
      {
      case ARM_OP_ADD:
      case ARM_OP_ADC:
      case ARM_OP_CMN:
        if ((op1>>31 & 0x1)==(op2>>31 & 0x1) &&
            (op1>>31 & 0x1)!=(dest64>>31 & 0x1))
        {
          flags_out=flags_out | ARM_CPSR_V;
        }
        break;
      case ARM_OP_SUB:
      case ARM_OP_SBC:
      case ARM_OP_CMP:
        if ((op1>>31 & 0x1)!=(op2>>31 & 0x1) &&
            (op1>>31 & 0x1)!=(dest64>>31 & 0x1))
        {
          flags_out=flags_out | ARM_CPSR_V;
        }
        break;
      case ARM_OP_RSB:
      case ARM_OP_RSC:
        if ((op1>>31 & 0x1)!=(op2>>31 & 0x1) &&
            (op2>>31 & 0x1)!=(dest64>>31 & 0x1))
        {
          flags_out=flags_out | ARM_CPSR_V;
        }
      }
    case ARM_OP_AND:
    case ARM_OP_EOR:
    case ARM_OP_TST:
    case ARM_OP_TEQ:
    case ARM_OP_ORR:
    case ARM_OP_MOV:
    case ARM_OP_BIC:
    case ARM_OP_MVN:
      flags_out=(flags_out & ~(ARM_CPSR_N | ARM_CPSR_Z)) |
                ((dest64 & 0xFFFFFFFF)==0 ? ARM_CPSR_Z : 0) |
                ((dest64>>(63-ARM_CPSR_Ns)) & ARM_CPSR_N);
    }
  }

  printd(4,"Completed operation\n",0);

  (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED;
  return flags_out;
}



ui8 system_execute_Mul (struct system_instr *instr,i32f flags_in,
                        void *fu,char **output_buffer)
{
  i64 acc,op2,op1;
  i32f op,ops;

  op=(*instr).op;
  ops=(*instr).ops;


  if ((*instr).status==0)
  {
    /* MULs and MLAs take 1S + nI cycles:
       (unsigned) op2 <= &000000FF   n=1
       (unsigned) op2 <= &0000FFFF   n=2
       else                          n=3
    */
    op2=(*instr).regs_val[5] & 0xFFFFFFFF;

    if (op2<0x100)
    {
      (*instr).status=-3;
    }
    else if (op2<0x10000)
    {
      (*instr).status=-4;
    }
    else
    {
      (*instr).status=-5;
    }
  }


  if ((++((*instr).status))==-1)
  {
    op2=(*instr).regs_val[5];

    printd(4,"The intial op2 is %d (&",op2);
    printdc(4,"%X)\n",op2);

    if (((ops>>8) & 0xF)==15)
    {
      op2+=8;
    }

    op1=(*instr).regs_val[4];

    printd(4,"The intial op1 is %d (&",op1);
    printdc(4,"%X)\n",op1);

    if (((ops>>4) & 0xF)==15)
    {
      op1+=8;
    }

    op1=op1*op2;

    if ((op & 0xFF)==ARM_OP_MLA)
    {
      op2=(*instr).regs_val[6];

      printd(4,"The intial acc is %d (&",op2);
      printdc(4,"%X)\n",op2);

      if (((ops>>12) & 0xF)==15)
      {
        op2+=8;
      }

      op1=op1+op2;
    }

    (*instr).regs_val[4]=(i32f) op1;

    if ((op & ARM_OP_S)!=0)
    {
      flags_in=(flags_in & ~(ARM_CPSR_N | ARM_CPSR_Z)) |
                (op1==0 ? ARM_CPSR_Z : 0) |
                ((((i32f) op1)>>(31-ARM_CPSR_Ns)) & ARM_CPSR_N);
    }

    (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED;
    return flags_in;
  }
  else
  {
    *output_buffer+=oprintf(*output_buffer," (multi-cycle mul)");
  }

  return false;
}



ui8 system_execute_Branch (struct system_instr *instr,
                           i32f flags_in,void *fu,
                           char **output_buffer)
{
  i32f op,ops;

  op=(*instr).op;
  ops=(*instr).ops;

  if ((op & 0xFF)==ARM_OP_BL)
  {
    (*instr).regs_val[5]=(*instr).regs_val[4]+4;

    #ifdef ARM_RETURNPRED
    system_returnpred_push((*instr).regs_val[5]);
    #endif
  }

  (*instr).regs_val[4]+=ops;

  (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED;
  return flags_in;
}



ui8 system_execute_SingleMem (struct system_instr *instr,
                              i32f flags_in,void *fu,
                              char **output_buffer)
{
  i32f op,ops,op1,op2,op2shift,temp;
  struct system_lsu_request *request;

  op=(*instr).op;
  ops=(*instr).ops;


  if ((*instr).status==0)
  {
    /* temp[0] holds the base; temp[1] holds the base(op)index.
       Which one is used when is dependent on the writeback and
       indexing settings of the instruction.
    */

    switch (op & (0xF<<16))
    {
    case ARM_OP2_CONST:
      op2=(ops>>8) & 0xFFF;
      break;
    case ARM_OP2_REG:
      op2=(*instr).regs_val[6];
      break;
    case ARM_OP2_REG__RRX:
      op2=(*instr).regs_val[6];
      op2=((op2>>1) & 0x7FFFFFFF) |
          (flags_in & ARM_CPSR_C)<<(31-ARM_CPSR_Cs);
      break;
    default:
      op2=(*instr).regs_val[6];

      if ((op & (0xF<<16))<(6<<16))                 /* Shift by reg */
      {
        error_fatal("STR/LDR with an op2 with shift by reg (system_execute_SingleMem)",3);
      }
      else                                        /* Shift by const */
      {
        op2shift=(ops>>12) & 0x1F;

        if (op2shift==0)
        {
          op2shift=32;
        }
      }

      switch (op & (0xF<<16))
      {
      case ARM_OP2_REG__LSL_CONST:
        op2=op2<<op2shift;
        break;
      case ARM_OP2_REG__LSR_CONST:
        op2=(op2>>op2shift);
        op2=op2 & ((1<<(32-op2shift))-1);
        break;
      case ARM_OP2_REG__ASR_CONST:
      case ARM_OP2_REG__ASR_REG:
        temp=op2>>31 & 0x1;
        op2=op2>>op2shift;
        op2=op2 & ((1<<(32-op2shift))-1);
        op2=op2 | ~((temp<<(32-op2shift))-1);
        break;
      case ARM_OP2_REG__ROR_CONST:
      case ARM_OP2_REG__ROR_REG:
        op2=((op2>>op2shift) & ((1<<(32-op2shift))-1)) |
            (op2<<(32-op2shift));
      }
    }

    op1=(*instr).regs_val[5];

    if (((ops>>4) & 0xF)==15)
    {
      op1+=8;
    }

    (*instr).temp[0]=op1;
    if (((ops>>23) & 0x1)!=0)                       /* addr=op1+op2 */
    {
      (*instr).temp[1]=op1+op2;
    }
    else                                            /* addr=op1-op2 */
    {
      (*instr).temp[1]=op1-op2;
    }

    (*instr).status=1;

    *output_buffer+=oprintf(*output_buffer," (address generation)");

    return false;
  }

  if ((*instr).status==1)
  {
    if ((request=(struct system_lsu_request*)
                  processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(0)))!=0)
    {
      if ((*request).index==-1)
      {
        processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(0),0);
        mem_free(request);

        *output_buffer+=oprintf(*output_buffer," (preceding LSU request serviced)");
      }
      else
      {
        #ifdef ARM_ROB_QUICKMEM
        (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_WAITING;
        #endif

        *output_buffer+=oprintf(*output_buffer," (stalled on preceding LSU request)");

        return false;
      }
    }

    request=mem_alloc(sizeof(struct system_lsu_request));
    printd(128,"singlemem request at &%X\n",request);

    (*request).addr=(*instr).temp[(ops>>24) & 0x1];
    (*request).word=((ops>>22) & 0x1)==0 ? true : false;
    (*request).index=0;

    if ((op & 0xFF)==ARM_OP_LDR)
    {
      (*request).load=true;
    }
    else
    {
      (*request).load=false;
      temp=(*instr).regs_val[4];
      (*request).value=(ops & 0xF)==15 ? temp+12 : temp;
    }

    processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(0),request);

    (*instr).status=2;

    *output_buffer+=oprintf(*output_buffer," (requesting %s)",
                            (op & 0xFF)==ARM_OP_LDR ? "load" :
                            "store");

    if (((ops>>21) & 0x1)!=0)
    {
      (*instr).regs_val[5]=(*instr).temp[1];
      *output_buffer+=oprintf(*output_buffer," (writeback)");
    }

    if ((op & 0xFF)==ARM_OP_LDR)
    {
      return false;
    }
    else
    {
      /* If saving, do not bother to wait for the LSU to
         acknowledge the write.
      */
      (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED;
      return flags_in;
    }
  }

  if ((*instr).status==2)
  {
    if ((*(request=(struct system_lsu_request*)
                   processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(0)))).
        index==-1)
    {
      if ((op & 0xFF)==ARM_OP_LDR)
      {
        (*instr).regs_val[4]=(*request).value;
      }

      processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(0),0);
      mem_free(request);

      *output_buffer+=oprintf(*output_buffer," (LSU request serviced)");

      (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED;
      return flags_in;
    }
    else
    {
      *output_buffer+=oprintf(*output_buffer," (waiting for LSU to respond)");
    }
  }

  return false;
}



ui8 system_execute_MultiMem (struct system_instr *instr,
                             i32f flags_in,void *fu,
                             char **output_buffer)
{
  i32f op,ops,op1;
  struct system_lsu_request *request;
  i32 temp,count,unit_loop;
  ui8 ready_flag,request_status;

  op=(*instr).op;
  ops=(*instr).ops;

  printd(4,"Entering system_execute_MultiMem\n",0);

  count=((ops>>15) & 0x1)+((ops>>14) & 0x1)+((ops>>13) & 0x1)+
        ((ops>>12) & 0x1)+((ops>>11) & 0x1)+((ops>>10) & 0x1)+
        ((ops>>9) & 0x1)+((ops>>8) & 0x1)+((ops>>7) & 0x1)+
        ((ops>>6) & 0x1)+((ops>>5) & 0x1)+((ops>>4) & 0x1)+
        ((ops>>3) & 0x1)+((ops>>2) & 0x1)+((ops>>1) & 0x1)+
        ((ops>>0) & 0x1);

  if ((*instr).status==0)
  {
    /* temp[0] contains the first address for transfer; temp[1]
       contains the address to be written back to the pointer
       register, if writeback is set.
    */
    temp=count<<2;

    op1=(*instr).regs_val[4];

    if ((ops & (1<<23))==0)                       /* Decrementation */
    {
      op1-=temp;
      (*instr).temp[1]=op1;
      op1+=4-(ops>>(24-2)) & (0x1<<2);
    }
    else                                          /* Incrementation */
    {
      (*instr).temp[1]=op1+temp;
      op1+=(ops>>(24-2)) & (0x1<<2);
    }

    (*instr).temp[0]=op1;

    (*instr).status=2;

    *output_buffer+=oprintf(*output_buffer," (address generation)");
  }

  printd(4,"  Checking for pipe status\n",0);

  if ((*instr).status==2)
  {
    if (processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(0))==0)
    {
      printd(4,"    All pipes clear before the instruction\n",0);

      (*instr).status=16+count;
    }
    else
    {
      ready_flag=true;

      for (unit_loop=0;unit_loop<ARM_MULTIMEM_BANDWIDTH;unit_loop++)
      {
        if ((request=
             processsim_pipe_read(fu,
                                  SYSTEM_EXECUTE_LSU(unit_loop)))==0)
        {
          break;
        }

        if ((*request).index==-1)
        {
          processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(unit_loop),0);
          mem_free(request);

          *output_buffer+=oprintf(*output_buffer," (preceding LSU request serviced on pipe %d)",unit_loop);
        }
        else
        {
          ready_flag=false;

          *output_buffer+=oprintf(*output_buffer," (stalled on preceding LSU request on pipe %d)",unit_loop);
        }
      }

      if (ready_flag==true)
      {
        printd(4,"    All pipes not clear before the instruction now clear\n",0);

        (*instr).status=16+count;
      }
      else
      {
        #ifdef ARM_ROB_QUICKMEM
        (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_WAITING;
        #endif

        printd(4,"    Not all pipes clear\n",0);
      }
    }

    return false;
  }

  printd(4,"  Checking for request completion: value is %d\n",(*instr).status);

  if ((*instr).status>16)
  {
    ready_flag=true;

    for (unit_loop=0;unit_loop<ARM_MULTIMEM_BANDWIDTH;unit_loop++)
    {
      if ((request=
           processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(unit_loop)))==0)
      {
        break;
      }

      if ((*request).index!=-1)
      {
        ready_flag=false;
        break;
      }
    }

    if (ready_flag==true)
    {
      /* An old LSU request batch needs to be tidied up or there
         was no previous request batch.
      */

      printd(4,"    Ready for tidying up:\n",0);

      for (unit_loop=0;unit_loop<ARM_MULTIMEM_BANDWIDTH;unit_loop++)
      {
        printd(4,"      Loop val %d\n",(*instr).status);

        if ((request=
             processsim_pipe_read(fu,SYSTEM_EXECUTE_LSU(unit_loop)))==
            0)
        {
          printd(4,"        Is a zero pipe\n",0);

          break;
        }
        else
        {
          printd(4,"        Is a serviced, but unretired request\n",0);
        }

        if ((op & 0xFF)==ARM_OP_LDM)
        {
          (*instr).regs_val[16+count-(*instr).status+5]=(*request).
                                                        value;
        }

        processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(unit_loop),0);
        mem_free(request);

        *output_buffer+=
        oprintf(*output_buffer," (completed %s PR%d)",
                (op & 0xFF)==ARM_OP_LDM ? "load to" : "store from",
                (op & 0xFF)==ARM_OP_LDM ?
                (*instr).regs_wr[16+count-(*instr).status+5] :
                (*instr).regs_rd[16+count-(*instr).status+5]);

        (*instr).status--;
      }
    }
    else
    {
      /* Stall until it completes. */
      *output_buffer+=
      oprintf(*output_buffer," (stalled on %s PR%d)",
              (op & 0xFF)==ARM_OP_LDM ? "load to" : "store from",
              (op & 0xFF)==ARM_OP_LDM ?
              (*instr).regs_wr[16+count-(*instr).status+5] :
              (*instr).regs_rd[16+count-(*instr).status+5]);

      return false;
    }

    printd(4,"  Making requests: value is %d\n",(*instr).status);

    request_status=(*instr).status;

/*    *output_buffer+=oprintf(*output_buffer," (bandwidth of %d words)",ARM_MULTIMEM_BANDWIDTH);*/

    for (unit_loop=0;unit_loop<ARM_MULTIMEM_BANDWIDTH;unit_loop++)
    {
/*      *output_buffer+=oprintf(*output_buffer," (fetch loop %d)",unit_loop);*/

      printd(4,"    Making request for status value %d\n",request_status);

      if (request_status>16)
      {
        printd(4,"      This is a valid request (i.e.>16)\n",0);

        request=mem_alloc(sizeof(struct system_lsu_request));
        printd(128,"multimem request at &%X\n",request);

        (*request).addr=(*instr).temp[0];
        (*request).word=true;
        (*request).index=0;

        if ((op & 0xFF)==ARM_OP_LDM)
        {
          printd(4,"      a load\n",0);

          (*request).load=true;
        }
        else
        {
          printd(4,"      a store\n",0);

          (*request).load=false;
          temp=(*instr).regs_val[16+count-request_status+5];
          (*request).value=(request_status==17 &&
                            ((ops>>15) & 0x1)==0x1) ? temp+12 : temp;
        }

        printd(4,"      writing to pipe %d\n",unit_loop);

        processsim_pipe_write(fu,SYSTEM_EXECUTE_LSU(unit_loop),
                              request);

        (*instr).temp[0]+=4;

        *output_buffer+=
        oprintf(*output_buffer," (requesting %s PR%d)",
                (op & 0xFF)==ARM_OP_LDM ? "load to" : "store from",
                (op & 0xFF)==ARM_OP_LDM ?
                (*instr).regs_wr[16+count-request_status+5] :
                (*instr).regs_rd[16+count-request_status+5]);

        request_status--;
      }
      else
      {
        printd(4,"      Outside request status value bounds i.e complete.\n",0);

        if (unit_loop==0)
        {
          printd(4,"        One first pipe check i.e. fully complete\n",0);

          if (((ops>>21) & 0x1)!=0)
          {
            (*instr).regs_val[4]=(*instr).temp[1];
          }

          *output_buffer+=oprintf(*output_buffer," (writeback)");

          (*instr).rob_status=SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED;
          return flags_in;
        }

        break;
      }

/*      *output_buffer+=oprintf(*output_buffer," (out end fetch loop %d)",unit_loop);*/
    }
  }

  return false;
}



/* -------------------------------------------------------------------
   void system_retire

   The Functional Unit representing the ARM's in-order Retirement
   Unit.
------------------------------------------------------------------- */
void system_retire (void *fu)
{
  char *output_buffer;
  struct system_instr *instr;
  i32f op,ops,use_n,use_z,use_c,use_v,use_in,use_iz,use_ic,use_iv;
  i32 regs_loop,regs_index,unit_loop;

  output_buffer=processsim_output_read(fu);

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=4>Retirement Unit</TH>"
          "</TR>");

  printd(4,"\nEntered system_retire:\n",0);

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_RETIRE;unit_loop++)
  {
    printd(4,"\n  Pipe %d:\n",unit_loop);

    if ((instr=processsim_pipe_read(fu,
                                    SYSTEM_RETIRE_ROB(unit_loop)))!=0)
    {
      if (processsim_pipe_read(PROCESSSIM_FU_NULL,
                               SYSTEM_CU_BRANCH)!=0)
      {
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle colspan=4>"
                "Flush: Control Unit Branch line raised"
                "</TD></TR>");
      }
      else
      {

{char ob[512];
 *(ob+system_decode2_print(ob,instr))=0;
        printd(4,"    Retiring %s:\n",ob);
}

        op=(*instr).op;
        ops=(*instr).ops;

        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle>");

        output_buffer+=system_decode2_print(output_buffer,instr);


        if ((*instr).rob_status==
            SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED)
        {
          if ((op & 0xFF)<0x10)
          {
            if ((op & 0xC)!=0x8)
            {
              printd(4,"      Written back the DataProc dest to R%d\n",ops & 0xF);
              printd(4,"      Value was %d\n",(*instr).regs_val[4]);

              arm.r[ops & 0xF]=(*instr).regs_val[4];
            }

            if ((ops & 0xF)==15)
            {
              #ifdef ARM_RETURNPRED
              if ((op & 0xFF)==ARM_OP_MOV)
              {
                system_returnpred_r15dest(instr);
              }
              else
              {
                processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,
                                      (void*) 1);
                arm.r[15]-=4;
              }
              #else
              system_returnpred_r15dest(instr);
              #endif
            }
          }
          else
          {
            switch (op & 0xFF)
            {
            case ARM_OP_MLA:
            case ARM_OP_MUL:
              arm.r[ops & 0xF]=(*instr).regs_val[4];

              if ((ops & 0xF)==15)
              {
                processsim_pipe_write(PROCESSSIM_FU_NULL,
                                      SYSTEM_CU_BRANCH,(void*) 1);
                arm.r[15]-=4;
              }
              break;
            case ARM_OP_BL:
              arm.r[14]=(*instr).regs_val[5];
            case ARM_OP_B:
              #ifdef ARM_BRANCHPRED
              if (ARM_BRANCHPRED!=-1)
              {
                ui8 *branchpred_buffer_internal;

                branchpred_buffer_internal=
                system_branchpred_write(arm.r[15]);

                if (*branchpred_buffer_internal==0)
                {
                  *branchpred_buffer_internal=
                  ARM_BRANCHPRED_DYNAMIC_WEAKEST_TAKEN;
                }
                else
                {
                  (*branchpred_buffer_internal)++;

                  if (*branchpred_buffer_internal>
                      ARM_BRANCHPRED_DYNAMIC_STRONGEST_TAKEN)
                  {
                    *branchpred_buffer_internal=
                    ARM_BRANCHPRED_DYNAMIC_STRONGEST_TAKEN;
                  }
                }
              }

              (performance.branchpred_count)++;

              if ((*instr).branchpred_taken==false)
              {
                processsim_pipe_write(PROCESSSIM_FU_NULL,
                                      SYSTEM_CU_BRANCH,(void*) 1);

                output_buffer+=oprintf(output_buffer," (branch mispredicted)");

                (performance.branchpred_mispredict)++;
              }
              else
              {
                output_buffer+=oprintf(output_buffer," (branch predicted)");
              }
              #else
              processsim_pipe_write(PROCESSSIM_FU_NULL,
                                    SYSTEM_CU_BRANCH,(void*) 1);
              #endif

              arm.r[15]=(*instr).regs_val[4]-4;
              break;
            case ARM_OP_STR:
            case ARM_OP_LDR:
              if (((ops>>21) & 0x1)!=0)
              {
                arm.r[(ops>>4) & 0xF]=(*instr).regs_val[5];
              }

              if ((op & 0xFF)==ARM_OP_LDR)
              {
                arm.r[ops & 0xF]=(*instr).regs_val[4];

                if ((ops & 0xF)==15)
                {
                  system_returnpred_r15dest(instr);
                }
              }
              break;
            case ARM_OP_STM:
            case ARM_OP_LDM:
              if (((ops>>21) & 0x1)!=0)
              {
                arm.r[(ops>>16) & 0xF]=(*instr).regs_val[4];
              }

              if ((op & 0xFF)==ARM_OP_LDM)
              {
                regs_index=5;

                for (regs_loop=0;regs_loop<16;regs_loop++)
                {
                  if (((ops>>regs_loop) & 0x1)==0x1)
                  {
                    arm.r[regs_loop]=(*instr).regs_val[regs_index++];
                  }
                }

                if (((ops>>15) & 0x1)!=0)
                {
                  system_returnpred_r15dest(instr);
                }
              }
              break;
            case ARM_OP_MRS:
            case ARM_OP_MSR:
              error_fatal("Eek!",0);
            default:
              error_fatal("Attempted to retire an undefined instr",6);
            }
          }

          /* Update arm.cpsr. */
          use_n=(*instr).regs_wr[0]>>(31-ARM_CPSR_Ns);
          use_z=(*instr).regs_wr[1]>>(31-ARM_CPSR_Zs);
          use_c=(*instr).regs_wr[2]>>(31-ARM_CPSR_Cs);
          use_v=(*instr).regs_wr[3]>>(31-ARM_CPSR_Vs);

          arm.cpsr=(arm.cpsr & ((use_n & ARM_CPSR_N) |
                                (use_z & ARM_CPSR_Z) |
                                (use_c & ARM_CPSR_C) |
                                (use_v & ARM_CPSR_V) | 0x0FFFFFFF)) |
                   ((*instr).regs_val[0] & ~use_n) |
                   ((*instr).regs_val[1] & ~use_z) |
                   ((*instr).regs_val[2] & ~use_c) |
                   ((*instr).regs_val[3] & ~use_v);
        }
        else
        {
          #ifdef ARM_BRANCHPRED
          if ((op & 0xFF)==ARM_OP_B || (op & 0xFF)==ARM_OP_BL)
          {
            ui8 *branchpred_buffer_internal;

            if (ARM_BRANCHPRED!=-1)
            {
              branchpred_buffer_internal=
              system_branchpred_write(arm.r[15]);

              (*branchpred_buffer_internal)--;

              if (*branchpred_buffer_internal<
                  ARM_BRANCHPRED_DYNAMIC_STRONGEST_NOTTAKEN)
              {
                *branchpred_buffer_internal=
                ARM_BRANCHPRED_DYNAMIC_STRONGEST_NOTTAKEN;
              }
            }

            (performance.branchpred_count)++;

            if ((*instr).branchpred_taken==true)
            {
              processsim_pipe_write(PROCESSSIM_FU_NULL,
                                    SYSTEM_CU_BRANCH,(void*) 1);

              output_buffer+=oprintf(output_buffer," (branch mispredicted)");

              (performance.branchpred_mispredict)++;
            }
            else
            {
              output_buffer+=oprintf(output_buffer," (branch predicted)");
            }
          }
          #endif
        }

        arm.r[15]+=4;
        arm.pr[15]=arm.r[15];

        arm_instrexecuted++;
        (performance.instr_executed)++;

        output_buffer+=
        oprintf(output_buffer,
                "</TD>"
                "</TR>");
      }

      bitmaps_del((*instr).regs_bitmap_rd);
      bitmaps_del((*instr).regs_bitmap_wr);
      mem_free(instr);

      processsim_pipe_write(fu,SYSTEM_RETIRE_ROB(unit_loop),0);
    }
  }

  oprintf(output_buffer,"</TABLE>");
}



#endif
