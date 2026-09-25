#include "cmdParser.hpp"
#include "format.hpp"
#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        const auto info = getInfo(argc, argv);
        if (info.help) {
            std::cout << "SbTeX — Stratish bitmap typesetter\n"
                "Usage: sbtex -i TEXT [-o output.png] [-f sans] [--columns 16]\n"
                "  -i, --input TEXT    Text to render (ASCII letters, spaces, newlines)\n"
                "  -o, --output FILE   PNG destination; default '-' means stdout\n"
                "  -f, --font NAME     Font folder name; default sans\n"
                "  --fonts-dir PATH    Parent of font folders (overrides SBTEX_FONTS_DIR)\n"
                "  --columns N         Wrap after N cells, from 1 to 128\n"
                "  -v, -V              Print font/output details to stderr\n"
                "  -h, --help          Show this help\n"
                "Missing glyphs use _fail.png with a warning.\n";
            return 0;
        }
        if (info.advice) { std::cout << "Don't Panic!\n"; return 42; }
        const auto font = resolveFont(info, argv[0]);
        if (info.verbosity) std::cerr << "Font: " << font << '\n';
        render(info, font);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "sbtex: " << error.what() << '\n';
        return 1;
    }
}
