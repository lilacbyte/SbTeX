#include "cmdParser.hpp"
#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#ifndef SBTEX_FONT_DIR
#define SBTEX_FONT_DIR ""
#endif
#ifndef SBTEX_SOURCE_FONT_DIR
#define SBTEX_SOURCE_FONT_DIR ""
#endif

Info getInfo(int argc, char* argv[]) {
    Info info;
    bool has_input = false;
    for (int i = 1; i < argc; ++i) {
        const std::string option = argv[i];
        auto value = [&]() -> std::string {
            if (++i >= argc) throw std::runtime_error("missing value for " + option);
            return argv[i];
        };
        if (option == "-i" || option == "--input") {
            info.input = value();
            has_input = true;
        } else if (option == "-o" || option == "--output") info.outfile = value();
        else if (option == "-f" || option == "--font") info.font = value();
        else if (option == "--fonts-dir") info.fonts_dir = value();
        else if (option == "--columns") {
            const auto text = value();
            const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), info.columns);
            if (error != std::errc{} || end != text.data() + text.size() || info.columns < 1 || info.columns > 128)
                throw std::runtime_error("--columns must be between 1 and 128");
        } else if (option == "-v") ++info.verbosity;
        else if (option == "-V") info.verbosity += 2;
        else if (option == "-h" || option == "--help") info.help = true;
        else if (option == "--advice") info.advice = true;
        else throw std::runtime_error("unknown option: " + option);
    }
    if (!has_input && !info.help && !info.advice) throw std::runtime_error("input required: -i TEXT (see --help)");
    if (info.font.empty() || std::filesystem::path(info.font).filename() != info.font || info.font == "." || info.font == "..")
        throw std::runtime_error("--font must be a font name, not a path; use --fonts-dir for its parent directory");
    return info;
}

std::filesystem::path resolveFont(const Info& info, const char* executable) {
    namespace fs = std::filesystem;
    std::vector<fs::path> roots;
    if (!info.fonts_dir.empty()) roots.push_back(info.fonts_dir);
    else if (const char* env = std::getenv("SBTEX_FONTS_DIR"); env && *env) roots.emplace_back(env);
    else {
        std::error_code error;
        auto binary = fs::read_symlink("/proc/self/exe", error);
        if (error) binary = fs::absolute(executable);
        roots.push_back(binary.parent_path() / "fonts");
        roots.push_back(binary.parent_path() / "../share/sbtex/fonts");
        roots.emplace_back(SBTEX_FONT_DIR);
        roots.emplace_back(SBTEX_SOURCE_FONT_DIR);
    }
    std::string searched;
    for (const auto& root : roots) {
        if (root.empty()) continue;
        const auto font = root / info.font;
        if (fs::is_directory(font / "glyphs")) return fs::weakly_canonical(font);
        searched += "\n  " + font.string();
    }
    throw std::runtime_error("font '" + info.font + "' not found. Searched:" + searched + "\nUse --fonts-dir PATH or SBTEX_FONTS_DIR.");
}
