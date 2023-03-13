#pragma once
#include "env.h"

NODE_EXTERN v8::Maybe<int> SpinJspyLoop(
    node::CommonEnvironmentSetup* setup,
                            std::function<void(v8::Isolate*)> tickCallback);

// Return the native require function
NODE_EXTERN v8::Local<v8::Function> GetNativeRequireFunction(
    node::CommonEnvironmentSetup* setup);
