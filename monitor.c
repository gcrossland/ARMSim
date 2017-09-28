/* -------------------------------------------------------------------
   ARMSim V1.06                                        C COMPONENTS
   (c) Geoffrey Crossland 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "header.h"


i32 arm_instrexecuted=0;
#ifdef ARM_TRACE_CHECK
fileh arm_trace_file=0;
#endif
struct system_performance performance;



/* -------------------------------------------------------------------
   void arm_trace

   Checks the current state of the ARM against a trace file.
------------------------------------------------------------------- */
#ifdef ARM_TRACE_CHECK
void arm_trace (i32 instrexecuted)
{
  char buffer[1024];
  i32f trace_values[16];
  i32 trace_loop,trace_fail;
  FILE *file_handle;

  if (instrexecuted==0)
  {
    return;
  }


  if (arm_trace_file==0)
  {
    arm_trace_file=file_open_old(ARM_TRACE_CHECK);

    for (trace_loop=0;trace_loop<16*(1+ARM_HANDICAP);trace_loop++)
    {
      file_get_i32(arm_trace_file);
    }
  }

  while ((instrexecuted--)>1)
  {
    if (feof((FILE*) arm_trace_file)!=0)
    {
/*      processsim_main_output();*/
      sprintf(buffer,"End of Source Trace on instr %d",performance.instr_executed);
      error_nonfatal(buffer,ERROR_RESULT_ok);
      error_exit();
    }

    for (trace_loop=0;trace_loop<16;trace_loop++)
    {
      file_get_i32(arm_trace_file);
    }
  }

  if (feof((FILE*) arm_trace_file)!=0)
  {
/*    processsim_main_output();*/
    sprintf(buffer,"End of Source Trace on instr %d/cycle %d\n",performance.instr_executed,processsim_main_clocks());
    error_nonfatal(buffer,ERROR_RESULT_ok);
    error_exit();
  }

  trace_fail=-1;

  for (trace_loop=0;trace_loop<16;trace_loop++)
  {
    if ((trace_values[trace_loop]=file_get_i32(arm_trace_file))
        !=arm.r[trace_loop] && trace_fail==-1)
    {
      trace_fail=trace_loop;
    }
  }

  printd(1,"Trace  R15=&%X\n",trace_values[15]);
  printd(1,"ARMSim R15=&%X\n",arm.r[15]);

  if (trace_fail!=-1)
  {
/*    processsim_main_output();*/
    sprintf(buffer,"Mismatched Execution on instr %d/cycle %d on reg index %d with PC &%X (trace quotes %d/&%X, ARMSim produces %d/&%X)",performance.instr_executed,processsim_main_clocks(),trace_fail,arm.r[15],trace_values[trace_fail],trace_values[trace_fail],arm.r[trace_fail],arm.r[trace_fail]);
    error_nonfatal(buffer,ERROR_RESULT_ok);

/*    sprintf(buffer,"armsim.txt");
    file_handle=fopen(buffer,"w");
    fprintf(file_handle,"Details of execution mismatch\n\n");
    fprintf(file_handle,"Reg\tTrace\tARMSim\n");

    for (trace_loop=0;trace_loop<16;trace_loop++)
    {
      fprintf(file_handle,"R%d\t&%X\t&%X\n",trace_loop,
              trace_values[trace_loop],arm.r[trace_loop]);
    }

    fclose(file_handle);*/

    sprintf(buffer,"armsim.htm");
    file_handle=fopen(buffer,"w");
    fprintf(file_handle,"<HTML><HEAD><TITLE>"
                        "Details of ARMSIm Execution Mismatch"
                        "</TITLE></HEAD><BODY>"
                        "Mismatched Execution on instr %d/cycle %d with PC &amp;%X"
                        "<BR>"
                        "<TABLE border=1>"
                        "<TR>"
                        "<TH>ARM Register</TH>"
                        "<TH>Trace Value</TH>"
                        "<TH>ARMSim Value</TH>"
                        "</TR>",performance.instr_executed,
            processsim_main_clocks(),arm.r[15]);

    for (trace_loop=0;trace_loop<16;trace_loop++)
    {
      fprintf(file_handle,"<TR>"
                          "<TD>R%d</TD>"
                          "<TD>&amp;%X</TD>"
                          "<TD>&amp;%X</TD>"
                          "</TR>",trace_loop,trace_values[trace_loop],
              arm.r[trace_loop]);
    }

    fprintf(file_handle,"</TABLE></BODY></HTML>");

    fclose(file_handle);

    error_exit();
  }
}
#endif



/* -------------------------------------------------------------------
   void system_cu

   The Functional Unit representing the ARM's Control Unit.
------------------------------------------------------------------- */
void system_cu (void *fu)
{
  double cycle;
  char *output_buffer=processsim_output_read(fu);
  ui8 loop;

  cycle=((double) processsim_main_clocks())+0.0001;


  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=6>Control Unit</TH>"
          "</TR>");

  if (processsim_pipe_read(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH)!=0)
  {
    processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,0);

    output_buffer+=
    oprintf(output_buffer,
            "<TR><TD align=left valign=middle colspan=6>"
            "Control Unit Branch line raised"
            "</TD></TR>");
  }

  for (loop=0;loop<8;loop++)
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle width=10%%>R%d</TD>"
            "<TD align=left valign=middle width=20%%>%d</TD>"
            "<TD align=left valign=middle width=20%%>&amp;%08X</TD>"
            "<TD align=left valign=middle width=10%%>R%d</TD>"
            "<TD align=left valign=middle width=20%%>%d</TD>"
            "<TD align=left valign=middle width=20%%>&amp;%08X</TD>"
            "</TR>",
            loop,arm.r[loop],arm.r[loop],loop+8,arm.r[loop+8],
            arm.r[loop+8]);
  }

  output_buffer+=
  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle colspan=3>CPSR</TD>"
          "<TD align=left valign=middle colspan=3>&amp;%08X</TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle width=10%%>N</TD>"
          "<TD align=left valign=middle colspan=2>%d</TD>"
          "<TD align=left valign=middle width=10%%>C</TD>"
          "<TD align=left valign=middle colspan=2>%d</TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle width=10%%>Z</TD>"
          "<TD align=left valign=middle colspan=2>%d</TD>"
          "<TD align=left valign=middle width=10%%>V</TD>"
          "<TD align=left valign=middle colspan=2>%d</TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle colspan=6><EM>Overall Performance</EM></TD>"
          "</TR>"
          "<TR>"
          "<TD align=left valign=middle colspan=2>IPC:</TD>"
          "<TD align=left valign=middle colspan=4>%.3f instrs/cycle</TD>"
          "</TR>",
          arm.cpsr,(arm.cpsr>>ARM_CPSR_Ns) & 0x1,
          (arm.cpsr>>ARM_CPSR_Cs) & 0x1,
          (arm.cpsr>>ARM_CPSR_Zs) & 0x1,
          (arm.cpsr>>ARM_CPSR_Vs) & 0x1,
          ((double) performance.instr_executed)/cycle);

/*
  output_buffer+=
  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle colspan=2>Clock Speed:</TD>"
          "<TD align=left valign=middle colspan=4>%.9f MHz</TD>"
          "</TR>",(double) (cycle/(processsim_main_secs()*
                                   ((double) 1000000))));
*/

  #ifdef ARM_BRANCHPRED
  output_buffer+=
  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle colspan=2>Branch Mispredictions:</TD>"
          "<TD align=left valign=middle colspan=4>%.2f</TD>"
          "</TR>",performance.branchpred_count==0 ? ((double) 0) :
                  ((double) (((f32) performance.
                                    branchpred_mispredict)/
                             ((f32) performance.branchpred_count))));

  if (ARM_BRANCHPRED!=-1)
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=2>Prediction via BPB:</TD>"
            "<TD align=left valign=middle colspan=4>%.2f</TD>"
            "</TR>",performance.branchpred_count==0 ? ((double) 0) :
                    ((double) (((f32) performance.branchpred_dynamic)/
                               ((f32) performance.
                                      branchpred_count))));
  }
  #endif

  #ifdef ARM_RETURNPRED
  output_buffer+=
  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle colspan=2>Return Mispredictions:</TD>"
          "<TD align=left valign=middle colspan=4>%.2f</TD>"
          "</TR>",performance.returnpred_count==0 ? ((double) 0) :
                  ((double) (((f32) performance.
                                    returnpred_mispredict)/
                             ((f32) performance.returnpred_count))));
  #endif

  oprintf(output_buffer,"</TABLE>");
}



/* -------------------------------------------------------------------
   void system_mem

   The Functional Unit representing the ARM's Memory (used for
   display purposes only).
------------------------------------------------------------------- */
void system_mem (void *fu)
{
  struct system_lsu_state *state;
  char *output_buffer;
  i32f loop,word;
  ui8f **mem_array;

  state=processsim_workspace_read(fu);
  output_buffer=processsim_output_read(fu);

  mem_array=(*state).mem_array;

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=3>Memory</TH>"
          "</TR>");

  for (loop=SYSTEM_CU_MEM_START;loop<SYSTEM_CU_MEM_START+
                                     (SYSTEM_CU_MEM_WORDS<<2);loop+=4)
  {
    word=system_lsu_access(mem_array,loop,0,true,true);

    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle>&amp;%08X</TD>"
            "<TD align=left valign=middle>%d</TD>"
            "<TD align=left valign=middle>&amp;%08X</TD>"
            "</TR>",
            loop,word,word);
  }

  output_buffer+=oprintf(output_buffer,"</TABLE>");
}



/* -------------------------------------------------------------------
   void system_focpred

   The Functional Unit representing the ARM's Flow of Control
   Prediction Unit i.e. that virtual hardware which handles the BPB
   and the RPB.
------------------------------------------------------------------- */
void system_focpred (void *fu)
{
  char *output_buffer=processsim_output_read(fu);
  i32 entry_loop,way_loop;
  i32f *buffer_addrs_internal;
  ui8 *buffer_internal;

/*if (processsim_main_clocks()>0x48968){
printd(50,"Entering focpred on cycle %d:\n",processsim_main_clocks());
  processsim_main_print();
}*/

  output_buffer+=
  oprintf(output_buffer,
          "<TABLE border=1 width=100%%>"
          "<TR>"
          "<TH align=center valign=middle colspan=4>Flow of Control Prediction Unit</TH>"
          "</TR>");

  #ifdef ARM_BRANCHPRED
  output_buffer+=
  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle colspan=4><EM>Branch Prediction Buffer</EM></TD>"
          "</TR>"
          "<TR>"
          "<TH align=left valign=middle>Entry</TD>"
          "<TH align=left valign=middle>Way</TD>"
          "<TH align=left valign=middle>Status</TD>"
          "<TH align=left valign=middle>addr</TD>"
          "</TR>");

/*if (processsim_main_clocks()>0x48968)
  processsim_main_print();*/

  if (ARM_BRANCHPRED==0)
  {
    for (entry_loop=0;entry_loop<ARM_BRANCHPRED_ENTRIES;entry_loop++)
    {
      output_buffer+=
      oprintf(output_buffer,
              "<TR>"
              "<TD align=left valign=middle colspan=2>Entry &amp;%X (%d)</TD>"
              "<TD align=left valign=middle colspan=2>%d</TD>"
              "</TR>",entry_loop,entry_loop,*(arm.branchpred_buffer+
                                              entry_loop));
    }
  }
  else
  {
    buffer_internal=arm.branchpred_buffer;
    buffer_addrs_internal=arm.branchpred_buffer_addrs;

    for (entry_loop=0;entry_loop<(ARM_BRANCHPRED_ENTRIES/
                                  ARM_BRANCHPRED);entry_loop++)
    {
      for (way_loop=0;way_loop<ARM_BRANCHPRED;way_loop++)
      {
        output_buffer+=
        oprintf(output_buffer,
                "<TR>"
                "<TD align=left valign=middle>&amp;%X</TD>"
                "<TD align=left valign=middle>&amp;%X</TD>"
                "<TD align=left valign=middle>%d</TD>"
                "<TD align=left valign=middle>&amp;%X</TD>"
                "</TR>",entry_loop,way_loop,*buffer_internal,
                *buffer_addrs_internal);

        buffer_internal++;
        buffer_addrs_internal++;
      }
    }
  }
  #endif

  #ifdef ARM_RETURNPRED
  output_buffer+=
  oprintf(output_buffer,
          "<TR>"
          "<TD align=left valign=middle colspan=4><EM>Return Prediction Buffer</EM></TD>"
          "</TR>");

  buffer_internal=arm.returnpred_buffer+ARM_RETURNPRED-1;

  for (entry_loop=ARM_RETURNPRED;entry_loop>0;entry_loop--)
  {
    output_buffer+=
    oprintf(output_buffer,
            "<TR>"
            "<TD align=left valign=middle colspan=4>&amp;%X</TD>"
            "</TR>",*buffer_internal);

    buffer_internal--;
  }
  #endif

/*if (processsim_main_clocks()>0x48968)
  processsim_main_print();*/

  oprintf(output_buffer,"</TABLE>");
}
