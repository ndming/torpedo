#pragma once

#include <torpedo/math/vec2.h>

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace tpd {
    class Control;

    template<typename T>
    concept ControlImpl = std::is_base_of_v<Control, T> && std::is_final_v<T> && std::constructible_from<T, GLFWwindow*>;

    class Control {
    public:
        explicit Control(GLFWwindow* window) : _glfwWindow(window) {}

        Control(const Control&) = delete;
        Control& operator=(const Control&) = delete;

        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void scrollCallback(GLFWwindow* window, double offsetX, double offsetY);

    protected:
        void updateDeltaMouse();
        [[nodiscard]] vec2 getMousePosition() const;
        [[nodiscard]] bool mousePositionIn(const vk::Viewport& viewport) const;

        vec2 _deltaMousePosition{ 0.f, 0.f };
        vec2 _deltaScroll{ 0.f, 0.f };

        bool _mouseLeftDragging{ false };
        bool _mouseRightDragging{ false };
        bool _mouseMiddleDragging{ false };

    private:
        GLFWwindow* _glfwWindow;

        vec2 _lastMousePos{ 0.f, 0.f };
    };
}