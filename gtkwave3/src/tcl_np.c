/*
 * Copyright (c) 2003-2005 Active State Corporation.
 * See the file LICENSE.TXT for information on usage and redistribution
 * and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <config.h>
#include "globals.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "tcl_np.h"
#include "tcl_helper.h"

#if !defined __MINGW32__ && !defined _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef HAVE_LIBTCL


/* ======== Np... Begin */
#ifndef LIB_RUNTIME_DIR
#   define LIB_RUNTIME_DIR ""
#endif
#  define XP_UNIX 1

/*
 * Default directory in which to look for Tcl libraries.  The
 * symbol is defined by Makefile.
 */

static char defaultLibraryDir[sizeof(LIB_RUNTIME_DIR)+200] = LIB_RUNTIME_DIR;

#ifdef WIN32

/* #include "shlwapi.h" */

#  ifndef TCL_LIB_FILE
#     define TCL_LIB_FILE "tcl85.dll"
#  endif

/*
 * Reference to ourselves
 */
static HINSTANCE nptclInst = NULL;

/*
 *----------------------------------------------------------------------
 *
 * NpLoadLibrary --
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

extern int NpLoadLibrary(HMODULE *tclHandle, char *dllName, int dllNameSize) {
  char *envdll, libname[MAX_PATH];
  HMODULE handle = (HMODULE) NULL;
  
  /*
   * Try a user-supplied Tcl dll to start with.
   */
  envdll = getenv("TCL_PLUGIN_DLL");
  if (envdll != NULL) {
    NpLog("Attempt to load Tcl dll (TCL_PLUGIN_DLL) '%s'\n", envdll);
    handle = LoadLibrary(envdll);
    if (handle) {
      memcpy(libname, envdll, MAX_PATH);
    }
  }

  if (!handle) {
    /*
     * Try based on full path.
     */
    snprintf(libname, MAX_PATH, "%stcl%d%d.dll", defaultLibraryDir, 
	     TCL_MAJOR_VERSION, TCL_MINOR_VERSION);
    NpLog("Attempt to load Tcl dll (default) '%s'\n", libname);
    handle = LoadLibrary(libname);
  }

  if (!handle) {
    /*
     * Try based on anywhere in the path.
     */
    snprintf(libname, MAX_PATH, "tcl%d%d.dll", TCL_MAJOR_VERSION, 
	     TCL_MINOR_VERSION);
    NpLog("Attempt to load Tcl dll (libpath) '%s'\n", libname);
    handle = LoadLibrary(libname);
  }

  if (!handle) {
    /*
     * Try based on ActiveTcl registry entry
     */
    char path[MAX_PATH], vers[MAX_PATH];
    DWORD result, size = MAX_PATH;
    HKEY regKey;
#  define TCL_REG_DIR_KEY "Software\\ActiveState\\ActiveTcl"
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TCL_REG_DIR_KEY, 0,
			  KEY_READ, &regKey);
    if (result != ERROR_SUCCESS) {
      NpLog("Could not access registry \"HKLM\\%s\"\n", TCL_REG_DIR_KEY);
      
      result = RegOpenKeyEx(HKEY_CURRENT_USER, TCL_REG_DIR_KEY, 0,
			    KEY_READ, &regKey);
      if (result != ERROR_SUCCESS) {
	NpLog("Could not access registry \"HKCU\\%s\"\n", TCL_REG_DIR_KEY);
	return TCL_ERROR;
      }
    }
    
    result = RegQueryValueEx(regKey, "CurrentVersion", NULL, NULL,
			     vers, &size);
    RegCloseKey(regKey);
    if (result != ERROR_SUCCESS) {
      NpLog("Could not access registry \"%s\" CurrentVersion\n",
	    TCL_REG_DIR_KEY);
      return TCL_ERROR;
    }
    
    snprintf(path, MAX_PATH, "%s\\%s", TCL_REG_DIR_KEY, vers);
    
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &regKey);
    if (result != ERROR_SUCCESS) {
      NpLog("Could not access registry \"%s\"\n", path);
      return TCL_ERROR;
    }
    
    size = MAX_PATH;
    result = RegQueryValueEx(regKey, NULL, NULL, NULL, path, &size);
    RegCloseKey(regKey);
    if (result != ERROR_SUCCESS) {
      NpLog("Could not access registry \"%s\" Default\n", TCL_REG_DIR_KEY);
      return TCL_ERROR;
    }
    
    NpLog("Found current Tcl installation at \"%s\"\n", path);
    
    snprintf(libname, MAX_PATH, "%s\\bin\\tcl%d%d.dll", path,
	     TCL_MAJOR_VERSION, TCL_MINOR_VERSION);
    NpLog("Attempt to load Tcl dll (registry) '%s'\n", libname);
    handle = LoadLibrary(libname);
  }

  if (!handle) {
    NpLog("NpLoadLibrary: could not find dll '%s'\n", libname);
    return TCL_ERROR;
  }
  
  *tclHandle = handle;
  if (dllNameSize > 0) {
    /*
     * Use GetModuleFileName to ensure that we have a fully-qualified
     * path, no matter which route above succeeded.
     */
    if (!GetModuleFileNameA(handle, dllName, dllNameSize)) {
      int length;
      char *msgPtr;
      DWORD code = GetLastError();
      
      length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM
			      | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, code,
			      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			      (char *) &msgPtr, 0, NULL);
      NpLog("GetModuleFileNameA ERROR: %d (%s)\n", code,
	    (length == 0) ? "unknown error" : msgPtr);
      if (length > 0) {
	LocalFree(msgPtr);
      }
    }
  }
  return TCL_OK;
}

/*
 * DLL entry point
 */

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved) {
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    nptclInst = hDLL;
    break;
    
  case DLL_PROCESS_DETACH:
    nptclInst = NULL;
    break;
  }
  return TRUE;
}

#else 
/* !WIN32 */

#  include <string.h>
#  ifndef TCL_LIB_FILE
#    define TCL_LIB_FILE "libtcl" TCL_VERSION SHLIB_SUFFIX
#  endif
#  ifndef TCL_KIT_DLL
#    define TCL_KIT_DLL "tclplugin" SHLIB_SUFFIX
#  endif

/*
 * In some systems, like SunOS 4.1.3, the RTLD_NOW flag isn't defined
 * and this argument to dlopen must always be 1.  The RTLD_GLOBAL
 * flag is needed on some systems (e.g. SCO and UnixWare) but doesn't
 * exist on others;  if it doesn't exist, set it to 0 so it has no effect.
 */

#  ifndef RTLD_NOW
#     define RTLD_NOW 1
#  endif

#  ifndef RTLD_GLOBAL
#     define RTLD_GLOBAL 0
#  endif

/*
 *----------------------------------------------------------------------
 *
 * NpLoadLibrary --
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

EXTERN int NpLoadLibrary(HMODULE *tclHandle, char *dllName, int dllNameSize) {
  char *envdll, libname[MAX_PATH];
  HMODULE handle = (HMODULE) NULL;
  
  *tclHandle = NULL;
  
  /*
   * Try a user-supplied Tcl dll to start with.
   */
  envdll = getenv("TCL_PLUGIN_DLL");
  if (envdll != NULL) {
    NpLog("Attempt to load Tcl dll (TCL_PLUGIN_DLL) '%s'\n", envdll);
    handle = dlopen(envdll, RTLD_NOW | RTLD_GLOBAL);
    if (handle) {
      memcpy(libname, envdll, MAX_PATH);
    }
  }
  
  if (!handle) {
    /*
     * Try based on full path.
     */
    snprintf(libname, MAX_PATH, "%s%s", defaultLibraryDir, TCL_LIB_FILE);
    NpLog("Attempt to load Tcl dll (default) '%s'\n", libname);
    handle = dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
  }
  
  if (!handle) {
    /*
     * Try based on anywhere in the path.
     */
    strncpy(libname, TCL_LIB_FILE, MAX_PATH);
    NpLog("Attempt to load Tcl dll (libpath) '%s'\n", libname);
    handle = dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
  }
  
  if (!handle) {
    /*
     * Try different versions anywhere in the path.
     */
    char *pos;
    
    pos = strstr(libname, "tcl")+4;
    if (*pos == '.') {
      pos++;
    }
    *pos = '9'; /* count down from '8' to '4'*/
    while (!handle && (--*pos > '3')) {
      NpLog("Attempt to load Tcl dll (default_ver) '%s'\n", libname);
      handle = dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
    }
  }
  
  if (!handle) {
    NpPlatformMsg("Failed to load Tcl dll!", "NpCreateMainInterp");
    return TCL_ERROR;
  }
  
  *tclHandle = handle;
  if (dllNameSize > 0) {
#  ifdef HAVE_DLADDR
    /*
     * Use dladdr if possible to get the real libname we are loading.
     * Grab any symbol - we just need one for reverse mapping
     */
    int (* tcl_Init)(Tcl_Interp *) =
      (int (*)(Tcl_Interp *)) dlsym(handle, "Tcl_Init");
    Dl_info info;
    
    if (tcl_Init && dladdr(tcl_Init, &info)) {
      NpLog("using dladdr '%s' => '%s'\n", libname, info.dli_fname);
      snprintf(dllName, dllNameSize, info.dli_fname);
    } else
#  endif
      snprintf(dllName, dllNameSize, libname);
  }
  return TCL_OK;
}
#endif 
/* !WIN32 */

/* **** Cinterp */
/*
 * Static variables in this file:
 */

static char dllName[MAX_PATH] = "";

#ifdef USE_TCL_STUBS
static HMODULE tclHandle = NULL;	/* should be the same in any thread */
static int tclHandleCnt  = 0;		/* only close on last count */
static int (* tcl_createThread)(Tcl_ThreadId *, Tcl_ThreadCreateProc,
	ClientData, int, int) = NULL;
#endif

static Tcl_Interp * (* tcl_createInterp)() = NULL;
static void (* tcl_findExecutable)(const char *) = NULL;
/*
 * We want the Tcl_InitStubs func static to ourselves - before Tcl
 * is loaded dynamically and possibly changes it.
 */
#ifdef USE_TCL_STUBS
static CONST char *(* tcl_initStubs)(Tcl_Interp *, CONST char *, int)
    = Tcl_InitStubs;
#endif

/*
 * We possibly have per-thread interpreters, as well as one constant, global
 * main intepreter.  The main interpreter runs from NP_Initialize to
 * NP_Shutdown.  tsd interps are used for each instance, but the main
 * interpreter will be used if it is in the same thread.
 *
 * XXX [hobbs]: While we have made some efforts to allow for multi-thread
 * safety, this is not currently in use.  Firefox (up to 1.5) runs all plugin
 * instances in one thread, and we have requested the same from the
 * accompanying pluginhostctrl ActiveX control.  The threading bits here are
 * mostly functional, but require marshalling via a master thread to guarantee
 * fully thread-safe operation.
 */
typedef struct ThreadSpecificData {
    Tcl_Interp *interp;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;
static Tcl_Interp *mainInterp = NULL;

/*
 *----------------------------------------------------------------------
 *
 * NpInitInterp --
 *
 *	Initializes a main or instance interpreter.
 *
 * Results:
 *	A standard Tcl error code.
 *
 * Side effects:
 *	Initializes the interp.
 *
 *----------------------------------------------------------------------
 */

int NpInitInterp(Tcl_Interp *interp) {
  Tcl_Preserve((ClientData) interp);
  
  /*
   * Set sharedlib in interp while we are here.  This will be used to
   * base the location of the default pluginX.Y package in the stardll
   * usage scenario.
   */
  if (Tcl_SetVar2(interp, "plugin", "sharedlib", dllName, TCL_GLOBAL_ONLY)
      == NULL) {
    NpPlatformMsg("Failed to set plugin(sharedlib)!", "NpInitInterp");
    return TCL_ERROR;
  }
  
  /*
   * The plugin doesn't directly call Tk C APIs - it's all managed at
   * the Tcl level, so we can just pkg req Tk here instead of calling
   * Tk_InitStubs.
   */
  if (TCL_OK != Tcl_Init(interp)) {
    fprintf(stderr, "TCLINIT | Tcl_Init error: %s\n", 
	    Tcl_GetStringResult (GLOBALS->interp));
    exit(EXIT_FAILURE);
  }
  NpLog("Tcl_PkgRequire(\"Tk\", \"%s\", 0)\n", TK_VERSION);
  if (1 && Tcl_PkgRequire(interp, "Tk", TK_VERSION, 0) == NULL) {
    NpPlatformMsg(Tcl_GetStringResult(interp),
		  "NpInitInterp Tcl_PkgRequire(Tk)");
    NpPlatformMsg("Failed to create initialize Tk", "NpInitInterp");
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NpCreateMainInterp --
 *
 *	Create the main interpreter.
 *
 * Results:
 *	The pointer to the main interpreter.
 *
 * Side effects:
 *	Will panic if called twice. (Must call DestroyMainInterp in between)
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *NpCreateMainInterp() {
  ThreadSpecificData *tsdPtr;
  Tcl_Interp *interp;
  
#ifdef USE_TCL_STUBS
  /*
   * Determine the libname and version number dynamically
   */
  if (tclHandle == NULL) {
    /*
     * First see if some other part didn't already load Tcl.
     */
    /* DLSYM(tclHandle, "Tcl_CreateInterp", Tcl_Interp * (*)(), tcl_createInterp); */

    if ((tcl_createInterp == NULL)
	&& (NpLoadLibrary(&tclHandle, dllName, MAX_PATH) != TCL_OK)) {
      NpPlatformMsg("Failed to load Tcl dll!", "NpCreateMainInterp");
      return NULL;
    }
    NpLog("NpCreateMainInterp: Using dll '%s'\n", dllName);
    
    tclHandleCnt++;
    DLSYM(tclHandle, "Tcl_CreateInterp", Tcl_Interp * (*)(),
	  tcl_createInterp);
    if (tcl_createInterp == NULL) {
#ifndef WIN32
      char *error = dlerror();
      if (error != NULL) {
	NpPlatformMsg(error, "NpCreateMainInterp");
      }
#endif
      return NULL;
    }
    DLSYM(tclHandle, "Tcl_CreateThread", 
	  int (*)(Tcl_ThreadId *, Tcl_ThreadCreateProc, ClientData, int, int),
	  tcl_createThread);
    DLSYM(tclHandle, "Tcl_FindExecutable", void (*)(const char *),
	  tcl_findExecutable);

  } else {
    tclHandleCnt++;
  }
#else
  tcl_createInterp   = Tcl_CreateInterp;
  tcl_findExecutable = Tcl_FindExecutable;
#endif

  if (dllName[0] == '\0') {
#ifdef WIN32
    GetModuleFileNameA((HINSTANCE) tclHandle, dllName, MAX_PATH);
#elif defined(HAVE_DLADDR)
    Dl_info info;
    if (dladdr(tcl_createInterp, &info)) {
      NpLog("NpCreateMainInterp: using dladdr '%s' => '%s'\n",
	    dllName, info.dli_fname);
      snprintf(dllName, MAX_PATH, info.dli_fname);
    }
#endif
    }
  NpLog("Tcl_FindExecutable(%s)\n", dllName);
  tcl_findExecutable((dllName[0] == '\0') ? NULL : dllName);

  /*
   * We do not operate in a fully threaded environment.  The ActiveX
   * control is set for pure single-apartment threading and Firefox runs
   * that way by default.  Otherwise we would have to create a thread for
   * the main/master and marshall calls through it.
   *   Tcl_CreateThread(&tid, ThreadCreateProc, clientData,
   *     TCL_THREAD_STACK_DEFAULT, TCL_THREAD_JOINABLE);
   */
  interp = tcl_createInterp();
  if (interp == (Tcl_Interp *) NULL) {
    NpPlatformMsg("Failed to create main interpreter!",
		  "NpCreateMainInterp");
    return NULL;
  }
  
  /*
   * Until Tcl_InitStubs is called, we cannot make any Tcl API
   * calls without grabbing them by symbol out of the dll.
   * This will be Tcl_PkgRequire for non-stubs builds.
   */
#ifdef USE_TCL_STUBS
  NpLog("Tcl_InitStubs(%p)\n", interp);
  if (tcl_initStubs(interp, TCL_VERSION, 0) == NULL) {
    NpPlatformMsg("Failed to create initialize Tcl stubs!",
		  "NpCreateMainInterp");
    return NULL;
  }
#endif

  /*
   * From now until shutdown we need this interp alive, hence we
   * preserve it here and release it at NpDestroyInterp time.
   */
  
  tsdPtr = TCL_TSD_INIT(&dataKey);
  tsdPtr->interp = interp;
  mainInterp = interp;
  
  if (NpInitInterp(interp) != TCL_OK) {
    return NULL;
  }
  
  return interp;
}

/*
 *----------------------------------------------------------------------
 *
 * NpGetMainInterp --
 *
 *	Gets the main interpreter. It must exist or we panic.
 *
 * Results:
 *	The main interpreter.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *NpGetMainInterp() {
  if (mainInterp == NULL) {
    NpPanic("BUG: Main interpreter does not exist");
  }
  return mainInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * NpDestroyMainInterp --
 *
 *	Destroys the main interpreter and performs cleanup actions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys the main interpreter and unloads Tcl.
 *
 *----------------------------------------------------------------------
 */

void NpDestroyMainInterp() {
  /*
   * We are not going to use the main interpreter after this point
   * because this may be the last call from the browser.
   * Could possibly do this as a ThreadExitHandler, but that seems to
   * have race/order issues for reload in Firefox.
   */
  if (mainInterp) {
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    NpLog("Tcl_DeleteInterp(%p) MAIN\n", mainInterp);
    Tcl_DeleteInterp(mainInterp);
    Tcl_Release((ClientData) mainInterp);
    tsdPtr->interp = mainInterp = (Tcl_Interp *) NULL;
  }

  /*
   * We are done using Tcl, so call Tcl_Finalize to get it to unload
   * cleanly.  With stubs, we need to handle dll finalization.
   */
  
#ifdef USE_TCL_STUBS
  tclHandleCnt--;
  if (tclHandle && tclHandleCnt <= 0) {
    Tcl_Finalize();
    dlclose(tclHandle);
    tclHandle = NULL;
  } else {
    Tcl_ExitThread(0);
  }
#else
  Tcl_Finalize();
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * NpGetInstanceInterp --
 *
 *	Gets an instance interpreter.  If one doesn't exist, make a new one.
 *
 * Results:
 *	The main interpreter.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *NpGetInstanceInterp() {
  ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
  Tcl_Interp *interp;
  
  if (tsdPtr->interp != NULL) {
    NpLog("NpGetInstanceInterp - use main interp %p\n", tsdPtr->interp);
    return tsdPtr->interp;
  }
  
  interp = Tcl_CreateInterp();
  NpLog("NpGetInstanceInterp - create interp %p\n", interp);
  
  if (NpInitInterp(interp) != TCL_OK) {
    NpLog("NpGetInstanceInterp: NpInitInterp(%p) != TCL_OK\n", interp);
    return NULL;
  }

  /*
   * We rely on NpInit to inform the user if initialization failed.
   */
  #ifdef nodef
  if (NpInit(interp) != TCL_OK) {
    NpLog("NpGetInstanceInterp: NpInit(%p) != TCL_OK\n", interp);
    return NULL;
  }
  #endif
  return interp;
}

/*
 *----------------------------------------------------------------------
 *
 * NpDestroyInstanceInterp --
 *
 *	Destroys an instance interpreter and performs cleanup actions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys the main interpreter and unloads Tcl.
 *
 *----------------------------------------------------------------------
 */

void NpDestroyInstanceInterp(Tcl_Interp *interp) {
  ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
  
  if (tsdPtr->interp == interp) {
    NpLog("NpDestroyInstanceInterp(%p) - using main interp\n", interp);
    return;
  }
  NpLog("Tcl_DeleteInterp(%p) INSTANCE\n", interp);
  Tcl_DeleteInterp(interp);
  Tcl_Release((ClientData) interp);
}


/* ======== Np... End */

#else

static void dummy_compilation_unit(void)
{

}

#endif

/*
 * $Id$
 * $Log$
 * Revision 1.3  2009/10/26 22:44:01  gtkwave
 * output style fixes, remove double init for bluespec
 *
 * Revision 1.2  2009/10/24 01:46:32  gtkwave
 * remove dead code
 *
 * Revision 1.1  2009/10/24 01:45:16  gtkwave
 * initial version
 *
 */