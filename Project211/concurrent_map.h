#pragma once

#include <cstdlib>
#include <future>
#include <mutex>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <type_traits>

#include "HashedString.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

public:
    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket) : guard(bucket.mutex), ref_to_value(bucket.map[key]) {}
    };

    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count) {}

    Access operator[](const Key& key) {
        return { key, GetBucket(key) };
    }

    void erase(const Key& key) {
        Bucket& bucket = GetBucket(key);
        std::lock_guard guard(bucket.mutex);
        bucket.map.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets_) {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    std::vector<Bucket> buckets_;

    Bucket& GetBucket(const Key& key) {
        if constexpr (std::is_integral_v<Key>) {
            return buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        }
        else {
            uint64_t k = reinterpret_cast<uint64_t>(HashedString::hash_name(key));
            return buckets_[k % buckets_.size()];
        }
    }
};