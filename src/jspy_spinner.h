#pragma once
#include "env.h"

v8::Maybe<int> SpinJspyLoop(node::CommonEnvironmentSetup* setup,
                            std::function<void(v8::Isolate*)> tickCallback);

// Return the native require function
v8::Local<v8::Function> GetNativeRequireFunction(
    node::CommonEnvironmentSetup* setup);
