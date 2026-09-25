#pragma once
#include <filesystem>
#include <string>

struct Color {
    double r, g, b;
};

struct Info {
    std::string input;
    std::string font = "sans";
    std::filesystem::path fonts_dir;
    std::filesystem::path outfile = "-";
    int columns = 16;
    int verbosity = 0;
    bool help = false;
    bool advice = false;
    Color background{1, 1, 1};
    Color color{0, 0, 0};
    bool recolor = false;
};

Info getInfo(int argc, char* argv[]);
std::filesystem::path resolveFont(const Info& info, const char* executable);
