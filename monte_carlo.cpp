/*
Benchmark performance of blocking_queue against
sequential and fully parallel workloads.

Copyright 2021. Andrew Wang.
*/
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <thread>
#include <utility>
#include <vector>

#include "blocking_queue.h"
using std::accumulate;
using std::atomic;
using std::cout;
using std::default_random_engine;
using std::endl;
using std::ios_base;
using std::make_pair;
using std::pair;
using std::thread;
using std::uniform_real_distribution;
using std::vector;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::system_clock;
using std::this_thread::sleep_for;

/**
 * Reports the Monte Carlo estimate and error to std::cout.
 * @param in_circle The number of points in the circle.
 * @param total The total number of points generated.
 */
void report_accuracy(uint64_t in_circle, uint64_t total);

/**
 * Reports the time to std::cout in human-readable format.
 * @param dur The time duration between begin and end.
 */
void report_time(nanoseconds dur);

/**
 * Run the monitor part of the experiment using blocking_queue.
 * @param threads_per_type Number of producers and consumers each.
 * @param points_per_thread Number of points processed per thread.
 * @param sleep_ns Number of ns to sleep between each point.
 */
void execute_monitor(uint64_t threads_per_type, uint64_t points_per_thread,
                     uint64_t sleep_ns);

/**
 * Run the sequential part of the experiment.
 * @param total_points Number of points to generate.
 * @param sleep_ns Number of ns to sleep between each point.
 */
void execute_sequential(uint64_t total_points, uint64_t sleep_ns);

/**
 * Run the parallel part of the experiment without any waiting.
 * @param num_threads Number of worker threads to produce.
 * @param points_per_thread Number of points processed per thread.
 * @param sleep_ns Number of ns to sleep between each point.
 */
void execute_parallel(uint64_t num_threads, uint64_t points_per_thread,
                      uint64_t sleep_ns);

int main() {
  ios_base::sync_with_stdio(false);
  static constexpr uint64_t POINTS_PER_TYPE = 1 << 15;
  static const auto THREADS_PER_TYPE = thread::hardware_concurrency() / 2;
  static constexpr uint64_t SLEEP_NS = 50;
  cout << "MONTE CARLO PI ESTIMATOR\n------------------------\n"
       << "\tAdditional " << SLEEP_NS << " ns added per point.\n";

  execute_monitor(THREADS_PER_TYPE, 2 * POINTS_PER_TYPE, SLEEP_NS);
  execute_sequential(2 * THREADS_PER_TYPE * POINTS_PER_TYPE, SLEEP_NS);
  execute_parallel(2 * THREADS_PER_TYPE, POINTS_PER_TYPE, SLEEP_NS);
}

void report_time(nanoseconds dur) {
  const auto output_dur = duration_cast<milliseconds>(dur);
  cout << "\tElapsed time: " << output_dur.count() << " ms\n";
}

void report_accuracy(uint64_t in_circle, uint64_t total) {
  const auto est =
      static_cast<double>(4 * in_circle) / static_cast<double>(total);
  const auto error = (est < M_PI ? M_PI - est : est - M_PI) / M_PI;
  cout << "\tEstimate: " << est << "\n\tPercent error: " << 100 * error << '\n';
}

void execute_monitor(uint64_t threads_per_type, uint64_t points_per_thread,
                     uint64_t sleep_ns) {
  cout << "Monitor execution using blocking_queue.\n"
       << "Running " << threads_per_type << " producers and "
       << threads_per_type << " consumers, each processing "
       << points_per_thread << " points..." << endl;
  blocking_queue<pair<double, double>> points;
  atomic<uint64_t> in_circle(0);

  auto produce = [&] {
    const auto seed = system_clock::now().time_since_epoch().count();
    default_random_engine gen(seed);
    uniform_real_distribution<double> distr(-1.0, 1.0);
    for (uint64_t i = 0; i < points_per_thread; ++i) {
      sleep_for(nanoseconds(sleep_ns));
      const auto pt = make_pair(distr(gen), distr(gen));
      points.push(pt);
    }
  };

  auto consume = [&] {
    for (uint64_t i = 0; i < points_per_thread; ++i) {
      sleep_for(nanoseconds(sleep_ns));
      const auto pt = points.pop();
      const auto dist_sq = pt.first * pt.first + pt.second * pt.second;
      if (dist_sq < 1.0) ++in_circle;
    }
  };

  vector<thread> producers, consumers;
  producers.reserve(threads_per_type);
  consumers.reserve(threads_per_type);

  const auto start = high_resolution_clock::now();
  for (uint64_t i = 0; i < threads_per_type; ++i) {
    producers.emplace_back(produce);
    consumers.emplace_back(consume);
  }
  for (auto& producer : producers) producer.join();
  for (auto& consumer : consumers) consumer.join();

  report_time(high_resolution_clock::now() - start);
  report_accuracy(in_circle.load(), threads_per_type * points_per_thread);
}

void execute_sequential(uint64_t total_points, uint64_t sleep_ns) {
  cout << "Sequential execution using iteration.\n"
       << "Processing " << total_points << " points iteratively..." << endl;

  const auto seed = system_clock::now().time_since_epoch().count();
  default_random_engine gen(seed);
  uniform_real_distribution<double> distr(-1.0, 1.0);
  uint64_t in_circle = 0;

  const auto start = high_resolution_clock::now();
  for (uint64_t i = 0; i < total_points; ++i) {
    sleep_for(nanoseconds(sleep_ns));
    const auto first = distr(gen);
    const auto second = distr(gen);
    if (first * first + second * second < 1.0) ++in_circle;
  }

  report_time(high_resolution_clock::now() - start);
  report_accuracy(in_circle, total_points);
}

void execute_parallel(uint64_t num_threads, uint64_t points_per_thread,
                      uint64_t sleep_ns) {
  cout << "Parallel execution using non-blocking threads.\n"
       << "Running " << num_threads << " threads, each processing "
       << points_per_thread << " points..." << endl;

  vector<uint64_t> counts(num_threads, 0);
  auto worker = [&](uint64_t idx) {
    const auto seed = system_clock::now().time_since_epoch().count();
    default_random_engine gen(seed);
    uniform_real_distribution<double> distr(-1.0, 1.0);
    for (uint64_t i = 0; i < points_per_thread; ++i) {
      sleep_for(nanoseconds(sleep_ns));
      const auto first = distr(gen);
      const auto second = distr(gen);
      if (first * first + second * second < 1.0) ++counts[idx];
    }
  };

  vector<thread> threads;
  threads.reserve(num_threads);

  const auto start = high_resolution_clock::now();
  for (uint64_t idx = 0; idx < num_threads; ++idx)
    threads.emplace_back(worker, idx);
  for (auto& thread : threads) thread.join();

  report_time(high_resolution_clock::now() - start);
  report_accuracy(accumulate(counts.begin(), counts.end(), 0),
                  num_threads * points_per_thread);
}
