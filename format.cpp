#include "format.hpp"
#include <cairo.h>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
using Surface = std::unique_ptr<cairo_surface_t, decltype(&cairo_surface_destroy)>;
using Context = std::unique_ptr<cairo_t, decltype(&cairo_destroy)>;
void check(cairo_status_t status, const std::string& operation) {
    if (status != CAIRO_STATUS_SUCCESS) throw std::runtime_error(operation + ": " + cairo_status_to_string(status));
}
Surface load(const std::filesystem::path& path) {
    Surface surface(cairo_image_surface_create_from_png(path.string().c_str()), cairo_surface_destroy);
    check(cairo_surface_status(surface.get()), "cannot load glyph " + path.string());
    return surface;
}
cairo_status_t write_stdout(void*, const unsigned char* data, unsigned int length) {
    return std::fwrite(data, 1, length, stdout) == length ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}
}

void render(const Info& info, const std::filesystem::path& font) {
    // A cell per letter, preserving explicit line breaks. Full composite-word
    // grammar was never implemented in this checkout.
    std::vector<std::string> lines(1);
    for (unsigned char ch : info.input) {
        if (ch == '\r') continue;
        if (ch == '\n') lines.emplace_back();
        else {
            if (ch >= 128 || (ch < 32 && ch != '\t'))
                throw std::runtime_error("input supports ASCII text only");
            if (lines.back().size() == static_cast<std::size_t>(info.columns)) lines.emplace_back();
            if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
            lines.back() += ch == '\t' ? ' ' : static_cast<char>(ch);
        }
        if (lines.size() > 256) throw std::runtime_error("input exceeds 256 rendered lines");
    }
    constexpr int cell = 100;
    constexpr int margin = 20;
    std::size_t columns = 1;
    for (const auto& line : lines) columns = std::max(columns, line.size());
    const int width = static_cast<int>(columns) * cell + 2 * margin;
    const int height = static_cast<int>(lines.size()) * cell + 2 * margin;
    if (static_cast<long long>(width) * height > 32000000)
        throw std::runtime_error("output exceeds 32 million pixels; use shorter input");

    // Load every used asset before opening the output, so font errors cannot
    // truncate an existing destination.
    std::map<char, Surface> glyphs;
    std::set<char> missing;
    for (const auto& line : lines) for (char ch : line) {
        if (ch == ' ' || glyphs.contains(ch)) continue;
        auto path = font / "glyphs" / (std::string(1, ch) + ".png");
        if (ch < 'A' || ch > 'Z' || !std::filesystem::exists(path)) {
            missing.insert(ch);
            path = font / "_fail.png";
            if (!std::filesystem::exists(path)) path = font.parent_path() / "_fail.png";
        }
        glyphs.emplace(ch, load(path));
    }
    for (char ch : missing) std::cerr << "sbtex: missing glyph '" << ch << "'; using _fail.png\n";

    Surface output(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height), cairo_surface_destroy);
    check(cairo_surface_status(output.get()), "cannot create image");
    Context context(cairo_create(output.get()), cairo_destroy);
    cairo_set_source_rgb(context.get(), 1, 1, 1);
    cairo_paint(context.get());
    for (std::size_t row = 0; row < lines.size(); ++row) {
        for (std::size_t col = 0; col < lines[row].size(); ++col) {
            const auto ch = lines[row][col];
            if (ch == ' ') continue;
            auto* glyph = glyphs.at(ch).get();
            const auto w = cairo_image_surface_get_width(glyph);
            const auto h = cairo_image_surface_get_height(glyph);
            const double scale = std::min(double(cell) / w, double(cell) / h);
            cairo_save(context.get());
            cairo_translate(context.get(), margin + col * cell + (cell - w * scale) / 2,
                            margin + row * cell + (cell - h * scale) / 2);
            cairo_scale(context.get(), scale, scale);
            cairo_set_source_surface(context.get(), glyph, 0, 0);
            cairo_paint(context.get());
            cairo_restore(context.get());
        }
    }
    check(cairo_status(context.get()), "cannot render image");
    if (info.outfile == "-") {
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif
        check(cairo_surface_write_to_png_stream(output.get(), write_stdout, nullptr), "cannot write stdout");
        if (std::fflush(stdout) != 0) throw std::runtime_error("cannot flush stdout");
    } else check(cairo_surface_write_to_png(output.get(), info.outfile.string().c_str()), "cannot write " + info.outfile.string());
    if (info.verbosity) std::cerr << "Output: " << width << 'x' << height << " PNG\n";
}
