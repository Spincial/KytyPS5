#ifndef EMULATOR_SRC_GRAPHICS_HOST_GPU_RENDERER_PIPELINE_SHADERCOMPILERTHREAD_H_
#define EMULATOR_SRC_GRAPHICS_HOST_GPU_RENDERER_PIPELINE_SHADERCOMPILERTHREAD_H_

#include "common/threads.h"
#include "common/uniqueFunction.h"

#include <deque>
#include <thread>

namespace Libs::Graphics {

// Dedicated worker thread for shader compilation and closely related tasks (shader
// translation, resource materialization, SPIR-V validation/dumps, shader module
// creation). It keeps that work off the main GPU thread (Thread_Gpu).
class ShaderCompilerThread {
public:
	using Task = Common::UniqueFunction<void>;

	ShaderCompilerThread() = default;
	~ShaderCompilerThread();
	ShaderCompilerThread(const ShaderCompilerThread&)            = delete;
	ShaderCompilerThread& operator=(const ShaderCompilerThread&) = delete;

	// Spawns the worker thread; idempotent while it is running.
	void Start();
	// Drains every queued task, then joins the worker thread.
	void Stop();
	[[nodiscard]] bool IsRunning() const;
	// Number of tasks that have fully completed so far.
	[[nodiscard]] uint64_t CompletedCount() const;
	// Blocks until more than `known` tasks have completed.
	void WaitNextCompletion(uint64_t known) const;

	// Queues a task for the worker thread. The task must not be submitted while the
	// thread is stopping; ownership of all captured state transfers to the worker.
	void Submit(Task&& task);

private:
	static void ThreadRun(void* data);

	mutable Common::Mutex m_mutex;
	Common::CondVar       m_work_available;
	mutable Common::CondVar m_completed;
	std::deque<Task>      m_tasks;
	std::jthread          m_thread;
	mutable uint64_t      m_completed_count = 0;
	bool                  m_running  = false;
	bool                  m_stopping = false;
};

} // namespace Libs::Graphics

#endif // EMULATOR_SRC_GRAPHICS_HOST_GPU_RENDERER_PIPELINE_SHADERCOMPILERTHREAD_H_
