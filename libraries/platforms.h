/* -------------------------------------------------------------------
   Platform Header V1.08                                HEADER FILE
   (c) Geoffrey Crossland 1998, 1999, 2003

   V1.00 : Supports ARM and x86 machines.
   V1.01 : Now supports the StrongARM, reserves a x86 Win32
           platform option and defines a new general INTELARCH
           option.
   V1.02 : Observes the portability of Win32 software. Like I'm
           going to find an Alpha around here...
   V1.05 : Includes new types for characters (chr) and file handles
           (fileh). Also reworked the machine types.
   V1.06 : Now defines true and false, more for convenience than
           for anything else.
   V1.07 : Similarly, null has now been added here.
   V1.08 : Formalises the position (previously implicitly taken but
           never made concrete) that ANSI C implementations are
           used as a fallback when no appropriate platform-specific
           version is provided. The effect is that, while there is
           now a new ANSI definition option, most existing code
           conforms to the rule already (since the standard pattern
           is #ifdef WIN32/#else/#endif).
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Constants, library includes and headers and... stuff...
------------------------------------------------------------------- */
/*#define RISCOS_ARM
#define RISCOS_ARM_STRONG
#define DOS_IA_32
#define WIN32_IA_32*/


#define true   1
#define false  0
#define null   ((void*) 0)



/* -------------------------------------------------------------------
   Configurations for pre-built platform names and other
   implications
------------------------------------------------------------------- */
/* Platform Types:
   ===============

   OS Types:
   ---------
   RISCOS
   DOS
   WIN32
   ANSI

   Processor Types:
   ----------------
   ARM (Subgroups: STRONG)
   IA (Subgroups: 16,32)
*/
#ifdef RISCOS_ARM
#define RISCOS
#define ARM
#endif

#ifdef RISCOS_ARM_STRONG
#define RISCOS
#define ARM
#define ARM_STRONG
#endif

#ifdef DOS_IA_32
#define DOS
#define IA
#define IA_32
#endif

#ifdef WIN32_IA_32
#ifndef WIN32
#define WIN32
#endif
#define IA
#define IA_32
#endif

#ifndef RISCOS
#ifndef DOS
#ifndef WIN32
#ifndef ANSI
#define ANSI
#endif
#endif
#endif
#endif



/* -------------------------------------------------------------------
   Platform-specific declarations
------------------------------------------------------------------- */
#ifdef RISCOS
typedef char chr;
typedef FILE *fileh;
#endif

#ifdef DOS
typedef char chr;
typedef FILE *fileh;
#endif

#ifdef ANSI
typedef char chr;
typedef FILE *fileh;
#endif

#ifdef WIN32A
typedef TCHAR chr;
/* typedef HANDLE fileh; for Win32 file handling routines */
typedef FILE *fileh;                         /* for ANSI C routines */
#endif


#ifdef ARM
typedef signed long long int i64f;
typedef signed int i32f;
typedef signed short int i16f;
typedef signed char i8f;
typedef unsigned long long int ui64f;
typedef unsigned int ui32f;
typedef unsigned short int ui16f;
typedef unsigned char ui8f;

typedef double ieee64f;
typedef float ieee32f;
#ifdef REAL_FLOAT
typedef ieee64f f64f;
typedef ieee32f f32f;
#endif
#ifdef REAL_FIXED
typedef i64f f64f;
typedef i32f f32f;
#endif

typedef i64f i64;
typedef i32f i32;
typedef i32f i16;
typedef i32f i8;
typedef ui64f ui64;
typedef ui32f ui32;
typedef i32f ui16;
typedef i32f ui8;

typedef ieee64f ieee64;
typedef ieee32f ieee32;
#ifdef REAL_FLOAT
typedef f64f f64;
typedef f32f f32;
#endif
#ifdef REAL_FIXED
typedef i64f f64;
typedef i32f f32;
#endif

typedef i32 p32;
typedef ui16f p16;
typedef ui8f p8;
#endif

#ifdef IA
typedef signed long long int i64f;
typedef signed int i32f;
typedef signed short int i16f;
typedef signed char i8f;
typedef unsigned long long int ui64f;
typedef unsigned int ui32f;
typedef unsigned short int ui16f;
typedef unsigned char ui8f;

typedef double ieee64f;
typedef float ieee32f;
#ifdef REAL_FLOAT
typedef ieee64f f64f;
typedef ieee32f f32f;
#endif
#ifdef REAL_FIXED
typedef i64f f64f;
typedef i32f f32f;
#endif

typedef i64f i64;
typedef i32f i32;
typedef i32f i16;
typedef i32f i8;
typedef ui64f ui64;
typedef ui32f ui32;
typedef i32f ui16;
typedef i32f ui8;

typedef ieee64f ieee64;
typedef ieee32f ieee32;
#ifdef REAL_FLOAT
typedef f64f f64;
typedef f32f f32;
#endif
#ifdef REAL_FIXED
typedef i64f f64;
typedef i32f f32;
#endif

typedef i32 p32;
typedef ui16f p16;
typedef ui8f p8;
#endif
