#include "graphics/host_gpu/renderer/pipeline/shaderCompilerThread.h"

#include "common/assert.h"
#include "common/profiler.h"

namespace Libs::Graphics {

ShaderCompilerThread::~ShaderCompilerThread() {
	Stop();
}

void ShaderCompilerThread::Start() {
	Common::LockGuard lock(m_mutex);
	if (m_running) {
		return;
	}
	m_running  = true;
	m_stopping = false;
	m_thread   = std::jthread(ThreadRun, this);
}

bool ShaderCompilerThread::IsRunning() const {
	Common::LockGuard lock(m_mutex);
	return m_running;
}

void ShaderCompilerThread::Stop() {
	{
		Common::LockGuard lock(m_mutex);
		if (!m_running) {
			return;
		}
		m_stopping = true;
		// The worker drains every queued task before observing the stop request.
		m_work_available.SignalAll();
	}
	if (m_thread.joinable()) {
		m_thread.join();
	}
	{
		Common::LockGuard lock(m_mutex);
		m_running = false;
	}
}

void ShaderCompilerThread::Submit(Task&& task) {
	EXIT_IF(!task);
	Common::LockGuard lock(m_mutex);
	EXIT_IF(m_stopping);
	m_tasks.push_back(std::move(task));
	m_work_available.Signal();
}

void ShaderCompilerThread::ThreadRun(void* data) {
	auto* self = static_cast<ShaderCompilerThread*>(data);
	EXIT_IF(self == nullptr);
	KYTY_PROFILER_THREAD("Thread_ShaderCompiler");

	for (;;) {
		Task task;
		{
			Common::LockGuard lock(self->m_mutex);
			if (self->m_tasks.empty()) {
				if (self->m_stopping) {
					return;
				}
				self->m_work_available.Wait(&self->m_mutex);
				continue;
			}
			task = std::move(self->m_tasks.front());
			self->m_tasks.pop_front();
		}
		task();
	}
}

} // namespace Libs::Graphics
