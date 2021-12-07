#pragma once

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace bench {

class Stopwatch final {
public:
	Stopwatch() noexcept {
		reset();
	}

	void reset() noexcept {
		start_time_ = clock_.now();
	}

	template <typename Resolution = std::chrono::microseconds>
	[[nodiscard]] Resolution elapsed() const noexcept {
		return std::chrono::duration_cast<Resolution>(clock_.now() - start_time_);
	}

private:
	using Clock = std::chrono::high_resolution_clock;
	Clock clock_;
	Clock::time_point start_time_;
};

class HttpStatis final {
public:
	static HttpStatis& get() {
		static HttpStatis statis;
		return statis;
	}

	void set_test_request_size(size_t num_test_request,
		size_t num_clients,
		size_t threads) {
		num_test_request_ = num_test_request;
		num_clients_ = num_clients;
		threads_ = threads;
		num_update_size_ = num_test_request_ / 10;
		watch_.reset();
	}

	bool stop_test() const noexcept {
		return request_ == num_test_request_;
	}

	void update(size_t transferred_size) {
		auto num_request = ++request_;
		total_transferred_ += transferred_size;
		if (num_request % num_update_size_ == 0) {
			std::cout << "Completed " << num_request << " requests" << std::endl;
		}		
	}

	void show_statistic() {
		std::cout.setf(std::ios::showpoint);

		auto elapsed = watch_.elapsed<std::chrono::seconds>();
		auto requst_per_seconds = request_ / static_cast<double>(elapsed.count());
		std::cout << "Use threads: " << threads_ << std::endl;
		std::cout << "Number of clients: " << num_clients_ << std::endl;
		std::cout << "Requests per second: " << std::fixed << std::setprecision(2) << requst_per_seconds << " /sec" << std::endl;
		std::cout << "Total transferred: " << total_transferred_ << " /bytes" << std::endl;
	}

private:
	HttpStatis() = default;

	size_t num_clients_{ 0 };
	size_t num_test_request_{ 0 };
	size_t num_update_size_{ 0 };
	size_t threads_{0};
	std::atomic<size_t> request_{ 0 };
	std::atomic<size_t> total_transferred_{ 0 };
	Stopwatch watch_;
};

}