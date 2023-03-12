#include <Python.h>
#include "NodeRT.h"
//#include "jsi.h"

NodeRT* rt = nullptr;

extern "C" {
/*
 * Implements an example function.
 */
PyDoc_STRVAR(JsPyBridge_example_doc, "example(obj, number)\
\
Example function");

PyDoc_STRVAR(JsPyBridge_init_nodejs_runtime_doc,
             "init_nodejs_runtime(obj, number)\
\
Initialize the Node.js runtime");

PyObject* JsPyBridge_example(PyObject* self, PyObject* args, PyObject* kwargs) {
  /* Shared references that do not need Py_DECREF before returning. */
  PyObject* obj = NULL;
  int number = 0;

  /* Parse positional and keyword arguments */
  static char* keywords[] = {"obj", "number", NULL};
  if (!PyArg_ParseTupleAndKeywords(
          args, kwargs, "Oi", keywords, &obj, &number)) {
    return NULL;
  }

  /* Function implementation starts here */

  if (number < 0) {
    PyErr_SetObject(PyExc_ValueError, obj);
    return NULL; /* return NULL indicates error */
  }

  Py_RETURN_NONE;
}

PyObject* JsPyBridge_init_nodejs_runtime(PyObject* self, PyObject* args) {
  printf("1. init\n");

  int argc = 1;
  PyObject* pyArgv = 0;
  static char* keywords[] = {"argc", "argv", NULL};
  if (!PyArg_ParseTuple(args, "iO", &argc, &pyArgv)) {
    return NULL;
  }

  if (rt == nullptr) {
    rt = new NodeRT();
  } else {
    // Raise a Python exception that we already init'ed
    PyErr_SetString(PyExc_RuntimeError, "NodeRT already initialized");
    return NULL;
  }
  
  int argvCount = PyList_Size(pyArgv);
  assert(argc == argvCount);

  auto arguments = new const char*[argc];

  for (int i = 0; i < argc; i++) {
    /* grab the string object from the next element of the list */
    auto strObj = PyList_GetItem(pyArgv, i); /* Can't fail */
    auto line = PyUnicode_AsUTF8(strObj);
    arguments[i] = line;
  }

  // char *argv[] = {"C:\\Program Files\\nodejs\\node.exe"};
  int code = rt->Init(argc, (char**)arguments);
  printf("start code : %d\n", code);
  // rt->sayHello();
  printf("2. Init'ed\n");

  delete[] arguments;
  Py_RETURN_NONE;
}

PyObject* JsPyBridge_get_require_function(PyObject* self, PyObject* args) {
  printf("1\n");

  if (rt == nullptr) {
    // Raise a Python exception that we already init'ed
    PyErr_SetString(PyExc_RuntimeError, "Node runtime not initialized");
    return NULL;
  }

  auto jso = JSObject::CreateProxy(0, rt->jsi.get());
  if (jso == nullptr) {
    PyErr_SetString(PyExc_RuntimeError, "Internal error");
    return NULL;
  }
  
  return (PyObject*)jso;
}

/*
 * List of functions to add to JsPyBridge in exec_JsPyBridge().
 */
static PyMethodDef JsPyBridge_functions[] = {
    {"example", (PyCFunction)JsPyBridge_example, METH_VARARGS | METH_KEYWORDS, JsPyBridge_example_doc},
    {"init_nodejs_runtime", (PyCFunction)JsPyBridge_init_nodejs_runtime, METH_VARARGS | METH_KEYWORDS, JsPyBridge_init_nodejs_runtime_doc},
    {"get_require_function", (PyCFunction)JsPyBridge_get_require_function, METH_VARARGS | METH_KEYWORDS, JsPyBridge_example_doc},
    {NULL, NULL, 0, NULL} /* marks end of array */
};

/*
 * Initialize JsPyBridge. May be called multiple times, so avoid
 * using static state.
 */
int exec_JsPyBridge(PyObject* module) {
  PyModule_AddFunctions(module, JsPyBridge_functions);

  PyModule_AddStringConstant(module, "__author__", "extre");
  PyModule_AddStringConstant(module, "__version__", "1.0.0");
  PyModule_AddIntConstant(module, "year", 2022);

  return 0; /* success */
}

/*
 * Documentation for JsPyBridge.
 */
PyDoc_STRVAR(JsPyBridge_doc, "The JsPyBridge module");

static PyModuleDef_Slot JsPyBridge_slots[] = {{Py_mod_exec, exec_JsPyBridge},
                                              {0, NULL}};

static PyModuleDef JsPyBridge_def = {
    PyModuleDef_HEAD_INIT,
    "JsPyBridge",
    JsPyBridge_doc,
    0,    /* m_size */
    NULL, /* m_methods */
    JsPyBridge_slots,
    NULL, /* m_traverse */
    NULL, /* m_clear */
    NULL, /* m_free */
};

PyMODINIT_FUNC PyInit_JsPyBridge() {
  /* NodeRT* rt = new NodeRT();
  int code = rt->Init(0, (char**)0);
  rt->End();
  delete rt;*/
  return PyModuleDef_Init(&JsPyBridge_def);
}
};
