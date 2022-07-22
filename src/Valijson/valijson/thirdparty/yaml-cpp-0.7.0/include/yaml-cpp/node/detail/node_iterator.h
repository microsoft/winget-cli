#ifndef VALUE_DETAIL_NODE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "yaml-cpp/dll.h"
#include "yaml-cpp/node/ptr.h"
#include <cstddef>
#include <iterator>
#include <memory>
#include <map>
#include <utility>
#include <vector>

namespace YAML {
namespace detail {
struct iterator_type {
  enum value { NoneType, Sequence, Map };
};

template <typename V>
struct node_iterator_value : public std::pair<V*, V*> {
  using kv = std::pair<V*, V*>;

  node_iterator_value() : kv(), pNode(nullptr) {}
  explicit node_iterator_value(V& rhs) : kv(), pNode(&rhs) {}
  explicit node_iterator_value(V& key, V& value) : kv(&key, &value), pNode(nullptr) {}

  V& operator*() const { return *pNode; }
  V& operator->() const { return *pNode; }

  V* pNode;
};

using node_seq = std::vector<node *>;
using node_map = std::vector<std::pair<node*, node*>>;

template <typename V>
struct node_iterator_type {
  using seq = node_seq::iterator;
  using map = node_map::iterator;
};

template <typename V>
struct node_iterator_type<const V> {
  using seq = node_seq::const_iterator;
  using map = node_map::const_iterator;
};

template <typename V>
class node_iterator_base {
 private:
  struct enabler {};

  struct proxy {
    explicit proxy(const node_iterator_value<V>& x) : m_ref(x) {}
    node_iterator_value<V>* operator->() { return std::addressof(m_ref); }
    operator node_iterator_value<V>*() { return std::addressof(m_ref); }

    node_iterator_value<V> m_ref;
  };

 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = node_iterator_value<V>;
  using difference_type = std::ptrdiff_t;
  using pointer = node_iterator_value<V>*;
  using reference = node_iterator_value<V>;
  using SeqIter = typename node_iterator_type<V>::seq;
  using MapIter = typename node_iterator_type<V>::map;

  node_iterator_base()
      : m_type(iterator_type::NoneType), m_seqIt(), m_mapIt(), m_mapEnd() {}
  explicit node_iterator_base(SeqIter seqIt)
      : m_type(iterator_type::Sequence),
        m_seqIt(seqIt),
        m_mapIt(),
        m_mapEnd() {}
  explicit node_iterator_base(MapIter mapIt, MapIter mapEnd)
      : m_type(iterator_type::Map),
        m_seqIt(),
        m_mapIt(mapIt),
        m_mapEnd(mapEnd) {
    m_mapIt = increment_until_defined(m_mapIt);
  }

  template <typename W>
  node_iterator_base(const node_iterator_base<W>& rhs,
                     typename std::enable_if<std::is_convertible<W*, V*>::value,
                                             enabler>::type = enabler())
      : m_type(rhs.m_type),
        m_seqIt(rhs.m_seqIt),
        m_mapIt(rhs.m_mapIt),
        m_mapEnd(rhs.m_mapEnd) {}

  template <typename>
  friend class node_iterator_base;

  template <typename W>
  bool operator==(const node_iterator_base<W>& rhs) const {
    if (m_type != rhs.m_type)
      return false;

    switch (m_type) {
      case iterator_type::NoneType:
        return true;
      case iterator_type::Sequence:
        return m_seqIt == rhs.m_seqIt;
      case iterator_type::Map:
        return m_mapIt == rhs.m_mapIt;
    }
    return true;
  }

  template <typename W>
  bool operator!=(const node_iterator_base<W>& rhs) const {
    return !(*this == rhs);
  }

  node_iterator_base<V>& operator++() {
    switch (m_type) {
      case iterator_type::NoneType:
        break;
      case iterator_type::Sequence:
        ++m_seqIt;
        break;
      case iterator_type::Map:
        ++m_mapIt;
        m_mapIt = increment_until_defined(m_mapIt);
        break;
    }
    return *this;
  }

  node_iterator_base<V> operator++(int) {
    node_iterator_base<V> iterator_pre(*this);
    ++(*this);
    return iterator_pre;
  }

  value_type operator*() const {
    switch (m_type) {
      case iterator_type::NoneType:
        return value_type();
      case iterator_type::Sequence:
        return value_type(**m_seqIt);
      case iterator_type::Map:
        return value_type(*m_mapIt->first, *m_mapIt->second);
    }
    return value_type();
  }

  proxy operator->() const { return proxy(**this); }

  MapIter increment_until_defined(MapIter it) {
    while (it != m_mapEnd && !is_defined(it))
      ++it;
    return it;
  }

  bool is_defined(MapIter it) const {
    return it->first->is_defined() && it->second->is_defined();
  }

 private:
  typename iterator_type::value m_type;

  SeqIter m_seqIt;
  MapIter m_mapIt, m_mapEnd;
};

using node_iterator = node_iterator_base<node>;
using const_node_iterator = node_iterator_base<const node>;
}
}

#endif  // VALUE_DETAIL_NODE_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
