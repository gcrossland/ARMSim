/* -------------------------------------------------------------------
   ARMSim V1.06                                        C COMPONENTS
   (c) Geoffrey Crossland 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "header.h"



/* -------------------------------------------------------------------
   ui8 system_branchpred_read

   Returns the value from the BPB entry for the given address, or
   0, if none has been allocated.
------------------------------------------------------------------- */
#ifdef ARM_BRANCHPRED
ui8 system_branchpred_read (i32f addr)
{
  i32f loop,*buffer_addrs_internal;
  ui8 buffer_element,*buffer_internal;

/* one ret */ ui8 val;

  switch (ARM_BRANCHPRED)
  {
  case -1:
    buffer_element=0;
    break;
  case 0:
     buffer_element=*(arm.branchpred_buffer+
                      (addr>>2 & (ARM_BRANCHPRED_ENTRIES-1)));

buffer_internal=(arm.branchpred_buffer+
(addr>>2 & (ARM_BRANCHPRED_ENTRIES-1)));
val=-1;

     break;
  default:
    loop=(addr>>2 & ((ARM_BRANCHPRED_ENTRIES/ARM_BRANCHPRED)-1))*
         ARM_BRANCHPRED;

    buffer_internal=arm.branchpred_buffer+loop;
    buffer_addrs_internal=arm.branchpred_buffer_addrs+loop;

    buffer_element=0;
val=-1;

    for (loop=0;loop<ARM_BRANCHPRED;loop++)
    {
      if (*buffer_addrs_internal==addr)
      {
        buffer_element=*buffer_internal;
        val=loop;
        loop=ARM_BRANCHPRED;

        buffer_internal--;
        buffer_addrs_internal--;
      }

      buffer_internal++;
      buffer_addrs_internal++;
    }

/*if (loop==ARM_BRANCHPRED)
ob+=oprintf(ob," (no BPB entry present)");*/

  }

/*  ob+=oprintf(ob," (accessing BPB for &amp;%X at &amp;%X (way index %d) for buffer element value &amp;%X)",addr,((i32f) buffer_internal)-((i32f) (arm.branchpred_buffer)),val,buffer_element);*/

  return buffer_element;
}
#endif



/* -------------------------------------------------------------------
   ui8 *system_branchpred_write

   Returns a pointer to the BPB entry for the given address,
   allocating one if none is present.
------------------------------------------------------------------- */
#ifdef ARM_BRANCHPRED
ui8 *system_branchpred_write (i32f addr)
{
  ui8 *buffer_internal,*buffer_internal_end;
  i32f index,temp,*buffer_addrs_internal;

  if (ARM_BRANCHPRED==0)
  {
    buffer_internal=arm.branchpred_buffer+
                    ((addr>>2) & (ARM_BRANCHPRED_ENTRIES-1));
  }
  else
  {
    index=(addr>>2 & ((ARM_BRANCHPRED_ENTRIES/ARM_BRANCHPRED)-1))*
          ARM_BRANCHPRED;

    buffer_internal=arm.branchpred_buffer+index;
    buffer_internal_end=buffer_internal+ARM_BRANCHPRED;
    buffer_addrs_internal=arm.branchpred_buffer_addrs+index;

    while (buffer_internal<buffer_internal_end &&
           *buffer_addrs_internal!=addr)
    {
      buffer_internal++;
      buffer_addrs_internal++;
    }

    if (buffer_internal==buffer_internal_end)
    {
      buffer_internal=arm.branchpred_buffer+index;
      buffer_addrs_internal=arm.branchpred_buffer_addrs+index;

      while (buffer_internal<buffer_internal_end &&
             *buffer_addrs_internal!=0)
      {
        buffer_internal++;
        buffer_addrs_internal++;
      }

      if (buffer_internal==buffer_internal_end)
      {
/*        ob+=oprintf(ob," (BPB entry reallocated for this request)");*/

        buffer_internal=arm.branchpred_buffer+index;
        buffer_addrs_internal=arm.branchpred_buffer_addrs+index;

        #ifdef ARM_BRANCHPRED_WAYASSOC_RANDOM
        buffer_internal+=(temp=processsim_main_clocks() &
                               (ARM_BRANCHPRED-1));
        buffer_addrs_internal+=temp;
        #endif

        #ifdef ARM_BRANCHPRED_WAYASSOC_LRU
        /* The higher the way value, the more recently used. */
        for (temp=0;temp<(ARM_BRANCHPRED-1);temp++)
        {
          *(buffer_internal+temp)=*(buffer_internal+temp+1);
          *(buffer_addrs_internal+temp)=*(buffer_addrs_internal+
                                          temp+1);
        }

        buffer_internal+=ARM_BRANCHPRED-1;
        buffer_addrs_internal+=ARM_BRANCHPRED-1;
        #endif
      }
/*      else
      {
        ob+=oprintf(ob," (BPB entry allocated from an unused element)");
      }*/

      *buffer_addrs_internal=addr;
      *buffer_internal=0;
    }
/*    else
    {
      ob+=oprintf(ob," (BPB entry already present)");
    }*/
  }

/*  ob+=oprintf(ob," (accessing BPB for &amp;%X at &amp;%X for buffer element value &amp;%X)",addr,((i32f) buffer_internal)-((i32f) (arm.branchpred_buffer)),*buffer_internal);*/

  return buffer_internal;
}
#endif
