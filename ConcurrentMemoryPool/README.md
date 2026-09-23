# ConcurrentMemoryPool

C++20 并发小对象内存池。为每个线程维护本地空闲链表，并通过按大小分类的中央缓存批量补给，减少多线程频繁分配时的锁竞争。超过 256 KiB 的分配安全地回退到全局 `operator new`。

## 特性

- Header-only，无第三方依赖
- 208 个大小类别，覆盖 1 B 至 256 KiB
- 线程本地缓存 + 分桶中央缓存
- `allocate` / `deallocate` 与类型化 `create` / `destroy` API
- CMake、CTest 及 GitHub Actions CI

## 使用

```cpp
#include <cmp/concurrent_memory_pool.hpp>

cmp::ConcurrentMemoryPool pool;
auto* value = pool.create<int>(42);
pool.destroy(value);
```

`deallocate` 必须传入与 `allocate` 相同的字节数。实例可由多个线程并发使用；对齐要求超过 `std::max_align_t` 的类型不在当前版本支持范围内。

## 构建和测试

要求：CMake 3.20+ 与 C++20 编译器。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Windows + Visual Studio 多配置生成器下，基准程序通常位于 `build/Release/cmp_benchmark.exe`。

## 许可证

MIT License，详见 [LICENSE](LICENSE)。
