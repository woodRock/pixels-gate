#pragma once
#include <algorithm>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace PixelsEngine {

using Entity = uint32_t;
const Entity INVALID_ENTITY = 0xFFFFFFFF;

class ComponentPool {
public:
  virtual ~ComponentPool() = default;
  virtual void Remove(Entity entity) = 0;
};

template <typename T> class TCompPool : public ComponentPool {
public:
  void Remove(Entity entity) override { m_Components.erase(entity); }

  T &Add(Entity entity, T component) {
    m_Components[entity] = component;
    return m_Components[entity];
  }

  T *Get(Entity entity) {
    if (m_Components.find(entity) != m_Components.end()) {
      return &m_Components[entity];
    }
    return nullptr;
  }

  bool Has(Entity entity) const {
    return m_Components.find(entity) != m_Components.end();
  }

  std::unordered_map<Entity, T> &GetAll() { return m_Components; }

private:
  std::unordered_map<Entity, T> m_Components;
};

class Registry {
public:
  Entity CreateEntity() {
    Entity entity = m_NextEntity++;
    m_Entities.insert(entity);
    return entity;
  }

  void DestroyEntity(Entity entity) {
    m_Entities.erase(entity);
    for (auto &pair : m_ComponentPools) {
      pair.second->Remove(entity);
    }
  }

  bool Valid(Entity entity) const {
    return m_Entities.find(entity) != m_Entities.end();
  }

  template <typename T> T &AddComponent(Entity entity, T component) {
    return GetPool<T>()->Add(entity, component);
  }

  template <typename T> T *GetComponent(Entity entity) {
    return GetPool<T>()->Get(entity);
  }

  template <typename T> bool HasComponent(Entity entity) {
    return GetPool<T>()->Has(entity);
  }

  template <typename T> void RemoveComponent(Entity entity) {
    GetPool<T>()->Remove(entity);
  }

  template <typename T> std::unordered_map<Entity, T> &View() {
    return GetPool<T>()->GetAll();
  }

private:
  template <typename T> TCompPool<T> *GetPool() {
    auto index = std::type_index(typeid(T));
    if (m_ComponentPools.find(index) == m_ComponentPools.end()) {
      m_ComponentPools[index] = std::make_unique<TCompPool<T>>();
    }
    return static_cast<TCompPool<T> *>(m_ComponentPools[index].get());
  }

  Entity m_NextEntity = 0;
  std::unordered_set<Entity> m_Entities;
  std::unordered_map<std::type_index, std::unique_ptr<ComponentPool>>
      m_ComponentPools;
};

} // namespace PixelsEngine
