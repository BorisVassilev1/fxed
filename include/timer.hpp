#include <functional>
#include <thread>
#include <optional>
#include <ostream>

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

class Stopwatch {
	std::chrono::steady_clock::time_point startTime;
	std::ostream						 &out;
	std::string_view					  name;
	bool								  stopped = false;

   public:
	inline Stopwatch(std::ostream &out, std::string_view name = "")
		: startTime(std::chrono::steady_clock::now()), out(out), name(name) {}

	~Stopwatch() { stop(); }

	inline void stop() {
		if (stopped) return;
		auto endTime  = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		out << name << "Elapsed time: " << duration << " ms" << std::endl;
		stopped = true;
	}
};

}	  // namespace nri
