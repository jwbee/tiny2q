#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

namespace tiny2q {
// recents is a circular buffer of recently-seen keys of type T and capacity sz.
template <typename T> class recents {
public:
  explicit recents(size_t sz) : cur_(0), v_(sz, std::numeric_limits<T>::max()) {}
  bool test(const T &c) const {
    return std::find(v_.cbegin(), v_.cend(), c) != v_.cend();
  }
  void add(const T &c) {
    v_[cur_] = c;
    cur_ += 1;
    cur_ %= v_.size();
  }

private:
  typename std::vector<T>::size_type cur_;
  std::vector<T> v_;
};
} // namespace tiny2q