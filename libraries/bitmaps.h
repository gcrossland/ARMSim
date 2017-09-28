/* -------------------------------------------------------------------
   Bitmaps Library V1.00                                HEADER FILE
   (c) Geoffrey Crossland 2000

   V1.00 : Implements variable-length bitmap operations, including
           bit set, clear and invert and the logical operations or,
           and, eor and not.
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   General macro, variable and structure definitions
------------------------------------------------------------------- */
typedef i32f bitmap_unit;


#define BITMAP_UNIT 32U



/* -------------------------------------------------------------------
   External variables and routines
------------------------------------------------------------------- */
extern void *bitmaps_gen (i32 width);
extern void bitmaps_del (void *bitmap);
extern void bitmaps_set (void *bitmap,i32 bit);
extern void bitmaps_clear (void *bitmap,i32 bit);
extern void bitmaps_inv (void *bitmap,i32 bit);
extern ui8 bitmaps_get (void *bitmap,i32 bit);
extern void bitmaps_wipe (void *bitmap,i32 width);
extern void bitmaps_copy (void *bitmap_dest,void *bitmap_op1,
                          i32 width);
extern void bitmaps_not (void *bitmap_dest,void *bitmap_op1,
                         i32 width);
extern void bitmaps_or (void *bitmap_dest,void *bitmap_op1,
                        void *bitmap_op2,i32 width);
extern void bitmaps_and (void *bitmap_dest,void *bitmap_op1,
                         void *bitmap_op2,i32 width);
extern void bitmaps_eor (void *bitmap_dest,void *bitmap_op1,
                         void *bitmap_op2,i32 width);
extern ui8 bitmaps_test_or (void *bitmap_op1,void *bitmap_op2,
                            i32 width);
extern ui8 bitmaps_test_and (void *bitmap_op1,void *bitmap_op2,
                             i32 width);
extern ui8 bitmaps_test_eor (void *bitmap_op1,void *bitmap_op2,
                             i32 width);
