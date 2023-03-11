#if 0
#include "jsi.h"

JSInterfaceForPython::Result JSInterfaceForPython::call(
    int ffid, std::vector<Argument> arguments, int argsLen) {
  // Create a handle scope to keep the temporary object references.
  v8::HandleScope handle_scope(nodeEnv->isolate());

  v8::Local<v8::Context> context =
      v8::Local<v8::Context>::New(nodeEnv->isolate(), nodeEnv->context());

  // Enter this processor's context so all the remaining operations
  // take place there
  v8::Context::Scope context_scope(context);

  PersistentValue v = m.at(ffid);  // THROWS

  auto x = ((v8::Persistent<v8::Function,
                            v8::CopyablePersistentTraits<v8::Function>>*)&v);
  auto local = v8::Local<v8::Function>::New(nodeEnv->isolate(), *x);
  auto jsArgs = new v8::Local<v8::Value>[argsLen];
  int i = 0;
  for (auto& argument : arguments) {
    if (argument.type == ArgumentType::JSObject) {
      auto storedArgument = this->m[argument.ffid];
      if (storedArgument.IsEmpty()) {
        return {Error};
      } else {
        jsArgs[i++] = (storedArgument.Get(nodeEnv->isolate()));
      }
    } else if (argument.type == ArgumentType::None) {
      jsArgs[i++] = (v8::Undefined(nodeEnv->isolate()));
    } else {
      switch (argument.type) {
        case ArgumentType::StringUtf8: {
          v8::Local<v8::String> s =
              v8::String::NewFromUtf8(nodeEnv->isolate(),
                                      PyUnicode_AsUTF8(argument.pythonObject))
                  .ToLocalChecked();
          jsArgs[i++] = (s);
          break;
        }
        case ArgumentType::Integer: {
          int64_t i = PyLong_AsLongLong(argument.pythonObject);
          if (i == -1 && PyErr_Occurred()) {
            return {Error};
          }
          jsArgs[i++] = (v8::Integer::New(nodeEnv->isolate(), i));
          break;
        }
        case ArgumentType::BigInteger: {
          int64_t i = PyLong_AsLongLong(argument.pythonObject);
          if (i == -1 && PyErr_Occurred()) {
            return {Error};
          }
          jsArgs[i++] = (v8::BigInt::New(nodeEnv->isolate(), i));
          break;
        }
        case ArgumentType::Float64: {
          double d = PyFloat_AsDouble(argument.pythonObject);
          if (d == -1 && PyErr_Occurred()) {
            return {Error};
          }
          jsArgs[i++] = (v8::Number::New(nodeEnv->isolate(), d));
          break;
        }
        case ArgumentType::Boolean: {
          int b = PyObject_IsTrue(argument.pythonObject);
          if (b == -1 && PyErr_Occurred()) {
            return {Error};
          }
          jsArgs[i++] = (v8::Boolean::New(nodeEnv->isolate(), b));
          break;
        }
        case ArgumentType::Object: {
          v8::Local<v8::Object> o = v8::Object::New(nodeEnv->isolate());
          jsArgs[i++] = (o);
          break;
        }
        case ArgumentType::Function: {
          v8::Local<v8::Function> f =
              v8::Function::New(
                  context,
                  [](const v8::FunctionCallbackInfo<v8::Value>& info) {
                    v8::Isolate* isolate = info.GetIsolate();
                    v8::Local<v8::Context> context =
                        isolate->GetCurrentContext();
                    v8::Local<v8::Object> global = context->Global();
                    // v8::Local<v8::Value> f =
                    // global->Get(v8::String::NewFromUtf8(isolate, "f"));
                    // v8::Local<v8::Function> func =
                    // v8::Local<v8::Function>::Cast(f); v8::Local<v8::Value>
                    // result = func->Call(context, global, info.Length(),
                    // info.Data());
                    info.GetReturnValue().Set(v8::Undefined(isolate));
                  })
                  .ToLocalChecked();
          break;
        }
      }
    }
  }

  // Call v with no arguments
  auto result =
      local->Call(context, v8::Undefined(nodeEnv->isolate()), argsLen, jsArgs);

  if (result.IsEmpty()) {
    return {Empty};
  } else {
    // Read the v8::MaybeLocal into a v8::Local
    v8::Local<v8::Value> value;
    if (!result.ToLocal(&value)) {
      return {Empty};
    }
    if (value->IsFunction()) {
      v8::Local<v8::Function> val = v8::Local<v8::Function>::Cast(value);
      // val->This
    }
  }
}

#endif
