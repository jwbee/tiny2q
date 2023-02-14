#pragma once

#include <array>
#include <cstddef>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <memory_resource>

#include "absl/container/flat_hash_map.h"

#include "recents.h"

namespace tiny2q {

enum class condition { frequent, recent, absent };

template <typename K, typename V> class cache {
public:
  cache(long count)
      : arena_(new K[count_to_size_ratio * count]),
        mbr_(arena_.get(), count_to_size_ratio * count), pa_(&mbr_),
        frequent_lru_(count, sentinel, pa_),
        recents_(count / frequent_to_recent_ratio) {
    frequent_map_.reserve(count);
  }

  // Modifies *v only when return value is condition::frequent.
  condition test(const K &k, V *const v) {
    auto it = frequent_map_.find(k);
    if (it != frequent_map_.end()) {
        auto& hit_v = it->second;
      *v = hit_v.v;
      // This is now the most recently accessed frequent key. If it was not
      // already, move it to the front of the frequent keys.
      if (hit_v.it != frequent_lru_.begin()) {
        frequent_lru_.splice(frequent_lru_.begin(), frequent_lru_, hit_v.it);
      }
      return condition::frequent;
    }
    if (recents_.test(k)) {
      return condition::recent;
    }
    return condition::absent;
  }

  void observe(condition c, const K &k, V v) {
    switch (c) {
    case condition::frequent:
      break;
    case condition::recent: {
      // Promote a recent key to frequent. The least-recently-used item at the
      // back of the list will be overwritten.
      auto victim_it = std::prev(frequent_lru_.end());
      if (*victim_it != sentinel) {
        // We are evicting from frequent_map_. This is the common case, except
        // when the cache is being initially filled.
        frequent_map_.erase(*victim_it);
      }
      *victim_it = k;
      // Move k to front of the list because it is now the most recent key.
      frequent_lru_.splice(frequent_lru_.begin(), frequent_lru_, victim_it);
      frequent_map_.insert({k, {v, victim_it}});
    } break;
    case condition::absent:
      // Note key as recent. Do nothing
      recents_.add(k);
      break;
    }
  }

private:
  static constexpr auto sentinel = std::numeric_limits<K>::max();
  static constexpr auto frequent_to_recent_ratio = 8;
  static constexpr auto count_to_size_ratio = 32;

  using lru_iterator_type = typename std::pmr::list<K>::iterator;

  struct value {
    V v;
    lru_iterator_type it;
  };

  std::unique_ptr<K[]> arena_;
  std::pmr::monotonic_buffer_resource mbr_;
  std::pmr::polymorphic_allocator<K> pa_;
  std::pmr::list<K> frequent_lru_;
  absl::flat_hash_map<K, value> frequent_map_;
  recents<K> recents_;
};
} // namespace tiny2q