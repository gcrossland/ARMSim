/* -------------------------------------------------------------------
   ARMSim V1.06                                        C COMPONENTS
   (c) Geoffrey Crossland 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "header.h"


#define OUTPUT_SIZE 40960


char *options_output_dirname=NULL;  /* name by their switches? */
char *options_rom_filename=NULL;
i32 options_stepsecs=-1;
i32 options_output_start=-1;
#ifndef ARM_TRACE_CHECK
i32 options_output_end=-1;
#endif
struct system_arm arm;



/* -------------------------------------------------------------------
   int main
------------------------------------------------------------------- */
int main (int argc,char *argv[])
{
  ui8 options_notehelp=false;
  chr options_switch;
  i32 options_temp;
  #ifdef ARM_BRANCHPRED
  ui8 *branchpred_internal,*branchpred_end;
  i32f *branchpred_addrs_internal;
  #endif
  i32 bandwidth_loop;
  void *fu_lsu,*fu_fetch,*fu_cu,*fu_mem,*fu_focpred;
  #ifndef ARM_SUPERSCALAR_SHORT
  void *fu_decode;
  #endif
  #ifdef ARM_SUPERSCALAR
  void *fu_decode2,*fu_rob,
       *fu_execute_dataproc[ARM_SUPERSCALAR_DATAPROC],
       *fu_execute_mul[ARM_SUPERSCALAR_MUL],
       *fu_execute_singlemem[ARM_SUPERSCALAR_SINGLEMEM],
       *fu_execute_multimem[ARM_SUPERSCALAR_MULTIMEM],
       *fu_execute_branch[ARM_SUPERSCALAR_BRANCH],
       *fu_execute_psrtrans[ARM_SUPERSCALAR_PSRTRANS],*fu_retire;
  i32 unit_loop;
  #else
  void *fu_execute;
  #endif
  struct system_lsu_state *lsu_state;
  struct system_fetch_state *fetch_state;
  #ifdef ARM_SUPERSCALAR
  struct system_instr **fetches_internal,**fetches_internal_end;
  struct system_decode2_state *decode2_state;
  struct system_rob_state *rob_state;
  struct system_instr **rob_internal,**rob_internal_end;
  #endif
  struct system_execute_state *execute_state;
  i32 rom_file_size,rom_working_size;
  ui8f *rom_cache,*rom_cache_internal,*rom_cache_end,
       **mem_array_internal,**mem_array_end;

//  win32_task_handle=(HANDLE) hInstance;

  setvbuf(stdin,0,_IONBF,0);
  setvbuf(stdout,0,_IONBF,0);
  setvbuf(stderr,0,_IONBF,0);

  debug_init();
  debug_newstream(0,NULL);
  debug_newstream(1,"debug_stream");
  debug_newstream(50,"processsim_stream");
  debug_newstream(60,"bitmaps_stream");
  debug_newstream(4,"system_stream_verbose");
  debug_newstream(5,"system_stream_overview");
  debug_newstream(128,"mem_trace_stream");
  #ifdef WIN32A
  trapmanager_init();
  mem_lazy_init();
  #endif


  /* Process command line parameters. */
  argv++;
  argc--;

  if (argc<1)
  {
    printf("Syntax: armsim [options...]\n"
           "See \"armsim -h\" for help\n");
    return true;
  }

  while ((argc--)>0)
  {
    if (**argv=='-')
    {
      switch (options_switch=(chr) *(*argv+1))
      {
      case 'v':
      case 'h':
        printf("ARMSim v1.06 ("
               #ifdef WIN32A
               "Win32"
               #else
               "ANSI C"
               #endif
               " edition) "
               #ifdef ARM_TRACE_CHECK
               "(validating against '"
               ARM_TRACE_CHECK
               "') "
               #endif
               "(c) Geoff Crossland 2002\n");

        if (options_switch=='h')
        {
          printf("Available from http://www-users.york.ac.uk/~gegc100/\n"
                 "\n"
                 "Syntax: armsim [options...]\n"
                 "\n"
                 "Options: -v        prints version information and exits\n"
                 "         -h        prints syntax and options details and\n"
                 "                   exits\n"
                 "         -r file   specifies ROM file (loaded at &00000000)\n"
                 "         -d secs   specifies the delay, in seconds, between\n"
                 "                   stepping output files (default is 1)\n"
                 "         -s cycle  start producing output files on a given\n"
                 "                   cycle (default is 0)\n"
                 #ifndef ARM_TRACE_CHECK
                 "         -e cycle  stop simulating immediately before a\n"
                 "                   given cycle (default is not to stop)\n"
                 #endif
                 "         out_dir   output file destination directory\n"
                 "                   (default is 'output')\n"
                 "\n"
                 "ARMSim is a simulation of a superscalar, out-of-order execution\n"
                 "engine for the ARM ISA, taking a ROM file and producing cycle-\n"
                 "by-cycle telemetry in the directory given on the command line.\n");
        }

        return true;
      case 'r':
        argv++;
        argc--;

        if (argc<0)
        {
          printf("-r switch incomplete (filename required)\n"
                 "See \"armsim -h\" for help\n");
          return true;
        }

        options_rom_filename=*argv;
        break;
      case 'd':
      case 's':
      #ifndef ARM_TRACE_CHECK
      case 'e':
      #endif
        argv++;
        argc--;

        if (argc<0)
        {
          printf("-%c switch incomplete (",(char) options_switch);

          switch (options_switch)
          {
          case 'd':
            printf("integer time in seconds required)\n");
            break;
          case 's':
          #ifndef ARM_TRACE_CHECK
          case 'e':
          #endif
            printf("integer required)\n");
          }

          printf("See \"armsim -h\" for help\n");
          return true;
        }

        if ((options_temp=main_parseint(*argv))==-1)
        {
          printf("-%c switch invalid (could not parse value as an integer)\n"
                 "See \"armsim -h\" for help\n",
                 (char) options_switch);
          return true;
        }

        switch (options_switch)
        {
        case 'd':
          options_stepsecs=options_temp;
          break;
        case 's':
          options_output_start=options_temp;
        #ifndef ARM_TRACE_CHECK
          break;
        case 'e':
          options_output_end=options_temp;
        #endif
        }
        break;
      default:
/*        printf("Unknown switch '%s'\n"
               "See \"armsim -h\" for help\n",(*argv+1));*/

        if (options_output_dirname==NULL)
        {
          options_output_dirname=*argv;
        }
        else
        {
          #ifdef ARM_TRACE_CHECK
          if (options_switch=='e')
          {
            printf("output directory specified as '%s' and then as '%s'\n"
                   "(note that -e switch not available in validating builds)\n"
                   "See \"armsim -h\" for help\n",
                   options_output_dirname,*argv);
          }
          else
          #endif
          {
            printf("output directory specified as '%s' and then as '%s'\n"
                   "See \"armsim -h\" for help\n",
                   options_output_dirname,*argv);
          }

          return true;
        }
      }
    }
    else
    {
      if (options_output_dirname==NULL)
      {
        options_output_dirname=*argv;
      }
      else
      {
        printf("output directory specified as '%s' and then as '%s'\n"
               "See \"armsim -h\" for help\n",
               options_output_dirname,*argv);
        return true;
      }
    }

    argv++;
  }

  if (options_rom_filename==NULL)
  {
    printf("No ROM file specified\n"
           "See \"armsim -h\" for help\n");
    return true;
  }

  if (options_output_dirname==NULL)
  {
    options_output_dirname="Output";
  }

  if (options_stepsecs==-1)
  {
    options_stepsecs=1;
  }

  if (options_output_start==-1)
  {
    options_output_start=0;
  }

  #ifndef ARM_TRACE_CHECK
  if (options_output_end==-1)
  {
    options_output_end=0x7FFFFFFF;
  }
  #endif

  if (options_notehelp)
  {
    printf("See \"armsim -h\" for help\n");
  }


  /* Establish the system: generate the ARM registers, functional
     units, pipes and the functional units' initial workspaces.
  */
  arm.r[0]=arm.r[1]=arm.r[2]=arm.r[3]=
  arm.r[4]=arm.r[5]=arm.r[6]=arm.r[7]=
  arm.r[8]=arm.r[9]=arm.r[10]=arm.r[11]=
  arm.r[12]=arm.r[13]=arm.r[14]=0;
  arm.r[15]=ARM_HANDICAP<<2;
  arm.cpsr=0x10;
  #ifdef ARM_SUPERSCALAR
  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_REGS;unit_loop++)
  {
    arm.pr[unit_loop]=0;
  }
  #endif

  performance.instr_executed=0;
  #ifdef ARM_BRANCHPRED
  performance.branchpred_count=performance.branchpred_mispredict=
  performance.branchpred_dynamic=0;

  if (ARM_BRANCHPRED==-1)
  {
    arm.branchpred_buffer=0;
  }
  else
  {
    branchpred_end=(branchpred_internal=
                    arm.branchpred_buffer=
                    mem_alloc(sizeof(ui8)*ARM_BRANCHPRED_ENTRIES))+
                    ARM_BRANCHPRED_ENTRIES;

    if (ARM_BRANCHPRED>0)
    {
      branchpred_addrs_internal=arm.branchpred_buffer_addrs=
      mem_alloc(sizeof(i32f)*ARM_BRANCHPRED_ENTRIES);

      do
      {
        *((i32f*) branchpred_internal)=0;
        branchpred_internal+=4;

        *(branchpred_addrs_internal+0)=0;
        *(branchpred_addrs_internal+1)=0;
        *(branchpred_addrs_internal+2)=0;
        *(branchpred_addrs_internal+3)=0;
        branchpred_addrs_internal+=4;
      }
      while (branchpred_internal<branchpred_end);
    }
    else
    {
      do
      {
        *((i32f*) branchpred_internal)=0;
        branchpred_internal+=4;
      }
      while (branchpred_internal<branchpred_end);
    }
  }
  #endif

  #ifdef ARM_RETURNPRED
  branchpred_end=(branchpred_internal=
                  arm.returnpred_buffer=
                  mem_alloc(sizeof(ui8)*ARM_RETURNPRED))+
                  ARM_RETURNPRED;
  do
  {
    *((i32f*) branchpred_internal)=0;
    branchpred_internal+=4;
  }
  while (branchpred_internal<branchpred_end);
  #endif

  processsim_init(1,options_output_dirname,options_stepsecs);

  fu_cu=processsim_fu_gen(&system_cu,0,OUTPUT_SIZE,SYSTEM_CU_X,
                          SYSTEM_CU_Y);
  fu_mem=processsim_fu_gen(&system_mem,0,OUTPUT_SIZE,SYSTEM_MEM_X,
                           SYSTEM_MEM_Y);
  #ifdef ARM_BRANCHPRED
  #define ARM_TEMP
  #endif
  #ifdef ARM_RETURNPRED
  #define ARM_TEMP
  #endif
  #ifdef ARM_TEMP
  fu_focpred=processsim_fu_gen(&system_focpred,0,OUTPUT_SIZE,
                               SYSTEM_FOCPRED_X,SYSTEM_FOCPRED_Y);
  #endif
  #undef ARM_TEMP
  fu_lsu=processsim_fu_gen(&system_lsu,SYSTEM_LSU_count,
                           OUTPUT_SIZE,SYSTEM_LSU_X,SYSTEM_LSU_Y);
  fu_fetch=processsim_fu_gen(&system_fetch,SYSTEM_FETCH_count,
                             OUTPUT_SIZE,SYSTEM_FETCH_X,SYSTEM_FETCH_Y);
  #ifndef ARM_SUPERSCALAR_SHORT
  fu_decode=processsim_fu_gen(&system_decode,SYSTEM_DECODE_count,
                              OUTPUT_SIZE,SYSTEM_DECODE_X,SYSTEM_DECODE_Y);
  #endif
  processsim_pipe_gen(PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH,
                      PROCESSSIM_FU_NULL,SYSTEM_CU_BRANCH);

  #ifdef ARM_SUPERSCALAR
  fu_decode2=processsim_fu_gen(&system_decode2,SYSTEM_DECODE2_count,
                               OUTPUT_SIZE,SYSTEM_DECODE2_X,
                               SYSTEM_DECODE2_Y);

  fu_rob=processsim_fu_gen(&system_rob,SYSTEM_ROB_count,OUTPUT_SIZE,
                           SYSTEM_ROB_X,SYSTEM_ROB_Y);

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_DATAPROC;unit_loop++)
  {
    fu_execute_dataproc[unit_loop]=
    processsim_fu_gen(&system_execute,SYSTEM_EXECUTE_count,OUTPUT_SIZE,
                      SYSTEM_DATAPROC_X(unit_loop),
                      SYSTEM_DATAPROC_Y(unit_loop));
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_MUL;unit_loop++)
  {
    fu_execute_mul[unit_loop]=
    processsim_fu_gen(&system_execute,SYSTEM_EXECUTE_count,OUTPUT_SIZE,
                      SYSTEM_MUL_X(unit_loop),
                      SYSTEM_MUL_Y(unit_loop));
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_SINGLEMEM;unit_loop++)
  {
    fu_execute_singlemem[unit_loop]=
    processsim_fu_gen(&system_execute,SYSTEM_EXECUTE_count,OUTPUT_SIZE,
                      SYSTEM_SINGLEMEM_X(unit_loop),
                      SYSTEM_SINGLEMEM_Y(unit_loop));
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_MULTIMEM;unit_loop++)
  {
    fu_execute_multimem[unit_loop]=
    processsim_fu_gen(&system_execute,SYSTEM_EXECUTE_count,OUTPUT_SIZE,
                      SYSTEM_MULTIMEM_X(unit_loop),
                      SYSTEM_MULTIMEM_Y(unit_loop));
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_BRANCH;unit_loop++)
  {
    fu_execute_branch[unit_loop]=
    processsim_fu_gen(&system_execute,SYSTEM_EXECUTE_count,OUTPUT_SIZE,
                      SYSTEM_BRANCH_X(unit_loop),
                      SYSTEM_BRANCH_Y(unit_loop));
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_PSRTRANS;unit_loop++)
  {
    fu_execute_psrtrans[unit_loop]=
    processsim_fu_gen(&system_execute,SYSTEM_EXECUTE_count,OUTPUT_SIZE,
                      SYSTEM_PSRTRANS_X(unit_loop),
                      SYSTEM_PSRTRANS_Y(unit_loop));
  }

  fu_retire=processsim_fu_gen(&system_retire,SYSTEM_RETIRE_count,
                              OUTPUT_SIZE,SYSTEM_RETIRE_X,SYSTEM_RETIRE_Y);


  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_FETCH;unit_loop++)
  {
    processsim_pipe_gen(fu_fetch,SYSTEM_FETCH_LSU(unit_loop),
                        fu_lsu,SYSTEM_LSU_FETCH(unit_loop));
    #ifdef ARM_SUPERSCALAR_SHORT
    processsim_pipe_gen(fu_fetch,SYSTEM_FETCH_DECODE(unit_loop),
                        fu_decode2,SYSTEM_DECODE2_DECODE(unit_loop));
    #else
    processsim_pipe_gen(fu_fetch,SYSTEM_FETCH_DECODE(unit_loop),
                        fu_decode,SYSTEM_DECODE_FETCH(unit_loop));
    processsim_pipe_gen(fu_decode,SYSTEM_DECODE_DECODE2(unit_loop),
                        fu_decode2,SYSTEM_DECODE2_DECODE(unit_loop));
    #endif
    processsim_pipe_gen(fu_decode2,SYSTEM_DECODE2_ROB(unit_loop),
                        fu_rob,SYSTEM_ROB_DECODE2(unit_loop));
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_DATAPROC;unit_loop++)
  {
    processsim_pipe_gen(fu_rob,SYSTEM_ROB_DATAPROC(unit_loop),
                        fu_execute_dataproc[unit_loop],
                        SYSTEM_EXECUTE_ROB);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_MUL;unit_loop++)
  {
    processsim_pipe_gen(fu_rob,SYSTEM_ROB_MUL(unit_loop),
                        fu_execute_mul[unit_loop],
                        SYSTEM_EXECUTE_ROB);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_SINGLEMEM;unit_loop++)
  {
    processsim_pipe_gen(fu_rob,SYSTEM_ROB_SINGLEMEM(unit_loop),
                        fu_execute_singlemem[unit_loop],
                        SYSTEM_EXECUTE_ROB);
    processsim_pipe_gen(fu_execute_singlemem[unit_loop],
                        SYSTEM_EXECUTE_LSU(0),fu_lsu,
                        SYSTEM_LSU_SINGLEMEM(unit_loop));
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_MULTIMEM;unit_loop++)
  {
    processsim_pipe_gen(fu_rob,SYSTEM_ROB_MULTIMEM(unit_loop),
                        fu_execute_multimem[unit_loop],
                        SYSTEM_EXECUTE_ROB);

    for (bandwidth_loop=0;bandwidth_loop<ARM_MULTIMEM_BANDWIDTH;
         bandwidth_loop++)
    {
      processsim_pipe_gen(fu_execute_multimem[unit_loop],
                          SYSTEM_EXECUTE_LSU(bandwidth_loop),fu_lsu,
                          SYSTEM_LSU_MULTIMEM
                          (unit_loop*ARM_MULTIMEM_BANDWIDTH+
                           bandwidth_loop));
    }
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_BRANCH;unit_loop++)
  {
    processsim_pipe_gen(fu_rob,SYSTEM_ROB_BRANCH(unit_loop),
                        fu_execute_branch[unit_loop],
                        SYSTEM_EXECUTE_ROB);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_PSRTRANS;unit_loop++)
  {
    processsim_pipe_gen(fu_rob,SYSTEM_ROB_PSRTRANS(unit_loop),
                        fu_execute_psrtrans[unit_loop],
                        SYSTEM_EXECUTE_ROB);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_RETIRE;unit_loop++)
  {
    processsim_pipe_gen(fu_rob,SYSTEM_ROB_RETIRE(unit_loop),
                        fu_retire,SYSTEM_RETIRE_ROB(unit_loop));
  }
  #else
  fu_execute=processsim_fu_gen(&system_execute,SYSTEM_EXECUTE_count,
                               OUTPUT_SIZE,SYSTEM_EXECUTE_X,
                               SYSTEM_EXECUTE_Y);
  processsim_pipe_gen(fu_fetch,SYSTEM_FETCH_LSU,
                      fu_lsu,SYSTEM_LSU_FETCH);
  processsim_pipe_gen(fu_fetch,SYSTEM_FETCH_DECODE,
                      fu_decode,SYSTEM_DECODE_FETCH);
  processsim_pipe_gen(fu_decode,SYSTEM_DECODE_EXECUTE,
                      fu_execute,SYSTEM_EXECUTE_DECODE);

  for (bandwidth_loop=0;bandwidth_loop<ARM_MULTIMEM_BANDWIDTH;
       bandwidth_loop++)
  {
    processsim_pipe_gen(fu_execute,SYSTEM_EXECUTE_LSU(bandwidth_loop),
                        fu_lsu,SYSTEM_LSU_EXECUTE(bandwidth_loop));
  }
  #endif


  lsu_state=mem_alloc(sizeof(struct system_lsu_state));
  processsim_workspace_write(fu_lsu,lsu_state);
  processsim_workspace_write(fu_mem,lsu_state);

  mem_array_internal=(*lsu_state).mem_array=
  mem_alloc(sizeof(ui8f**)*((0x10000000/SYSTEM_LSU_MEMBLOCK)*0x10));
  mem_array_end=mem_array_internal+((0x10000000/SYSTEM_LSU_MEMBLOCK)*
                                    0x10);
  do
  {
    *(mem_array_internal+0)=0;
    *(mem_array_internal+1)=0;
    *(mem_array_internal+2)=0;
    *(mem_array_internal+3)=0;
    mem_array_internal+=4;
  }
  while(mem_array_internal<mem_array_end);


  (*lsu_state).index_alloc=1;
  (*lsu_state).index_done=1;


  rom_working_size=((rom_file_size=file_getsize(options_rom_filename)) &
                   ~(SYSTEM_LSU_MEMBLOCK-1))+SYSTEM_LSU_MEMBLOCK;

  printd(1,"The file size is &%X\n",rom_file_size);
  printd(1,"The used size is &%X\n",rom_working_size);

  rom_cache_end=(rom_cache=mem_alloc(rom_working_size))+
                rom_working_size;
  file_load(options_rom_filename,rom_file_size,rom_cache);

  rom_cache_internal=rom_cache+rom_file_size;
  while (rom_cache_internal<rom_cache_end)
  {
    *(rom_cache_internal++)=0;
  }

  rom_cache_internal=rom_cache;
  mem_array_internal=(*lsu_state).mem_array;
  do
  {
    *(mem_array_internal++)=rom_cache_internal;
    rom_cache_internal+=SYSTEM_LSU_MEMBLOCK;
  }
  while (rom_cache_internal<rom_cache_end);



/*  {
    int file_size,size;
    ui8f *cache,*cache_internal_end,*cache_end,*target,
         **mem_array_internal;

    size=file_size=file_getsize(rom_filename);
    size=(size & ~(SYSTEM_LSU_MEMBLOCK-1))+SYSTEM_LSU_MEMBLOCK;

    printd(1,"The file size is &%X\n",file_size);
    printd(1,"The used size is &%X\n",size);

    cache=mem_alloc(size);
    cache_end=cache+size;
    file_load(rom_filename,file_size,cache);

    mem_array_internal=(*lsu_state).mem_array;

    do
    {
      printd(1,"ROM alloc loop:\n",0);

      *(mem_array_internal++)=target=mem_alloc(SYSTEM_LSU_MEMBLOCK);

      printd(1,"  written to &%X\n",mem_array_internal-1);

      cache_internal_end=cache+SYSTEM_LSU_MEMBLOCK;

      do
      {
        *(target++)=*(cache++);
      }
      while (cache<cache_internal_end);
    }
    while (cache<cache_end);
  }*/



/*{
  ui8f **level1,**level1_end=(*lsu_state).mem_array+((0x10000000/SYSTEM_LSU_MEMBLOCK)*0x10);
  FILE *file_handle;

  file_handle=fopen("memout.txt","w");

  for (level1=(*lsu_state).mem_array;level1<level1_end;level1++)
  {
    ui8f *level2,*level2_end;

    fprintf(file_handle,"Block %d:\n",level1-(*lsu_state).mem_array);

    if (*level1==((ui8f*) 0))
    {
    }
    else
    {
      level2=*level1;
      level2_end=level2+SYSTEM_LSU_MEMBLOCK;

      for (;level2<level2_end;level2++)
      {
        fprintf(file_handle,"  &%X\n",*level2);
      }
    }
  }

  fclose(file_handle);

  exit(0);
}
*/


  fetch_state=mem_alloc(sizeof(struct system_fetch_state));
  processsim_workspace_write(fu_fetch,fetch_state);
  (*fetch_state).addr=arm.r[15];
  #ifdef ARM_SUPERSCALAR
  fetches_internal=(*fetch_state).fetches;
  fetches_internal_end=fetches_internal+ARM_SUPERSCALAR_FETCH*2;
  do
  {
    *(fetches_internal++)=(struct system_instr*) 0;
  }
  while (fetches_internal<fetches_internal_end);
  #endif


  #ifdef ARM_SUPERSCALAR
  decode2_state=mem_alloc(sizeof(struct system_decode2_state));
  processsim_workspace_write(fu_decode2,decode2_state);

  for (unit_loop=0;unit_loop<16;unit_loop++)
  {
    (*decode2_state).reg_map[unit_loop]=
    (*decode2_state).reg_invmap[unit_loop]=unit_loop;
  }

  (*decode2_state).reg_map[16]=ARM_CPSR_Ns;
  (*decode2_state).reg_invmap[ARM_CPSR_Ns]=16;
  (*decode2_state).reg_map[17]=ARM_CPSR_Zs;
  (*decode2_state).reg_invmap[ARM_CPSR_Zs]=17;
  (*decode2_state).reg_map[18]=ARM_CPSR_Cs;
  (*decode2_state).reg_invmap[ARM_CPSR_Cs]=18;
  (*decode2_state).reg_map[19]=ARM_CPSR_Vs;
  (*decode2_state).reg_invmap[ARM_CPSR_Vs]=19;

  for (unit_loop=20;unit_loop<ARM_SUPERSCALAR_REGS;unit_loop++)
  {
    (*decode2_state).reg_map[unit_loop]=-1;
  }


  rob_state=mem_alloc(sizeof(struct system_rob_state));
  processsim_workspace_write(fu_rob,rob_state);

  rob_internal_end=(rob_internal=(*rob_state).rob)+
                   ARM_SUPERSCALAR_ROB;
  do
  {
    *(rob_internal++)=(struct system_instr*) 0;
  }
  while (rob_internal<rob_internal_end);

  (*rob_state).regs_bitmap_rd=bitmaps_gen(ARM_SUPERSCALAR_REGS);
  (*rob_state).regs_bitmap_wr=bitmaps_gen(ARM_SUPERSCALAR_REGS);


  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_DATAPROC;unit_loop++)
  {
    execute_state=mem_alloc(sizeof(struct system_execute_state));
    (*execute_state).cycles_processing=0;
    (*execute_state).cycles_loaded=0;
    processsim_workspace_write(fu_execute_dataproc[unit_loop],
                               execute_state);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_MUL;unit_loop++)
  {
    execute_state=mem_alloc(sizeof(struct system_execute_state));
    (*execute_state).cycles_processing=0;
    (*execute_state).cycles_loaded=0;
    processsim_workspace_write(fu_execute_mul[unit_loop],
                               execute_state);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_SINGLEMEM;unit_loop++)
  {
    execute_state=mem_alloc(sizeof(struct system_execute_state));
    (*execute_state).cycles_processing=0;
    (*execute_state).cycles_loaded=0;
    processsim_workspace_write(fu_execute_singlemem[unit_loop],
                               execute_state);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_MULTIMEM;unit_loop++)
  {
    execute_state=mem_alloc(sizeof(struct system_execute_state));
    (*execute_state).cycles_processing=0;
    (*execute_state).cycles_loaded=0;
    processsim_workspace_write(fu_execute_multimem[unit_loop],
                               execute_state);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_BRANCH;unit_loop++)
  {
    execute_state=mem_alloc(sizeof(struct system_execute_state));
    (*execute_state).cycles_processing=0;
    (*execute_state).cycles_loaded=0;
    processsim_workspace_write(fu_execute_branch[unit_loop],
                               execute_state);
  }

  for (unit_loop=0;unit_loop<ARM_SUPERSCALAR_PSRTRANS;unit_loop++)
  {
    execute_state=mem_alloc(sizeof(struct system_execute_state));
    (*execute_state).cycles_processing=0;
    (*execute_state).cycles_loaded=0;
    processsim_workspace_write(fu_execute_psrtrans[unit_loop],
                               execute_state);
  }
  #else
  execute_state=mem_alloc(sizeof(struct system_execute_state));
  (*execute_state).cycles_processing=0;
  (*execute_state).cycles_loaded=0;
  processsim_workspace_write(fu_execute,execute_state);
  #endif


  processsim_lock();

  printd(128,"\n\nFUNCTIONAL ALLOCATIONS FROM HERE ON IN:\n\n\n",0);
  printd(0,"Here!\n",0);

  #ifdef ARM_TRACE_CHECK
  while (processsim_main_clocks()<options_output_start)
  {
    printd(4,"\nCycle %d:\n\n",processsim_main_clocks());
    arm_trace(arm_instrexecuted);
    arm_instrexecuted=0;
    processsim_main(false);
    printd(4,"\n\n",0);
  }

  while (true)
  {
    printd(4,"\nCycle %d:\n\n",processsim_main_clocks());
    arm_trace(arm_instrexecuted);
    arm_instrexecuted=0;
    processsim_main(true);
    printd(4,"\n\n",0);
  }
  #else
  while (processsim_main_clocks()<options_output_start)
  {
    printd(4,"\nCycle %d:\n\n",processsim_main_clocks());
    processsim_main(false);
    printd(4,"\n\n",0);
  }

  while (processsim_main_clocks()<options_output_end)
  {
    printd(4,"\nCycle %d:\n\n",processsim_main_clocks());
    processsim_main(true);
    printd(4,"\n\n",0);
  }
  #endif

  printf("Time taken: %f seconds\n",(double) processsim_main_secs());

  debug_fin();

  return true;
}



/* -------------------------------------------------------------------
   i32 main_parseint

   Returns a string representing an unsigned integer in base 10 as
   an i32 or -1 on failure.
------------------------------------------------------------------- */
i32 main_parseint (char *input)
{
  /* Our base-guessing heuristic is simple. Inputs starting "&",
     "0x" or "0X" are taken to be hex thereafter. Otherwise,
     strings with only decimal digits are assumed to be base 10 and
     then strings with only hex digits are assumed to be base 16.
  */
  chr temp;
  i32 result;

  if ((temp=*input)==0)
  {
    return -1;
  }

  if (temp=='&')
  {
    return main_parseint_hex(input+1);
  }

  if (temp=='0' && ((temp=*(input+1))=='x' || temp=='X'))
  {
    return main_parseint_hex(input+2);
  }

  if ((result=main_parseint_dec(input))==-1)
  {
    result=main_parseint_hex(input);
  }

  return result;
}



/* -------------------------------------------------------------------
   i32 main_parseint_dec

   Returns a string representing an unsigned integer in base 10 as
   an i32 or -1 on failure.
------------------------------------------------------------------- */
i32 main_parseint_dec (char *input)
{
  chr temp;
  i32 accumulator=0;

  if ((temp=*input)==0)
  {
    /* Treat the empty string as invalid. */
    return -1;
  }

  do
  {
    if (!isdigit(temp))
    {
      return -1;
    }

    accumulator=(temp-'0')+accumulator*10;
  }
  while ((temp=*(++input))!=0);

  return accumulator;
}



/* -------------------------------------------------------------------
   i32 main_parseint_hex

   Returns a string representing an unsigned integer in base 16 as
   an i32 or -1 on failure.
------------------------------------------------------------------- */
i32 main_parseint_hex (char *input)
{
  chr temp;
  i32 accumulator=0;

  if ((temp=*input)==0)
  {
    /* Treat the empty string as invalid. */
    return -1;
  }

  do
  {
    if (!isxdigit(temp))
    {
      return -1;
    }

    if (isdigit(temp))
    {
      accumulator=(temp-'0')+accumulator*16;
    }
    else
    {
      accumulator=(toupper(temp)-'A'+10)+accumulator*16;
    }
  }
  while ((temp=*(++input))!=0);

  return accumulator;
}