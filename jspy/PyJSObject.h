#pragma once
#include "jsi.h"

typedef struct {
  PyObject_HEAD;
  JSInterfaceForPython* jsi;
  int ffid;
} JSProxyObject;

class JSObject {
  int ffid = -1;
  JSInterfaceForPython* jsi = nullptr;

 public:
  JSObject(int ffid, JSInterfaceForPython* jsi) : ffid(ffid) {
    this->jsi = jsi;
  }
  int getFfid() { return ffid; }

 private:
  static PyObject* getAttributeTrap(PyObject* self, char* name) {
    auto obj = (JSProxyObject*)self;
    printf("TRAP! Called %s (jsi %d, ffid %d)\n", name, obj->jsi, obj->ffid);

#if 0
    auto jsr = obj->jsi->get(obj->ffid, name);
    printf("JS Res %d\n", jsr.resultType);

    switch (jsr.resultType) {
      case JSInterfaceForPython::Integer:
        return PyLong_FromLong(jsr.integerOrBooleanValue);
      case JSInterfaceForPython::Boolean:
        return PyBool_FromLong(jsr.integerOrBooleanValue);
      case JSInterfaceForPython::Float:
        return PyFloat_FromDouble(jsr.floatValue);
      case JSInterfaceForPython::String:
        return PyUnicode_FromString(jsr.stringValue.c_str());
      case JSInterfaceForPython::Object:
        return (PyObject*)JSObject::CreateProxy(jsr.ffid, obj->jsi);
    }
#endif

    Py_RETURN_NONE;
  }

  static PyObject* callTrap(PyObject* self, PyObject* args, PyObject* kwargs) {
    auto obj = (JSProxyObject*)self;
    int nargs = PyTuple_GET_SIZE(args);
    std::vector<JSInterfaceForPython::Argument> jsArgs;
    for (int i = 0; i < nargs; i++) {
      PyObject* arg = PyTuple_GET_ITEM(args, i);
      if (arg == Py_None) {
        jsArgs.push_back({JSInterfaceForPython::ArgumentType::None});
        continue;
      }
      auto aType = JSInterfaceForPython::GetPythonArgumentType(arg);
      jsArgs.push_back({aType, arg});
    }
    auto res = obj->jsi->call(obj->ffid, jsArgs, nargs);

    switch (res.resultType) {
      case JSInterfaceForPython::Empty:
        Py_RETURN_NONE;
      case JSInterfaceForPython::Error:
        // Return a Python exception
        PyErr_SetString(PyExc_RuntimeError, res.stringValue.c_str());
        return nullptr;
      case JSInterfaceForPython::Integer:
        return PyLong_FromLong(res.integerOrBooleanValue);
      case JSInterfaceForPython::Boolean:
        return PyBool_FromLong(res.integerOrBooleanValue);
      case JSInterfaceForPython::Float:
        return PyFloat_FromDouble(res.floatValue);
      case JSInterfaceForPython::String:
        return PyUnicode_FromString(res.stringValue.c_str());
      case JSInterfaceForPython::Object:
        auto jsp = (PyObject*)JSObject::CreateProxy(res.ffid, obj->jsi,
        res.stringValue);
        if (jsp != nullptr) {
          return jsp;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "Internal error");
    Py_RETURN_NONE;
  }


  static PyObject* reprTrap(PyObject* self) {
    auto obj = (JSProxyObject*)self;
    auto res = obj->jsi->inspect(obj->ffid);

    if (res.resultType == JSInterfaceForPython::String) {
      return PyUnicode_FromString(res.stringValue.c_str());
    }
    return PyUnicode_FromFormat("<JSProxyObject Trap %d>", obj->ffid);
  }

 public:
  static JSProxyObject* CreateProxy(int ffid,
                                    JSInterfaceForPython* jsi,
                                    std::string name = "") {
    static PyTypeObject type = {PyVarObject_HEAD_INIT(NULL, 0)};
    type.tp_name = "JSProxyObject";
    type.tp_basicsize = sizeof(JSProxyObject);
    type.tp_doc = PyDoc_STR("(JS Proxy Object)");
    type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    // traps
    type.tp_getattr = getAttributeTrap;
    type.tp_call = callTrap;
    type.tp_repr = reprTrap;
    // type.tp_vectorcall
    //  allocs
    type.tp_new = PyType_GenericNew;
    type.tp_finalize = [](PyObject* self) {
      auto p = (JSProxyObject*)self;
      printf("GC'd the JS Proxy Object with FFID %d\n", p->ffid);
    };

    if (PyType_Ready(&type) < 0) {
      PyErr_SetString(PyExc_RuntimeError, "Failed to create JSProxyObject");
      return 0;
    }

    JSProxyObject* proxy = PyObject_NEW(JSProxyObject, &type);
    proxy->ffid = ffid;
    proxy->jsi = jsi;
    return proxy;
  }
};
