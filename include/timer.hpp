#include <functional>
#include <thread>
#include <optional>

namespace nri {
class Timer {
	std::chrono::milliseconds	interval;
	std::function<void()>		f;
	std::optional<std::jthread> thread;
	volatile bool				running = 1;

   public:
	Timer() : interval(0), f([]() {}), thread(std::nullopt) {}
	Timer(std::function<void()> &&f, std::chrono::milliseconds interval)
		: interval(interval), f(std::move(f)), thread(std::nullopt) {}

	void start() {
		thread = std::jthread([this]() {
			while (this->running) {
				if (this->interval.count() > 0) std::this_thread::sleep_for(this->interval);
				this->f();
			}
		});
	}

	void stop() { thread = std::nullopt; }

	Timer(Timer &&)			   = default;
	Timer &operator=(Timer &&) = default;

	~Timer() { running = 0; }
};
}	  // namespace nri
