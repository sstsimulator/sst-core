#ifndef sst_core_python_macros_h
#define sst_core_python_macros_h

#if PY_MAJOR_VERSION >= 3
 #define SST_PY_OBJ_HEAD PyVarObject_HEAD_INIT(nullptr, 0)
 #define SST_ConvertToPythonLong(x) PyLong_FromLong(x)
 #define SST_ConvertToCppLong(x) PyLong_AsLong(x)
 #define SST_ConvertToPythonString(x) PyUnicode_FromString(x)
 #define SST_ConvertToCppString(x) PyUnicode_AsUTF8(x)
 #define SST_TP_FINALIZE nullptr,
 #define SST_TP_VECTORCALL_OFFSET 0,
 #define SST_TP_PRINT
 #define SST_TP_COMPARE(x)
 #define SST_TP_RICH_COMPARE(x) x,
 #define SST_TP_AS_SYNC nullptr,
 #define SST_PY_INIT_MODULE(name, methods, moddef) PyModule_Create(&moddef)
 #if PY_MINOR_VERSION >= 8
   #define SST_TP_VECTORCALL nullptr,
 #else
   #define SST_TP_VECTORCALL
 #endif
#else
 #define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
 #define SST_PY_OBJ_HEAD PyVarObject_HEAD_INIT(nullptr, 0)
 #define SST_ConvertToPythonLong(x) PyInt_FromLong(x)
 #define SST_ConvertToCppLong(x) PyInt_AsLong(x)
 #define SST_ConvertToPythonString(x) PyString_FromString(x)
 #define SST_ConvertToCppString(x) PyString_AsString(x)
 #define SST_TP_FINALIZE
 #define SST_TP_VECTORCALL_OFFSET
 #define SST_TP_PRINT nullptr,
 #define SST_TP_COMPARE(x) x,
 #define SST_TP_RICH_COMPARE(x) nullptr,
 #define SST_TP_AS_SYNC
 #define SST_TP_VECTORCALL
 #define SST_PY_INIT_MODULE(name, methods, moddef) Py_InitModule(name, methods)
#endif

#endif
