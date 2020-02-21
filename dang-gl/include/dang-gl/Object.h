#pragma once

#include "dang-utils/enum.h"

#include "Binding.h"
#include "ObjectBase.h"
#include "Window.h"
#include "GLFW.h"

namespace dang::gl
{

class GLFW;

struct ObjectInfo {
    // Inherite and implement the following:
    // static GLuint create();
    // static void destroy(GLuint handle);
    // static void bind(GLuint handle);

    // static constexpr BindingPoint BindingPoint = BindingPoint::TODO;

    // Specify a custom binding class if necessary:
    // Note: Must be default-constructible
    using Binding = Binding;
};

template <class TInfo>
class Object : public ObjectBase {
public:
    Object()
        : ObjectBase(TInfo::create(), GLFW::Instance.activeWindow())
    {
    }

    ~Object()
    {
        if (handle() != 0)
            TInfo::destroy(handle());
    }

    typename TInfo::Binding& binding() const
    {
        return window().binding<TInfo>();
    }

    void bind()
    {
        binding().bind<TInfo>(this);
    }
};

}
