#pragma once

#include "dang-utils/enum.h"
#include "dang-math/vector.h"

#include "ObjectTypes.h"
#include "Binding.h"

namespace dang::gl
{

class WindowInfo {
public:
    WindowInfo();

    dmath::ivec2 size() const;
    void setSize(dmath::ivec2 size);

    int width() const;
    void setWidth(int width);

    int height() const;
    void setHeight(int height);

    std::string title() const;
    void setTitle(std::string title);

    GLFWwindow* createWindow() const;

private:
    dmath::ivec2 size_;
    std::string title_;
};

class Window {
public:
    Window(GLFWwindow* handle);
    Window(const WindowInfo& info = WindowInfo());
    ~Window();

    GLFWwindow* handle();

    template <class TInfo>
    Binding& binding();

    bool shouldClose();

    void update();
    void render();

    void step();
    void run();

private:
    GLFWwindow* handle_;
    dutils::EnumArray<ObjectType, std::unique_ptr<Binding>> bindings_;
};

template<class TInfo>
inline Binding& Window::binding()
{
    if (const auto& binding = bindings_[TInfo::Type])
        return *binding;
    return *(bindings_[TInfo::Type] = std::make_unique<TInfo::Binding>());
}

}
