#include "torpedo/utils/Log.h"

#include <plog/Init.h>
#include <plog/Util.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Record.h>

#include <iomanip>

namespace plog {
    class Formatter {
    public:
        static util::nstring header() {
            return util::nstring();
        }

        static util::nstring format(const Record& record) {
            tm t;
            util::localtime_s(&t, &record.getTime().time);  // using local time

            util::nostringstream ss;
            ss << t.tm_year + 1900 << "-" << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mon + 1 << PLOG_NSTR("-") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mday << PLOG_NSTR(" ");
            ss << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_sec << PLOG_NSTR(".") << std::setfill(PLOG_NSTR('0')) << std::setw(3) << static_cast<int> (record.getTime().millitm) << PLOG_NSTR(" ");
            ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << severityToString(record.getSeverity()) << PLOG_NSTR(" ");
            ss << PLOG_NSTR("[") << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << record.getTid() << PLOG_NSTR("] ");
            ss << record.getMessage() << PLOG_NSTR("\n");

            return ss.str();
        }
    };
}

void tpd::utils::plantConsoleLogger() {
    static auto appender = plog::ColorConsoleAppender<plog::Formatter>();
#ifdef NDEBUG
    init(plog::info, &appender);
#else
    init(plog::debug, &appender);
#endif
}
