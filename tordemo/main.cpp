#include <torpedo/graphics/Engine.h>
#include <GLFW/glfw3.h>

#include <plog/Init.h>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

int main() {
    auto appender = plog::ColorConsoleAppender<plog::TxtFormatter>();
#ifdef NDEBUG
    init(plog::info, &appender);
#else
    init(plog::debug, &appender);
#endif

    try {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        const auto window = glfwCreateWindow(1280, 768, "Hello, Triangle!", nullptr, nullptr);

        const auto engine = tpd::Engine::create();

        const auto renderer = engine->createRenderer(tpd::RenderEngine::Forward, window);
        engine->destroyRenderer(renderer);

    } catch (const std::exception& e) {
        PLOGE << e.what();
        return 1;
    }
}
