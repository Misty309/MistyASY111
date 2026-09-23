#include <cmp/concurrent_memory_pool.hpp>
#include <array>
#include <cassert>
#include <thread>
#include <vector>

int main() {
  cmp::ConcurrentMemoryPool pool;
  for (auto size : std::array<std::size_t, 7>{1, 8, 129, 1025, 8193, 65537, 300000}) {
    void* block = pool.allocate(size);
    assert(block != nullptr);
    pool.deallocate(block, size);
  }
  auto* number = pool.create<int>(42);
  assert(*number == 42);
  pool.destroy(number);

  std::vector<std::thread> workers;
  for (int i = 0; i < 8; ++i) workers.emplace_back([&pool] {
    for (int n = 0; n < 20000; ++n) {
      const auto size = static_cast<std::size_t>(n % 4096 + 1);
      void* block = pool.allocate(size);
      pool.deallocate(block, size);
    }
  });
  for (auto& worker : workers) worker.join();
}
