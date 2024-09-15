#include <TordemoApplication.h>

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
        const auto app = std::make_unique<TordemoApplication>();
        app->run();
    } catch (const std::exception& e) {
        PLOGE << e.what();
    }
}
