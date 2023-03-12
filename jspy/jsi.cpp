#include "jsi.h"

using Result = JSInterfaceForPython::Result;

JSInterfaceForPython::Result JSInterfaceForPython::call(
    int ffid, std::vector<Argument> arguments, int argsLen) {
  v8::Locker locker(nodeEnv->isolate());
  // Create a handle scope to keep the temporary object references.
  v8::HandleScope handle_scope(nodeEnv->isolate());

  v8::Local<v8::Context> context =
      v8::Local<v8::Context>::New(nodeEnv->isolate(), nodeEnv->context());

  // Enter this processor's context so all the remaining operations
  // take place there
  v8::Context::Scope context_scope(context);

  PersistentValue v = m.at(ffid);  // THROWS
  auto local =
      v8::Local<v8::Value>::New(nodeEnv->isolate(), v).As<v8::Function>();
  auto jsArgs = new v8::Local<v8::Value>[argsLen];
  int i = 0;
  for (auto& argument : arguments) {
    if (argument.type == ArgumentType::JSObject) {
      auto storedArgument = this->m[argument.ffid];
      if (storedArgument.IsEmpty()) {
        return {Error};
      } else {
        jsArgs[i++] = storedArgument.Get(nodeEnv->isolate());
      }
    } else if (argument.type == ArgumentType::None) {
      jsArgs[i++] = v8::Undefined(nodeEnv->isolate());
    } else {
      switch (argument.type) {
        case ArgumentType::StringUtf8: {
          v8::Local<v8::String> s =
              v8::String::NewFromUtf8(nodeEnv->isolate(),
                                      PyUnicode_AsUTF8(argument.pythonObject))
                  .ToLocalChecked();
          printf("Passing parameter '%s' to ffid %d\n",
                 PyUnicode_AsUTF8(argument.pythonObject),
                 ffid);
          jsArgs[i++] = s;
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

  delete[] jsArgs;

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
      printf("Called method is a function\n");
    } else if (value->IsObject()) {
      printf("Called method is a object\n");
      // Convert to a v8::Persistent
      v8::Local<v8::Object> val = v8::Local<v8::Object>::Cast(value);
      v8::Persistent<v8::Object> p(nodeEnv->isolate(), val);
      // Store the persistent in the map
      int ffid = this->ffidCounter++;
      this->m[ffid] = p;

      // Get the name of the object's constructor, so we don't have to write a
      // generic "Proxy" in python
      auto constructorName = val->GetConstructorName();
      v8::String::Utf8Value utf8(nodeEnv->isolate(), constructorName);
      std::string strName = *utf8;

      return {ResultType::Object, 0, 0, ffid, strName};
    } else if (value->IsString()) {
      printf("Called method is a string\n");
      v8::Local<v8::String> val = v8::Local<v8::String>::Cast(value);
      // Get C string from v8::String
      v8::String::Utf8Value utf8(nodeEnv->isolate(), val);
      std::string str(*utf8);
      return {ResultType::String, 0, 0, 0, str};
    } else if (value->IsNumber()) {
      printf("Called method is a number\n");
      v8::Local<v8::Number> val = v8::Local<v8::Number>::Cast(value);
      return {ResultType::Float, val->Value()};
    } else if (value->IsBigInt()) {
      printf("Called method is a bigint\n");
      v8::Local<v8::BigInt> val = v8::Local<v8::BigInt>::Cast(value);
      return {ResultType::Integer, 0, val->Int64Value()};
    } else if (value->IsBoolean()) {
      printf("Called method is a boolean\n");
      v8::Local<v8::Boolean> val = v8::Local<v8::Boolean>::Cast(value);
      return {ResultType::Boolean, 0, val->Value()};
    } else if (value->IsUndefined()) {
      printf("Called method is a undefined\n");
      return {ResultType::Empty};
    } else if (value->IsNull()) {
      printf("Called method is a null\n");
      return {ResultType::Empty};
    } else {
      printf("Called method is a unknown\n");
      return {ResultType::Empty};
    }
  }
}

/* JSInterfaceForPython::JSInterfaceForPython(NodeEnvironment* nodeEnv)
    : nodeEnv(nodeEnv) {}

JSInterfaceForPython::~JSInterfaceForPython() {}*/

Result JSInterfaceForPython::get(int ffid, std::string attribute) {
  v8::Locker locker(nodeEnv->isolate());
  v8::HandleScope handle_scope(nodeEnv->isolate());
  v8::Local<v8::Context> context =
      v8::Local<v8::Context>::New(nodeEnv->isolate(), nodeEnv->context());
  v8::Context::Scope context_scope(context);

  PersistentValue v = m.at(ffid);  // THROWS
  auto local = v8::Local<v8::Value>::New(nodeEnv->isolate(), v);

  if (local->IsObject()) {
    auto obj = local.As<v8::Object>();
    // Get attribute of obj
    auto maybeValue =
        obj->Get(context,
                 v8::String::NewFromUtf8(nodeEnv->isolate(), attribute.c_str())
                     .ToLocalChecked());
    if (maybeValue.IsEmpty()) {
      return {Empty};
    } else {
      auto val = maybeValue.ToLocalChecked();
      if (val->IsInt32()) {
        auto intVal = val->IntegerValue(context).ToChecked();
        return {Integer, 0, intVal};
      } else if (val->IsNumber()) {
        auto floatVal = val->NumberValue(context).ToChecked();
        return {Float, floatVal};
      } else if (val->IsString()) {
        auto stringVal = val->ToString(context).ToLocalChecked();
        std::string str = *v8::String::Utf8Value(nodeEnv->isolate(), stringVal);
        return {String, 0, 0, 0, str};
      } else if (val->IsBoolean()) {
        auto boolVal = val->BooleanValue(nodeEnv->isolate());
        return {Boolean, 0, boolVal};
      } else if (val->IsNullOrUndefined()) {
        return {Empty};
      } else {
        // This is some other type of object, we need to store it
        v8::Persistent<v8::Value> persistent;
        persistent.Reset(nodeEnv->isolate(), val);
        auto ffid = AddPersistent(persistent);
        return {Object, 0, 0, ffid};
      }
    }
  }

  return Result();
}

Result JSInterfaceForPython::inspect(int ffid) {
  v8::Locker locker(nodeEnv->isolate());
  v8::HandleScope handle_scope(nodeEnv->isolate());
  v8::Local<v8::Context> context =
      v8::Local<v8::Context>::New(nodeEnv->isolate(), nodeEnv->context());
  v8::Context::Scope context_scope(context);

  PersistentValue v = m.at(ffid);  // THROWS
  auto local = v8::Local<v8::Value>::New(nodeEnv->isolate(), v);

  PersistentValue inspectFn = m.at(1);  // THROWS
  auto inspectLocal = v8::Local<v8::Value>::New(nodeEnv->isolate(), inspectFn)
                          .As<v8::Function>();

  // make options object
  v8::Local<v8::Object> options = v8::Object::New(nodeEnv->isolate());
  options
      ->Set(
          context,
          v8::String::NewFromUtf8(nodeEnv->isolate(), "depth").ToLocalChecked(),
          v8::Number::New(nodeEnv->isolate(), 2))
      .Check();
  options
      ->Set(context,
            v8::String::NewFromUtf8(nodeEnv->isolate(), "colors")
                .ToLocalChecked(),
            v8::Boolean::New(nodeEnv->isolate(), true))
      .Check();

  v8::Local<v8::Value> args[] = {local, options};
  auto maybeValue = inspectLocal->Call(context, context->Global(), 2, args);
  if (maybeValue.IsEmpty()) {
    return {Empty};
  }

  auto val = maybeValue.ToLocalChecked();
  if (val->IsString()) {
    auto str = val->ToString(context).ToLocalChecked();
    std::string strVal = *v8::String::Utf8Value(nodeEnv->isolate(), str);
    return {String, 0, 0, 0, strVal};
  }
  
  return {Empty};
}

void JSInterfaceForPython::loadInspectFunction(
    v8::Local<v8::Function>& requireFn) {
  // Call the requireFn with the argument "util"
  v8::Local<v8::Value> argv[] = {
      v8::String::NewFromUtf8(nodeEnv->isolate(), "util").ToLocalChecked()};
  auto maybeResult = requireFn->Call(
      nodeEnv->context(), v8::Undefined(nodeEnv->isolate()), 1, argv);
  if (maybeResult.IsEmpty()) {
    return;
  }

  // Get the inspect function from the result
  auto util = maybeResult.ToLocalChecked();
  if (!util->IsObject()) {
    return;
  }

  auto obj = util.As<v8::Object>();
  auto maybeInspect = obj->Get(
      nodeEnv->context(),
      v8::String::NewFromUtf8(nodeEnv->isolate(), "inspect").ToLocalChecked());

  if (maybeInspect.IsEmpty()) {
    return;
  }

  // Store the inspect function in the map
  v8::Local<v8::Value> inspect = maybeInspect.ToLocalChecked();
  v8::Persistent<v8::Value> persistent;
  persistent.Reset(nodeEnv->isolate(), inspect);
  auto ffid = AddPersistent(persistent);
  m[ffid] = persistent;
}
