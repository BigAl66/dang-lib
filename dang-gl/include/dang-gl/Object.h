#pragma once

#include "Context.h"
#include "ObjectHandle.h"
#include "ObjectType.h"
#include "ObjectWrapper.h"

namespace dang::gl
{

/// <summary>Serves as a base class for all GL-Objects of the template specified type.</summary>
template <ObjectType Type>
class Object {
public:
    using Handle = ObjectHandle<Type>;
    using Wrapper = ObjectWrapper<Type>;

    /// <summary>Destroys the GL-Object.</summary>
    ~Object()
    {
        destroy();
    }

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    void destroy()
    {
        if (!*this)
            return;
        Wrapper::destroy(handle_);
        handle_ = {};
        context_ = nullptr;
    }

    /// <summary>For valid objects, returns the associated GL-Context in form of a window.</summary>
    Context& context() const noexcept
    {
        return *context_;
    }

    /// <summary>Returns the context for this object type.</summary>
    auto& objectContext() const
    {
        return context_->contextFor<Type>();
    }

    /// <summary>Returns the handle of the GL-Object or InvalidHandle for default constructed objects.</summary>
    Handle handle() const noexcept
    {
        return handle_;
    }

    /// <summary>Whether the object is valid.</summary>
    explicit operator bool() const noexcept
    {
        return bool{ handle_ };
    }

    void swap(Object& other) noexcept
    {
        using std::swap;
        swap(context_, other.context_);
        swap(handle_, other.handle_);
        swap(label_, other.label_);
    }

    friend void swap(Object& lhs, Object& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    /// <summary>Sets an optional label for the object, which is used in by OpenGL generated debug messages.</summary>
    void setLabel(std::optional<std::string> label)
    {
        label_ = std::move(label);
        if (label_)
            glObjectLabel(toGLConstant(Type), handle_.unwrap(), static_cast<GLsizei>(label_->length()), label_->c_str());
        else
            glObjectLabel(toGLConstant(Type), handle_.unwrap(), 0, nullptr);
    }

    /// <summary>Returns the label used in OpenGL generated debug messages.</summary>
    const std::optional<std::string>& label() const
    {
        return label_;
    }

protected:
    Object()
        : context_(Context::current)
        , handle_(Wrapper::create())
    {
        assert(context_);
    }

    Object(Object&& other) noexcept
        : context_(std::move(other.context_))
        , handle_(std::exchange(other.handle_, {}))
        , label_(std::move(other.label_))
    {
    }

    Object& operator=(Object&& other) noexcept
    {
        if (this == &other)
            return *this;
        destroy();
        context_ = std::move(other.context_);
        handle_ = std::exchange(other.handle_, {});
        label_ = std::move(other.label_);
        return *this;
    }

private:
    Context* context_ = nullptr;
    Handle handle_;
    std::optional<std::string> label_;
};

/// <summary>A base class for GL-Objects, which can be bound without a target.</summary>
template <ObjectType Type>
class ObjectBindable : public Object<Type> {
public:
    /// <summary>Resets the bound object in the context if the object is still bound.</summary>
    ~ObjectBindable()
    {
        if (*this)
            this->objectContext().reset(this->handle());
    }

    ObjectBindable(const ObjectBindable&) = delete;
    ObjectBindable& operator=(const ObjectBindable&) = delete;

    /// <summary>Binds the object.</summary>
    void bind() const
    {
        this->objectContext().bind(this->handle());
    }

protected:
    ObjectBindable() = default;

    ObjectBindable(ObjectBindable&&) = default;
    ObjectBindable& operator=(ObjectBindable&&) = default;
};

}
