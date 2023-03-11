#include "NodeSpinner.h"
#include "jspy_spinner.h" // libnode

v8::Maybe<int> JSSpinner::SpinDaLoop(node::CommonEnvironmentSetup *setup) {
  auto onTick = [this](v8::Isolate* isolate) -> void {
    printf("ticking!\n");
    /* while (jobs.size_approx() > 0) {
      Job job;
      if (jobs.try_dequeue(job)) {
        switch (job.action) {
          case Action::Call: {
            jsi->call(job.ffid);
          }
        }
      }
    }*/
  };

  return SpinJspyLoop(setup, onTick);
};

