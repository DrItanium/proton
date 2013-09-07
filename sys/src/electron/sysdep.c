   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.24  05/17/06            */
   /*                                                     */
   /*               SYSTEM DEPENDENT MODULE               */
   /*******************************************************/

/*************************************************************/
/* Purpose: Isolation of system dependent routines.          */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*      6.23: Modified GenOpen to check the file length      */
/*            against the system constant FILENAME_MAX.      */
/*                                                           */
/*      6.24: Support for run-time programs directly passing */
/*            the hash tables for initialization.            */
/*                                                           */
/*            Made gensystem functional for Xcode.           */ 
/*                                                           */
/*            Added BeforeOpenFunction and AfterOpenFunction */
/*            hooks.                                         */
/*                                                           */
/*            Added environment parameter to GenClose.       */
/*            Added environment parameter to GenOpen.        */
/*                                                           */
/*            Updated UNIX_V gentime functionality.          */
/*                                                           */
/*            Removed GenOpen check against FILENAME_MAX.    */
/*                                                           */
/*************************************************************/

#define _SYSDEP_SOURCE_

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#if WIN_MVC
#define _UNICODE
#define UNICODE 
#include <Windows.h>
#endif

#if WIN_MVC
#include <sys\types.h>
#include <sys\timeb.h>
#include <io.h>
#include <fcntl.h>
#include <limits.h>
#include <process.h>
#include <signal.h>
#endif



#if   UNIX_7 || WIN_GCC
#include <sys/types.h>
#include <sys/timeb.h>
#include <signal.h>
#endif

#if   UNIX_V || LINUX || DARWIN
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <signal.h>
#endif

#include "argacces.h"
#include "bmathfun.h"
#include "commline.h"
#include "conscomp.h"
#include "constrnt.h"
#include "constrct.h"
#include "cstrcpsr.h"
#include "emathfun.h"
#include "envrnmnt.h"
#include "filecom.h"
#include "iofun.h"
#include "memalloc.h"
#include "miscfun.h"
#include "multifld.h"
#include "multifun.h"
#include "parsefun.h"
#include "prccode.h"
#include "prdctfun.h"
#include "proflfun.h"
#include "prcdrfun.h"
#include "router.h"
#include "sortfun.h"
#include "strngfun.h"
#include "textpro.h"
#include "utility.h"
#include "watch.h"

#include "sysdep.h"

#if DEFFACTS_CONSTRUCT
#include "dffctdef.h"
#endif

#if DEFRULE_CONSTRUCT
#include "ruledef.h"
#endif

#if DEFGENERIC_CONSTRUCT
#include "genrccom.h"
#endif

#if DEFFUNCTION_CONSTRUCT
#include "dffnxfun.h"
#endif

#if DEFGLOBAL_CONSTRUCT
#include "globldef.h"
#endif

#if DEFTEMPLATE_CONSTRUCT
#include "tmpltdef.h"
#endif

#if OBJECT_SYSTEM
#include "classini.h"
#endif

#include "moduldef.h"

#if DEVELOPER
#include "developr.h"
#endif

/* Electron Includes */
#include "arch.h"
#include "binops.h"
#if !WIN_MVC
#include "shellvar.h"
#endif


/********************/
/* ENVIRONMENT DATA */
/********************/

#define SYSTEM_DEPENDENT_DATA 58

struct systemDependentData
  { 
   void (*RedrawScreenFunction)(void *);
   void (*PauseEnvFunction)(void *);
   void (*ContinueEnvFunction)(void *,int);
/*
#if ! WINDOW_INTERFACE
#if WIN_MVC
   void (interrupt *OldCtrlC)(void);
   void (interrupt *OldBreak)(void);
#endif
#endif
*/
#if WIN_MVC
   int BinaryFileHandle;
   unsigned char getcBuffer[7];
   int getcLength;
   int getcPosition;
#endif
#if (! WIN_MVC)
   FILE *BinaryFP;
#endif
   int (*BeforeOpenFunction)(void *);
   int (*AfterOpenFunction)(void *);
   jmp_buf *jmpBuffer;
  };

#define SystemDependentData(theEnv) ((struct systemDependentData *) GetEnvironmentData(theEnv,SYSTEM_DEPENDENT_DATA))

/****************************************/
/* GLOBAL EXTERNAL FUNCTION DEFINITIONS */
/****************************************/

   extern void                    UserFunctions(void);
   extern void                    EnvUserFunctions(void *);

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    InitializeSystemDependentData(void *);
   static void                    SystemFunctionDefinitions(void *);
   static void                    InitializeKeywords(void *);
   static void                    InitializeNonportableFeatures(void *);
#if   (UNIX_V || LINUX || DARWIN || UNIX_7 || WIN_GCC || WIN_MVC) && (! WINDOW_INTERFACE)
   static void                    CatchCtrlC(int);
#endif
/*
#if   (WIN_MVC) && (! WINDOW_INTERFACE)
   static void interrupt          CatchCtrlC(void);
   static void                    RestoreInterruptVectors(void);
#endif
*/

/********************************************************/
/* InitializeSystemDependentData: Allocates environment */
/*    data for system dependent routines.               */
/********************************************************/
static void InitializeSystemDependentData(
  void *theEnv)
  {
   AllocateEnvironmentData(theEnv,SYSTEM_DEPENDENT_DATA,sizeof(struct systemDependentData),NULL);
  }

/**************************************************/
/* InitializeEnvironment: Performs initialization */
/*   of the KB environment.                       */
/**************************************************/
#if ALLOW_ENVIRONMENT_GLOBALS
globle void InitializeEnvironment()
   {
    if (GetCurrentEnvironment() == NULL)
      { CreateEnvironment(); }
   }
#endif

/*****************************************************/
/* EnvInitializeEnvironment: Performs initialization */
/*   of the KB environment.                          */
/*****************************************************/
globle void EnvInitializeEnvironment(
  void *vtheEnvironment,
  struct symbolHashNode **symbolTable,
  struct floatHashNode **floatTable,
  struct integerHashNode **integerTable,
  struct bitMapHashNode **bitmapTable,
  struct externalAddressHashNode **externalAddressTable)
  {
   struct environmentData *theEnvironment = (struct environmentData *) vtheEnvironment;
   
   /*================================================*/
   /* Don't allow the initialization to occur twice. */
   /*================================================*/

   if (theEnvironment->initialized) return;
     
   /*================================*/
   /* Initialize the memory manager. */
   /*================================*/

   InitializeMemory(theEnvironment);

   /*===================================================*/
   /* Initialize environment data for various features. */
   /*===================================================*/
   
   InitializeCommandLineData(theEnvironment);
#if CONSTRUCT_COMPILER && (! RUN_TIME)
   InitializeConstructCompilerData(theEnvironment);
#endif
   InitializeConstructData(theEnvironment);
   InitializeEvaluationData(theEnvironment);
   InitializeExternalFunctionData(theEnvironment);
   InitializeMultifieldData(theEnvironment);
   InitializePrettyPrintData(theEnvironment);
   InitializePrintUtilityData(theEnvironment);
   InitializeScannerData(theEnvironment);
   InitializeSystemDependentData(theEnvironment);
   InitializeUserDataData(theEnvironment);
   InitializeUtilityData(theEnvironment);
#if DEBUGGING_FUNCTIONS
   InitializeWatchData(theEnvironment);
#endif
   
   /*===============================================*/
   /* Initialize the hash tables for atomic values. */
   /*===============================================*/

   InitializeAtomTables(theEnvironment,symbolTable,floatTable,integerTable,bitmapTable,externalAddressTable);

   /*=========================================*/
   /* Initialize file and string I/O routers. */
   /*=========================================*/

   InitializeDefaultRouters(theEnvironment);

   /*=========================================================*/
   /* Initialize some system dependent features such as time. */
   /*=========================================================*/

   InitializeNonportableFeatures(theEnvironment);

   /* BEGIN ELECTRON */

   /*==================================*/
   /* Initialize Arch Detect Features. */
   /*==================================*/
   ArchitectureDetectionFunctionDefinitions(theEnvironment);

   /*===============================*/
   /* Initialize Logical Operations.*/
   /*===============================*/
   BinaryOperationsFunctionDefinitions(theEnvironment);

   /* END ELECTRON */

   /*=============================================*/
   /* Register system and user defined functions. */
   /*=============================================*/

   SystemFunctionDefinitions(theEnvironment);
   UserFunctions();
   EnvUserFunctions(theEnvironment);

   /*====================================*/
   /* Initialize the constraint manager. */
   /*====================================*/

   InitializeConstraints(theEnvironment);

   /*==========================================*/
   /* Initialize the expression hash table and */
   /* pointers to specific functions.          */
   /*==========================================*/

   InitExpressionData(theEnvironment);

   /*===================================*/
   /* Initialize the construct manager. */
   /*===================================*/

#if ! RUN_TIME
   InitializeConstructs(theEnvironment);
#endif

   /*=====================================*/
   /* Initialize the defmodule construct. */
   /*=====================================*/

   AllocateDefmoduleGlobals(theEnvironment);

   /*===================================*/
   /* Initialize the defrule construct. */
   /*===================================*/

#if DEFRULE_CONSTRUCT
   InitializeDefrules(theEnvironment);
#endif

   /*====================================*/
   /* Initialize the deffacts construct. */
   /*====================================*/

#if DEFFACTS_CONSTRUCT
   InitializeDeffacts(theEnvironment);
#endif

   /*=====================================================*/
   /* Initialize the defgeneric and defmethod constructs. */
   /*=====================================================*/

#if DEFGENERIC_CONSTRUCT
   SetupGenericFunctions(theEnvironment);
#endif

   /*=======================================*/
   /* Initialize the deffunction construct. */
   /*=======================================*/

#if DEFFUNCTION_CONSTRUCT
   SetupDeffunctions(theEnvironment);
#endif

   /*=====================================*/
   /* Initialize the defglobal construct. */
   /*=====================================*/

#if DEFGLOBAL_CONSTRUCT
   InitializeDefglobals(theEnvironment);
#endif

   /*=======================================*/
   /* Initialize the deftemplate construct. */
   /*=======================================*/

#if DEFTEMPLATE_CONSTRUCT
   InitializeDeftemplates(theEnvironment);
#endif

   /*=============================*/
   /* Initialize COOL constructs. */
   /*=============================*/

#if OBJECT_SYSTEM
   SetupObjectSystem(theEnvironment);
#endif

   /*=====================================*/
   /* Initialize the defmodule construct. */
   /*=====================================*/

   InitializeDefmodules(theEnvironment);

   /*======================================================*/
   /* Register commands and functions for development use. */
   /*======================================================*/

#if DEVELOPER
   DeveloperCommands(theEnvironment);
#endif

   /*=========================================*/
   /* Install the special function primitives */
   /* used by procedural code in constructs.  */
   /*=========================================*/

   InstallProcedurePrimitives(theEnvironment);

   /*==============================================*/
   /* Install keywords in the symbol table so that */
   /* they are available for command completion.   */
   /*==============================================*/

   InitializeKeywords(theEnvironment);

   /*========================*/
   /* Issue a clear command. */
   /*========================*/
   
   EnvClear(theEnvironment);

   /*=============================*/
   /* Initialization is complete. */
   /*=============================*/

   theEnvironment->initialized = TRUE;
  }

/******************************************************/
/* SetRedrawFunction: Sets the redraw screen function */
/*   for use with a user interface that may be        */
/*   overwritten by execution of a command.           */
/******************************************************/
globle void SetRedrawFunction(
  void *theEnv,
  void (*theFunction)(void *))
  {
   SystemDependentData(theEnv)->RedrawScreenFunction = theFunction;
  }

/******************************************************/
/* SetPauseEnvFunction: Set the normal state function */
/*   which puts terminal in a normal state.           */
/******************************************************/
globle void SetPauseEnvFunction(
  void *theEnv,
  void (*theFunction)(void *))
  {
   SystemDependentData(theEnv)->PauseEnvFunction = theFunction;
  }

/*********************************************************/
/* SetContinueEnvFunction: Sets the continue environment */
/*   function which returns the terminal to a special    */
/*   screen interface state.                             */
/*********************************************************/
globle void SetContinueEnvFunction(
  void *theEnv,
  void (*theFunction)(void *,int))
  {
   SystemDependentData(theEnv)->ContinueEnvFunction = theFunction;
  }

/*******************************************************/
/* GetRedrawFunction: Gets the redraw screen function. */
/*******************************************************/
globle void (*GetRedrawFunction(void *theEnv))(void *)
  {
   return SystemDependentData(theEnv)->RedrawScreenFunction;
  }

/*****************************************************/
/* GetPauseEnvFunction: Gets the normal state function. */
/*****************************************************/
globle void (*GetPauseEnvFunction(void *theEnv))(void *)
  {
   return SystemDependentData(theEnv)->PauseEnvFunction;
  }

/*********************************************/
/* GetContinueEnvFunction: Gets the continue */
/*   environment function.                   */
/*********************************************/
globle void (*GetContinueEnvFunction(void *theEnv))(void *,int)
  {
   return SystemDependentData(theEnv)->ContinueEnvFunction;
  }


/**************************************************/
/* SystemFunctionDefinitions: Sets up definitions */
/*   of system defined functions.                 */
/**************************************************/
static void SystemFunctionDefinitions(
  void *theEnv)
  {
   ProceduralFunctionDefinitions(theEnv);
   MiscFunctionDefinitions(theEnv);

#if IO_FUNCTIONS
   IOFunctionDefinitions(theEnv);
#endif

   PredicateFunctionDefinitions(theEnv);
   BasicMathFunctionDefinitions(theEnv);
   FileCommandDefinitions(theEnv);
   SortFunctionDefinitions(theEnv);

#if DEBUGGING_FUNCTIONS
   WatchFunctionDefinitions(theEnv);
#endif

#if MULTIFIELD_FUNCTIONS
   MultifieldFunctionDefinitions(theEnv);
#endif

#if STRING_FUNCTIONS
   StringFunctionDefinitions(theEnv);
#endif

#if EXTENDED_MATH_FUNCTIONS
   ExtendedMathFunctionDefinitions(theEnv);
#endif

#if TEXTPRO_FUNCTIONS || HELP_FUNCTIONS
   HelpFunctionDefinitions(theEnv);
#endif

#if CONSTRUCT_COMPILER && (! RUN_TIME)
   ConstructsToCCommandDefinition(theEnv);
#endif

#if PROFILING_FUNCTIONS
   ConstructProfilingFunctionDefinitions(theEnv);
#endif

   ParseFunctionDefinitions(theEnv);
  }
  
/*********************************************************/
/* gentime: A function to return a floating point number */
/*   which indicates the present time. Used internally   */
/*   for timing rule firings and debugging.              */
/*********************************************************/
globle double gentime()
  {
#if UNIX_V || DARWIN
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
   struct timespec now;
   clock_gettime(

#if defined(_POSIX_MONOTONIC_CLOCK)
       CLOCK_MONOTONIC,
#else
       CLOCK_REALTIME,
#endif
       &now);
  return (now.tv_nsec / 1000000000.0) + now.tv_sec;
#else
   struct timeval now;
   gettimeofday(&now, 0);
   return (now.tv_usec / 1000000.0) + now.tv_sec;
#endif

#elif LINUX
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) && defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199309L)
   struct timespec now;
   clock_gettime(

#if defined(_POSIX_MONOTONIC_CLOCK)
       CLOCK_MONOTONIC,
#else
       CLOCK_REALTIME,
#endif
       &now);
  return (now.tv_nsec / 1000000000.0) + now.tv_sec;
#else
   struct timeval now;
   gettimeofday(&now, 0);
   return (now.tv_usec / 1000000.0) + now.tv_sec;
#endif

#elif UNIX_7
   struct timeval now;
   gettimeofday(&now, 0);
   return (now.tv_usec / 1000000.0) + now.tv_sec;

#else
   return((double) clock() / (double) CLOCKS_PER_SEC);
#endif
  }

/*****************************************************/
/* gensystem: Generic routine for passing a string   */
/*   representing a command to the operating system. */
/*****************************************************/
globle void gensystem(
  void *theEnv)
  {
   char *commandBuffer = NULL;
   size_t bufferPosition = 0;
   size_t bufferMaximum = 0;
   int numa, i;
   DATA_OBJECT tempValue;
   char *theString;

   /*===========================================*/
   /* Check for the corret number of arguments. */
   /*===========================================*/

   if ((numa = EnvArgCountCheck(theEnv,(char*)"system",AT_LEAST,1)) == -1) return;

   /*============================================================*/
   /* Concatenate the arguments together to form a single string */
   /* containing the command to be sent to the operating system. */
   /*============================================================*/

   for (i = 1 ; i <= numa; i++)
     {
      EnvRtnUnknown(theEnv,i,&tempValue);
      if ((GetType(tempValue) != STRING) &&
          (GetType(tempValue) != SYMBOL))
        {
         SetHaltExecution(theEnv,TRUE);
         SetEvaluationError(theEnv,TRUE);
         ExpectedTypeError2(theEnv,(char*)"system",i);
         return;
        }

     theString = DOToString(tempValue);

     commandBuffer = AppendToString(theEnv,theString,commandBuffer,&bufferPosition,&bufferMaximum);
    }

   if (commandBuffer == NULL) return;

   /*=======================================*/
   /* Execute the operating system command. */
   /*=======================================*/

#if   UNIX_7 || UNIX_V || LINUX || DARWIN || WIN_MVC || WIN_GCC 
   if (SystemDependentData(theEnv)->PauseEnvFunction != NULL) (*SystemDependentData(theEnv)->PauseEnvFunction)(theEnv);
   system(commandBuffer);
   if (SystemDependentData(theEnv)->ContinueEnvFunction != NULL) (*SystemDependentData(theEnv)->ContinueEnvFunction)(theEnv,1);
   if (SystemDependentData(theEnv)->RedrawScreenFunction != NULL) (*SystemDependentData(theEnv)->RedrawScreenFunction)(theEnv);
#else

#endif

   /*==================================================*/
   /* Return the string buffer containing the command. */
   /*==================================================*/

   rm(theEnv,commandBuffer,bufferMaximum);

   return;
  }

/*******************************************/
/* gengetchar: Generic routine for getting */
/*    a character from stdin.              */
/*******************************************/
globle int gengetchar(
  void *theEnv)
  {
/*
#if WIN_MVC
   if (SystemDependentData(theEnv)->getcLength ==
       SystemDependentData(theEnv)->getcPosition)
     {
      TCHAR tBuffer = 0;
      DWORD count = 0;
      WCHAR wBuffer = 0;

      ReadConsole(GetStdHandle(STD_INPUT_HANDLE),&tBuffer,1,&count,NULL);
      
      wBuffer = tBuffer;
      
      SystemDependentData(theEnv)->getcLength = 
         WideCharToMultiByte(CP_UTF8,0,&wBuffer,1,
                             (char *) SystemDependentData(theEnv)->getcBuffer,
                             7,NULL,NULL);
                             
      SystemDependentData(theEnv)->getcPosition = 0;
     }
     
   return SystemDependentData(theEnv)->getcBuffer[SystemDependentData(theEnv)->getcPosition++];
#else
*/
   return(getc(stdin));
/*
#endif
*/
  }

/***********************************************/
/* genungetchar: Generic routine for ungetting */
/*    a character from stdin.                  */
/***********************************************/
globle int genungetchar(
  void *theEnv,
  int theChar)
  {
  /*
#if WIN_MVC
   if (SystemDependentData(theEnv)->getcPosition > 0)
     { 
      SystemDependentData(theEnv)->getcPosition--;
      return theChar;
     }
   else
     { return EOF; }
#else
*/
   return(ungetc(theChar,stdin));
/*
#endif
*/
  }

/****************************************************/
/* genprintfile: Generic routine for print a single */
/*   character string to a file (including stdout). */
/****************************************************/
globle void genprintfile(
  void *theEnv,
  FILE *fptr,
  char *str)
  {
   if (fptr != stdout)
     {
      fprintf(fptr,"%s",str);
      fflush(fptr);
     }
   else
     {
#if WIN_MVC
/*
      int rv;
      wchar_t *wbuffer;
      size_t len = strlen(str);

      wbuffer = genalloc(theEnv,sizeof(wchar_t) * (len + 1));
      rv = MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,str,-1,wbuffer,len+1);
      
      fwprintf(fptr,L"%ls",wbuffer);
      fflush(fptr);
      genfree(theEnv,wbuffer,sizeof(wchar_t) * (len + 1));
*/
      fprintf(fptr,"%s",str);
      fflush(fptr);
#else
      fprintf(fptr,"%s",str);
      fflush(fptr);
#endif
     }
  }
  
/***********************************************************/
/* InitializeNonportableFeatures: Initializes non-portable */
/*   features. Currently, the only non-portable feature    */
/*   requiring initialization is the interrupt handler     */
/*   which allows execution to be halted.                  */
/***********************************************************/
static void InitializeNonportableFeatures(
  void *theEnv)
  {
#if ! WINDOW_INTERFACE

#if UNIX_V || LINUX || DARWIN || UNIX_7 || WIN_GCC || WIN_MVC
   signal(SIGINT,CatchCtrlC);
#endif

#if !WIN_MVC
   /*==================================================*/
   /* Initialize Shell Variable Manipulation Functions */
   /*==================================================*/
   ShellVariableQueryFunctions(theEnv);
#endif

/*
#if WIN_MVC
   SystemDependentData(theEnv)->OldCtrlC = _dos_getvect(0x23);
   SystemDependentData(theEnv)->OldBreak = _dos_getvect(0x1b);
   _dos_setvect(0x23,CatchCtrlC);
   _dos_setvect(0x1b,CatchCtrlC);
   atexit(RestoreInterruptVectors);
#endif
*/
#endif
  }

/*************************************************************/
/* Functions For Handling Control C Interrupt: The following */
/*   functions handle interrupt processing for several       */
/*   machines. For the Macintosh control-c is not handle,    */
/*   but a function is provided to call periodically which   */
/*   calls SystemTask (allowing periodic tasks to be handled */
/*   by the operating system).                               */
/*************************************************************/

#if ! WINDOW_INTERFACE

#if   UNIX_V || LINUX || DARWIN || UNIX_7 || WIN_GCC || WIN_MVC || DARWIN
/**********************************************/
/* CatchCtrlC: VMS and UNIX specific function */
/*   to allow control-c interrupts.           */
/**********************************************/
static void CatchCtrlC(
  int sgnl)
  {
#if ALLOW_ENVIRONMENT_GLOBALS
   SetHaltExecution(GetCurrentEnvironment(),TRUE);
   CloseAllBatchSources(GetCurrentEnvironment());
#endif
   signal(SIGINT,CatchCtrlC);
  }
#endif

#if   WIN_MVC
/******************************************************/
/* CatchCtrlC: IBM Microsoft C and Borland Turbo C    */
/*   specific function to allow control-c interrupts. */
/******************************************************/
/*
static void interrupt CatchCtrlC()
  {
#if ALLOW_ENVIRONMENT_GLOBALS
   SetHaltExecution(GetCurrentEnvironment(),TRUE);
   CloseAllBatchSources(GetCurrentEnvironment());
#endif
  }
*/
/**************************************************************/
/* RestoreInterruptVectors: IBM Microsoft C and Borland Turbo */
/*   C specific function for restoring interrupt vectors.     */
/**************************************************************/
/*
static void RestoreInterruptVectors()
  {
#if ALLOW_ENVIRONMENT_GLOBALS
   void *theEnv;
   
   theEnv = GetCurrentEnvironment();

   _dos_setvect(0x23,SystemDependentData(theEnv)->OldCtrlC);
   _dos_setvect(0x1b,SystemDependentData(theEnv)->OldBreak);
#endif
  }
*/
#endif

#endif

/**************************************/
/* genexit:  A generic exit function. */
/**************************************/
globle void genexit(
  void *theEnv,
  int num)
  {
   if (SystemDependentData(theEnv)->jmpBuffer != NULL)
     { longjmp(*SystemDependentData(theEnv)->jmpBuffer,1); }
     
   exit(num);
  }

/**************************************/
/* SetJmpBuffer: */
/**************************************/
globle void SetJmpBuffer(
  void *theEnv,
  jmp_buf *theJmpBuffer)
  {
   SystemDependentData(theEnv)->jmpBuffer = theJmpBuffer;
  }
  
/******************************************/
/* genstrcpy: Generic genstrcpy function. */
/******************************************/
char *genstrcpy(
  char *dest,
  const char *src)
  {
   return strcpy(dest,src);
  }

/********************************************/
/* genstrncpy: Generic genstrncpy function. */
/********************************************/
char *genstrncpy(
  char *dest,
  const char *src,
  size_t n)
  {
   return strncpy(dest,src,n);
  }
  
/******************************************/
/* genstrcat: Generic genstrcat function. */
/******************************************/
char *genstrcat(
  char *dest,
  const char *src)
  {
   return strcat(dest,src);
  }

/********************************************/
/* genstrncat: Generic genstrncat function. */
/********************************************/
char *genstrncat(
  char *dest,
  const char *src,
  size_t n)
  {
   return strncat(dest,src,n);
  }
  
/*****************************************/
/* gensprintf: Generic sprintf function. */
/*****************************************/
int gensprintf(
  char *buffer,
  const char *restrictStr,
  ...)
  {
   va_list args;
   int rv;
   
   va_start(args,restrictStr);
   
   rv = vsprintf(buffer,restrictStr,args);
   
   va_end(args);
   
   return rv;
  }
  
/******************************************************/
/* genrand: Generic random number generator function. */
/******************************************************/
int genrand()
  {
   return(rand());
  }
  
/**********************************************************************/
/* genseed: Generic function for seeding the random number generator. */
/**********************************************************************/
globle void genseed(
  int seed)
  {
   srand((unsigned) seed);
  }

/*********************************************/
/* gengetcwd: Generic function for returning */
/*   the current directory.                  */
/*********************************************/
globle char *gengetcwd(
  char *buffer,
  int buflength)
  {
   return(getcwd(buffer,buflength));
  }

/****************************************************/
/* genremove: Generic function for removing a file. */
/****************************************************/
globle int genremove(
  char *fileName)
  {
   if (remove(fileName)) return(FALSE);

   return(TRUE);
  }

/****************************************************/
/* genrename: Generic function for renaming a file. */
/****************************************************/
globle int genrename(
  char *oldFileName,
  char *newFileName)
  {
   if (rename(oldFileName,newFileName)) return(FALSE);

   return(TRUE);
  }

/**************************************/
/* EnvSetBeforeOpenFunction: Sets the */
/*  value of BeforeOpenFunction.      */
/**************************************/
globle int (*EnvSetBeforeOpenFunction(void *theEnv,
                                      int (*theFunction)(void *)))(void *)
  {
   int (*tempFunction)(void *);

   tempFunction = SystemDependentData(theEnv)->BeforeOpenFunction;
   SystemDependentData(theEnv)->BeforeOpenFunction = theFunction;
   return(tempFunction);
  }

/*************************************/
/* EnvSetAfterOpenFunction: Sets the */
/*  value of AfterOpenFunction.      */
/*************************************/
globle int (*EnvSetAfterOpenFunction(void *theEnv,
                                     int (*theFunction)(void *)))(void *)
  {
   int (*tempFunction)(void *);

   tempFunction = SystemDependentData(theEnv)->AfterOpenFunction;
   SystemDependentData(theEnv)->AfterOpenFunction = theFunction;
   return(tempFunction);
  }

/*********************************************/
/* GenOpen: Trap routine for opening a file. */
/*********************************************/
globle FILE *GenOpen(
  void *theEnv,
  char *fileName,
  char *accessType)
  {
   FILE *theFile;
   
   if (SystemDependentData(theEnv)->BeforeOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->BeforeOpenFunction)(theEnv); }

#if WIN_MVC
#if _MSC_VER >= 1400
   fopen_s(&theFile,fileName,accessType);
#else
   theFile = fopen(fileName,accessType);
#endif
#else
   theFile = fopen(fileName,accessType);
#endif
   
   if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }
     
   return theFile;
  }
  
/**********************************************/
/* GenClose: Trap routine for closing a file. */
/**********************************************/
globle int GenClose(
  void *theEnv,
  FILE *theFile)
  {
   int rv;
   
   if (SystemDependentData(theEnv)->BeforeOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->BeforeOpenFunction)(theEnv); }

   rv = fclose(theFile);

   if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }
   
   return rv;
  }
  
/************************************************************/
/* GenOpenReadBinary: Generic and machine specific code for */
/*   opening a file for binary access. Only one file may be */
/*   open at a time when using this function since the file */
/*   pointer is stored in a global variable.                */
/************************************************************/
globle int GenOpenReadBinary(
  void *theEnv,
  char *funcName,
  char *fileName)
  {
   if (SystemDependentData(theEnv)->BeforeOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->BeforeOpenFunction)(theEnv); }

#if WIN_MVC

   SystemDependentData(theEnv)->BinaryFileHandle = _open(fileName,O_RDONLY | O_BINARY);
   if (SystemDependentData(theEnv)->BinaryFileHandle == -1)
     {
      if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
        { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }
      OpenErrorMessage(theEnv,funcName,fileName);
      return(FALSE);
     }
#endif

#if (! WIN_MVC)

   if ((SystemDependentData(theEnv)->BinaryFP = fopen(fileName,"rb")) == NULL)
     {
      if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
        { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }
      OpenErrorMessage(theEnv,funcName,fileName);
      return(FALSE);
     }
#endif

   if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }

   return(TRUE);
  }

/***********************************************/
/* GenReadBinary: Generic and machine specific */
/*   code for reading from a file.             */
/***********************************************/
globle void GenReadBinary(
  void *theEnv,
  void *dataPtr,
  size_t size)
  {
#if WIN_MVC
   char *tempPtr;

   tempPtr = (char *) dataPtr;
   while (size > INT_MAX)
     {
      _read(SystemDependentData(theEnv)->BinaryFileHandle,tempPtr,INT_MAX);
      size -= INT_MAX;
      tempPtr = tempPtr + INT_MAX;
     }

   if (size > 0) 
     { _read(SystemDependentData(theEnv)->BinaryFileHandle,tempPtr,(unsigned int) size); }
#endif

#if (! WIN_MVC)
   fread(dataPtr,size,1,SystemDependentData(theEnv)->BinaryFP); 
#endif
  }

/***************************************************/
/* GetSeekCurBinary:  Generic and machine specific */
/*   code for seeking a position in a file.        */
/***************************************************/
globle void GetSeekCurBinary(
  void *theEnv,
  long offset)
  {

#if WIN_MVC
   _lseek(SystemDependentData(theEnv)->BinaryFileHandle,offset,SEEK_CUR);
#endif

#if (! WIN_MVC)
   fseek(SystemDependentData(theEnv)->BinaryFP,offset,SEEK_CUR);
#endif
  }
  
/***************************************************/
/* GetSeekSetBinary:  Generic and machine specific */
/*   code for seeking a position in a file.        */
/***************************************************/
globle void GetSeekSetBinary(
  void *theEnv,
  long offset)
  {

#if WIN_MVC
   _lseek(SystemDependentData(theEnv)->BinaryFileHandle,offset,SEEK_SET);
#endif

#if (! WIN_MVC)
   fseek(SystemDependentData(theEnv)->BinaryFP,offset,SEEK_SET);
#endif
  }

/************************************************/
/* GenTellBinary:  Generic and machine specific */
/*   code for telling a position in a file.     */
/************************************************/
globle void GenTellBinary(
  void *theEnv,
  long *offset)
  {

#if WIN_MVC
   *offset = _lseek(SystemDependentData(theEnv)->BinaryFileHandle,0,SEEK_CUR);
#endif

#if (! WIN_MVC)
   *offset = ftell(SystemDependentData(theEnv)->BinaryFP);
#endif
  }

/****************************************/
/* GenCloseBinary:  Generic and machine */
/*   specific code for closing a file.  */
/****************************************/
globle void GenCloseBinary(
  void *theEnv)
  {
   if (SystemDependentData(theEnv)->BeforeOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->BeforeOpenFunction)(theEnv); }


#if WIN_MVC
   _close(SystemDependentData(theEnv)->BinaryFileHandle);
#endif

#if (! WIN_MVC)
   fclose(SystemDependentData(theEnv)->BinaryFP);
#endif

   if (SystemDependentData(theEnv)->AfterOpenFunction != NULL)
     { (*SystemDependentData(theEnv)->AfterOpenFunction)(theEnv); }
  }
  
/***********************************************/
/* GenWrite: Generic routine for writing to a  */
/*   file. No machine specific code as of yet. */
/***********************************************/
globle void GenWrite(
  void *dataPtr,
  size_t size,
  FILE *fp)
  {
   if (size == 0) return;
#if UNIX_7
   fwrite(dataPtr,size,1,fp);
#else
   fwrite(dataPtr,size,1,fp);
#endif
  }

/*********************************************/
/* InitializeKeywords: Adds key words to the */
/*   symbol table so that they are available */
/*   for command completion.                 */
/*********************************************/
static void InitializeKeywords(
  void *theEnv)
  {
#if (! RUN_TIME) && WINDOW_INTERFACE
   void *ts;

   /*====================*/
   /* construct keywords */
   /*====================*/

   ts = EnvAddSymbol(theEnv,"defrule");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"defglobal");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"deftemplate");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"deffacts");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"deffunction");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"defmethod");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"defgeneric");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"defclass");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"defmessage-handler");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"definstances");
   IncrementSymbolCount(ts);

   /*=======================*/
   /* set-strategy keywords */
   /*=======================*/

   ts = EnvAddSymbol(theEnv,"depth");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"breadth");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"lex");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"mea");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"simplicity");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"complexity");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"random");
   IncrementSymbolCount(ts);

   /*==================================*/
   /* set-salience-evaluation keywords */
   /*==================================*/

   ts = EnvAddSymbol(theEnv,"when-defined");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"when-activated");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"every-cycle");
   IncrementSymbolCount(ts);

   /*======================*/
   /* deftemplate keywords */
   /*======================*/

   ts = EnvAddSymbol(theEnv,"field");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"multifield");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"default");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"type");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"allowed-symbols");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"allowed-strings");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"allowed-numbers");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"allowed-integers");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"allowed-floats");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"allowed-values");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"min-number-of-elements");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"max-number-of-elements");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"NONE");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"VARIABLE");
   IncrementSymbolCount(ts);

   /*==================*/
   /* defrule keywords */
   /*==================*/

   ts = EnvAddSymbol(theEnv,"declare");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"salience");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"test");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"or");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"and");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"not");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"logical");
   IncrementSymbolCount(ts);

   /*===============*/
   /* COOL keywords */
   /*===============*/

   ts = EnvAddSymbol(theEnv,"is-a");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"role");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"abstract");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"concrete");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"pattern-match");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"reactive");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"non-reactive");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"slot");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"field");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"multiple");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"single");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"storage");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"shared");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"local");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"access");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"read");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"write");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"read-only");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"read-write");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"initialize-only");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"propagation");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"inherit");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"no-inherit");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"source");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"composite");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"exclusive");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"allowed-lexemes");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"allowed-instances");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"around");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"before");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"primary");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"after");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"of");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"self");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"visibility");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"override-message");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"private");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"public");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"create-accessor");
   IncrementSymbolCount(ts);

   /*================*/
   /* watch keywords */
   /*================*/

   ts = EnvAddSymbol(theEnv,"compilations");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"deffunctions");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"globals");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"rules");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"activations");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"statistics");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"facts");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"generic-functions");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"methods");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"instances");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"slots");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"messages");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"message-handlers");
   IncrementSymbolCount(ts);
   ts = EnvAddSymbol(theEnv,"focus");
   IncrementSymbolCount(ts);
#else
#endif
  }
