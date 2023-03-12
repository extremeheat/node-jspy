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

  // Due to weirdness with private headers being inaccessable to us (env.h),
  // we have to put this method into Node.js code which has access to all the headers opposed to here
  // TODO investigate further
  return SpinJspyLoop(setup, onTick);
};
