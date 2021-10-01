# Blocking Queue

A thread safe, templated, blocking queue encapsulating a monitor. Allows multiple threads to concurrently read and write an arbitrary amount of data into a FIFO queue.

## Description

The `blocking_queue` class has methods `push` to enqueue an element and `pop` to dequeue an element. In addition, `empty` and `size` methods provide info about the number of elements. All operations are potentially blocking. Copy construction and assignment is disallowed for `blocking_queue`. To use the blocking queue, simply add `#include "blocking_queue.h"`.

## Monte Carlo Benchmark

In `monte_carlo.cpp`, the same Monte Carlo experiment (computing the value of pi) is conducted in 3 different ways.

- Monitor: Equal numbers of producers and consumers are created. Producers push randomly generated points onto a `blocking_queue`. Consumers pop points off the `blocking_queue` and process them.
- Sequential: A classic iterative Monte Carlo algorithm written with a `for`-loop.
- Parallel: The same iterative algorithm where the work is split into a number of threads. Their individual result is aggregated at the end.

A single point can be processed almost immediately. To simulate a more expensive operation, a miniscule wait time is added before processing each point. This is achieved using `std::this_thread::sleep_for`.

Compile the benchmark program with the `Makefile`. Sample output is shown below.
```text
MONTE CARLO PI ESTIMATOR
------------------------
        Additional 50 ns added per point.
Monitor execution using blocking_queue.
Running 6 producers and 6 consumers, each processing 65536 points...
        Elapsed time: 5055 ms
        Estimate: 3.14134
        Percent error: 0.00813526
Sequential execution using iteration.
Processing 393216 points iteratively...
        Elapsed time: 27848 ms
        Estimate: 3.14122
        Percent error: 0.0120209
Parallel execution using non-blocking threads.
Running 12 threads, each processing 32768 points...
        Elapsed time: 2367 ms
        Estimate: 3.13972
        Percent error: 0.0596197
```
