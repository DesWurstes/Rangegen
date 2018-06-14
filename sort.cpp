#include <algorithm>
#include <vector>
#include <cstring>

#include <iostream>
#include <cstdio>

#if 0
static bool cmp(unsigned char (*a), unsigned char (*b)) {
  std::cout << "hai!\n";
  if (a[0] < b[0]) return true;
  if (a[0] > b[0]) return false;
  if (a[1] < b[1]) return true;
  if (a[1] > b[1]) return false;
  for (int i = 2; i < 40; i++) {
    if (a[i] < b[i]) return true;
    if (a[i] > b[i]) return false;
  }
  return false;
}

static bool cmp_tmp(long int a, long int b) {
  bool k = cmp((unsigned char *)a,(unsigned char *) b);
  std::cout << k << " bool\n";
  return k;
}
#endif

class wrapper {
    unsigned char* base;
    bool own;            // a trick to allow temporaries: only them have own == true
public:
    // constructor using a existing array
    wrapper(unsigned char* arr): base(arr), own(false) {}

    ~wrapper() {              // destructor
        if (own) {
            delete[] base;    // destruct array for a temporary wrapper
        }
    }
    // move constructor - in fact copy to a temporary array
    wrapper(wrapper&& src): base(new unsigned char[40]), own(true) {
        for(int i = 0; i < 40; i++) {
            base[i] = src.base[i];
        }
    }
    // move assignment operator - in fact also copy
    wrapper& operator = (wrapper&& src) {
        for(int i = 0; i < 40; i++) {
            base[i] = src.base[i];
        }
        return *this;
    }
    // native ordering based on lexicographic string order
    bool operator <(const wrapper& other) const {
        if (base[0] < other.base[0]) return true;
        if (base[0] > other.base[0]) return false;
        if (base[1] < other.base[1]) return true;
        if (base[1] > other.base[1]) return false;
        for (int i = 2; i < 40; i++) {
          if (base[i] < other.base[i]) return true;
          if (base[i] > other.base[i]) return false;
        }
        return false;
    }
    const unsigned char* value() const {
      // access to the underlying string for tests
        return base;
    }
};

extern "C" int sorter(unsigned char ranges_arr[][40], int range_count) noexcept {
  std::vector<wrapper> v{ranges_arr, &ranges_arr[range_count]};
  try {
    std::sort(v.begin(), v.end());
    // std::bad_alloc& ba
  } catch (...) {
    return 0;
  }
  return 1;
}
