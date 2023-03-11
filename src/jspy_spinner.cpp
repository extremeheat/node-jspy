#include "jspy_spinner.h"

v8::Maybe<int> SpinJspyLoop(node::CommonEnvironmentSetup* setup,
                            std::function<void(v8::Isolate*)> tickCallback) {
  auto env = setup->env();
  CHECK_NOT_NULL(env);
  node::MultiIsolatePlatform* platform = GetMultiIsolatePlatform(env);
  CHECK_NOT_NULL(platform);

  v8::Isolate* isolate = env->isolate();
  v8::HandleScope handle_scope(isolate);
  auto context = setup->context();
  v8::Context::Scope context_scope(context);
  v8::SealHandleScope seal(isolate);

  if (env->is_stopping()) return v8::Nothing<int>();

  env->set_trace_sync_io(env->options()->trace_sync_io);
  bool running = true;

  {
    bool more;
    env->performance_state()->Mark(
        node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_START);
    do {
      if (env->is_stopping()) break;
      uv_run(env->event_loop(), UV_RUN_DEFAULT);
      if (env->is_stopping()) break;

      platform->DrainTasks(isolate);

      more = uv_loop_alive(env->event_loop());
      if (more && !env->is_stopping()) continue;

      if (EmitProcessBeforeExit(env).IsNothing()) break;

      // Emit `beforeExit` if the loop became alive either after emitting
      // event, or after running some callbacks.
      more = uv_loop_alive(env->event_loop());

      tickCallback(isolate);
    } while (running && more == true && !env->is_stopping());
    env->performance_state()->Mark(
        node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_EXIT);
  }
  if (env->is_stopping()) return v8::Nothing<int>();
  env->set_trace_sync_io(false);
  env->PrintInfoForSnapshotIfDebug();
  env->VerifyNoStrongBaseObjects();
  return EmitProcessExit(env);
}
