#pragma once
#include <node.h>
#include "concurrentqueue.h"
#include "jsi.h"

struct JSSpinner {
  bool running = false;
  struct Job {
    int ffid;
    JSInterfaceForPython::Action action;
    std::string attribute;
    //std::vector<JSInterfaceForPython::Argument> arguments;
    std::function<void()> callback;
  };
  
  /* 
  std::shared_ptr<JSInterfaceForPython> jsi;
  JSSpinner(std::shared_ptr<JSInterfaceForPython> &jsi) : jsi(jsi){};
  */

  moodycamel::ConcurrentQueue<Job> jobs;
  v8::Maybe<int> SpinDaLoop(node::CommonEnvironmentSetup* env);
  
  inline void StopEventLoop() {
    // ask to the event loop
    this->running = false;
  }
};
