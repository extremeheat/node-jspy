#pragma once
#include <assert.h>
#include <node.h>
#include <iostream>
#include <thread>
#include "uv.h"
#include "jsi.h"
#include "PyJSObject.h"
#include "NodeSpinner.h"

// Note: This file is being referred to from doc/api/embedding.md, and excerpts
// from it are included in the documentation. Try to keep these in sync.

static int RunNodeInstance(node::MultiIsolatePlatform* platform,
                           const std::vector<std::string>& args,
                           const std::vector<std::string>& exec_args);

struct NodeRT {
  std::unique_ptr<node::MultiIsolatePlatform> platform;
  std::shared_ptr<node::CommonEnvironmentSetup> setup;
  std::shared_ptr<JSInterfaceForPython> jsi;
  std::shared_ptr<JSSpinner> spinner;
  std::thread event_loop_thread;

  bool wasInited = false;

  int Init(int argc, char** argv);

 private:
  bool InitNodeInstance(node::MultiIsolatePlatform* platform,
                       const std::vector<std::string>& args,
                       const std::vector<std::string>& exec_args);

 public:
  int sayHello();

  int StartEventLoop();

  int StopEventLoop() {
    // stop the loop
    //spinner->StopEventLoop();
  }

  int End() {
    this->platform.release();
    this->setup = nullptr;
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    return 0;
  }
};
