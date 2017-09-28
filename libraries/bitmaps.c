/* -------------------------------------------------------------------
   Bitmaps Library V1.00                               C COMPONENTS
   (c) Geoffrey Crossland 2000
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "../header.h"



/* -------------------------------------------------------------------
   void *bitmaps_gen

   Generates a bitmap of the requested width.
------------------------------------------------------------------- */
extern void bp (int stream,void *bm,i32 width);
void bp (int stream,void *bm,i32 width)
{
  bitmap_unit *b;

  b=((bitmap_unit*) bm)+(width/BITMAP_UNIT);

  if ((width & (BITMAP_UNIT-1))!=0)
  {
    printdc(stream,"%08X",*b & ((1<<(width & (BITMAP_UNIT-1)))-1));
    width=width & ~(BITMAP_UNIT-1);
  }

  do
  {
    b--;
    printdc(stream,"%08X",*b);
  }
  while ((width-=BITMAP_UNIT)>0);
}


void *bitmaps_gen (i32 width)
{
  bitmap_unit *bitmap,*bitmap_end,*bitmap_internal;

  printd(60,"Entering bitmaps_gen:\n",0);

  width=1+(width/BITMAP_UNIT);

  bitmap_internal=bitmap=mem_alloc(sizeof(bitmap_unit)*width);
  bitmap_end=bitmap_internal+width;

  printd(128,"bitmap at &%X\n",bitmap);

  printd(60,"  allocated at &%X\n",bitmap);

  do
  {
    *(bitmap_internal++)=0;
  }
  while (bitmap_internal<bitmap_end);

  printd(60,"\n",0);

  return bitmap;
}



/* -------------------------------------------------------------------
   void bitmaps_del

   Deletes a bitmap.
------------------------------------------------------------------- */
void bitmaps_del (void *bitmap)
{
  printd(60,"Entering bitmaps_del:\n",0);

  printd(60,"  freeing &%X\n",bitmap);

  mem_free(bitmap);
}



/* -------------------------------------------------------------------
   void bitmaps_set

   Sets a particular bit in a bitmap.
------------------------------------------------------------------- */
void bitmaps_set (void *bitmap,i32 bit)
{
  bitmap_unit *element;

/*  element=((bitmap_unit*) bitmap)+(bit & ~(BITMAP_UNIT-1));*/
  element=((bitmap_unit*) bitmap)+(bit/BITMAP_UNIT);

  printd(60,"Entering bitmaps_set:\n",0);
  printd(60,"  Bit is %d\n",bit);
  printd(60,"  Offset is %d\n",element-((bitmap_unit*) bitmap));

  *element=*element | 1<<(bit & (BITMAP_UNIT-1));
}



/* -------------------------------------------------------------------
   void bitmaps_clear

   Clears a particular bit in a bitmap.
------------------------------------------------------------------- */
void bitmaps_clear (void *bitmap,i32 bit)
{
  bitmap_unit *element;

/*  element=((bitmap_unit*) bitmap)+(bit & ~(BITMAP_UNIT-1));*/
  element=((bitmap_unit*) bitmap)+(bit/BITMAP_UNIT);

  *element=*element & ~(1<<(bit & (BITMAP_UNIT-1)));
}



/* -------------------------------------------------------------------
   void bitmaps_inv

   Inverts a particular bit in a bitmap.
------------------------------------------------------------------- */
void bitmaps_inv (void *bitmap,i32 bit)
{
  bitmap_unit *element;

/*  element=((bitmap_unit*) bitmap)+(bit & ~(BITMAP_UNIT-1));*/
  element=((bitmap_unit*) bitmap)+(bit/BITMAP_UNIT);

  *element=*element ^ 1<<(bit & (BITMAP_UNIT-1));
}



/* -------------------------------------------------------------------
   ui8 bitmaps_get

   Returns the value in particular bit in a bitmap.
------------------------------------------------------------------- */
ui8 bitmaps_get (void *bitmap,i32 bit)
{
  bitmap_unit *element;

/*  element=((bitmap_unit*) bitmap)+(bit & ~(BITMAP_UNIT-1));*/
  element=((bitmap_unit*) bitmap)+(bit/BITMAP_UNIT);

  return ((ui8) (((*element)>>(bit & (BITMAP_UNIT-1))) & 0x1));
}



/* -------------------------------------------------------------------
   void bitmaps_wipe

   Sets all bits in a bitmap to zero.
------------------------------------------------------------------- */
void bitmaps_wipe (void *bitmap,i32 width)
{
  do
  {
    *((bitmap_unit*) bitmap)=0;

    bitmap=(void*) (((bitmap_unit*) bitmap)+1);
  }
  while ((width-=BITMAP_UNIT)>0);
}



/* -------------------------------------------------------------------
   void bitmaps_copy

   Copies the contents of one bitmap into another.
------------------------------------------------------------------- */
void bitmaps_copy (void *bitmap_dest,void *bitmap_op1,i32 width)
{
  do
  {
    *((bitmap_unit*) bitmap_dest)=*((bitmap_unit*) bitmap_op1);

    bitmap_dest=(void*) (((bitmap_unit*) bitmap_dest)+1);
    bitmap_op1=(void*) (((bitmap_unit*) bitmap_op1)+1);
  }
  while ((width-=BITMAP_UNIT)>0);
}



/* -------------------------------------------------------------------
   void bitmaps_not

   Logically NOTs a bitmaps, generates a new bitmap and returns the
   result in it.
------------------------------------------------------------------- */
void bitmaps_not (void *bitmap_dest,void *bitmap_op1,i32 width)
{
  do
  {
    *((bitmap_unit*) bitmap_dest)=~*((bitmap_unit*) bitmap_op1);

    bitmap_dest=(void*) (((bitmap_unit*) bitmap_dest)+1);
    bitmap_op1=(void*) (((bitmap_unit*) bitmap_op1)+1);
  }
  while ((width-=BITMAP_UNIT)>0);
}



/* -------------------------------------------------------------------
   void bitmaps_or

   Logically ORs two identically-long bitmaps together, generates a
   new bitmap and returns the result in it.
------------------------------------------------------------------- */
void bitmaps_or (void *bitmap_dest,void *bitmap_op1,void *bitmap_op2,
                 i32 width)
{
  i32 ow=width;
  bitmap_unit *b=bitmap_dest;

  printd(60,"Entering bitmaps_or:\n",0);

  printd(60,"  source bitmaps at &%X and &",bitmap_op1);
  printdc(60,"%X\n",bitmap_op2);
  printd(60,"  result bitmap at &%X\n",bitmap_dest);

  do
  {
    *((bitmap_unit*) bitmap_dest)=*((bitmap_unit*) bitmap_op1) |
                                  *((bitmap_unit*) bitmap_op2);

    bitmap_dest=(void*) (((bitmap_unit*) bitmap_dest)+1);
    bitmap_op1=(void*) (((bitmap_unit*) bitmap_op1)+1);
    bitmap_op2=(void*) (((bitmap_unit*) bitmap_op2)+1);
  }
  while ((width-=BITMAP_UNIT)>0);

  printd(60,"  now has the value &",0);
  bp(60,b,ow);

  printd(60,"\n",0);
}



/* -------------------------------------------------------------------
   void bitmaps_and

   Logically ANDs two identically-long bitmaps together, generates
   a new bitmap and returns the result in it.
------------------------------------------------------------------- */
void bitmaps_and (void *bitmap_dest,void *bitmap_op1,
                  void *bitmap_op2,i32 width)
{
  do
  {
    *((bitmap_unit*) bitmap_dest)=*((bitmap_unit*) bitmap_op1) &
                                  *((bitmap_unit*) bitmap_op2);

    bitmap_dest=(void*) (((bitmap_unit*) bitmap_dest)+1);
    bitmap_op1=(void*) (((bitmap_unit*) bitmap_op1)+1);
    bitmap_op2=(void*) (((bitmap_unit*) bitmap_op2)+1);
  }
  while ((width-=BITMAP_UNIT)>0);
}



/* -------------------------------------------------------------------
   void bitmaps_eor

   Logically EORs two identically-long bitmaps together, generates
   a new bitmap and returns the result in it.
------------------------------------------------------------------- */
void bitmaps_eor (void *bitmap_dest,void *bitmap_op1,
                  void *bitmap_op2,i32 width)
{
  do
  {
    *((bitmap_unit*) bitmap_dest)=*((bitmap_unit*) bitmap_op1) ^
                                  *((bitmap_unit*) bitmap_op2);

    bitmap_dest=(void*) (((bitmap_unit*) bitmap_dest)+1);
    bitmap_op1=(void*) (((bitmap_unit*) bitmap_op1)+1);
    bitmap_op2=(void*) (((bitmap_unit*) bitmap_op2)+1);
  }
  while ((width-=BITMAP_UNIT)>0);
}



/* -------------------------------------------------------------------
   ui8 bitmaps_test_or

   Logically ORs two identically-long bitmaps together, returning
   true if the result is 0 or false otherwise
------------------------------------------------------------------- */
ui8 bitmaps_test_or (void *bitmap_op1,void *bitmap_op2,i32 width)
{
  while (width>=BITMAP_UNIT)
  {
    if ((*((bitmap_unit*) bitmap_op1) |
         *((bitmap_unit*) bitmap_op2))!=0)
    {
      return false;
    }

    bitmap_op1=(void*) (((bitmap_unit*) bitmap_op1)+1);
    bitmap_op2=(void*) (((bitmap_unit*) bitmap_op2)+1);

    width-=BITMAP_UNIT;
  }

  if ((*((bitmap_unit*) bitmap_op1) & ((1<<width)-1) |
       *((bitmap_unit*) bitmap_op2) & ((1<<width)-1))!=0)
  {
    return false;
  }

  return true;
}



/* -------------------------------------------------------------------
   ui8 bitmaps_test_and

   Logically ANDs two identically-long bitmaps together, returning
   true if the result is 0 or false otherwise
------------------------------------------------------------------- */
ui8 bitmaps_test_and (void *bitmap_op1,void *bitmap_op2,i32 width)
{
  printd(60,"Entering bitmaps_test_and:\n",0);

  printd(60,"  op1 is at &%X\n",bitmap_op1);
  printd(60,"  op2 is at &%X\n",bitmap_op2);

  printd(60,"  op1: &",0);
  bp(60,bitmap_op1,width);
  printd(60,"\n",0);
  printd(60,"  op2: &",0);
  bp(60,bitmap_op2,width);
  printd(60,"\n",0);

  while (width>=BITMAP_UNIT)
  {
    if ((*((bitmap_unit*) bitmap_op1) &
         *((bitmap_unit*) bitmap_op2))!=0)
    {
      printd(60,"  Not zero\n",0);
      return false;
    }

    bitmap_op1=(void*) (((bitmap_unit*) bitmap_op1)+1);
    bitmap_op2=(void*) (((bitmap_unit*) bitmap_op2)+1);

    width-=BITMAP_UNIT;
  }

  printd(60,"  Going to partial:\n",0);

  if ((*((bitmap_unit*) bitmap_op1) & ((1<<width)-1) &
       *((bitmap_unit*) bitmap_op2) & ((1<<width)-1))!=0)
  {
    printd(60,"  Not zero\n",0);
    return false;
  }

  printd(60,"  Zero\n",0);

  return true;
}



/* -------------------------------------------------------------------
   ui8 bitmaps_test_eor

   Logically EORs two identically-long bitmaps together, returning
   true if the result is 0 or false otherwise
------------------------------------------------------------------- */
ui8 bitmaps_test_eor (void *bitmap_op1,void *bitmap_op2,i32 width)
{
  while (width>=BITMAP_UNIT)
  {
    if ((*((bitmap_unit*) bitmap_op1) ^
         *((bitmap_unit*) bitmap_op2))!=0)
    {
      return false;
    }

    bitmap_op1=(void*) (((bitmap_unit*) bitmap_op1)+1);
    bitmap_op2=(void*) (((bitmap_unit*) bitmap_op2)+1);

    width-=BITMAP_UNIT;
  }

  if ((*((bitmap_unit*) bitmap_op1) & ((1<<width)-1) ^
       *((bitmap_unit*) bitmap_op2) & ((1<<width)-1))!=0)
  {
    return false;
  }

  return true;
}
