#include <chrono>
#include <cmp/concurrent_memory_pool.hpp>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  cmp::ConcurrentMemoryPool pool;
  constexpr int workers = 8;
  constexpr int iterations = 100000;
  const auto begin = std::chrono::steady_clock::now();
  std::vector<std::thread> threads;
  for (int i = 0; i < workers; ++i) threads.emplace_back([&] {
    for (int n = 0; n < iterations; ++n) {
      void* block = pool.allocate(64);
      pool.deallocate(block, 64);
    }
  });
  for (auto& thread : threads) thread.join();
  const auto elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - begin).count();
  std::cout << workers * iterations << " alloc/free pairs in "
            << elapsed << " seconds\n";
}
