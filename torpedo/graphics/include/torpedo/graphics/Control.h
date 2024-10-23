#pragma once

#include "torpedo/graphics/Context.h"
#include "torpedo/graphics/View.h"

#include <glm/glm.hpp>

namespace tpd {
    class Control {
    public:
        explicit Control(const Context& context);

        virtual void update(const View& view, float deltaTimeMillis) = 0;

        virtual ~Control();

    protected:
        void updateDeltaMouse();
        [[nodiscard]] glm::vec2 getMousePosition() const;
        [[nodiscard]] bool mousePositionIn(const vk::Viewport& viewport) const;

        glm::vec2 _deltaMouse{ 0.0f, 0.0f };
        glm::vec2 _deltaScroll{ 0.0f, 0.0f };

        bool _mouseLeftDragging{ false };
        bool _mouseMiddleDragging{ false };

    private:
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void scrollCallback(GLFWwindow* window, double offsetX, double offsetY);
        const Context* _context;

        glm::vec2 _lastMouse{ 0.0f, 0.0f };
    };
}

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline tpd::Control::Control(const Context& context) : _context{ &context } {
    context.addPointer(this);
    glfwSetMouseButtonCallback(_context->getWindow(), mouseButtonCallback);
    glfwSetScrollCallback(_context->getWindow(), scrollCallback);
}
