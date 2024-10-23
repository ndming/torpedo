#pragma once

#include <GLFW/glfw3.h>

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_set>

namespace tpd {
    class StandardRenderer;
    class Control;

    class Context final {
    public:
        static std::unique_ptr<Context> create(std::string_view name, bool plantLogger = true);
        explicit Context(GLFWwindow* window, void* appender = nullptr);

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        [[nodiscard]] GLFWwindow* getWindow() const;

        void loop(const std::function<void()>& onRender) const;
        void loop(const std::function<void(float)>& onRender) const;

        void addPointer(StandardRenderer* pointer) const;
        void addPointer(Control* pointer) const;

        void removePointer(StandardRenderer* pointer) const;
        void removePointer(Control* pointer) const;

        ~Context();

        struct ContextPointer {
            std::unordered_set<StandardRenderer*> renderers{};
            std::unordered_set<Control*> controls{};
        };

    private:
        GLFWwindow* _window;
        void* _appender;

        ContextPointer* _contextPointer;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Context::Context(GLFWwindow* const window, void* const appender) : _window{ window }, _appender{ appender } {
    _contextPointer = new ContextPointer{};
    glfwSetWindowUserPointer(_window, _contextPointer);
}

inline GLFWwindow* tpd::Context::getWindow() const {
    return _window;
}

inline void tpd::Context::addPointer(StandardRenderer* const pointer) const {
    _contextPointer->renderers.insert(pointer);
}

inline void tpd::Context::addPointer(Control* const pointer) const {
    _contextPointer->controls.insert(pointer);
}

inline void tpd::Context::removePointer(StandardRenderer* const pointer) const {
    _contextPointer->renderers.erase(pointer);
}

inline void tpd::Context::removePointer(Control* const pointer) const {
    _contextPointer->controls.erase(pointer);
}
