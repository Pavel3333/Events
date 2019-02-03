
#ifndef Py_PYDEBUG_H
#define Py_PYDEBUG_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(int) Py_DebugFlag;
#define Py_DebugFlag (*Py_DebugFlag)

PyAPI_DATA(int) Py_VerboseFlag;
#define Py_VerboseFlag (*Py_VerboseFlag)

PyAPI_DATA(int) Py_InteractiveFlag;
#define Py_InteractiveFlag (*Py_InteractiveFlag)

PyAPI_DATA(int) Py_InspectFlag;
#define Py_InspectFlag (*Py_InspectFlag)

PyAPI_DATA(int) Py_OptimizeFlag;
#define Py_OptimizeFlag (*Py_OptimizeFlag)

PyAPI_DATA(int) Py_NoSiteFlag;
#define Py_NoSiteFlag (*Py_NoSiteFlag)

PyAPI_DATA(int) Py_BytesWarningFlag;
#define Py_BytesWarningFlag (*Py_BytesWarningFlag)

PyAPI_DATA(int) Py_UseClassExceptionsFlag;
#define Py_UseClassExceptionsFlag (*Py_UseClassExceptionsFlag)

PyAPI_DATA(int) Py_FrozenFlag;
#define Py_FrozenFlag (*Py_FrozenFlag)

PyAPI_DATA(int) Py_TabcheckFlag;
#define Py_TabcheckFlag (*Py_TabcheckFlag)

PyAPI_DATA(int) Py_UnicodeFlag;
#define Py_UnicodeFlag (*Py_UnicodeFlag)

PyAPI_DATA(int) Py_IgnoreEnvironmentFlag;
#define Py_IgnoreEnvironmentFlag (*Py_IgnoreEnvironmentFlag)

PyAPI_DATA(int) Py_DivisionWarningFlag;
#define Py_DivisionWarningFlag (*Py_DivisionWarningFlag)

PyAPI_DATA(int) Py_DontWriteBytecodeFlag;
#define Py_DontWriteBytecodeFlag (*Py_DontWriteBytecodeFlag)

PyAPI_DATA(int) Py_NoUserSiteDirectory;
#define Py_NoUserSiteDirectory (*Py_NoUserSiteDirectory)

/* _XXX Py_QnewFlag should go away in 3.0.  It's true iff -Qnew is passed,
  on the command line, and is used in 2.2 by ceval.c to make all "/" divisions
  true divisions (which they will be in 3.0). */
PyAPI_DATA(int) _Py_QnewFlag;
#define _Py_QnewFlag (*_Py_QnewFlag)

/* Warn about 3.x issues */
PyAPI_DATA(int) Py_Py3kWarningFlag;
#define Py_Py3kWarningFlag (*Py_Py3kWarningFlag)

PyAPI_DATA(int) Py_HashRandomizationFlag;
#define Py_HashRandomizationFlag (*Py_HashRandomizationFlag)

/* this is a wrapper around getenv() that pays attention to
   Py_IgnoreEnvironmentFlag.  It should be used for getting variables like
   PYTHONPATH and PYTHONHOME from the environment */
#define Py_GETENV(s) (Py_IgnoreEnvironmentFlag ? NULL : getenv(s))

PyAPI_FUNC(void) Py_FatalError(const char *message);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYDEBUG_H */
