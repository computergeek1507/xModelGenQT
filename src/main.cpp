#include "mainwindow.h"

#include <QApplication>
#include <QStringList>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#ifdef _WIN32
#include <spdlog/sinks/msvc_sink.h>
#endif

#include <memory>
#include <vector>

namespace {

// Configure the default spdlog logger: a rotating log file plus, on Windows, the
// Visual Studio debug output window (handy since this is a windowed app with no
// console). Failures are swallowed so logging never prevents the app from starting.
void init_logging()
{
    try {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "xmodel_gen.log", 5 * 1024 * 1024, 3));
#ifdef _WIN32
        sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#endif

        auto logger = std::make_shared<spdlog::logger>("xmodel_gen", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        spdlog::set_default_logger(logger);
    } catch (const spdlog::spdlog_ex&) {
        // Continue without logging.
    }
}

}  // namespace

int main(int argc, char *argv[])
{
    init_logging();
    spdlog::info("xModelGen starting");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // Optionally drive from the command line (for testing/automation):
    //   xmodel_gen <file.dxf> [holeDiameterMm] [wireGapMm]
    QStringList const args = a.arguments();
    if (args.size() > 2) {
        bool ok = false;
        double const dia = args.at(2).toDouble(&ok);
        if (ok) {
            w.setHoleDiameter(dia);
        }
    }
    if (args.size() > 1) {
        w.loadDxf(args.at(1));
    }
    if (args.size() > 3) {
        bool ok = false;
        double const gap = args.at(3).toDouble(&ok);
        if (ok) {
            w.autoWireFromFirst(gap);
        }
    }

    int const rc = a.exec();

    spdlog::info("xModelGen exiting with code {}", rc);
    spdlog::shutdown();
    return rc;
}
