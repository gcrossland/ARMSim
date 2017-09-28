/* -------------------------------------------------------------------
   ARMSim V1.06                                        C COMPONENTS
   (c) Geoffrey Crossland 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "header.h"



/* -------------------------------------------------------------------
   void system_returnpred_push

   Pushes a return address onto the RPB.
------------------------------------------------------------------- */
#ifdef ARM_RETURNPRED
void system_returnpred_push (i32f addr)
{
  i32 loop;
  i32f *buffer_internal;

  buffer_internal=arm.returnpred_buffer;

  for (loop=ARM_RETURNPRED-1;loop>0;loop--)
  {
    *buffer_internal=*(buffer_internal+1);
    buffer_internal++;
  }

  *buffer_internal=addr;

/*  ob+=oprintf(ob," (RPB push)");*/
}
#endif



/* -------------------------------------------------------------------
   i32f system_returnpred_pull

   Retrieves the most recent return address on the RPB and removes
   it.
------------------------------------------------------------------- */
#ifdef ARM_RETURNPRED
i32f system_returnpred_pull (void)
{
  i32 loop;
  i32f *buffer_internal,addr;

  buffer_internal=arm.returnpred_buffer+ARM_RETURNPRED-1;
  addr=*buffer_internal;

  for (loop=ARM_RETURNPRED-1;loop>0;loop--)
  {
    *buffer_internal=*(buffer_internal-1);
    buffer_internal--;
  }

  *buffer_internal=0;

/*  ob+=oprintf(ob," (RPB pull)");*/

  return addr;
}
#endif



/* -------------------------------------------------------------------
   void system_returnpred_flush

   Flushes the RPB.
------------------------------------------------------------------- */
#ifdef ARM_RETURNPRED
void system_returnpred_flush (void)
{
  i32f *buffer_internal,*buffer_internal_end;

  buffer_internal_end=(buffer_internal=arm.returnpred_buffer)+
                      ARM_RETURNPRED;

  do
  {
    *(buffer_internal++)=0;
  }
  while (buffer_internal<buffer_internal_end);
}
#endif



/* -------------------------------------------------------------------
   void system_returnpred_r15dest

   Manages writes to R15 i.e. flushes the pipeline, if necessary,
   and manages the RPB. if return prediction is active. R15 is
   decremented, so that the post-instruction R15 incrementaton is
   negated.
------------------------------------------------------------------- */
void system_returnpred_r15dest (struct system_instr *instr)
{
  #ifdef ARM_RETURNPRED
  performance.returnpred_count++;

  if (arm.r[15]!=(*instr).returnpred_addr)
  {
    performance.returnpred_mispredict++;
    system_returnpred_flush();

    processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,(void*)
                                                              1);
  }

/*  if ((*instr).returnpred_addr==-1)
  {
    error_fatal("Bad choice of unconditional returns",0);
  }*/
  #else
  processsim_pipe_write(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,(void*)
                                                            1);
  #endif

  arm.r[15]-=4;
}
