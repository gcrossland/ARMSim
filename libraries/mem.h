/* -------------------------------------------------------------------
   Memory Handling Library V1.06                        HEADER FILE
   (c) Geoffrey Crossland 1999, 2000, 2002

   V1.00 : Simple memory allocation abstraction, including
           replacable ANSI C library and Win32 code.
   V1.05 : New support for a lazy allocator (under Win32) and
           delayed freeing.
   V1.06 : Reinstated ANSI C-based heap management.
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   General macro, variable and structure definitions
------------------------------------------------------------------- */
#define MEM_LAZY_start  0
#define MEM_LAZY_end    1
#define MEM_LAZY_next   2
#define MEM_LAZY_SIZE   4



/* -------------------------------------------------------------------
   External variables and routines
------------------------------------------------------------------- */
extern void *mem_alloc (i32 block_size);
extern void *mem_realloc (void *block,i32 block_size);
extern void mem_free (void *block);
extern void *mem_setlinear (i32 block_size);
extern void mem_clearlinear (void);

#ifdef WIN32A
extern void mem_lazy_init (void);
extern LONG mem_lazy_trap
               (struct _EXCEPTION_POINTERS *win32_exception_pointers);
extern void *mem_lazy_alloc (i32 block_size);
extern void mem_lazy_free (void *block);
#endif
