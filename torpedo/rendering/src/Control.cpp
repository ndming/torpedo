#include "torpedo/rendering/Control.h"
#include "torpedo/rendering/SurfaceRenderer.h"

void tpd::Control::mouseButtonCallback(GLFWwindow* window, const int button, const int action, [[maybe_unused]] const int mods) {
    for (const auto ptr = static_cast<Window::EventListener*>(glfwGetWindowUserPointer(window)); const auto control : ptr->controls) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                control->_mouseLeftDragging = true;
            }
            else if (action == GLFW_RELEASE) {
                control->_mouseLeftDragging = false;
            }
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                control->_mouseRightDragging = true;
            }
            else if (action == GLFW_RELEASE) {
                control->_mouseRightDragging = false;
            }
        }
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                control->_mouseMiddleDragging = true;
            }
            else if (action == GLFW_RELEASE) {
                control->_mouseMiddleDragging = false;
            }
        }
    }
}

void tpd::Control::scrollCallback(GLFWwindow* window, const double offsetX, const double offsetY) {
    for (const auto ptr = static_cast<Window::EventListener*>(glfwGetWindowUserPointer(window)); const auto control : ptr->controls) {
        control->_deltaScroll = { static_cast<float>(offsetX), static_cast<float>(offsetY) };
    }
}

void tpd::Control::updateDeltaMouse() {
    const auto mousePos = getMousePosition();
    _deltaMousePosition = mousePos - _lastMousePos;
    _lastMousePos = mousePos;
}

tpd::vec2 tpd::Control::getMousePosition() const {
    double mouseX, mouseY;
    glfwGetCursorPos(_glfwWindow, &mouseX, &mouseY);
    return { static_cast<float>(mouseX), static_cast<float>(mouseY) };
}

bool tpd::Control::mousePositionIn(const vk::Viewport& viewport) const {
    const auto mousePos = getMousePosition();
    return mousePos.x >= viewport.x &&
           mousePos.y >= viewport.y &&
           mousePos.x < viewport.x + viewport.width &&
           mousePos.y < viewport.y + viewport.height;
}
