#pragma once

#include <GLFW/glfw3.h>

#include <functional>
#include <memory>
#include <string_view>

namespace tpd {
    class Context final {
    public:
        static std::unique_ptr<Context> create(std::string_view name, bool plantLogger = true);
        explicit Context(GLFWwindow* window, void* appender = nullptr);

        [[nodiscard]] void* getWindow() const;

        void loop(const std::function<void()>& onRender) const;
        void loop(const std::function<void(float)>& onRender) const;

        ~Context();

    private:
        GLFWwindow* _window;
        void* _appender;
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Context::Context(GLFWwindow* const window, void* const appender) : _window{ window }, _appender{ appender } {
}

inline void* tpd::Context::getWindow() const {
    return _window;
}
