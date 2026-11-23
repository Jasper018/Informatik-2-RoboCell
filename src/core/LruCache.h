#pragma once

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <list>
#include <optional>

namespace RoboCell {

/**
 * @brief Thread-sicherer Least-Recently-Used (LRU) Cache für Schichtprotokolle (Erweiterung 5).
 */
template <typename Key, typename Value>
class LruCache {
public:
    explicit LruCache(size_t maxCapacity = 128)
        : m_maxCapacity(maxCapacity > 0 ? maxCapacity : 1) {}

    void setMaxCapacity(size_t cap) {
        QMutexLocker locker(&m_mutex);
        m_maxCapacity = cap > 0 ? cap : 1;
        evictIfNeeded();
    }

    std::optional<Value> get(const Key& key) {
        QMutexLocker locker(&m_mutex);
        auto it = m_map.find(key);
        if (it == m_map.end()) {
            ++m_misses;
            return std::nullopt;
        }
        // Move to front (most recently used)
        m_order.splice(m_order.begin(), m_order, it.value().second);
        ++m_hits;
        return it.value().first;
    }

    void put(const Key& key, const Value& value) {
        QMutexLocker locker(&m_mutex);
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            it.value().first = value;
            m_order.splice(m_order.begin(), m_order, it.value().second);
            return;
        }
        m_order.push_front(key);
        m_map.insert(key, qMakePair(value, m_order.begin()));
        evictIfNeeded();
    }

    void clear() {
        QMutexLocker locker(&m_mutex);
        m_map.clear();
        m_order.clear();
        m_hits = 0;
        m_misses = 0;
    }

    size_t size() const {
        QMutexLocker locker(&m_mutex);
        return m_order.size();
    }

    size_t hits() const {
        QMutexLocker locker(&m_mutex);
        return m_hits;
    }

    size_t misses() const {
        QMutexLocker locker(&m_mutex);
        return m_misses;
    }

private:
    void evictIfNeeded() {
        while (m_order.size() > m_maxCapacity) {
            const Key& oldest = m_order.back();
            m_map.remove(oldest);
            m_order.pop_back();
        }
    }

    size_t m_maxCapacity;
    std::list<Key> m_order;
    using ListIter = typename std::list<Key>::iterator;
    QHash<Key, QPair<Value, ListIter>> m_map;
    size_t m_hits = 0;
    size_t m_misses = 0;
    mutable QMutex m_mutex;
};

} // namespace RoboCell
