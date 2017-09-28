/* -------------------------------------------------------------------
   Memory Handling Library V1.06                        C COMPONENTS
   (c) Geoffrey Crossland 1999, 2000, 2002
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
#include "../header.h"



/* -------------------------------------------------------------------
   void *mem_alloc

   Allocates a block of memory.
------------------------------------------------------------------- */
ui8 mem_linear_status=false;
void *mem_linear_block=NULL;
void *mem_linear_block_internal=NULL;
/*struct mem_allocs
{
  void *ptr;
  i32 size;
};
struct mem_allocs *mem_alloceds=0;
i32 mem_alloctotal=0;*/


void *mem_alloc (i32 block_size)
{
  #ifdef WIN32A
  void *block;

  printd(1,"  about to allocate:\n",0);

  if (mem_linear_status==false)
  {
    if ((block=GlobalAlloc((UINT) (GMEM_FIXED | GMEM_NOCOMPACT |
                                   GMEM_ZEROINIT),
                           (DWORD) block_size))==NULL)
    {
      printd(1,"  allocation failed\n",0);
      return false;
    }
  }
  else
  {
    block=mem_linear_block_internal;
    mem_linear_block_internal+=block_size;
  }

  printd(1,"  At &%X.\n",block);
  #else
  void *block;
  i32 *clear_end,*clear_loop;

  if (mem_linear_status==false)
  {
    if ((block=malloc(((size_t) block_size)))==NULL)
    {
      return false;
    }
  }
  else
  {
    block=mem_linear_block_internal;
    mem_linear_block_internal+=block_size;
  }
  #endif

/*  if (mem_alloceds==0)
  {
    struct mem_allocs *bi,*be;
    bi=mem_alloceds=malloc(sizeof(struct mem_allocs)*10000);
    be=bi+10000;

    do
    {
      (*(bi++)).ptr=0;
    }
    while (bi<be);
  }

  {
    struct mem_allocs *bi=mem_alloceds;
    while ((*(bi++)).ptr!=0);
    (*(bi-1)).ptr=block;
    (*(bi-1)).size=block_size;
    mem_alloctotal+=block_size;
  }*/

  return block;
}



/* -------------------------------------------------------------------
   void *mem_realloc

   Changes the size of a block of memory, keeping as much of the
   original data as the new size of the block will allow.
------------------------------------------------------------------- */
void *mem_realloc (void *block,i32 block_size)
{
  #ifdef WIN32A
  if ((block=GlobalReAlloc(block,(DWORD) block_size,(UINT) 0))==NULL)
  {
    return false;
  }
  #else
  if ((block=realloc(block,((size_t) block_size)))==NULL)
  {
    return false;
  }
  #endif

  return block;
}



/* -------------------------------------------------------------------
   void mem_free

   Frees an allocated block.
------------------------------------------------------------------- */
void mem_free (void *block)
{
  #ifdef WIN32A
  GlobalFree(block);
  #else
  free(block);
  #endif

/*  {
    struct mem_allocs *bi=mem_alloceds;
    while ((*(bi++)).ptr!=block);
    (*(bi-1)).ptr=0;
    mem_alloctotal-=(*(bi-1)).size;
  }

  printd(128,"TOTAL ALLOC: %d bytes\n",mem_alloctotal);

  {
    struct mem_allocs *b,*bi,*be;

    printd(128,"Current Allocations:\n",0);

    bi=b=mem_alloceds;
    be=bi+10000;

    do
    {
      if ((*bi).ptr!=0)
      {
        printd(128,"&%X\n",(*bi).ptr);
      }
      bi++;
    }
    while (bi<be);

    printd(128,"\n\n",0);
  }*/

}



/* -------------------------------------------------------------------
   void mem_setlinear

   Sets the memory allocation routines into linear debug mode and
   grabs a linear block of the requested size.
------------------------------------------------------------------- */
void *mem_setlinear (i32 block_size)
{
  void *block;

  #ifdef WIN32A
  block=GlobalAlloc((UINT) (GMEM_FIXED | GMEM_NOCOMPACT |
                            GMEM_ZEROINIT),(DWORD) block_size);
  #else
  block=malloc(block_size);
  #endif

  if (block==NULL)
  {
    printd(1,"  linear allocation failed\n",0);
    return false;
  }

  mem_linear_status=true;

  return (mem_linear_block_internal=mem_linear_block=block);
}



/* -------------------------------------------------------------------
   void mem_clearlinear

   Disables linear debug allocation mode. This automatically frees
   any memory allocated in linear debug mode.
------------------------------------------------------------------- */
void mem_clearlinear (void)
{
  mem_linear_status=false;

  #ifdef WIN32A
  GlobalFree(mem_linear_block);
  #else
  free(mem_linear_block);
  #endif
}

 

/* -------------------------------------------------------------------
   void *mem_lazy_init

   Initialises the lazy allocation subsystem.
------------------------------------------------------------------- */
#ifdef WIN32A
i32 *mem_lazy_allocations_list_head=0,
    *mem_lazy_allocations_list_internal=0;
#endif



#ifdef WIN32A
void mem_lazy_init (void)
{
  i32 *list;

  list=mem_lazy_allocations_list_head=
  mem_lazy_allocations_list_internal=
  mem_alloc(MEM_LAZY_SIZE*sizeof(i32));

  *(list+MEM_LAZY_start)=0;
  *(list+MEM_LAZY_end)=0;
  *(list+MEM_LAZY_next)=0;

/*  SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)
                              &mem_lazy_trap);*/
  trapmanager_add((LPTOP_LEVEL_EXCEPTION_FILTER) &mem_lazy_trap);
}
#endif



/* -------------------------------------------------------------------
   LONG mem_lazy_trap

   The Win32 exception handler to trap address exceptions.
   Enumerates the lazy allocations list and compares the limits
   with the aborting address; if any match, a page is committed.
------------------------------------------------------------------- */
#ifdef WIN32A
LONG mem_lazy_trap
                (struct _EXCEPTION_POINTERS *win32_exception_pointers)
{
  EXCEPTION_RECORD *win32_exception_record;
  CONTEXT *win32_context;
  i32 addr,*list;

  printd(10,"\n\nENTERING mem_lazy_trap:\n",0);
  printd(10,"=======================\n",0);

  win32_exception_record=(*win32_exception_pointers).ExceptionRecord;
  win32_context=(*win32_exception_pointers).ContextRecord;

  if ((*win32_exception_record).ExceptionCode!=
      EXCEPTION_ACCESS_VIOLATION)
  {
    printd(10,"Not for us...\n",0);
    return EXCEPTION_CONTINUE_SEARCH;
  }

  addr=(i32) (*win32_exception_record).ExceptionInformation[1];

  printd(10,"Violation on accessing &%X\n",addr);

  list=mem_lazy_allocations_list_head;

  do
  {
    printd(10,"List pointer &%X:\n",list);

    if (addr>=*(list+MEM_LAZY_start) && addr<*(list+MEM_LAZY_end))
    {
      printd(10,"  A hit!\n",0);

      addr=addr & ~0xFFF;

      printd(10,"  Violation on accessing page &%X\n",addr);

      /*addr=*/

      VirtualAlloc((LPVOID) addr,(DWORD) 0x1000,(DWORD) MEM_COMMIT,
                   (DWORD) PAGE_EXECUTE_READWRITE);

      printd(10,"  Page committed at &%X\n",addr);

      return EXCEPTION_CONTINUE_EXECUTION;
    }

    list=(i32*) *(list+MEM_LAZY_next);
  }
  while (list!=0);

  printd(10,"Enumeration terminated\n",0);
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif



/* -------------------------------------------------------------------
   void *mem_lazy_alloc

   Lazily allocates a block of memory.
------------------------------------------------------------------- */
#ifdef WIN32A
void *mem_lazy_alloc (i32 block_size)
{
  i32 *list;
  void *block;

  printd(10,"\n\nENTERING mem_lazy_alloc:\n",0);
  printd(10,"========================\n",0);

  block=(void*) VirtualAlloc((LPVOID) NULL,(DWORD) block_size,
                             (DWORD) MEM_RESERVE,
                             (DWORD) PAGE_EXECUTE_READWRITE);

  printd(10,"Allocated &%X\n",block);
  printd(10,"Last Win32 error &%X\n",GetLastError());

  if (block!=((void*) 0))
  {
    list=(i32*) mem_alloc(MEM_LAZY_SIZE*sizeof(i32));

    printd(10,"New list element at &%X\n",list);

    *(list+MEM_LAZY_start)=(i32) block;
    *(list+MEM_LAZY_end)=((i32) block)+block_size;
    *(list+MEM_LAZY_next)=0;

    *(mem_lazy_allocations_list_internal+MEM_LAZY_next)=(i32) list;
    mem_lazy_allocations_list_internal=list;
  }

  return block;
}
#endif



/* -------------------------------------------------------------------
   void *mem_lazy_free

   Frees a lazily allocated block.
------------------------------------------------------------------- */
#ifdef WIN32A
void mem_lazy_free (void *block)
{
  i32 *list,*list_previous;

  printd(10,"\n\nENTERING mem_lazy_free:\n",0);
  printd(10,"=======================\n",0);

  list=mem_lazy_allocations_list_head;

  while (((void*) *((list=(i32*) *((list_previous=list)+
                                   MEM_LAZY_next))+MEM_LAZY_start))!=
         block);

  printd(10,"Releasing &%X\n",*(list+MEM_LAZY_start));

  *(list_previous+MEM_LAZY_next)=*(list+MEM_LAZY_next);

  VirtualFree((LPVOID) *(list+MEM_LAZY_start),
              (DWORD) (*(list+MEM_LAZY_end)-*(list+MEM_LAZY_start)),
              (DWORD) MEM_DECOMMIT);
  VirtualFree((LPVOID) *(list+MEM_LAZY_start),(DWORD) 0,
              (DWORD) MEM_RELEASE);

  mem_free((void*) list);
}
#endif
