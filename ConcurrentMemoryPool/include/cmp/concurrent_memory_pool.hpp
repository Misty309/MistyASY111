#pragma once
#include <array>
#include <cstddef>
#include <mutex>
#include <new>
#include <utility>

namespace cmp {

class ConcurrentMemoryPool {
 public:
  static constexpr std::size_t kMaxPooledSize = 256 * 1024;

  [[nodiscard]] void* allocate(std::size_t bytes) {
    if (bytes == 0) bytes = 1;
    if (bytes > kMaxPooledSize) return ::operator new(bytes);
    const auto index = size_class(bytes);
    auto& local = local_cache().lists[index];
    if (void* block = local.pop()) return block;
    refill(local, index);
    return local.pop();
  }

  void deallocate(void* block, std::size_t bytes) noexcept {
    if (!block) return;
    if (bytes == 0) bytes = 1;
    if (bytes > kMaxPooledSize) { ::operator delete(block); return; }
    const auto index = size_class(bytes);
    auto& local = local_cache().lists[index];
    local.push(block);
    if (local.count > kLocalLimit) flush(local, index, local.count / 2);
  }

  template <class T, class... Args>
  [[nodiscard]] T* create(Args&&... args) {
    static_assert(alignof(T) <= alignof(std::max_align_t),
                  "over-aligned types are unsupported");
    void* storage = allocate(sizeof(T));
    try { return ::new (storage) T(std::forward<Args>(args)...); }
    catch (...) { deallocate(storage, sizeof(T)); throw; }
  }

  template <class T>
  void destroy(T* object) noexcept {
    if (!object) return;
    object->~T();
    deallocate(object, sizeof(T));
  }

 private:
  struct FreeList {
    void* head = nullptr;
    std::size_t count = 0;
    void push(void* block) noexcept {
      *static_cast<void**>(block) = head; head = block; ++count;
    }
    void* pop() noexcept {
      if (!head) return nullptr;
      void* result = head; head = *static_cast<void**>(head); --count;
      return result;
    }
  };
  struct LocalCache { std::array<FreeList, 208> lists{}; };
  static constexpr std::size_t kLocalLimit = 512;
  static constexpr std::size_t kBatch = 64;

  static std::size_t round_up(std::size_t n, std::size_t a) noexcept {
    return (n + a - 1) & ~(a - 1);
  }
  static std::size_t normalized_size(std::size_t n) noexcept {
    if (n <= 128) return round_up(n, 8);
    if (n <= 1024) return round_up(n, 16);
    if (n <= 8192) return round_up(n, 128);
    if (n <= 65536) return round_up(n, 1024);
    return round_up(n, 8192);
  }
  static std::size_t size_class(std::size_t n) noexcept {
    const auto s = normalized_size(n);
    if (s <= 128) return s / 8 - 1;
    if (s <= 1024) return 16 + (s - 128) / 16 - 1;
    if (s <= 8192) return 72 + (s - 1024) / 128 - 1;
    if (s <= 65536) return 128 + (s - 8192) / 1024 - 1;
    return 184 + (s - 65536) / 8192 - 1;
  }
  static std::size_t class_size(std::size_t i) noexcept {
    if (i < 16) return (i + 1) * 8;
    if (i < 72) return 128 + (i - 15) * 16;
    if (i < 128) return 1024 + (i - 71) * 128;
    if (i < 184) return 8192 + (i - 127) * 1024;
    return 65536 + (i - 183) * 8192;
  }
  static LocalCache& local_cache() {
    thread_local LocalCache cache;
    return cache;
  }
  static std::array<FreeList, 208>& central_lists() {
    static std::array<FreeList, 208> lists{};
    return lists;
  }
  static std::array<std::mutex, 208>& central_mutexes() {
    static std::array<std::mutex, 208> locks;
    return locks;
  }
  static void refill(FreeList& target, std::size_t index) {
    std::lock_guard lock(central_mutexes()[index]);
    auto& central = central_lists()[index];
    while (target.count < kBatch && central.head) target.push(central.pop());
    while (target.count < kBatch) target.push(::operator new(class_size(index)));
  }
  static void flush(FreeList& source, std::size_t index, std::size_t n) noexcept {
    std::lock_guard lock(central_mutexes()[index]);
    auto& central = central_lists()[index];
    while (n-- && source.head) central.push(source.pop());
  }
};

}  // namespace cmp
