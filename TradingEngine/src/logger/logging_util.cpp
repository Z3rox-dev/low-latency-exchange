#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

boost::log::sources::severity_logger_mt< boost::log::trivial::severity_level > g_logger;

void initializeLogging() {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        typedef sinks::asynchronous_sink< sinks::text_ostream_backend > async_sink;
        boost::shared_ptr< async_sink > sink = boost::make_shared< async_sink >();

        sink->locked_backend()->add_stream(
            boost::shared_ptr< std::ostream >(&std::cout, boost::null_deleter())
        );

        sink->set_formatter(
            expr::stream
                << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                << " [" << expr::attr< unsigned int >("LineID") << "]"
                << " [" << expr::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID") << "]"
                << " <" << logging::trivial::severity
                << "> " << expr::smessage
        );

        logging::core::get()->add_sink(sink);
        logging::add_common_attributes();
        logging::core::get()->set_filter(
            logging::trivial::severity >= logging::trivial::trace
        );
    });
}
