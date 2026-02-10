#pragma once

#include <raylib.h>

namespace graphics::fonts {

namespace detail {
inline Font font = {0};
inline Font font_tiny = {0};
} // namespace detail

[[nodiscard]] inline Font get() {
    return (detail::font.texture.id > 0) ? detail::font : GetFontDefault();
}

[[nodiscard]] inline Font get_tiny() {
    return (detail::font_tiny.texture.id > 0) ? detail::font_tiny : get();
}

void load_from_resources();
void unload();

} // namespace graphics::fonts
