#pragma once
#include <filesystem>
#include <string>

struct Info {
    std::string input;
    std::string font = "sans";
    std::filesystem::path fonts_dir;
    std::filesystem::path outfile = "-";
    int columns = 16;
    int verbosity = 0;
    bool help = false;
    bool advice = false;
};

Info getInfo(int argc, char* argv[]);
std::filesystem::path resolveFont(const Info& info, const char* executable);
