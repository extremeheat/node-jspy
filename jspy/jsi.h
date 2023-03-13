#pragma once
#include <vector>
#include <map>
#include <Python.h>

#include "node.h"
#include "v8.h"

// Ecma standardized
static const int64_t MAX_SAFE_INTEGER = 9007199254740991LL;
static const int64_t MIN_SAFE_INTEGER = -9007199254740991LL;

using PersistentValue = v8::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>>;

class JSInterfaceForPython {
 private:
  std::map<unsigned int, PersistentValue> m;  // FFID to v8 persistent value
  // Special cases:
  // 0 -> internal require function
  // 1 -> inspect function
  
  std::shared_ptr<node::CommonEnvironmentSetup> nodeEnv;
  unsigned int ffidCounter = 0;

public:
  enum class Action {
    Call,
    Get,
    Set,
    Inspect,
    Delete,
    Serialize,
    GetKeys,
    Free
  };

  enum class ArgumentType {
    None,

    StringUtf8,
    Integer,
    BigInteger,
    Float64,
    Boolean,
    Object,
    Function,

    JSObject,

    Array,
    Buffer,
  };
  
  struct Argument {
    ArgumentType type;
    PyObject* pythonObject = 0;
    int ffid;
  };
  
  std::string lastError;

  enum ResultType {
    Empty,
    Error,
    String,
    Integer,
    Float,
    Boolean,
    Object
  };

  struct Result {
    // This should be String, Integer, Float, Boolean, or Object
    ResultType resultType;
    double floatValue;
    long long integerOrBooleanValue;
    int ffid;  // if object
    std::string stringValue;
  };

  JSInterfaceForPython(std::shared_ptr<node::CommonEnvironmentSetup> &nodeEnv)
      : nodeEnv(nodeEnv) {}

  static ArgumentType GetPythonArgumentType(PyObject* obj) noexcept(false) {
    if (PyUnicode_Check(obj)) {
      return ArgumentType::StringUtf8;
    } else if (PyLong_Check(obj)) {
      PyLongObject *o = reinterpret_cast<PyLongObject *>(obj);
      // We could do a PyLongLong size comparison against MAX/MIN safe javascript intergers,
      // but the performance implication of this is big given the amount of overhead in Python
      // to convert a PyLong to a C long in _PyLong_AsByteArray twice:
      // https://github.com/python/cpython/blob/4a1dd734311891662a6fc3394f93db98c93e7219/Objects/longobject.c#L955
      // So instead we just compare the byte size which will give us a rough approximation
      // of if an integer is greater/less than +/- 2^50
      auto size = Py_SIZE(o);
      if (size > 8) {
        return ArgumentType::BigInteger;
      } else {
        return ArgumentType::Integer;
      }
    } else if (PyFloat_Check(obj)) {
      return ArgumentType::Float64;
    } else if (PyBool_Check(obj)) {
      return ArgumentType::Boolean;
    } else if (obj == Py_None) {
      return ArgumentType::None;
    } else {
      if (PyObject_HasAttrString(obj, "%ffid")) { // TODO: Make this an internal type
        return ArgumentType::JSObject;
      }
      return ArgumentType::Object;
    }
  }

  Result call(int ffid, std::vector<Argument> arguments, int argsLen);

  Result get(int ffid, std::string attribute);

  Result inspect(int ffid);

  int AddPersistent(v8::Persistent<v8::Value>& fn) {
    m[ffidCounter] = fn;
    return ffidCounter++;
  };

  int AddPersistentFn(v8::Persistent<v8::Function>& fn) {
    m[ffidCounter] = fn;
    return ffidCounter++;
  };

  void SetPersistentGlobalFn(v8::Persistent<v8::Function>& fn, int ffid) {
    m[ffid] = fn;
  };

  void loadInspectFunction(v8::Local<v8::Function> &requireFn);

  PersistentValue GetPersistent(int ffid) {
    return m.at(ffid);
  };
};
