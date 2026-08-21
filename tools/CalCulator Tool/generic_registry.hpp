// generic_registry.hpp
// =========================================================================
// A small, reusable, generic key/value registry template.
//
// C++17 feature: Template class
//   `Registry<Key, Value>` is not specific to tools at all — it is the same
//   generic map-with-manners that ANY subsystem needing "register by name,
//   look up later, list, remove" could reuse (an EventBus<Event> would be
//   built the exact same way). `ToolRegistry` (tool_registry.hpp) is built
//   directly on top of `Registry<std::string, Tool>` instead of hand-rolling
//   an unordered_map again.
//
// C++20 feature: Concepts
//   `RegistryValue` constrains what may be stored: it must be move-
//   constructible (so the registry can keep values by value efficiently).
//   The class template itself is constrained with a `requires` clause, so a
//   bad instantiation fails at the template definition with a readable
//   diagnostic instead of a wall of substitution-failure noise deep inside
//   std::unordered_map.
// =========================================================================
#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

template <typename Value>
concept RegistryValue = std::is_move_constructible_v<Value>;

template <typename Key, RegistryValue Value>
class Registry {
public:
    Registry() = default;

    // Inserts `value` under `key`. Returns false (without overwriting) if
    // `key` already exists and `overwrite` is false.
    bool insert(const Key& key, Value value, bool overwrite = false) {
        if (items_.contains(key) && !overwrite) {
            return false;
        }
        items_.insert_or_assign(key, std::move(value));
        return true;
    }

    void erase(const Key& key) { items_.erase(key); }

    // std::optional<T> (C++17): lookup can legitimately be empty — the key
    // may simply not exist yet. Returning a reference wrapper avoids a copy
    // of `Value` while still expressing "may be absent" in the type itself.
    [[nodiscard]] std::optional<std::reference_wrapper<const Value>> tryGet(const Key& key) const {
        auto it = items_.find(key);
        if (it == items_.end()) return std::nullopt;
        return std::cref(it->second);
    }

    [[nodiscard]] const Value* find(const Key& key) const {
        auto it = items_.find(key);
        return it == items_.end() ? nullptr : &it->second;
    }

    [[nodiscard]] bool contains(const Key& key) const { return items_.contains(key); }

    [[nodiscard]] std::size_t size() const { return items_.size(); }

    [[nodiscard]] std::vector<Key> keys() const {
        std::vector<Key> out;
        out.reserve(items_.size());
        // Range-based for + auto (C++17)
        for (const auto& [key, value] : items_) {  // structured bindings (C++17)
            out.push_back(key);
        }
        return out;
    }

    // Lets external code range-for over the registry directly, e.g.
    //   for (const auto& [name, tool] : toolRegistry.raw()) { ... }
    [[nodiscard]] const std::unordered_map<Key, Value>& raw() const { return items_; }

private:
    std::unordered_map<Key, Value> items_;
};
