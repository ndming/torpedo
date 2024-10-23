#include "torpedo/graphics/Control.h"

#include <plog/Log.h>

void tpd::Control::mouseButtonCallback(GLFWwindow* window, const int button, const int action, [[maybe_unused]] const int mods) {
    for (const auto pointer = static_cast<Context::ContextPointer*>(glfwGetWindowUserPointer(window)); const auto control : pointer->controls) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                control->_mouseLeftDragging = true;
            }
            else if (action == GLFW_RELEASE) {
                control->_mouseLeftDragging = false;
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
    for (const auto pointer = static_cast<Context::ContextPointer*>(glfwGetWindowUserPointer(window)); const auto control : pointer->controls) {
        control->_deltaScroll = { static_cast<float>(offsetX), static_cast<float>(offsetY) };
    }
}

void tpd::Control::updateDeltaMouse() {
    const auto mousePos = getMousePosition();
    _deltaMouse = mousePos - _lastMouse;
    _lastMouse = mousePos;
}

glm::vec2 tpd::Control::getMousePosition() const {
    double mouseX, mouseY;
    glfwGetCursorPos(_context->getWindow(), &mouseX, &mouseY);
    return { static_cast<float>(mouseX), static_cast<float>(mouseY) };
}

bool tpd::Control::mousePositionIn(const vk::Viewport& viewport) const {
    const auto mousePos = getMousePosition();
    return mousePos.x >= viewport.x &&
           mousePos.y >= viewport.y &&
           mousePos.x < viewport.x + viewport.width &&
           mousePos.y < viewport.y + viewport.height;
}

tpd::Control::~Control() {
    _context->removePointer(this);
}
