#include "md2view/cli.hpp"

#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/common.h>

#include <map>
#include <optional>
#include <ostream>
#include <span>
#include <string>

std::optional<CliArgs> parseCli(std::span<char const*> args,
                                std::ostream& out) {
    namespace po = boost::program_options;

    CliArgs result;
    std::string pak;
    std::string logLevel;

    po::options_description desc("Engine options");
    desc.add_options()("help,h", "Show this help message")(
        "width,W", po::value<int>(&result.width)->default_value(result.width),
        "Screen width")(
        "height,H",
        po::value<int>(&result.height)->default_value(result.height),
        "Screen height")("pak,p", po::value<std::string>(&pak),
                         "PAK file or directory to emulate as a PAK")(
        "log-level,l", po::value<std::string>(&logLevel)->default_value("info"),
        "Log level: debug, info, warn, error, off");

    po::variables_map variablesMap;
    try {
        po::store(
            po::parse_command_line(gsl_lite::narrow_cast<int>(args.size()),
                                   args.data(), desc),
            variablesMap);
        po::notify(variablesMap);
    } catch (po::error const& excp) {
        out << excp.what() << '\n' << desc << '\n';
        return std::nullopt;
    }

    if (variablesMap.contains("help")) {
        out << desc << '\n';
        return std::nullopt;
    }

    if (!pak.empty()) {
        result.pak = pak;
    }

    static std::map<std::string, spdlog::level::level_enum> const levels = {
        {"debug", spdlog::level::debug}, {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},   {"error", spdlog::level::err},
        {"off", spdlog::level::off},
    };
    auto it = levels.find(logLevel);
    if (it == levels.end()) {
        out << "unknown log level '" << logLevel
            << "'; choose: debug, info, warn, error, off\n";
        return std::nullopt;
    }
    result.logLevel = it->second;

    return result;
}
