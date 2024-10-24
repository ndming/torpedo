#include "torpedo/graphics/Context.h"

#include <plog/Init.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#include <chrono>

std::unique_ptr<tpd::Context> tpd::Context::create(const std::string_view name, const bool plantLogger) {
    plog::IAppender* appender = nullptr;
    if (plantLogger) {
        appender = new plog::ColorConsoleAppender<plog::TxtFormatter>();
#ifdef NDEBUG
        init(plog::info, appender);
#else
        init(plog::debug, appender);
#endif
    }

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    const auto window = glfwCreateWindow(1280, 768, name.data(), nullptr, nullptr);
    return std::make_unique<Context>(window, appender);
}

void tpd::Context::loop(const std::function<void()>& onRender) const {
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();
        onRender();
    }
}

void tpd::Context::loop(const std::function<void(float)>& onRender) const {
    auto lastTime = std::chrono::high_resolution_clock::now();;

    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();

        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTimeMillis = std::chrono::duration<float, std::milli>(currentTime - lastTime).count();
        onRender(deltaTimeMillis);

        lastTime = currentTime;
    }

    glfwSetFramebufferSizeCallback(_window, nullptr);
    glfwSetMouseButtonCallback(_window, nullptr);
}

tpd::Context::~Context() {
    glfwDestroyWindow(_window);
    glfwTerminate();
    if (_appender) {
        delete static_cast<plog::ColorConsoleAppender<plog::TxtFormatter>*>(_appender);
    }
    delete _contextPointer;
}
