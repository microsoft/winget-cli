#ifndef NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "yaml-cpp/node/detail/node.h"
#include "yaml-cpp/node/detail/node_data.h"

#include <algorithm>
#include <type_traits>

namespace YAML {
namespace detail {
template <typename Key, typename Enable = void>
struct get_idx {
  static node* get(const std::vector<node*>& /* sequence */,
                   const Key& /* key */, shared_memory_holder /* pMemory */) {
    return nullptr;
  }
};

template <typename Key>
struct get_idx<Key,
               typename std::enable_if<std::is_unsigned<Key>::value &&
                                       !std::is_same<Key, bool>::value>::type> {
  static node* get(const std::vector<node*>& sequence, const Key& key,
                   shared_memory_holder /* pMemory */) {
    return key < sequence.size() ? sequence[key] : nullptr;
  }

  static node* get(std::vector<node*>& sequence, const Key& key,
                   shared_memory_holder pMemory) {
    if (key > sequence.size() || (key > 0 && !sequence[key - 1]->is_defined()))
      return nullptr;
    if (key == sequence.size())
      sequence.push_back(&pMemory->create_node());
    return sequence[key];
  }
};

template <typename Key>
struct get_idx<Key, typename std::enable_if<std::is_signed<Key>::value>::type> {
  static node* get(const std::vector<node*>& sequence, const Key& key,
                   shared_memory_holder pMemory) {
    return key >= 0 ? get_idx<std::size_t>::get(
                          sequence, static_cast<std::size_t>(key), pMemory)
                    : nullptr;
  }
  static node* get(std::vector<node*>& sequence, const Key& key,
                   shared_memory_holder pMemory) {
    return key >= 0 ? get_idx<std::size_t>::get(
                          sequence, static_cast<std::size_t>(key), pMemory)
                    : nullptr;
  }
};

template <typename Key, typename Enable = void>
struct remove_idx {
  static bool remove(std::vector<node*>&, const Key&, std::size_t&) {
    return false;
  }
};

template <typename Key>
struct remove_idx<
    Key, typename std::enable_if<std::is_unsigned<Key>::value &&
                                 !std::is_same<Key, bool>::value>::type> {

  static bool remove(std::vector<node*>& sequence, const Key& key,
                     std::size_t& seqSize) {
    if (key >= sequence.size()) {
      return false;
    } else {
      sequence.erase(sequence.begin() + key);
      if (seqSize > key) {
          --seqSize;
      }
      return true;
    }
  }
};

template <typename Key>
struct remove_idx<Key,
                  typename std::enable_if<std::is_signed<Key>::value>::type> {

  static bool remove(std::vector<node*>& sequence, const Key& key,
                     std::size_t& seqSize) {
    return key >= 0 ? remove_idx<std::size_t>::remove(
                          sequence, static_cast<std::size_t>(key), seqSize)
                    : false;
  }
};

template <typename T>
inline bool node::equals(const T& rhs, shared_memory_holder pMemory) {
  T lhs;
  if (convert<T>::decode(Node(*this, pMemory), lhs)) {
    return lhs == rhs;
  }
  return false;
}

inline bool node::equals(const char* rhs, shared_memory_holder pMemory) {
  std::string lhs;
  if (convert<std::string>::decode(Node(*this, std::move(pMemory)), lhs)) {
    return lhs == rhs;
  }
  return false;
}

// indexing
template <typename Key>
inline node* node_data::get(const Key& key,
                            shared_memory_holder pMemory) const {
  switch (m_type) {
    case NodeType::Map:
      break;
    case NodeType::Undefined:
    case NodeType::Null:
      return nullptr;
    case NodeType::Sequence:
      if (node* pNode = get_idx<Key>::get(m_sequence, key, pMemory))
        return pNode;
      return nullptr;
    case NodeType::Scalar:
      throw BadSubscript(m_mark, key);
  }

  auto it = std::find_if(m_map.begin(), m_map.end(), [&](const kv_pair m) {
    return m.first->equals(key, pMemory);
  });

  return it != m_map.end() ? it->second : nullptr;
}

template <typename Key>
inline node& node_data::get(const Key& key, shared_memory_holder pMemory) {
  switch (m_type) {
    case NodeType::Map:
      break;
    case NodeType::Undefined:
    case NodeType::Null:
    case NodeType::Sequence:
      if (node* pNode = get_idx<Key>::get(m_sequence, key, pMemory)) {
        m_type = NodeType::Sequence;
        return *pNode;
      }

      convert_to_map(pMemory);
      break;
    case NodeType::Scalar:
      throw BadSubscript(m_mark, key);
  }

  auto it = std::find_if(m_map.begin(), m_map.end(), [&](const kv_pair m) {
    return m.first->equals(key, pMemory);
  });

  if (it != m_map.end()) {
    return *it->second;
  }

  node& k = convert_to_node(key, pMemory);
  node& v = pMemory->create_node();
  insert_map_pair(k, v);
  return v;
}

template <typename Key>
inline bool node_data::remove(const Key& key, shared_memory_holder pMemory) {
  if (m_type == NodeType::Sequence) {
    return remove_idx<Key>::remove(m_sequence, key, m_seqSize);
  }

  if (m_type == NodeType::Map) {
    kv_pairs::iterator it = m_undefinedPairs.begin();
    while (it != m_undefinedPairs.end()) {
      kv_pairs::iterator jt = std::next(it);
      if (it->first->equals(key, pMemory)) {
        m_undefinedPairs.erase(it);
      }
      it = jt;
    }

    auto iter = std::find_if(m_map.begin(), m_map.end(), [&](const kv_pair m) {
      return m.first->equals(key, pMemory);
    });

    if (iter != m_map.end()) {
      m_map.erase(iter);
      return true;
    }
  }

  return false;
}

// map
template <typename Key, typename Value>
inline void node_data::force_insert(const Key& key, const Value& value,
                                    shared_memory_holder pMemory) {
  switch (m_type) {
    case NodeType::Map:
      break;
    case NodeType::Undefined:
    case NodeType::Null:
    case NodeType::Sequence:
      convert_to_map(pMemory);
      break;
    case NodeType::Scalar:
      throw BadInsert();
  }

  node& k = convert_to_node(key, pMemory);
  node& v = convert_to_node(value, pMemory);
  insert_map_pair(k, v);
}

template <typename T>
inline node& node_data::convert_to_node(const T& rhs,
                                        shared_memory_holder pMemory) {
  Node value = convert<T>::encode(rhs);
  value.EnsureNodeExists();
  pMemory->merge(*value.m_pMemory);
  return *value.m_pNode;
}
}
}

#endif  // NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
