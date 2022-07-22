#ifndef SETTING_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define SETTING_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "yaml-cpp/noexcept.h"
#include <memory>
#include <utility>
#include <vector>

namespace YAML {

class SettingChangeBase {
 public:
  virtual ~SettingChangeBase() = default;
  virtual void pop() = 0;
};

template <typename T>
class Setting {
 public:
  Setting() : m_value() {}
  Setting(const T& value) : m_value() { set(value); }

  const T get() const { return m_value; }
  std::unique_ptr<SettingChangeBase> set(const T& value);
  void restore(const Setting<T>& oldSetting) { m_value = oldSetting.get(); }

 private:
  T m_value;
};

template <typename T>
class SettingChange : public SettingChangeBase {
 public:
  SettingChange(Setting<T>* pSetting)
      : m_pCurSetting(pSetting),
        m_oldSetting(*pSetting)  // copy old setting to save its state
  {}
  SettingChange(const SettingChange&) = delete;
  SettingChange(SettingChange&&) = delete;
  SettingChange& operator=(const SettingChange&) = delete;
  SettingChange& operator=(SettingChange&&) = delete;

  void pop() override { m_pCurSetting->restore(m_oldSetting); }

 private:
  Setting<T>* m_pCurSetting;
  Setting<T> m_oldSetting;
};

template <typename T>
inline std::unique_ptr<SettingChangeBase> Setting<T>::set(const T& value) {
  std::unique_ptr<SettingChangeBase> pChange(new SettingChange<T>(this));
  m_value = value;
  return pChange;
}

class SettingChanges {
 public:
  SettingChanges() : m_settingChanges{} {}
  SettingChanges(const SettingChanges&) = delete;
  SettingChanges(SettingChanges&&) YAML_CPP_NOEXCEPT = default;
  SettingChanges& operator=(const SettingChanges&) = delete;
  SettingChanges& operator=(SettingChanges&& rhs) YAML_CPP_NOEXCEPT {
    if (this == &rhs)
      return *this;

    clear();
    std::swap(m_settingChanges, rhs.m_settingChanges);

    return *this;
  }
  ~SettingChanges() { clear(); }

  void clear() YAML_CPP_NOEXCEPT {
    restore();
    m_settingChanges.clear();
  }

  void restore() YAML_CPP_NOEXCEPT {
    for (const auto& setting : m_settingChanges)
      setting->pop();
  }

  void push(std::unique_ptr<SettingChangeBase> pSettingChange) {
    m_settingChanges.push_back(std::move(pSettingChange));
  }

 private:
  using setting_changes = std::vector<std::unique_ptr<SettingChangeBase>>;
  setting_changes m_settingChanges;
};
}  // namespace YAML

#endif  // SETTING_H_62B23520_7C8E_11DE_8A39_0800200C9A66
