#include "fonts.h"

#include "resources.h"

namespace graphics::fonts {

namespace {

Font load_font_grid(const char* path,
                    int cell_w,
                    int cell_h,
                    int spacing_x,
                    int spacing_y,
                    int pad_x,
                    int pad_y,
                    int first_char,
                    int overlap,
                    int space_advance) {
    Font font = {0};
    Image image = LoadImage(path);
    if (!image.data) {
        return font;
    }

    Color* pixels = LoadImageColors(image);
    if (!pixels) {
        UnloadImage(image);
        return font;
    }

    int cols = (image.width - pad_x) / (cell_w + spacing_x);
    int rows = (image.height - pad_y) / (cell_h + spacing_y);
    int count = cols * rows;
    if (cols <= 0 || rows <= 0 || count <= 0) {
        UnloadImageColors(pixels);
        UnloadImage(image);
        return font;
    }

    font.texture = LoadTextureFromImage(image);
    if (font.texture.id == 0) {
        UnloadImageColors(pixels);
        UnloadImage(image);
        return font;
    }

    font.glyphCount = count;
    font.glyphPadding = 0;
    font.recs = (Rectangle*)MemAlloc(sizeof(Rectangle) * count);
    font.glyphs = (GlyphInfo*)MemAlloc(sizeof(GlyphInfo) * count);
    if (space_advance < 1) {
        space_advance = 1;
    }
    if (overlap < 0) {
        overlap = 0;
    }
    for (int i = 0; i < count; ++i) {
        int col = i % cols;
        int row = i / cols;
        int cell_x = pad_x + col * (cell_w + spacing_x);
        int cell_y = pad_y + row * (cell_h + spacing_y);
        int left = cell_w;
        int right = -1;
        for (int y = 0; y < cell_h; ++y) {
            int row_offset = (cell_y + y) * image.width + cell_x;
            for (int x = 0; x < cell_w; ++x) {
                if (pixels[row_offset + x].a > 0) {
                    if (x < left) left = x;
                    if (x > right) right = x;
                }
            }
        }
        int rec_w = 0;
        if (right >= left) {
            rec_w = right - left + 1;
            font.recs[i] = Rectangle{
                (float)(cell_x + left),
                (float)cell_y,
                (float)rec_w,
                (float)cell_h
            };
        } else {
            font.recs[i] = Rectangle{(float)cell_x, (float)cell_y, 0.0f, (float)cell_h};
        }
        font.glyphs[i].value = first_char + i;
        font.glyphs[i].offsetX = 0;
        font.glyphs[i].offsetY = 0;
        if (rec_w > 0) {
            int advance = rec_w - overlap;
            if (advance < 1) advance = 1;
            font.glyphs[i].advanceX = advance;
        } else {
            font.glyphs[i].advanceX = space_advance;
        }
        font.glyphs[i].image = Image{};
    }

    int digit_start = 48 - first_char;
    int digit_end = 57 - first_char;
    if (digit_start >= 0 && digit_end < count) {
        int max_advance = 0;
        for (int i = digit_start; i <= digit_end; ++i) {
            if (font.glyphs[i].advanceX > max_advance) {
                max_advance = font.glyphs[i].advanceX;
            }
        }
        if (max_advance > 0) {
            for (int i = digit_start; i <= digit_end; ++i) {
                font.glyphs[i].advanceX = max_advance;
            }
        }
    }

    font.baseSize = cell_h;
    UnloadImageColors(pixels);
    UnloadImage(image);
    TraceLog(LOG_INFO, "FONT: [%s] Grid font loaded (%i pixel size | %i glyphs)",
             path, font.baseSize, font.glyphCount);
    return font;
}

} // namespace

void load_from_resources() {
    std::string font_path = normalized_path("resources/fonts/generic.fnt");
    detail::font = LoadFont(font_path.c_str());

    std::string font_tiny_path = normalized_path("resources/fonts/tiny.png");
    detail::font_tiny = load_font_grid(font_tiny_path.c_str(), 5, 7, 0, 0, 0, 0, 0, 1, 3);
    if (detail::font_tiny.texture.id > 0 && detail::font_tiny.glyphCount > 0) {
        SetTextureFilter(detail::font_tiny.texture, TEXTURE_FILTER_POINT);
    }
}

void unload() {
    if (detail::font.texture.id > 0) {
        UnloadFont(detail::font);
        detail::font = Font{};
    }

    if (detail::font_tiny.texture.id > 0) {
        UnloadFont(detail::font_tiny);
        detail::font_tiny = Font{};
    }
}

} // namespace graphics::fonts
