#pragma once
#include <cinttypes>


// TODO: There are more correct ways to implement this

// Utility for placement new
inline void* operator new(std::size_t size, void* t) { return t; }

template<typename T, uint64_t N>
class Pool {
public:
  Pool();
  ~Pool() { }

  bool full() const { return freelist == nullptr; }
  T* alloc();
  void free(T*);

private:
  union element {
    element() { }
    ~element() { }
    element* next;
    T value;
  };
  element* freelist;
  element pool[N];
};

template<typename T, uint64_t N>
Pool<T, N>::Pool()
: freelist(pool)
{
  for(uint64_t i = 0; i < N - 1; ++i) {
    pool[i].next = pool + i + 1;
  }
  pool[N - 1].next = nullptr;
}

template<typename T, uint64_t N>
T* Pool<T, N>::alloc()
{
  if(freelist == nullptr) {
    return nullptr;
  }
  element* e = freelist;
  freelist = freelist->next;
  return &(e->value);
}

template<typename T, uint64_t N>
void Pool<T, N>::free(T* t)
{
  element* e = reinterpret_cast<element*>(t);
  e->next = freelist;
  freelist = e;
}
