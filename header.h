/* -------------------------------------------------------------------
   ARMSim V1.06                                         HEADER FILE
   (c) Geoffrey Crossland 2000, 2001, 2002

   V1.00 : Simulates the components of a processor implementing a
           reduced form of the ARM v3 ISA.
   V1.05 : Branch prediction can now be static or dynamic. Returns
           from BLs are predicted. The simulator has a superscalar
           mode, with in-order fetching and decoding, register
           renaming, out-of-order execution and in-order
           retirement.
   V1.06 : Instructions reading from and writing to R15 may be
           executed non-serially. Execution units give usage
           statistics.
------------------------------------------------------------------- */


#ifndef HEADER_ALREADYINCLUDED
#define HEADER_ALREADYINCLUDED



/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#define ANSI
#define IA
#define IA_32
#define REAL_FLOAT

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#ifdef WIN32A
#include <windows.h>
#endif

#include "libraries/platforms.h"
/*#include "libraries/debug.h"*/
#include "libraries/debug_stubs.h"
#include "libraries/error.h"
#include "libraries/mem.h"
#include "libraries/file.h"
#include "libraries/processsim.h"
#include "libraries/bitmaps.h"



/* -------------------------------------------------------------------
   General macro, variable and structure definitions
------------------------------------------------------------------- */
#include "config.h"


#define SYSTEM_INSTR_ROB_STATUS_WAITING          0
#define SYSTEM_INSTR_ROB_STATUS_READY            1
#define SYSTEM_INSTR_ROB_STATUS_ISSUED           2
#define SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED    4
#define SYSTEM_INSTR_ROB_STATUS_DONE_UNEXECUTED  8
#define SYSTEM_INSTR_SERIALISING_MISC            1
#define SYSTEM_INSTR_SERIALISING_R15READ         2
#define SYSTEM_INSTR_SERIALISING_R15WRITE        4

#define SYSTEM_LSU_MEMBLOCK  65536

#define SYSTEM_CU_BRANCH  0


struct system_arm
{
  i32f r[16];
  #ifdef ARM_SUPERSCALAR
  i32f pr[ARM_SUPERSCALAR_REGS];
  #endif
  i32f cpsr;
  #ifdef ARM_BRANCHPRED
  ui8 *branchpred_buffer;
  i32f *branchpred_buffer_addrs;
  #endif
  #ifdef ARM_RETURNPRED
  i32f *returnpred_buffer;
  #endif
};

struct system_performance
{
  i32 instr_executed;
  #ifdef ARM_BRANCHPRED
  i32 branchpred_count;
  i32 branchpred_mispredict;
  i32 branchpred_dynamic;
  #endif
  #ifdef ARM_RETURNPRED
  i32 returnpred_count;
  i32 returnpred_mispredict;
  #endif
};

struct system_lsu_state
{
  ui8f **mem_array;
  i32 index_alloc;
  i32 index_done;
};

struct system_lsu_request
{
  i32f addr;
  i32f value;
  ui8 word;
  ui8 load;
  i32 index;
};

struct system_fetch_state
{
  i32f addr;
  #ifdef ARM_SUPERSCALAR
  struct system_instr *fetches[ARM_SUPERSCALAR_FETCH*2];
  #endif
};

#ifdef ARM_SUPERSCALAR
struct system_decode2_state
{
  i32f reg_map[ARM_SUPERSCALAR_REGS];
  i32f reg_invmap[32];
};

struct system_rob_state
{
  struct system_instr *rob[ARM_SUPERSCALAR_ROB];
  void *regs_bitmap_rd;
  void *regs_bitmap_wr;
};
#endif

struct system_execute_state
{
  i32 cycles_processing;
  i32 cycles_loaded;
};

#define ARM_CPSR_Ns             31
#define ARM_CPSR_Zs             30
#define ARM_CPSR_Cs             29
#define ARM_CPSR_Vs             28
#define ARM_CPSR_N              (0x1<<ARM_CPSR_Ns)
#define ARM_CPSR_Z              (0x1<<ARM_CPSR_Zs)
#define ARM_CPSR_C              (0x1<<ARM_CPSR_Cs)
#define ARM_CPSR_V              (0x1<<ARM_CPSR_Vs)

#define ARM_OP_AND              0x0            /* Dest, Op1 and Op2 */
#define ARM_OP_EOR              0x1
#define ARM_OP_SUB              0x2
#define ARM_OP_RSB              0x3
#define ARM_OP_ADD              0x4
#define ARM_OP_ADC              0x5
#define ARM_OP_SBC              0x6
#define ARM_OP_RSC              0x7
#define ARM_OP_TST              0x8                  /* Op1 and Op2 */
#define ARM_OP_TEQ              0x9
#define ARM_OP_CMP              0xA
#define ARM_OP_CMN              0xB
#define ARM_OP_ORR              0xC            /* Dest, Op1 and Op2 */
#define ARM_OP_MOV              0xD                 /* Dest and Op2 */
#define ARM_OP_BIC              0xE            /* Dest, Op1 and Op2 */
#define ARM_OP_MVN              0xF                 /* Dest and Op2 */
#define ARM_OP_MUL              0x10
#define ARM_OP_MLA              0x11
#define ARM_OP_B                0x12
#define ARM_OP_BL               0x13
#define ARM_OP_STR              0x15
#define ARM_OP_LDR              0x16
#define ARM_OP_STM              0x17
#define ARM_OP_LDM              0x18
#define ARM_OP_MRS              0x1F
#define ARM_OP_MSR              0x20
#define ARM_OP_UNDEF            0xFF

#define ARM_OP_EQ               (0x0<<8)
#define ARM_OP_NE               (0x1<<8)
#define ARM_OP_CS               (0x2<<8)
#define ARM_OP_CC               (0x3<<8)
#define ARM_OP_MI               (0x4<<8)
#define ARM_OP_PL               (0x5<<8)
#define ARM_OP_VS               (0x6<<8)
#define ARM_OP_VC               (0x7<<8)
#define ARM_OP_HI               (0x8<<8)
#define ARM_OP_LS               (0x9<<8)
#define ARM_OP_GE               (0xA<<8)
#define ARM_OP_LT               (0xB<<8)
#define ARM_OP_GT               (0xC<<8)
#define ARM_OP_LE               (0xD<<8)
#define ARM_OP_AL               (0xE<<8)
#define ARM_OP_NV               (0xF<<8)

#define ARM_OP_S                (1<<12)

#define ARM_OP2_REG             (0<<16)
#define ARM_OP2_CONST           (1<<16)

#define ARM_OP2_REG__LSL_REG    (2<<16)
#define ARM_OP2_REG__LSR_REG    (3<<16)
#define ARM_OP2_REG__ASR_REG    (4<<16)
#define ARM_OP2_REG__ROR_REG    (5<<16)

#define ARM_OP2_REG__LSL_CONST  (6<<16)
#define ARM_OP2_REG__LSR_CONST  (7<<16)
#define ARM_OP2_REG__ASR_CONST  (8<<16)
#define ARM_OP2_REG__ROR_CONST  (9<<16)

#define ARM_OP2_REG__RRX        (10<<16)

/* Format of the Ops Word:
   =======================

   DataProc:
   ---------
   Bits 0-3: Destination
   Bits 4-7: Operand 1

   For OP2_CONST: Bits 8-15: rotatable value
                  Bits 16-19: ROR factor
   For OP2_REG: Bits 8-11: Operand 2
   For OP2_REG__XXX_REG: Bits 8-11: Operand 2
                         Bits 12-15: Register with Shift Value
   For OP2_REG__XXX_CONST: Bits 8-11: Operand 2
                           Bits 12-16: Constant


   SingleMem:
   ----------
   Bits 0-3: Destination/Source data
   Bits 4-7: Operand 1 (Address index)

   For OP2_CONST: Bits 8-19: unsigned value
   For OP2_REG: Bits 8-11: Operand 2
   For OP2_REG__XXX_CONST: Bits 8-11: Operand 2
                           Bits 12-16: Constant

   Bit 21: Indexing style (1 for writeback, 0 for no writeback)
   Bit 22: Data size (0 for word addressing, 1 for byte)
   Bit 23: Offset sign (0 for subtract offset from base, 1 for add)
   Bit 24: Index style (0 for post, 1 for pre)


   MultiMem:
   ---------
   Bits 0-15: Register bitmap
   Bits 16-19: Pointer register

   Bit 20: Reserved (undefined)
   Bit 21: Writeback flag (0 no, 1 yes)
   Bit 22: R15 and PSR control style (1 for S set, 0 for S clear)
   Bit 23: Direction (1 for incrementation, 0 for decrementation)
   Bit 24: Index style (0 for post, 1 for pre)


   Mul:
   ----
   Bits 0-3: Destination
   Bits 4-7: Operand 1
   Bits 8-11: Operand 2
   Bits 12-15: Accumulator


   Branch:
   -------
   Bits 0-31: Encoded PC-relative offset (add current addr to get
              target address; the +8 has been done in the
              disassembly)


   PSRTrans:
   ---------
   Bits 0-3: Status register protection mask qualifier (MSR only)
   Bits 5-7: Reserved (undefined)

   For OP2_REG: Bits 8-11: Source register
   For OP2_CONST: Bits 8-15: rotatable value
                  Bits 16-19: ROR factor


   Format of the MicroOp:
   ======================

   For MicroOps, all components of the traditional decoded ARM
   instruction format are retained, with the addition of:

   regs_bitmap_rd: Bit n: 1 for register read by instr, 0 for not
   regs_bitmap_wr: Bit n: 1 for register written to by instr, 0 for
                          not
   regs_rd[n]: Read instruction register n; also stores written
               values on exit, corresponding to regs_wr[] entries;
               -1 indicated unused
   regs_wr[n]: Written instruction register n; -1 indicates unused


   regs_rd[0]: Negative flag read identifier (-1 for unread,
               relevant physical register value for read)
   regs_rd[1]: Zero flag read identifier (-1 for unread,
               relevant physical register value for read)
   regs_rd[2]: Carry flag read identifier (-1 for unread,
               relevant physical register value for read)
   regs_rd[3]: Overflow flag read identifier (-1 for unread,
               relevant physical register value for read)

   regs_wr[0]: Negative flag write identifier (-1 for unread,
               relevant physical register value for write)
   regs_wr[1]: Zero flag write identifier (-1 for unread,
               relevant physical register value for write)
   regs_wr[2]: Carry flag write identifier (-1 for unread,
               relevant physical register value for write)
   regs_wr[3]: Overflow flag write identifier (-1 for unread,
               relevant physical register value for write)


   DataProc:
   ---------
   regs_wr[4]: Destination
   regs_rd[4]: Operand 1

   For OP2_REG: regs_rd[5]: Operand 2
   For OP2_REG__XXX_REG: regs_rd[5]: Operand 2
                         regs_rd[6]: Register with Shift Value
   For OP2_REG__XXX_CONST: regs_rd[5]: Operand 2


   SingleMem:
   ----------
   regs_rd[4]: Source regiser (STR)
   regs_wr[4]: Destination register (LDR)

   regs_rd[5]: Operand 1 (Address index)
   regs_wr[5]: Writeback for address

   For OP2_REG: regs_rd[6]: Operand 2
   For OP2_REG__XXX_CONST: regs_rd[6]: Operand 2


   MultiMem:
   ---------
   regs_rd[4]: Pointer register
   regs_wr[4]: Writeback for pointer
   regs_xx[n+5]: Register used for transfer, for 0<=n<c, c=register
                 transfer count, appearing such that lower values
                 for n correspond with lower ARM registers


   Mul:
   ----
   regs_wr[4]: Destination
   regs_rd[4]: Operand 1
   regs_rd[5]: Operand 2
   regs_rd[6]: Accumulator


   Branch:
   -------
   regs_rd[4]: PC
   regs_wr[4]: PC
   regs_wr[5]: R14 (for BLs)


   PSRTrans:
   ---------
   For OP2_REG: regs_xx[4]: Source register
*/

struct system_instr
{
  i32f word;
  i32f op;
  i32f ops;
  i32f temp[2];
  ui8 status;

  #ifdef ARM_SUPERSCALAR
  i32f regs_rd[21];
  i32f regs_wr[21];
  i32f regs_val[21];
  void *regs_bitmap_rd;
  void *regs_bitmap_wr;

  ui8 rob_status;
  ui8 serialising;

  #ifdef ARM_ROB_SPECEXEC_R15READ
  i32f pc;
  #endif
  #endif

  ui8 branchpred_taken;
  i32f returnpred_addr;
};



#define ROR(V,R) (((V)>>(R)) | ((V)<<(32-(R))))

#define oprintf(str,args...)                                         \
({                                                                   \
  int COUNT;                                                         \
                                                                     \
  if (processsim_main_outputting())                                  \
  {                                                                  \
    COUNT=sprintf(str,##args);                                       \
  }                                                                  \
  else                                                               \
  {                                                                  \
    COUNT=*str=0;                                                    \
  }                                                                  \
                                                                     \
  COUNT;                                                             \
})



/* -------------------------------------------------------------------
   External variables and routines
------------------------------------------------------------------- */
extern struct system_arm arm;
extern i32 arm_instrexecuted;
#ifdef ARM_TRACE_CHECK
extern i32 arm_trace_instr;
extern fileh arm_trace_file;
#endif
extern struct system_performance performance;


extern int main (int argc,char *argv[]);
extern i32 main_parseint (char *input);
extern i32 main_parseint_dec (char *input);
extern i32 main_parseint_hex (char *input);

#ifdef ARM_TRACE_CHECK
extern void arm_trace (i32 instrexecuted);
#endif
extern void system_cu (void *fu);
extern void system_mem (void *fu);
extern void system_focpred (void *fu);
extern void system_lsu (void *fu);
extern i32f system_lsu_access (ui8f **mem_array,i32f addr,i32f value,
                               ui8 word,ui8 load);
extern void system_fetch (void *fu);
extern void system_decode (void *fu);
#ifdef ARM_SUPERSCALAR
extern void system_decode2 (void *fu);
extern i32f system_decode2_editmap (i32f *reg_map,i32f *reg_invmap,
                                    i32f reg_logical,
                                    struct system_instr *instr);
extern void system_rob (void *fu);
extern struct system_instr *system_rob_issue (void *fu,
                                              struct system_instr
                                              *instr,
                                              i32 pipe_base,
                                              i32 pipe_count);
extern void system_execute (void *fu);
extern i32f system_execute_DataProc (struct system_instr *instr,
                                     i32f flags_in,void *fu,
                                     char **output_buffer);
extern i32f system_execute_Mul (struct system_instr *instr,
                                     i32f flags_in,void *fu,
                                     char **output_buffer);
extern i32f system_execute_Branch (struct system_instr *instr,
                                     i32f flags_in,void *fu,
                                     char **output_buffer);
extern i32f system_execute_SingleMem (struct system_instr *instr,
                                     i32f flags_in,void *fu,
                                     char **output_buffer);
extern i32f system_execute_MultiMem (struct system_instr *instr,
                                     i32f flags_in,void *fu,
                                     char **output_buffer);
extern void system_retire (void *fu);
#else
extern void system_execute (void *fu);
extern ui8 system_execute_DataProc (struct system_instr *instr,
                                    void *fu,char **output_buffer);
extern ui8 system_execute_Mul (struct system_instr *instr,
                               void *fu,char **output_buffer);
extern ui8 system_execute_Branch (struct system_instr *instr,
                                  void *fu,char **output_buffer);
extern ui8 system_execute_SingleMem (struct system_instr *instr,
                                     void *fu,char **output_buffer);
extern ui8 system_execute_MultiMem (struct system_instr *instr,
                                    void *fu,char **output_buffer);
#endif
extern ui8 system_branchpred_read (i32f addr);
extern ui8 *system_branchpred_write (i32f addr);
extern void system_returnpred_push (i32f addr);
extern i32f system_returnpred_pull (void);
extern void system_returnpred_flush (void);
extern void system_returnpred_r15dest (struct system_instr *instr);
extern struct system_instr *system_decode_disassemble
                                         (struct system_instr *instr);
extern i32 system_decode_print (char *output,
                                struct system_instr *instr);
#ifdef ARM_SUPERSCALAR
extern i32 system_decode2_print (char *output,
                                 struct system_instr *instr);
#endif


#endif
