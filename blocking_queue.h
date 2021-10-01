/*
Thread safe, templated, blocking queue.

Copyright 2021. Andrew Wang.
*/
#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

/**
 * Thread safe, templated, blocking queue.
 */
template <typename T>
class blocking_queue {
 private:
  // Internal queue.
  std::queue<T> m_queue;

  // Synchronization primitives.
  std::mutex m_mutex;
  std::condition_variable m_cv;

 public:
  /**
   * Default constructor initializes empty queue.
   */
  blocking_queue() = default;

  /**
   * Prevent copying construction of blocking queue.
   */
  blocking_queue(const blocking_queue<T>&) = delete;

  /**
   * Prevent assignment of blocking queue.
   */
  blocking_queue<T>& operator=(blocking_queue<T>) = delete;

  /**
   * Determines whether the queue is empty
   * at some non-deterministic time in the future.
   * @returns The queue's emptiness status.
   */
  bool empty();

  /**
   * Determines the number of elements in the queue
   * at some non-deterministic time in the future.
   * @returns The size of the queue.
   */
  size_t size();

  /**
   * Pushes an element onto the queue, blocking if needed.
   * @param elem The item to enqueue.
   */
  void push(const T& elem);

  /**
   * Removes and returns an element from the queue, blocking if needed.
   * @returns the next element in the queue.
   */
  T pop();
};

template <typename T>
bool blocking_queue<T>::empty() {
  std::lock_guard lock(m_mutex);
  return m_queue.empty();
}

template <typename T>
size_t blocking_queue<T>::size() {
  std::lock_guard lock(m_mutex);
  return m_queue.size();
}

template <typename T>
void blocking_queue<T>::push(const T& elem) {
  std::lock_guard lock(m_mutex);
  m_queue.push(elem);
  m_cv.notify_one();
}

template <typename T>
T blocking_queue<T>::pop() {
  std::unique_lock lock(m_mutex);
  m_cv.wait(lock, [this] { return !m_queue.empty(); });
  T elem = std::move(m_queue.front());
  m_queue.pop();
  return elem;
}
