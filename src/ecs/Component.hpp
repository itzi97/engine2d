#pragma once

/// Abstract base for all components.
class Component
{
public:
    Component()          = default;
    virtual ~Component() = default;

    Component(const Component&)            = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&)                 = default;
    Component& operator=(Component&&)      = default;

    virtual void Update([[maybe_unused]] float dt) {}
    virtual void Render() {}
};
