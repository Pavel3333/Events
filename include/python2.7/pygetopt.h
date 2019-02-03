
#ifndef Py_PYGETOPT_H
#define Py_PYGETOPT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(int) _PyOS_opterr;
#define _PyOS_opterr (*_PyOS_opterr)

PyAPI_DATA(int) _PyOS_optind;
#define _PyOS_optind (*_PyOS_optind)

PyAPI_DATA(char *) _PyOS_optarg;
#define _PyOS_optarg (*_PyOS_optarg)

PyAPI_FUNC(void) _PyOS_ResetGetOpt(void);
PyAPI_FUNC(int) _PyOS_GetOpt(int argc, char **argv, char *optstring);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYGETOPT_H */
