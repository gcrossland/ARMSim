/* -------------------------------------------------------------------
   ARMSim V1.06                                  CONFIGURATION FILE
   (c) Geoffrey Crossland 2000, 2001, 2002
------------------------------------------------------------------- */


/*#define ARM_TRACE_CHECK  "trace.txt"*/

#define ARM_HANDICAP     0

#define ARM_LSU_BANDWIDTH  6

#define ARM_BRANCHPRED                     4
#define ARM_BRANCHPRED_ENTRIES             32
//#define ARM_BRANCHPRED_WAYASSOC_RANDOM
#define ARM_BRANCHPRED_WAYASSOC_LRU
#define ARM_BRANCHPRED_DYNAMIC_STRONGEST_NOTTAKEN  1
#define ARM_BRANCHPRED_DYNAMIC_WEAKEST_NOTTAKEN    1
#define ARM_BRANCHPRED_DYNAMIC_WEAKEST_TAKEN       2
#define ARM_BRANCHPRED_DYNAMIC_STRONGEST_TAKEN     3

#define ARM_RETURNPRED                  16

#define ARM_SUPERSCALAR
//#define ARM_SUPERSCALAR_SHORT
#ifdef ARM_SUPERSCALAR_SHORT
#define ARM_SUPERSCALAR
#endif
#define ARM_SUPERSCALAR_FETCH               2
#define ARM_SUPERSCALAR_REGS                32
#define ARM_SUPERSCALAR_ROB                 32
#define ARM_SUPERSCALAR_DATAPROC            2
#define ARM_SUPERSCALAR_MUL                 1
#define ARM_SUPERSCALAR_SINGLEMEM           2
#define ARM_SUPERSCALAR_MULTIMEM            1
#define ARM_SUPERSCALAR_BRANCH              0
#define ARM_SUPERSCALAR_PSRTRANS            0
#define ARM_SUPERSCALAR_RETIRE              2

//#define ARM_REGS_ALLOCATE_FROMTOP
#define ARM_REGS_ALLOCATE_CYCLIC

#define ARM_ROB_SPECEXEC
#define ARM_ROB_SPECEXEC_R15READ
#define ARM_ROB_SPECEXEC_R15WRITE

#define ARM_ROB_QUICKMEM

#define ARM_MULTIMEM_BANDWIDTH  2

#ifdef ARM_SUPERSCALAR
#define SYSTEM_CU_X                 1
#define SYSTEM_CU_Y                 1
#define SYSTEM_MEM_X                2
#define SYSTEM_MEM_Y                1
#define SYSTEM_FOCPRED_X            3
#define SYSTEM_FOCPRED_Y            1
#define SYSTEM_LSU_X                0
#define SYSTEM_LSU_Y                1
#ifdef ARM_SUPERSCALAR_SHORT
#define SYSTEM_FETCH_X              1
#define SYSTEM_FETCH_Y              0
#else
#define SYSTEM_FETCH_X              0
#define SYSTEM_FETCH_Y              0
#define SYSTEM_DECODE_X             1
#define SYSTEM_DECODE_Y             0
#endif
#define SYSTEM_DECODE2_X            2
#define SYSTEM_DECODE2_Y            0
#define SYSTEM_ROB_X                3
#define SYSTEM_ROB_Y                0
#define SYSTEM_DATAPROC_X(I)        4
#define SYSTEM_DATAPROC_Y(I)        (I)
#define SYSTEM_MUL_X(I)             4
#define SYSTEM_MUL_Y(I)             ((I)+ARM_SUPERSCALAR_DATAPROC)
#define SYSTEM_SINGLEMEM_X(I)       4
#define SYSTEM_SINGLEMEM_Y(I)       ((I)+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL)
#define SYSTEM_MULTIMEM_X(I)        4
#define SYSTEM_MULTIMEM_Y(I)        ((I)+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL+ARM_SUPERSCALAR_SINGLEMEM)
#define SYSTEM_BRANCH_X(I)          4
#define SYSTEM_BRANCH_Y(I)          ((I)+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL+ARM_SUPERSCALAR_SINGLEMEM+ARM_SUPERSCALAR_MULTIMEM)
#define SYSTEM_PSRTRANS_X(I)        4
#define SYSTEM_PSRTRANS_Y(I)        ((I)+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL+ARM_SUPERSCALAR_SINGLEMEM+ARM_SUPERSCALAR_MULTIMEM+ARM_SUPERSCALAR_BRANCH)
#define SYSTEM_RETIRE_X             5
#define SYSTEM_RETIRE_Y             0
#define SYSTEM_LSU_FETCH(I)         (I)
#define SYSTEM_LSU_SINGLEMEM(I)     ((I)+ARM_SUPERSCALAR_FETCH)
#define SYSTEM_LSU_MULTIMEM(I)      ((I)+ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_SINGLEMEM)
#define SYSTEM_LSU_count            (ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_SINGLEMEM+ARM_SUPERSCALAR_MULTIMEM*ARM_MULTIMEM_BANDWIDTH)
#define SYSTEM_FETCH_LSU(I)         (2*(I)+0)
#define SYSTEM_FETCH_DECODE(I)      (2*(I)+1)
#define SYSTEM_FETCH_count          (2*ARM_SUPERSCALAR_FETCH)
#ifdef ARM_SUPERSCALAR_SHORT
#define SYSTEM_DECODE2_DECODE(I)    (2*(I)+0)
#define SYSTEM_DECODE2_ROB(I)       (2*(I)+1)
#define SYSTEM_DECODE2_count        (2*ARM_SUPERSCALAR_FETCH)
#else
#define SYSTEM_DECODE_FETCH(I)      (2*(I)+0)
#define SYSTEM_DECODE_DECODE2(I)    (2*(I)+1)
#define SYSTEM_DECODE_count         (2*ARM_SUPERSCALAR_FETCH)
#define SYSTEM_DECODE2_DECODE(I)    (2*(I)+0)
#define SYSTEM_DECODE2_ROB(I)       (2*(I)+1)
#define SYSTEM_DECODE2_count        (2*ARM_SUPERSCALAR_FETCH)
#endif
#define SYSTEM_ROB_DECODE2(I)       (I)
#define SYSTEM_ROB_RETIRE(I)        ((I)+ARM_SUPERSCALAR_FETCH)
#define SYSTEM_ROB_DATAPROC(I)      ((I)+ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_RETIRE)
#define SYSTEM_ROB_MUL(I)           ((I)+ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_RETIRE+ARM_SUPERSCALAR_DATAPROC)
#define SYSTEM_ROB_SINGLEMEM(I)     ((I)+ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_RETIRE+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL)
#define SYSTEM_ROB_MULTIMEM(I)      ((I)+ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_RETIRE+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL+ARM_SUPERSCALAR_SINGLEMEM)
#define SYSTEM_ROB_BRANCH(I)        ((I)+ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_RETIRE+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL+ARM_SUPERSCALAR_SINGLEMEM+ARM_SUPERSCALAR_MULTIMEM)
#define SYSTEM_ROB_PSRTRANS(I)      ((I)+ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_RETIRE+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL+ARM_SUPERSCALAR_SINGLEMEM+ARM_SUPERSCALAR_MULTIMEM+ARM_SUPERSCALAR_BRANCH)
#define SYSTEM_ROB_count            (ARM_SUPERSCALAR_FETCH+ARM_SUPERSCALAR_RETIRE+ARM_SUPERSCALAR_DATAPROC+ARM_SUPERSCALAR_MUL+ARM_SUPERSCALAR_SINGLEMEM+ARM_SUPERSCALAR_MULTIMEM+ARM_SUPERSCALAR_BRANCH+ARM_SUPERSCALAR_PSRTRANS)
#define SYSTEM_EXECUTE_ROB          0
#define SYSTEM_EXECUTE_LSU(I)       ((I)+1)
#define SYSTEM_EXECUTE_count        (1+ARM_MULTIMEM_BANDWIDTH)
#define SYSTEM_RETIRE_ROB(I)        (I)
#define SYSTEM_RETIRE_count         ARM_SUPERSCALAR_RETIRE
#else
#define SYSTEM_CU_X            1
#define SYSTEM_CU_Y            1
#define SYSTEM_MEM_X           2
#define SYSTEM_MEM_Y           1
#define SYSTEM_FOCPRED_X       3
#define SYSTEM_FOCPRED_Y       1
#define SYSTEM_LSU_X           0
#define SYSTEM_LSU_Y           1
#define SYSTEM_FETCH_X         0
#define SYSTEM_FETCH_Y         0
#define SYSTEM_DECODE_X        1
#define SYSTEM_DECODE_Y        0
#define SYSTEM_EXECUTE_X       2
#define SYSTEM_EXECUTE_Y       0
#define SYSTEM_LSU_FETCH       0
#define SYSTEM_LSU_EXECUTE(I)  ((I)+1)
#define SYSTEM_LSU_count       (1+ARM_MULTIMEM_BANDWIDTH)
#define SYSTEM_FETCH_LSU       0
#define SYSTEM_FETCH_DECODE    1
#define SYSTEM_FETCH_count     2
#define SYSTEM_DECODE_FETCH    0
#define SYSTEM_DECODE_EXECUTE  1
#define SYSTEM_DECODE_count    2
#define SYSTEM_EXECUTE_DECODE  0
#define SYSTEM_EXECUTE_LSU(I)  ((I)+1)
#define SYSTEM_EXECUTE_count   (1+ARM_MULTIMEM_BANDWIDTH)
#endif

#define SYSTEM_CU_MEM_START  0x02000000
#define SYSTEM_CU_MEM_WORDS  8