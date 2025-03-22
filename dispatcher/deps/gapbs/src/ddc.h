#pragma once
#include <sys/mman.h>

#ifndef MAP_DDC
#define MAP_DDC 0
#endif


template<typename T>
T* alloc_array_ddc(size_t n){
  size_t total_size = sizeof(T) * n;
  total_size = ((total_size + 4096 - 1) & ~(4096-1));

  void* addr = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_DDC, -1, 0);
  if (addr == MAP_FAILED) {
    return nullptr;
  }
  T* array = reinterpret_cast<T*>(addr);
  return array;
}