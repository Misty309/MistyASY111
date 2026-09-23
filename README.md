# MistyASY111

## ConcurrentMemoryPool

一个使用 C++20 实现的并发小对象内存池，采用线程本地缓存与按大小分类的中央缓存来减少多线程分配竞争。项目源码、测试、基准和构建说明位于 [ConcurrentMemoryPool](ConcurrentMemoryPool/)。

- 语言：C++20
- 构建：CMake 3.20+
- 范围：1 B–256 KiB 走内存池；更大的请求回退至全局分配器
- 状态：v1.0.0
