#include "NodeRT.h"
#include "NodeSpinner.h"
#include "PyJSObject.h"
#include "jspy_spinner.h"  // node

using V8 = v8::V8;

int NodeRT::Init(int argc, char** argv) {
  argv = uv_setup_args(argc, argv);
  std::vector<std::string> args = {"node"};  //(argv, argv + argc);
  std::vector<std::string> exec_args;
  std::vector<std::string> errors;
  int exit_code = node::InitializeNodeWithArgs(&args, &exec_args, &errors);

  for (const std::string& error : errors) {
    fprintf(
        stderr, "FAILED TO STARTUP %s: %s\n", args[0].c_str(), error.c_str());
  }

  if (exit_code != 0) {
    printf("BAD EXIT CODE: %d\n", exit_code);
    return exit_code;
  }

  this->platform = node::MultiIsolatePlatform::Create(4);
  V8::InitializePlatform(platform.get());
  V8::Initialize();

  printf("Initialized v8\n");

  //for (int i = 0; i < 200; i++) {
  //  int ret = InitNodeInstance(platform.get(), args, exec_args);
  //  this->sayHello();
  //  // Sleep(500);
  //  setup = nullptr;
  //}
  InitNodeInstance(platform.get(), args, exec_args);
  return 0;
}

bool NodeRT::InitNodeInstance(node::MultiIsolatePlatform* platform,
                     const std::vector<std::string>& args,
                     const std::vector<std::string>& exec_args) {
  std::vector<std::string> errors;
  this->setup =
      node::CommonEnvironmentSetup::Create(platform, &errors, args, exec_args);
  printf("Initialized Node.js setup\n");
  if (!setup) {
    for (const std::string& err : errors)
      fprintf(stderr, "%s: %s\n", args[0].c_str(), err.c_str());
    return 1;
  };
  wasInited = true;

  // Store a reference to the nodejs native require function
  {
    v8::Locker locker(setup->isolate());
    v8::HandleScope handle_scope(setup->isolate());
    v8::Isolate::Scope isolate_scope(setup->isolate());

    this->jsi = std::make_shared<JSInterfaceForPython>(setup);
    auto requireFn = GetNativeRequireFunction(setup.get());
    // make persistent
    v8::Persistent<v8::Function> persistentFn;
    persistentFn.Reset(setup->isolate(), requireFn);
    this->jsi->AddPersistentFn(persistentFn);
    this->jsi->loadInspectFunction(requireFn);
  }

  return true;
}

int NodeRT::sayHello() {
  v8::Isolate* isolate = setup->isolate();
  node::Environment* env = setup->env();

  v8::Locker locker(isolate);
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(setup->context());

  v8::MaybeLocal<v8::Value> loadenv_ret = node::LoadEnvironment(
      env,
      "const publicRequire ="
      "  require('module').createRequire(process.cwd() + '/');"
      "globalThis.require = publicRequire;"
      "globalThis.embedVars = { nön_ascıı: '🏳️‍🌈' };"
      "console.log('heya!', 'your args were', process.argv)");

  if (loadenv_ret.IsEmpty())  // There has been a JS exception.
    return 1;

  std::cout << "Spinning!" << std::endl;
  // int exit_code = node::SpinEventLoop(env).FromMaybe(1);

  JSSpinner spinner;
  int exit_code = spinner.SpinDaLoop(setup.get()).FromMaybe(1);

  node::Stop(env);
  std::cout << "Hello world! " << exit_code << std::endl;
  return 0;
}

int NodeRT::StartEventLoop() {
  //this->event_loop_thread = std::thread([&]() { spinner->SpinDaLoop(setup.get()); });
  return 0;
}
