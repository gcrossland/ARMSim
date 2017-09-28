/* -------------------------------------------------------------------
   ARMSim V1.06                                        C COMPONENTS
   (c) Geoffrey Crossland 2000, 2001, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "header.h"



/* -------------------------------------------------------------------
   struct system_instr *system_decode_disassemble

   Given a *system_instr with an instruction word, decodes the
   instruction, returning the pointer to the original structure.
------------------------------------------------------------------- */
struct system_instr *system_decode_disassemble (struct system_instr
                                                *instr)
{
  i32f arm_opword,code_op,code_ops;
  ui8 handled;

  arm_opword=(*instr).word;

  /* Deal with condition code: */
  code_op=((arm_opword>>20) & 0x00000F00);
  code_ops=0;

  /* Check for all opcode groups: DataProcOp2Imm     Mul
                                  DataProcOp2Reg     Branch
                                  MultiMem           SWI
                                  SingleMemOp2Imm    Swap
                                  SingleMemOp2Reg    CoOp
                                  CoMem              CoTransOp
                                  PSRTransReg        PSRTransImm

     For this version, SWIs, Swaps, CoOps, CoMems and CoTransOps, as
     well as subsets of other instructions, are parsed and flagged
     as Undefs.
  */

  handled=false;

  switch (arm_opword & (0x7<<25))
  {
  case (0x0<<25):   /* DataProcOp2Reg or Mul or Swap or PSRTransReg */
    /* Checking for MULs/MLAs/SWPs/MRSs/MSRs:
       * Has the same instruction block ID as DataProcsOp2Reg
       * If a probDataProcOp2Reg has a shift type ID with bit 0
         clear (i.e. shift by constant), then it can't be a MUL
       * If a probDataProcOp2Reg has a shift type ID with bit 1 set
         (i.e. shift by reg), then it must have bit 7 clear to be
         DataProcOp2Reg
       Therefore, propDataProcOp2Reg ops must have both bits
       4 and 7 set to be a MUL

       The SWP instruction also falls into this range. It carries
       the multiplications' distinctive bits 4 and 7 set feature
       and also uses another trick: the DataProc ops' SWP
       shadows are the test ops; SWP keeps bit 20 (the status
       flags bit) clear, while the test instructions need them set.
       Finally, the third nibble is clear.

       Register MSRs and MRSs also slots in over the tests, with
       the S bit clear, but the second nibble is &0 (no %1001 trick
       here).
    */

    if ((arm_opword & 0xF0)==0x90)                   /* Mul or Swap */
    {
      if ((arm_opword & ((15<<8)+(3<<20)+(3<<23)))==        /* Swap */
          ((0<<8)+(0<<20)+(2<<23)))
      {
        /* Swaps flgged as Undefs */
      }
      else
      {
        if ((arm_opword & (7<<22))==(0<<22))                 /* Mul */
        {
          handled=true;

          if ((arm_opword & (1<<21))==0)
          {
            code_op+=ARM_OP_MUL;
                     /* Specific op */
          }
          else
          {
            code_op+=ARM_OP_MLA;
          }

          code_op+=((arm_opword>>8) & 0x00001000)+ARM_OP2_REG;
                   /* S bit */
          code_ops=((arm_opword>>16) & 0xF)+
                   /* Destination register */
                   ((arm_opword & 0xF)<<4)+
                   /* Operand 1 */
                   (arm_opword & 0xFF00);
                   /* Operand 2 and Accumulator */
        }
      }
    }
    else                           /* DataProcOp2Reg or PSRTransReg */
    {
      if ((arm_opword & (0x19<<20))==                /* PSRTransReg */
          (0x10<<20))
      {
        if ((arm_opword & ((1<<21)+(0xFFF<<4)))==((1<<21)+(0xF00<<4)))
        {
          handled=true;

          code_op+=ARM_OP_MSR+ARM_OP2_REG;
          code_ops=((arm_opword<<8) & 0xF00)+
                   /* Source register */
                   ((arm_opword>>16) & 0xF)+
                   /* Mask */
                   ((arm_opword>>18) & 0x10);
                   /* Register selection flag */
        }
        else
        {
          if ((arm_opword & ((1<<21)+0xF0FFF))==((0<<21)+0xF0000))
          {
            handled=true;

            code_op+=ARM_OP_MRS+ARM_OP2_REG;
            code_ops=((arm_opword>>4) & 0xF00)+
                     /* Source register */
                     (0x9)+
                     /* Fake mask */
                     ((arm_opword>>18) & 0x10);
                     /* Register selection flag */
          }
        }
      }
      else                                        /* DataProcOp2Reg */
      {
        handled=true;

        code_op+=((arm_opword>>21) & 0x0000000F)+
                 /* Specific op */
                 ((arm_opword>>8) & 0x00001000);
                 /* S bit */
        code_ops=((arm_opword>>12) & 0x000000FF)+
                 /* Destination register in low nibble,
                    Operand 1 in high nibble */
                 ((arm_opword & 0x0000000F)<<8);
                 /* Operand 2 register */

        switch (arm_opword & 0x00000070)
        {
        case 0x00:
          if ((arm_opword & 0xF80)==0)
          {                                 /* LSL #0 i.e. no shift */
            code_op+=ARM_OP2_REG;
          }
          else
          {
            code_op+=ARM_OP2_REG__LSL_CONST;
            code_ops+=((arm_opword & 0x00000F80)<<5);
                      /* Shift value */
          }
          break;
        case 0x10:
          code_op+=ARM_OP2_REG__LSL_REG;
          code_ops+=((arm_opword & 0x00000F00)<<4);
                    /* Shift value */
          break;
        case 0x20:
          code_op+=ARM_OP2_REG__LSR_CONST;
          code_ops+=((arm_opword & 0x00000F80)<<5);
                    /* Shift value */

          /* A shift value of 32 is here kept encoded as zero: this
             removes the need to allocate the shift value an extra
             bit.
          */
          break;
        case 0x30:
          code_op+=ARM_OP2_REG__LSR_REG;
          code_ops+=((arm_opword & 0x00000F00)<<4);
                    /* Shift value */
          break;
        case 0x40:
          code_op+=ARM_OP2_REG__ASR_CONST;
          code_ops+=((arm_opword & 0x00000F80)<<5);
                    /* Shift value */
          break;
        case 0x50:
          code_op+=ARM_OP2_REG__ASR_REG;
          code_ops+=((arm_opword & 0x00000F00)<<4);
                    /* Shift value */
          break;
        case 0x60:
          if ((arm_opword & 0xF80)==0)               /* Shift value */
          {
            code_op+=ARM_OP2_REG__RRX;
/*          code_ops+=32<<12;*/
          }
          else
          {
            code_op+=ARM_OP2_REG__ROR_CONST;
            code_ops+=((arm_opword & 0x00000F80)<<5);
          }
          break;
        case 0x70:
          code_op+=ARM_OP2_REG__ROR_REG;
          code_ops+=((arm_opword & 0x00000F00)<<4);
                    /* Shift value */
        }
      }
    }
    break;
  case (0x1<<25):                  /* DataProcOp2Imm or PSRTransImm */
    if ((arm_opword & ((0x1B<<20)+(0xF<<12)))==      /* PSRTransImm */
        ((0x12<<20)+(0xF<<12)))
    {
      handled=true;

      code_op+=ARM_OP_MSR+ARM_OP2_CONST;
      code_ops=((arm_opword<<8) & 0xFFF00)+
               /* ROR factor and value */
               ((arm_opword>>16) & 0xF)+
               /* Mask */
               ((arm_opword>>18) & 0x10);
               /* Register selection flag */
    }
    else                                          /* DataProcOp2Imm */
    {
      handled=true;

      code_op+=((arm_opword>>21) & 0x0000000F)+
                   /* Specific op */
                   ((arm_opword>>8) & 0x00001000)+
                   /* S bit */
                   ARM_OP2_CONST;
      code_ops=((arm_opword>>12) & 0x000000FF)+
               /* Destination register in low nibble,
                  Operand 1 in high nibble */
               ((arm_opword & 0x00000FFF)<<8);
               /* ROR factor and value */
    }
    break;
  case (0x5<<25):                                         /* Branch */
    handled=true;

    if ((arm_opword & (1<<24))==0)
    {
      code_op+=ARM_OP_B;
                   /* Specific op */
    }
    else
    {
      code_op+=ARM_OP_BL;
    }

    code_ops=arm_opword & 0x00FFFFFF;
    code_ops=code_ops | (((code_ops & 0x800000)==0) ? 0x00000000 :
                                                      0xFF000000);
    code_ops=(code_ops<<2)+8;
    break;
  case (0x7<<25):                       /* SWI or CoOp or CoTransOp */
    /* SWIs, CoOps and CoTransOps flagged as Undefs */
    break;
  case (0x3<<25):                                /* SingleMemOp2Reg */
    handled=true;

    if ((arm_opword & (1<<20))==0)
    {
      code_op+=ARM_OP_STR;
    }
    else
    {
      code_op+=ARM_OP_LDR;
    }

    code_ops=(arm_opword & 0x1E00000)+
             /* Indexing, offset sign, data size
                and writeback flags */
             ((arm_opword>>12) & 0x000000FF)+
             /* Destination register in low nibble,
                Operand 1 in high nibble */
             ((arm_opword & 0x0000000F)<<8);
             /* Operand 2 register */

    if ((code_ops & (1<<24))==0)
    {
      /* If post-indexing is being used, copy the W bit from the
         writeback bit to the MMU style bit and force the
         writeback bit to 1.
      */

      code_ops=(code_ops | ((code_ops>>1) & (1<<20))) | (1<<21);
    }
    else
    {
      /* If pre-indexing is being used, make sure the MMU style bit
         is clear.
      */

      code_ops=code_ops & (~(1<<20));
    }

    switch (arm_opword & 0x00000070)
    {
    case 0x00:
      if ((arm_opword & 0xF80)==0)
      {                                     /* LSR #0 i.e. no shift */
        code_op+=ARM_OP2_REG;
      }
      else
      {
        code_op+=ARM_OP2_REG__LSL_CONST;
        code_ops+=((arm_opword & 0x00000F80)<<5);
                  /* Shift value */
      }
      break;
    case 0x20:
      code_op+=ARM_OP2_REG__LSR_CONST;
      code_ops+=((arm_opword & 0x00000F80)<<5);
                /* Shift value */

      /* Again, the shift value 32 is encoded as zero. */
      break;
    case 0x40:
      code_op+=ARM_OP2_REG__ASR_CONST;
      code_ops+=((arm_opword & 0x00000F80)<<5);
                /* Shift value */
      break;
    case 0x60:
      if ((arm_opword & 0xF80)==0)                   /* Shift value */
      {
        code_op+=ARM_OP2_REG__RRX;
/*          code_ops+=32<<12;*/
      }
      else
      {
        code_op+=ARM_OP2_REG__ROR_CONST;
        code_ops+=((arm_opword & 0x00000F80)<<5);
      }
      break;
    default:
      handled=false;
    }
    break;
  case (0x2<<25):                                /* SingleMemOp2Imm */
    handled=true;

    if ((arm_opword & (1<<20))==0)
    {
      code_op+=ARM_OP_STR+ARM_OP2_CONST;
    }
    else
    {
      code_op+=ARM_OP_LDR+ARM_OP2_CONST;
    }

    code_ops=(arm_opword & 0x1E00000)+
             /* Indexing, offset sign, data size
                and writeback flags */
             ((arm_opword>>12) & 0x000000FF)+
             /* Destination register in low nibble,
                Operand 1 in high nibble */
             ((arm_opword & 0x00000FFF)<<8);
             /* constant value */

    if ((code_ops & (1<<24))==0)
    {
      code_ops=(code_ops | ((code_ops>>1) & (1<<20))) | (1<<21);
    }
    else
    {
      code_ops=code_ops & (~(1<<20));
    }
    break;
  case (0x4<<25):                                       /* MultiMem */
    handled=true;

    code_op+=((arm_opword>>10) & 0x00001000);
                 /* S bit */

    if ((arm_opword & (1<<20))==0)
    {
      code_op+=ARM_OP_STM;
    }
    else
    {
      code_op+=ARM_OP_LDM;
    }

    code_ops=(arm_opword & 0x01E00000)+
             /* Indexing, direction and writeback flags */
             (arm_opword & 0x000FFFFF);
             /* Pointer register and register list */
    break;
  case (0x6<<25):                                          /* CoMem */
    /* CoMems flagged as Undefs */
    break;
  }

  /* The following code picks out some additional cases which we
     wish to be flagged, in this version, as Undefs, being Tests
     with destinations!=0 and MSR/MRSes trying to access the SPSR.
  */
/*  if (handled==false ||
      ((code_op & 0xC)==0x8 && (code_ops & 0xF)!=0) ||
      (((code_op & 0xFF)==ARM_OP_MRS ||
        (code_op & 0xFF)==ARM_OP_MSR) &&
       (code_ops & 0x10)==0x10) ||
      ((code_op & 0xFF)<0x10 && (code_ops & 0xF)==15 &&
       (code_op & ARM_OP_S)!=0))*/

  if (handled==false ||
      ((code_op & 0xFF)<0x10 && (code_op & 0xC)==0x8 &&
       (code_ops & 0xF)!=0) ||
      (((code_op & 0xFF)==ARM_OP_MRS ||
        (code_op & 0xFF)==ARM_OP_MSR) &&
       (code_ops & 0x10)==0x10))
  {
    code_op=ARM_OP_UNDEF;
/*    error_fatal("Attempted to decode an undefined instruction (system_decode)",2);*/
  }

  (*instr).op=code_op;
  (*instr).ops=code_ops;
  (*instr).status=0;



  printd(4,"  Instruction interpreted:\n",0);

  switch (code_op & 0xFF)
  {
  case ARM_OP_AND:
    printd(4,"    AND",0);
    break;
  case ARM_OP_EOR:
    printd(4,"    EOR",0);
    break;
  case ARM_OP_SUB:
    printd(4,"    SUB",0);
    break;
  case ARM_OP_RSB:
    printd(4,"    RSB",0);
    break;
  case ARM_OP_ADD:
    printd(4,"    ADD",0);
    break;
  case ARM_OP_ADC:
    printd(4,"    ADC",0);
    break;
  case ARM_OP_SBC:
    printd(4,"    SBC",0);
    break;
  case ARM_OP_RSC:
    printd(4,"    RSC",0);
    break;
  case ARM_OP_TST:
    printd(4,"    TST",0);
    break;
  case ARM_OP_TEQ:
    printd(4,"    TEQ",0);
    break;
  case ARM_OP_CMP:
    printd(4,"    CMP",0);
    break;
  case ARM_OP_CMN:
    printd(4,"    CMN",0);
    break;
  case ARM_OP_ORR:
    printd(4,"    ORR",0);
    break;
  case ARM_OP_MOV:
    printd(4,"    MOV",0);
    break;
  case ARM_OP_BIC:
    printd(4,"    BIC",0);
    break;
  case ARM_OP_MVN:
    printd(4,"    MVN",0);
    break;
  case ARM_OP_MUL:
    printd(4,"    MUL",0);
    break;
  case ARM_OP_MLA:
    printd(4,"    MLA",0);
    break;
  case ARM_OP_B:
    printd(4,"    B",0);
    break;
  case ARM_OP_BL:
    printd(4,"    BL",0);
    break;
/*  case ARM_OP_SWI:
    printd(4,"    SWI",0);
    break;*/
  case ARM_OP_STR:
    printd(4,"    STR",0);
    break;
  case ARM_OP_LDR:
    printd(4,"    LDR",0);
    break;
  case ARM_OP_STM:
    printd(4,"    STM",0);
    break;
  case ARM_OP_LDM:
    printd(4,"    LDM",0);
    break;
/*  case ARM_OP_SWP:
    printd(4,"    SWP",0);
    break;
  case ARM_OP_CDP:
    printd(4,"    CDP",0);
    break;
  case ARM_OP_STC:
    printd(4,"    STC",0);
    break;
  case ARM_OP_LDC:
    printd(4,"    LDC",0);
    break;
  case ARM_OP_MCR:
    printd(4,"    MCR",0);
    break;
  case ARM_OP_MRC:
    printd(4,"    MRC",0);
    break;*/
  case ARM_OP_MRS:
    printd(4,"    MRS",0);
    break;
  case ARM_OP_MSR:
    printd(4,"    MSR",0);
    break;
  }


  switch (code_op & 0xF00)
  {
  case ARM_OP_EQ:
    printd(4,"     with cond EQ\n",0);
    break;
  case ARM_OP_NE:
    printd(4,"     with cond NE\n",0);
    break;
  case ARM_OP_CS:
    printd(4,"     with cond CS\n",0);
    break;
  case ARM_OP_CC:
    printd(4,"     with cond CC\n",0);
    break;
  case ARM_OP_MI:
    printd(4,"     with cond MI\n",0);
    break;
  case ARM_OP_PL:
    printd(4,"     with cond PL\n",0);
    break;
  case ARM_OP_VS:
    printd(4,"     with cond VS\n",0);
    break;
  case ARM_OP_VC:
    printd(4,"     with cond VC\n",0);
    break;
  case ARM_OP_HI:
    printd(4,"     with cond HI\n",0);
    break;
  case ARM_OP_LS:
    printd(4,"     with cond LS\n",0);
    break;
  case ARM_OP_GE:
    printd(4,"     with cond GE\n",0);
    break;
  case ARM_OP_LT:
    printd(4,"     with cond LT\n",0);
    break;
  case ARM_OP_GT:
    printd(4,"     with cond GT\n",0);
    break;
  case ARM_OP_LE:
    printd(4,"     with cond LE\n",0);
    break;
  case ARM_OP_AL:
    printd(4,"    \n",0);
    break;
  case ARM_OP_NV:
    printd(4,"     with cond NV\n",0);
    break;
  }


  if ((code_op & ARM_OP_S)==ARM_OP_S)
  {
    printd(4,"    Will affect the status flags\n",0);
  }

  if ((code_op & 0xFF)<ARM_OP_B)
  {
    i32f val,ror;

    printd(4,"    Destination: R%d\n",(code_ops & 0xF));
    printd(4,"    Operand 1: R%d\n",((code_ops & 0xF0)>>4));

    switch (code_op & 0xF0000)
    {
    case ARM_OP2_REG:
      printd(4,"    Operand 2: R%d\n",((code_ops & 0xF00)>>8));
      break;
    case ARM_OP2_CONST:
      val=((code_ops>>8) & 0xFF);
      ror=((code_ops>>15) & 0x1E);
      printd(4,"    Operand 2 is a constant (%d,&",ROR(val,ror));
      printd(4,"    %X)\n",ROR(val,ror));
      break;
    case ARM_OP2_REG__LSL_REG:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    LSL R%d\n",code_ops>>12);
      break;
    case ARM_OP2_REG__LSR_REG:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    LSR R%d\n",code_ops>>12);
      break;
    case ARM_OP2_REG__ASR_REG:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    ASR R%d\n",code_ops>>12);
      break;
    case ARM_OP2_REG__ROR_REG:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    ROR R%d\n",code_ops>>12);
      break;
    case ARM_OP2_REG__LSL_CONST:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    LSL #%d\n",code_ops>>12);
      break;
    case ARM_OP2_REG__LSR_CONST:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    LSR #%d\n",code_ops>>12);
      break;
    case ARM_OP2_REG__ASR_CONST:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    ASR #%d\n",code_ops>>12);
      break;
    case ARM_OP2_REG__ROR_CONST:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    ROR #%d\n",code_ops>>12);
      break;
    case ARM_OP2_REG__RRX:
      printd(4,"    Operand 2 (R%d) is under RRX\n",((code_ops & 0xF00)>>8));
      break;
    }

    if ((code_op & 0xFF)==ARM_OP_MLA)
    {
      printd(4,"    Accumulator: R%d\n",((code_ops & 0xF000)>>12));
    }
  }

  if ((code_op & 0xFF)==ARM_OP_B ||
      (code_op & 0xFF)==ARM_OP_BL)
  {
/*    printd(4,"    Branch to &%X\n",addr_arm+code_ops);*/
  }

/*  if ((code_op & 0xFF)==ARM_OP_SWI)
  {
    printd(4,"    Instruction &%X\n",code_ops);
  }*/

  if ((code_op & 0xFF)==ARM_OP_STR ||
      (code_op & 0xFF)==ARM_OP_LDR)
  {

    printd(4,"      USR Style : ",0);
    if ((code_ops & (1<<20))==0)
    {
      printd(4,"    No\n",0);
    }
    else
    {
      printd(4,"    Yes\n",0);
    }

    printd(4,"      Writeback : ",0);
    if ((code_ops & (1<<21))==0)
    {
      printd(4,"    No\n",0);
    }
    else
    {
      printd(4,"    Yes\n",0);
    }

    printd(4,"      Data Size : ",0);
    if ((code_ops & (1<<22))==0)
    {
      printd(4,"    32 bits (aligned)\n",0);
    }
    else
    {
      printd(4,"    8 bits\n",0);
    }

    printd(4,"    Offset Sign : ",0);
    if ((code_ops & (1<<23))==0)
    {
      printd(4,"    Subtract from base\n",0);
    }
    else
    {
      printd(4,"    Add to base\n",0);
    }

    printd(4,"    Index Style : ",0);
    if ((code_ops & (1<<24))==0)
    {
      printd(4,"    Post-indexed\n",0);
    }
    else
    {
      printd(4,"    Pre-indexed\n",0);
    }

    printd(4,"    Destination: R%d\n",(code_ops & 0xF));
    printd(4,"    Operand 1 (Base): R%d\n",((code_ops & 0xF0)>>4));

    switch (code_op & 0xF0000)
    {
    case ARM_OP2_REG:
      printd(4,"    Operand 2: R%d\n",((code_ops & 0xF00)>>8));
      break;
    case ARM_OP2_CONST:
      printd(4,"    Operand 2 is a constant (%d)\n",((code_ops & 0xFFF00)>>8));
      break;
    case ARM_OP2_REG__LSL_CONST:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    LSL #%d\n",(code_ops>>12) & 0x1F);
      break;
    case ARM_OP2_REG__LSR_CONST:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    LSR #%d\n",(code_ops>>12) & 0x1F);
      break;
    case ARM_OP2_REG__ASR_CONST:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    ASR #%d\n",(code_ops>>12) & 0x1F);
      break;
    case ARM_OP2_REG__ROR_CONST:
      printd(4,"    Operand 2 (R%d) is under ",(code_ops & 0xF00)>>8);
      printd(4,"    ROR #%d\n",(code_ops>>12) & 0x1F);
      break;
    case ARM_OP2_REG__RRX:
      printd(4,"    Operand 2 (R%d) is under RRX\n",((code_ops & 0xF00)>>8));
      break;
    }
  }

  if ((code_op & 0xFF)==ARM_OP_STM ||
      (code_op & 0xFF)==ARM_OP_LDM)
  {
    ui8 rc;

    printd(4,"          Writeback : ",0);
    if ((code_ops & (1<<21))==0)
    {
      printd(4,"    No\n",0);
    }
    else
    {
      printd(4,"    Yes\n",0);
    }

    printd(4,"          Direction : ",0);
    if ((code_ops & (1<<23))==0)
    {
      printd(4,"    Decrementation\n",0);
    }
    else
    {
      printd(4,"    Incrementation\n",0);
    }

    printd(4,"        Index Style : ",0);
    if ((code_ops & (1<<24))==0)
    {
      printd(4,"    Post-indexed (on a word by word basis)\n",0);
    }
    else
    {
      printd(4,"    Pre-indexed (on a word by word basis)\n",0);
    }

    printd(4,"            Pointer : R%d\n",((code_ops>>16) & 0xF));

    printd(4,"    Register bitmap : ",0);
    for (rc=0;rc<16;rc++)
    {
      if (((code_ops>>rc) & 1)==1)
      {
        printd(4,"    R%d ",rc);
      }
    }
    printd(4,"    \n",0);
  }

/*  if ((code_op & 0xFF)==ARM_OP_SWP)
  {
    printd(4,"    Data Size : ",0);
    if ((code_ops & (1<<22))==0)
    {
      printd(4,"    32 bits (aligned)\n",0);
    }
    else
    {
      printd(4,"    8 bits\n",0);
    }

    printd(4,"    Pointer: R%d\n",((code_ops>>8) & 0xF));
    printd(4,"    Source: R%d\n",code_ops & 0xF);
    printd(4,"    Destination: R%d\n",((code_ops>>4) & 0xF));
  }

  if ((code_op & 0xFF)==ARM_OP_CDP)
  {
    printd(4,"    Coprocessor number: %d\n",((code_ops>>8) & 0xF));
    printd(4,"    Coprocessor operation: %d\n",((code_ops>>20) & 0xF));
    printd(4,"    Additional information: %d\n",((code_ops>>5) & 0x7));
    printd(4,"    Destination: CR%d\n",((code_ops>>12) & 0xF));
    printd(4,"    Operand 1: CR%d\n",((code_ops>>16) & 0xF));
    printd(4,"    Operand 2: CR%d\n",(code_ops & 0xF));
  }

  if ((code_op & 0xFF)==ARM_OP_STC ||
      (code_op & 0xFF)==ARM_OP_LDC)
  {
    printd(4,"    Coprocessor number: %d\n",((code_ops>>8) & 0xF));

    printd(4,"      Writeback : ",0);
    if ((code_ops & (1<<21))==0)
    {
      printd(4,"    No\n",0);
    }
    else
    {
      printd(4,"    Yes\n",0);
    }

    printd(4,"         Length : ",0);
    if ((code_ops & (1<<22))==0)
    {
      printd(4,"    Monoregister\n",0);
    }
    else
    {
      printd(4,"    Multiregister\n",0);
    }

    printd(4,"    Offset Sign : ",0);
    if ((code_ops & (1<<23))==0)
    {
      printd(4,"    Subtract from base\n",0);
    }
    else
    {
      printd(4,"    Add to base\n",0);
    }

    printd(4,"    Index Style : ",0);
    if ((code_ops & (1<<24))==0)
    {
      printd(4,"    Post-indexed\n",0);
    }
    else
    {
      printd(4,"    Pre-indexed\n",0);
    }

    printd(4,"    Destination: R%d\n",((code_ops>>12) & 0xF));
    printd(4,"    Operand 1 (Base): R%d\n",((code_ops>>16) & 0xF));
    printd(4,"    Offset: %d\n",((code_ops<<2) & 0x3FF));
  }

  if ((code_op & 0xFF)==ARM_OP_MCR ||
      (code_op & 0xFF)==ARM_OP_MRC)
  {
    printd(4,"    Coprocessor number: %d\n",((code_ops>>8) & 0xF));
    printd(4,"    Coprocessor operation: %d\n",((code_ops>>21) & 0x7));
    printd(4,"    Additional information: %d\n",((code_ops>>5) & 0x7));
    printd(4,"    ARM transfer: R%d\n",((code_ops>>12) & 0xF));
    printd(4,"    Coprocessor transfer: CR%d\n",((code_ops>>16) & 0xF));
    printd(4,"    Additional information: CR%d\n",(code_ops & 0xF));
  }*/

  if ((code_op & 0xFF)==ARM_OP_MSR ||
      (code_op & 0xFF)==ARM_OP_MRS)
  {
    printd(4,"    Status register : ",0);
    if ((code_ops & (1<<4))==0)
    {
      printd(4,"    CPSR\n",0);
    }
    else
    {
      printd(4,"    SPSR\n",0);
    }

    printd(4,"          Mask type : ",0);
    switch (code_ops & 0xF)
    {
    case 0x1:
      printd(4,"    Affect control bits only\n",0);
      break;
    case 0x8:
      printd(4,"    Affect flag bits only\n",0);
      break;
    case 0x9:
      printd(4,"    Affect flag and control bits only\n",0);
      break;
    default:
      printd(4,"    Invalid range\n",0);
      break;
    }

    switch (code_op & 0xF0000)
    {
    case ARM_OP2_REG:
      printd(4,"    Source/Destination: R%d\n",((code_ops & 0xF00)>>8));
      break;
    case ARM_OP2_CONST:
      {
        i32 val,ror;

        val=((code_ops>>8) & 0xFF);
        ror=((code_ops>>15) & 0x1E);

        printd(4,"    Source is a constant (%d,&",ROR(val,ror));
        printd(4,"    %X)\n",ROR(val,ror));
      }
    }
  }

  if ((code_op & 0xFF)==ARM_OP_UNDEF)
  {
    printd(4,"    Oh, dear...\n",0);
  }

  return instr;
}



/* -------------------------------------------------------------------
   i32 system_decode_print

   Sends to an input buffer a string representation of a
   *system_instr.
------------------------------------------------------------------- */
i32 system_decode_print (char *output,struct system_instr *instr)
{
  i32f op,ops;
  char *instrs[]={"AND","EOR","SUB","RSB","ADD","ADC","SBC","RSC",
                  "TST","TEQ","CMP","CMN","ORR","MOV","BIC","MVN",
                  "MUL","MLA","B","BL","XXX","STR","LDR","STM","LDM",
                  "XXX","XXX","XXX","XXX","XXX","XXX","MRS","MSR"},
       *conds[]={"EQ","NE","CS","CC","MI","PL","VS","VC",
                 "HI","LS","GE","LT","GT","LE","","NV"},
       *output_internal;
  i32 loop;

  op=(*instr).op;
  ops=(*instr).ops;
  output_internal=output;

  if ((op & 0xFF)==ARM_OP_UNDEF)
  {
    return oprintf(output_internal,"Undefined instruction &amp;%X",(*instr).word);
  }

  output_internal+=oprintf(output_internal,"%s",instrs[op & 0xFF]);

  if ((op & 0xFF)<0x10)
  {
    if ((op & 0xF)==ARM_OP_MOV || (op & 0xF)==ARM_OP_MVN)
    {
      output_internal+=
      oprintf(output_internal,"%s%s R%d,",
              (op & ARM_OP_S)==0 ? "" : "S",conds[(op>>8) & 0xF],
              ops & 0xF);
    }
    else if ((op & 0xC)==0x8)
    {
      output_internal+=
      oprintf(output_internal,"%s R%d,",conds[(op>>8) & 0xF],
              (ops>>4) & 0xF);
    }
    else
    {
      output_internal+=
      oprintf(output_internal,"%s%s R%d,R%d,",
              (op & ARM_OP_S)==0 ? "" : "S",conds[(op>>8) & 0xF],
              ops & 0xF,(ops>>4) & 0xF);
    }

    switch (op & (0xF<<16))
    {
    case ARM_OP2_CONST:
      output_internal+=oprintf(output_internal,"#%d",
                               ROR((ops>>8) & 0xFF,(ops>>15) & 0x1E));
      break;
    case ARM_OP2_REG:
      output_internal+=oprintf(output_internal,"R%d",(ops>>8) & 0xF);
      break;
    case ARM_OP2_REG__LSL_REG:
      output_internal+=oprintf(output_internal,"R%d,LSL R%d",
                               (ops>>8) & 0xF,(ops>>12) & 0xF);
      break;
    case ARM_OP2_REG__LSR_REG:
      output_internal+=oprintf(output_internal,"R%d,LSR R%d",
                               (ops>>8) & 0xF,(ops>>12) & 0xF);
      break;
    case ARM_OP2_REG__ASR_REG:
      output_internal+=oprintf(output_internal,"R%d,ASR R%d",
                               (ops>>8) & 0xF,(ops>>12) & 0xF);
      break;
    case ARM_OP2_REG__ROR_REG:
      output_internal+=oprintf(output_internal,"R%d,ROR R%d",
                               (ops>>8) & 0xF,(ops>>12) & 0xF);
      break;
    case ARM_OP2_REG__LSL_CONST:
      output_internal+=oprintf(output_internal,"R%d,LSL #%d",
                               (ops>>8) & 0xF,(ops>>12) & 0x1F);
      break;
    case ARM_OP2_REG__LSR_CONST:
      output_internal+=oprintf(output_internal,"R%d,LSR #%d",
                               (ops>>8) & 0xF,
                               ((ops>>12) & 0x1F)==0 ? 32 :
                                ((ops>>12) & 0x1F));
      break;
    case ARM_OP2_REG__ASR_CONST:
      output_internal+=oprintf(output_internal,"R%d,ASR #%d",
                               (ops>>8) & 0xF,
                               ((ops>>12) & 0x1F)==0 ? 32 :
                                ((ops>>12) & 0x1F));
      break;
    case ARM_OP2_REG__ROR_CONST:
      output_internal+=oprintf(output_internal,"R%d,ROR #%d",
                               (ops>>8) & 0xF,(ops>>12) & 0x1F);
      break;
    case ARM_OP2_REG__RRX:
      output_internal+=oprintf(output_internal,"R%d,RRX",
                               (ops>>8) & 0xF);
    }
  }
  else
  {
    switch (op & 0xFF)
    {
    case ARM_OP_MUL:
      output_internal+=oprintf(output_internal,"%s%s R%d,R%d,R%d",
                               (op & ARM_OP_S)==0 ? "" : "S",
                               conds[(op>>8) & 0xF],ops & 0xF,
                               (ops>>4) & 0xF,(ops>>8) & 0xF);
      break;
    case ARM_OP_MLA:
      output_internal+=oprintf(output_internal,"%s%s R%d,R%d,R%d,R%d",
                               (op & ARM_OP_S)==0 ? "" : "S",
                               conds[(op>>8) & 0xF],ops & 0xF,
                               (ops>>4) & 0xF,(ops>>8) & 0xF,
                               (ops>>12) & 0xF);
      break;
    case ARM_OP_B:
    case ARM_OP_BL:
      output_internal+=oprintf(output_internal,"%s R15+%d",
                               conds[(op>>8) & 0xF],ops);
      break;
    case ARM_OP_STR:
    case ARM_OP_LDR:
      output_internal+=oprintf(output_internal,"%s%s R%d,[R%d%s,",
                               ((ops>>22) & 0x1)==0 ? "" : "B",
                               conds[(op>>8) & 0xF],ops & 0xF,
                               (ops>>4) & 0xF,
                               ((ops>>24) & 0x1)==0 ? "]" : "");

      switch (op & (0xF<<16))
      {
      case ARM_OP2_CONST:
        output_internal+=oprintf(output_internal,"#%d",(ops>>8) &
                                                       0xFFF);
        break;
      case ARM_OP2_REG:
        output_internal+=oprintf(output_internal,"R%d",(ops>>8) &
                                                       0xF);
        break;
      case ARM_OP2_REG__LSL_CONST:
        output_internal+=oprintf(output_internal,"R%d,LSL #%d",
                                 (ops>>8) & 0xF,(ops>>12) & 0x1F);
        break;
      case ARM_OP2_REG__LSR_CONST:
        output_internal+=oprintf(output_internal,"R%d,LSR #%d",
                                 (ops>>8) & 0xF,
                                 ((ops>>12) & 0x1F)==0 ? 32 :
                                  ((ops>>12) & 0x1F));
        break;
      case ARM_OP2_REG__ASR_CONST:
        output_internal+=oprintf(output_internal,"R%d,ASR #%d",
                                 (ops>>8) & 0xF,
                                 ((ops>>12) & 0x1F)==0 ? 32 :
                                  ((ops>>12) & 0x1F));
        break;
      case ARM_OP2_REG__ROR_CONST:
        output_internal+=oprintf(output_internal,"R%d,ROR #%d",
                                 (ops>>8) & 0xF,(ops>>12) & 0x1F);
        break;
      case ARM_OP2_REG__RRX:
        output_internal+=oprintf(output_internal,"R%d,RRX",
                                 (ops>>8) & 0xF);
      }

      output_internal+=oprintf(output_internal,"%s%s",
                               ((ops>>24) & 0x1)==0 ? "" : "]",
                               (ops & ((1<<24) | (1<<21)))==
                               ((1<<24) | (1<<21)) ? "!" : "");
      break;

    case ARM_OP_STM:
    case ARM_OP_LDM:
      output_internal+=oprintf(output_internal,"%s%s%s R%d%s,{",
                               ((ops>>23) & 0x1)==0 ? "D" : "I",
                               ((ops>>24) & 0x1)==0 ? "A" : "B",
                               conds[(op>>8) & 0xF],(ops>>16) & 0xF,
                               ((ops>>21) & 0x1)==0 ? "" : "!");

      for (loop=0;loop<16;loop++)
      {
        if (((ops>>loop) & 0x1)!=0)
        {
          output_internal+=oprintf(output_internal,"R%d",loop);

          if (((ops & 0xFFFF) & (~((0x1<<(loop+1))-1)))!=0)
          {
            output_internal+=oprintf(output_internal,",");
          }
        }
      }

      output_internal+=oprintf(output_internal,"}");
    }
  }

  return (output_internal-output);
}



/* -------------------------------------------------------------------
   i32 system_decode2_print

   Sends to an input buffer a string representation of a
   *system_instr of the superscalar MicroOp format.
------------------------------------------------------------------- */
#ifdef ARM_SUPERSCALAR
i32 system_decode2_print (char *output,struct system_instr *instr)
{
  i32f op,ops;
  char *instrs[]={"AND","EOR","SUB","RSB","ADD","ADC","SBC","RSC",
                  "TST","TEQ","CMP","CMN","ORR","MOV","BIC","MVN",
                  "MUL","MLA","B","BL","XXX","STR","LDR","STM",
                  "LDM","XXX","XXX","XXX","XXX","XXX","XXX","MRS",
                  "MSR"},
         *conds[]={"EQ","NE","CS","CC","MI","PL","VS","VC",
                   "HI","LS","GE","LT","GT","LE","","NV"},
       *output_internal;
  i32 loop,temp;

  op=(*instr).op;
  ops=(*instr).ops;
  output_internal=output;

  if ((op & 0xFF)==ARM_OP_UNDEF)
  {
    return oprintf(output_internal,"Undefined instr &amp;%X",
                   (*instr).word);
  }

  output_internal+=oprintf(output_internal,"%s",instrs[op & 0xFF]);

  if ((op & 0xFF)<0x10)
  {
    if ((op & 0xF)==ARM_OP_MOV || (op & 0xF)==ARM_OP_MVN)
    {
      output_internal+=
      oprintf(output_internal,"%s%s R%d (PR%d),",
              (op & ARM_OP_S)==0 ? "" : "S",conds[(op>>8) & 0xF],
               ops & 0xF,(*instr).regs_wr[4]);
    }
    else if ((op & 0xC)==0x8)
    {
      output_internal+=
      oprintf(output_internal,"%s R%d (PR%d),",conds[(op>>8) & 0xF],
              (ops>>4) & 0xF,(*instr).regs_rd[4]);
    }
    else
    {
      output_internal+=
      oprintf(output_internal,"%s%s R%d (PR%d),R%d (PR%d),",
              (op & ARM_OP_S)==0 ? "" : "S",conds[(op>>8) & 0xF],
               ops & 0xF,(*instr).regs_wr[4],(ops>>4) & 0xF,
               (*instr).regs_rd[4]);
    }

    switch (op & (0xF<<16))
    {
    case ARM_OP2_CONST:
      output_internal+=oprintf(output_internal,"#%d",
                               ROR((ops>>8) & 0xFF,(ops>>15) & 0x1E));
      break;
    case ARM_OP2_REG:
      output_internal+=oprintf(output_internal,"R%d (PR%d)",
                               (ops>>8) & 0xF,(*instr).regs_rd[5]);
      break;
    case ARM_OP2_REG__LSL_REG:
      output_internal+=oprintf(output_internal,"R%d (PR%d),LSL R%d (PR%d)",
                               (ops>>8) & 0xF,(*instr).regs_rd[5],
                               (ops>>12) & 0xF,(*instr).regs_rd[6]);
      break;
    case ARM_OP2_REG__LSR_REG:
      output_internal+=oprintf(output_internal,"R%d (PR%d),LSR R%d (PR%d)",
                               (ops>>8) & 0xF,(*instr).regs_rd[5],
                               (ops>>12) & 0xF,(*instr).regs_rd[6]);
      break;
    case ARM_OP2_REG__ASR_REG:
      output_internal+=oprintf(output_internal,"R%d (PR%d),ASR R%d (PR%d)",
                               (ops>>8) & 0xF,(*instr).regs_rd[5],
                               (ops>>12) & 0xF,(*instr).regs_rd[6]);
      break;
    case ARM_OP2_REG__ROR_REG:
      output_internal+=oprintf(output_internal,"R%d (PR%d),ROR R%d (PR%d)",
                               (ops>>8) & 0xF,(*instr).regs_rd[5],
                               (ops>>12) & 0xF,(*instr).regs_rd[6]);
      break;
    case ARM_OP2_REG__LSL_CONST:
      output_internal+=oprintf(output_internal,"R%d (PR%d),LSL #%d",
                               (ops>>8) & 0xF,(*instr).regs_rd[5],
                               (ops>>12) & 0x1F);
      break;
    case ARM_OP2_REG__LSR_CONST:
      output_internal+=oprintf(output_internal,"R%d (PR%d),LSR #%d",
                               (ops>>8) & 0xF,(*instr).regs_rd[5],
                               ((ops>>12) & 0x1F)==0 ? 32 :
                                ((ops>>12) & 0x1F));
      break;
    case ARM_OP2_REG__ASR_CONST:
      output_internal+=oprintf(output_internal,"R%d (PR%d),ASR #%d",
                               (ops>>8) & 0xF,(*instr).regs_rd[5],
                               ((ops>>12) & 0x1F)==0 ? 32 :
                                ((ops>>12) & 0x1F));
      break;
    case ARM_OP2_REG__ROR_CONST:
      output_internal+=oprintf(output_internal,"R%d (PR%d),ROR #%d",
                               (ops>>8) & 0xF,(*instr).regs_rd[5],
                               (ops>>12) & 0x1F);
      break;
    case ARM_OP2_REG__RRX:
      output_internal+=oprintf(output_internal,"R%d (PR%d),RRX",
                               (ops>>8) & 0xF,(*instr).regs_rd[5]);
    }
  }
  else
  {
    switch (op & 0xFF)
    {
    case ARM_OP_MUL:
      output_internal+=oprintf(output_internal,"%s%s R%d (PR%d),R%d (PR%d),R%d (PR%d)",
                               (op & ARM_OP_S)==0 ? "" : "S",
                               conds[(op>>8) & 0xF],ops & 0xF,
                               (*instr).regs_wr[4],(ops>>4) & 0xF,
                               (*instr).regs_rd[4],(ops>>8) & 0xF,
                               (*instr).regs_rd[5]);
      break;
    case ARM_OP_MLA:
      output_internal+=oprintf(output_internal,"%s%s R%d (PR%d),R%d (PR%d),R%d (PR%d),R%d (PR%d)",
                               (op & ARM_OP_S)==0 ? "" : "S",
                               conds[(op>>8) & 0xF],ops & 0xF,
                               (*instr).regs_wr[4],(ops>>4) & 0xF,
                               (*instr).regs_rd[4],(ops>>8) & 0xF,
                               (*instr).regs_rd[5],(ops>>12) & 0xF,
                               (*instr).regs_rd[6]);
      break;
    case ARM_OP_B:
    case ARM_OP_BL:
      output_internal+=oprintf(output_internal,"%s R15+%d",
                               conds[(op>>8) & 0xF],ops);
      break;
    case ARM_OP_STR:
    case ARM_OP_LDR:
/*      #define HIGHLIGHT ((op & 0xFF)==ARM_OP_LDR && (ops & 0xF)==3 && (op & (0xF<<16))==ARM_OP2_CONST && ((ops>>8) & 0xFFF)==4)*/
      #define HIGHLIGHT false

      if (HIGHLIGHT)
      {
        output_internal+=oprintf(output_internal,"<FONT color=red>");
      }

      if (((ops>>21) & 0x1)==0)
      {
        output_internal+=oprintf(output_internal,"%s%s R%d (PR%d),[R%d (PR%d)%s,",
                                 ((ops>>22) & 0x1)==0 ? "" : "B",
                                 conds[(op>>8) & 0xF],ops & 0xF,
                                 (op & 0xFF)==ARM_OP_LDR ?
                                 (*instr).regs_wr[4] :
                                 (*instr).regs_rd[4],
                                 (ops>>4) & 0xF,
                                 (*instr).regs_rd[5],
                                 ((ops>>24) & 0x1)==0 ? "]" : "");
      }
      else
      {
        output_internal+=oprintf(output_internal,"%s%s R%d (PR%d),[R%d (PR%d->PR%d)%s,",
                                 ((ops>>22) & 0x1)==0 ? "" : "B",
                                 conds[(op>>8) & 0xF],ops & 0xF,
                                 (op & 0xFF)==ARM_OP_LDR ?
                                 (*instr).regs_wr[4] :
                                 (*instr).regs_rd[4],
                                 (ops>>4) & 0xF,
                                 (*instr).regs_rd[5],
                                 (*instr).regs_wr[5],
                                 ((ops>>24) & 0x1)==0 ? "]" : "");
      }

      switch (op & (0xF<<16))
      {
      case ARM_OP2_CONST:
        output_internal+=oprintf(output_internal,"#%d",(ops>>8) &
                                                       0xFFF);
        break;
      case ARM_OP2_REG:
        output_internal+=oprintf(output_internal,"R%d (PR%d)",
                                 (ops>>8) & 0xF,
                                 (*instr).regs_rd[6]);
        break;
      case ARM_OP2_REG__LSL_CONST:
        output_internal+=oprintf(output_internal,"R%d (PR%d),LSL #%d",
                                 (ops>>8) & 0xF,(*instr).regs_rd[6],
                                 (ops>>12) & 0x1F);
        break;
      case ARM_OP2_REG__LSR_CONST:
        output_internal+=oprintf(output_internal,"R%d (PR%d),LSR #%d",
                                 (ops>>8) & 0xF,(*instr).regs_rd[6],
                                 ((ops>>12) & 0x1F)==0 ? 32 :
                                  ((ops>>12) & 0x1F));
        break;
      case ARM_OP2_REG__ASR_CONST:
        output_internal+=oprintf(output_internal,"R%d (PR%d),ASR #%d",
                                 (ops>>8) & 0xF,(*instr).regs_rd[6],
                                 ((ops>>12) & 0x1F)==0 ? 32 :
                                  ((ops>>12) & 0x1F));
        break;
      case ARM_OP2_REG__ROR_CONST:
        output_internal+=oprintf(output_internal,"R%d (PR%d),ROR #%d",
                                 (ops>>8) & 0xF,(*instr).regs_rd[6],
                                 (ops>>12) & 0x1F);
        break;
      case ARM_OP2_REG__RRX:
        output_internal+=oprintf(output_internal,"R%d (PR%d),RRX",
                                 (ops>>8) & 0xF,
                                 (*instr).regs_rd[6]);
      }

      output_internal+=oprintf(output_internal,"%s%s",
                               ((ops>>24) & 0x1)==0 ? "" : "]",
                               (ops & ((1<<24) | (1<<21)))==
                               ((1<<24) | (1<<21)) ? "!" : "");

      if (HIGHLIGHT)
      {
        output_internal+=oprintf(output_internal,"<FONT color=red>");
      }
      break;
    case ARM_OP_STM:
    case ARM_OP_LDM:
      if (((ops>>21) & 0x1)==0)
      {
        output_internal+=oprintf(output_internal,"%s%s%s R%d (PR%d),{",
                                 ((ops>>23) & 0x1)==0 ? "D" : "I",
                                 ((ops>>24) & 0x1)==0 ? "A" : "B",
                                 conds[(op>>8) & 0xF],(ops>>16) & 0xF,
                                 (*instr).regs_rd[4]);
      }
      else
      {
        output_internal+=oprintf(output_internal,"%s%s%s R%d (PR%d=>PR%d)!,{",
                                 ((ops>>23) & 0x1)==0 ? "D" : "I",
                                 ((ops>>24) & 0x1)==0 ? "A" : "B",
                                 conds[(op>>8) & 0xF],(ops>>16) & 0xF,
                                 (*instr).regs_rd[4],
                                 (*instr).regs_wr[4]);
      }

      temp=5;

      if ((op & 0xFF)==ARM_OP_LDM)
      {
        for (loop=0;loop<16;loop++)
        {
          if (((ops>>loop) & 0x1)!=0)
          {
            output_internal+=oprintf(output_internal,"R%d (PR%d)",
                                     loop,(*instr).regs_wr[temp]);
            temp++;

            if (((ops & 0xFFFF) & (~((0x1<<(loop+1))-1)))!=0)
            {
              output_internal+=oprintf(output_internal,",");
            }
          }
        }
      }
      else
      {
        for (loop=0;loop<16;loop++)
        {
          if (((ops>>loop) & 0x1)!=0)
          {
            output_internal+=oprintf(output_internal,"R%d (PR%d)",
                                     loop,(*instr).regs_rd[temp]);
            temp++;

            if (((ops & 0xFFFF) & (~((0x1<<(loop+1))-1)))!=0)
            {
              output_internal+=oprintf(output_internal,",");
            }
          }
        }
      }

      output_internal+=oprintf(output_internal,"}");
    }
  }


  switch ((*instr).rob_status)
  {
  case SYSTEM_INSTR_ROB_STATUS_ISSUED:
    output_internal+=oprintf(output_internal," (issued)");
    break;
  case SYSTEM_INSTR_ROB_STATUS_DONE_EXECUTED:
  case SYSTEM_INSTR_ROB_STATUS_DONE_UNEXECUTED:
    output_internal+=oprintf(output_internal," (completed)");
  }

  if (((*instr).serialising & SYSTEM_INSTR_SERIALISING_MISC)!=0)
  {
    output_internal+=oprintf(output_internal," (miscallenously serialising)");
  }

  #ifndef ARM_ROB_SPECEXEC_R15READ
  if (((*instr).serialising & SYSTEM_INSTR_SERIALISING_R15READ)!=0)
  {
    output_internal+=oprintf(output_internal," (serialising by PC read)");
  }
  #endif

  #ifndef ARM_ROB_SPECEXEC_R15READ
  if (((*instr).serialising & SYSTEM_INSTR_SERIALISING_R15WRITE)!=0)
  {
    output_internal+=oprintf(output_internal," (serialising by PC write)");
  }
  #else
  if (((*instr).serialising & SYSTEM_INSTR_SERIALISING_R15WRITE)!=0)
  {
    output_internal+=oprintf(output_internal," (semi-serialising by PC write)");
  }
  #endif

  return (output_internal-output);
}
#endif
