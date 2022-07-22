#ifndef NODE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/node/detail/memory.h"
#include "yaml-cpp/node/detail/node.h"
#include "yaml-cpp/node/iterator.h"
#include "yaml-cpp/node/node.h"
#include <sstream>
#include <string>

namespace YAML {
inline Node::Node()
    : m_isValid(true), m_invalidKey{}, m_pMemory(nullptr), m_pNode(nullptr) {}

inline Node::Node(NodeType::value type)
    : m_isValid(true),
      m_invalidKey{},
      m_pMemory(new detail::memory_holder),
      m_pNode(&m_pMemory->create_node()) {
  m_pNode->set_type(type);
}

template <typename T>
inline Node::Node(const T& rhs)
    : m_isValid(true),
      m_invalidKey{},
      m_pMemory(new detail::memory_holder),
      m_pNode(&m_pMemory->create_node()) {
  Assign(rhs);
}

inline Node::Node(const detail::iterator_value& rhs)
    : m_isValid(rhs.m_isValid),
      m_invalidKey(rhs.m_invalidKey),
      m_pMemory(rhs.m_pMemory),
      m_pNode(rhs.m_pNode) {}

inline Node::Node(const Node& rhs) = default;

inline Node::Node(Zombie)
    : m_isValid(false), m_invalidKey{}, m_pMemory{}, m_pNode(nullptr) {}

inline Node::Node(Zombie, const std::string& key)
    : m_isValid(false), m_invalidKey(key), m_pMemory{}, m_pNode(nullptr) {}

inline Node::Node(detail::node& node, detail::shared_memory_holder pMemory)
    : m_isValid(true), m_invalidKey{}, m_pMemory(pMemory), m_pNode(&node) {}

inline Node::~Node() = default;

inline void Node::EnsureNodeExists() const {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  if (!m_pNode) {
    m_pMemory.reset(new detail::memory_holder);
    m_pNode = &m_pMemory->create_node();
    m_pNode->set_null();
  }
}

inline bool Node::IsDefined() const {
  if (!m_isValid) {
    return false;
  }
  return m_pNode ? m_pNode->is_defined() : true;
}

inline Mark Node::Mark() const {
  if (!m_isValid) {
    throw InvalidNode(m_invalidKey);
  }
  return m_pNode ? m_pNode->mark() : Mark::null_mark();
}

inline NodeType::value Node::Type() const {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  return m_pNode ? m_pNode->type() : NodeType::Null;
}

// access

// template helpers
template <typename T, typename S>
struct as_if {
  explicit as_if(const Node& node_) : node(node_) {}
  const Node& node;

  T operator()(const S& fallback) const {
    if (!node.m_pNode)
      return fallback;

    T t;
    if (convert<T>::decode(node, t))
      return t;
    return fallback;
  }
};

template <typename S>
struct as_if<std::string, S> {
  explicit as_if(const Node& node_) : node(node_) {}
  const Node& node;

  std::string operator()(const S& fallback) const {
    if (node.Type() == NodeType::Null)
      return "null";
    if (node.Type() != NodeType::Scalar)
      return fallback;
    return node.Scalar();
  }
};

template <typename T>
struct as_if<T, void> {
  explicit as_if(const Node& node_) : node(node_) {}
  const Node& node;

  T operator()() const {
    if (!node.m_pNode)
      throw TypedBadConversion<T>(node.Mark());

    T t;
    if (convert<T>::decode(node, t))
      return t;
    throw TypedBadConversion<T>(node.Mark());
  }
};

template <>
struct as_if<std::string, void> {
  explicit as_if(const Node& node_) : node(node_) {}
  const Node& node;

  std::string operator()() const {
    if (node.Type() == NodeType::Null)
      return "null";
    if (node.Type() != NodeType::Scalar)
      throw TypedBadConversion<std::string>(node.Mark());
    return node.Scalar();
  }
};

// access functions
template <typename T>
inline T Node::as() const {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  return as_if<T, void>(*this)();
}

template <typename T, typename S>
inline T Node::as(const S& fallback) const {
  if (!m_isValid)
    return fallback;
  return as_if<T, S>(*this)(fallback);
}

inline const std::string& Node::Scalar() const {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  return m_pNode ? m_pNode->scalar() : detail::node_data::empty_scalar();
}

inline const std::string& Node::Tag() const {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  return m_pNode ? m_pNode->tag() : detail::node_data::empty_scalar();
}

inline void Node::SetTag(const std::string& tag) {
  EnsureNodeExists();
  m_pNode->set_tag(tag);
}

inline EmitterStyle::value Node::Style() const {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  return m_pNode ? m_pNode->style() : EmitterStyle::Default;
}

inline void Node::SetStyle(EmitterStyle::value style) {
  EnsureNodeExists();
  m_pNode->set_style(style);
}

// assignment
inline bool Node::is(const Node& rhs) const {
  if (!m_isValid || !rhs.m_isValid)
    throw InvalidNode(m_invalidKey);
  if (!m_pNode || !rhs.m_pNode)
    return false;
  return m_pNode->is(*rhs.m_pNode);
}

template <typename T>
inline Node& Node::operator=(const T& rhs) {
  Assign(rhs);
  return *this;
}

inline Node& Node::operator=(const Node& rhs) {
  if (is(rhs))
    return *this;
  AssignNode(rhs);
  return *this;
}

inline void Node::reset(const YAML::Node& rhs) {
  if (!m_isValid || !rhs.m_isValid)
    throw InvalidNode(m_invalidKey);
  m_pMemory = rhs.m_pMemory;
  m_pNode = rhs.m_pNode;
}

template <typename T>
inline void Node::Assign(const T& rhs) {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  AssignData(convert<T>::encode(rhs));
}

template <>
inline void Node::Assign(const std::string& rhs) {
  EnsureNodeExists();
  m_pNode->set_scalar(rhs);
}

inline void Node::Assign(const char* rhs) {
  EnsureNodeExists();
  m_pNode->set_scalar(rhs);
}

inline void Node::Assign(char* rhs) {
  EnsureNodeExists();
  m_pNode->set_scalar(rhs);
}

inline void Node::AssignData(const Node& rhs) {
  EnsureNodeExists();
  rhs.EnsureNodeExists();

  m_pNode->set_data(*rhs.m_pNode);
  m_pMemory->merge(*rhs.m_pMemory);
}

inline void Node::AssignNode(const Node& rhs) {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  rhs.EnsureNodeExists();

  if (!m_pNode) {
    m_pNode = rhs.m_pNode;
    m_pMemory = rhs.m_pMemory;
    return;
  }

  m_pNode->set_ref(*rhs.m_pNode);
  m_pMemory->merge(*rhs.m_pMemory);
  m_pNode = rhs.m_pNode;
}

// size/iterator
inline std::size_t Node::size() const {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  return m_pNode ? m_pNode->size() : 0;
}

inline const_iterator Node::begin() const {
  if (!m_isValid)
    return const_iterator();
  return m_pNode ? const_iterator(m_pNode->begin(), m_pMemory)
                 : const_iterator();
}

inline iterator Node::begin() {
  if (!m_isValid)
    return iterator();
  return m_pNode ? iterator(m_pNode->begin(), m_pMemory) : iterator();
}

inline const_iterator Node::end() const {
  if (!m_isValid)
    return const_iterator();
  return m_pNode ? const_iterator(m_pNode->end(), m_pMemory) : const_iterator();
}

inline iterator Node::end() {
  if (!m_isValid)
    return iterator();
  return m_pNode ? iterator(m_pNode->end(), m_pMemory) : iterator();
}

// sequence
template <typename T>
inline void Node::push_back(const T& rhs) {
  if (!m_isValid)
    throw InvalidNode(m_invalidKey);
  push_back(Node(rhs));
}

inline void Node::push_back(const Node& rhs) {
  EnsureNodeExists();
  rhs.EnsureNodeExists();

  m_pNode->push_back(*rhs.m_pNode, m_pMemory);
  m_pMemory->merge(*rhs.m_pMemory);
}

template<typename Key>
std::string key_to_string(const Key& key) {
  return streamable_to_string<Key, is_streamable<std::stringstream, Key>::value>().impl(key);
}

// indexing
template <typename Key>
inline const Node Node::operator[](const Key& key) const {
  EnsureNodeExists();
  detail::node* value =
      static_cast<const detail::node&>(*m_pNode).get(key, m_pMemory);
  if (!value) {
    return Node(ZombieNode, key_to_string(key));
  }
  return Node(*value, m_pMemory);
}

template <typename Key>
inline Node Node::operator[](const Key& key) {
  EnsureNodeExists();
  detail::node& value = m_pNode->get(key, m_pMemory);
  return Node(value, m_pMemory);
}

template <typename Key>
inline bool Node::remove(const Key& key) {
  EnsureNodeExists();
  return m_pNode->remove(key, m_pMemory);
}

inline const Node Node::operator[](const Node& key) const {
  EnsureNodeExists();
  key.EnsureNodeExists();
  m_pMemory->merge(*key.m_pMemory);
  detail::node* value =
      static_cast<const detail::node&>(*m_pNode).get(*key.m_pNode, m_pMemory);
  if (!value) {
    return Node(ZombieNode, key_to_string(key));
  }
  return Node(*value, m_pMemory);
}

inline Node Node::operator[](const Node& key) {
  EnsureNodeExists();
  key.EnsureNodeExists();
  m_pMemory->merge(*key.m_pMemory);
  detail::node& value = m_pNode->get(*key.m_pNode, m_pMemory);
  return Node(value, m_pMemory);
}

inline bool Node::remove(const Node& key) {
  EnsureNodeExists();
  key.EnsureNodeExists();
  return m_pNode->remove(*key.m_pNode, m_pMemory);
}

// map
template <typename Key, typename Value>
inline void Node::force_insert(const Key& key, const Value& value) {
  EnsureNodeExists();
  m_pNode->force_insert(key, value, m_pMemory);
}

// free functions
inline bool operator==(const Node& lhs, const Node& rhs) { return lhs.is(rhs); }
}  // namespace YAML

#endif  // NODE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
