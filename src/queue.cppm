module;

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

export module queue;
using std::size_t;

export template <typename T> class CircularQueue {
private:
  std::unique_ptr<T[]> raw_buffer_;
  std::span<T> buffer_;
  size_t size_;
  std::atomic<size_t> head_;
  std::atomic<size_t> tail_;

  static bool is_power_of_two(size_t n) { return (n & (n - 1)) == 0; }

public:
  explicit CircularQueue(size_t n)
      : raw_buffer_(nullptr), buffer_(), size_(n), head_(0), tail_(0) {
    if (!is_power_of_two(n)) {
      throw std::invalid_argument("Size must be a power of 2");
    }
    raw_buffer_ = std::make_unique<T[]>(n);
    buffer_ = std::span<T>(raw_buffer_.get(), n);
  }

  bool push(const T &item) {
    size_t head = head_.load(std::memory_order_relaxed);
    size_t next_head = (head + 1) & (size_ - 1);

    if (next_head == tail_.load(std::memory_order_acquire)) {
      return false; // full
    }

    buffer_[head] = item;
    head_.store(next_head, std::memory_order_release);
    return true;
  }

  std::optional<T> pop() {
    size_t tail = tail_.load(std::memory_order_relaxed);

    if (tail == head_.load(std::memory_order_acquire)) {
      return std::nullopt; // empty
    }

    T item = buffer_[tail];
    tail_.store((tail + 1) & (size_ - 1), std::memory_order_release);
    return item;
  }

  bool empty() const {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
  }

  bool full() const {
    return ((head_.load(std::memory_order_acquire) + 1) & (size_ - 1)) ==
           tail_.load(std::memory_order_acquire);
  }

  size_t capacity() const { return size_; }
};