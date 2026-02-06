#pragma once

#include <flecs.h>
#include <raygui.h>

namespace ui {

// keep values aligned with rGuiLayout GuiControlType where possible.
enum UiControlType {
    UI_WINDOWBOX = 0,
    UI_GROUPBOX = 1,
    UI_LINE = 2,
    UI_PANEL = 3,
    UI_LABEL = 4,
    UI_BUTTON = 5,
    UI_LABELBUTTON = 6,
    UI_CHECKBOX = 7,
    UI_TOGGLE = 8,
    UI_TOGGLEGROUP = 9,
    UI_COMBOBOX = 10,
    UI_DROPDOWNBOX = 11,
    UI_TEXTBOX = 12,
    UI_TEXTBOX_MULTI = 13,
    UI_VALUEBOX = 14,
    UI_SPINNER = 15,
    UI_SLIDER = 16,
    UI_SLIDERBAR = 17,
    UI_PROGRESSBAR = 18,
    UI_STATUSBAR = 19,
    UI_SCROLLPANEL = 20,
    UI_LISTVIEW = 21,
    UI_COLORPICKER = 22,
    UI_DUMMYREC = 23,
    UI_TABBAR = 24,
    UI_TOGGLESLIDER = 25,
    UI_LISTVIEW_EX = 26,
    UI_MESSAGEBOX = 27,
    UI_TEXTINPUTBOX = 28,
    UI_COLORPANEL = 29,
    UI_COLORBAR_ALPHA = 30,
    UI_COLORBAR_HUE = 31,
    UI_COLORPICKER_HSV = 32,
    UI_COLORPANEL_HSV = 33,
    UI_VALUEBOX_FLOAT = 34,
    UI_GRID = 35,
};

enum UiControlFlags {
    UI_CONTROL_NONE = 0,
    UI_CONTROL_DISABLED = 1 << 0,
    UI_CONTROL_LOCKED = 1 << 1,
    UI_CONTROL_HIDDEN = 1 << 2,
};

struct UiControl {
    int type = UI_PANEL;
    unsigned int flags = UI_CONTROL_NONE;
};

struct UiRect {
    Rectangle bounds = {};
};

struct UiLayoutOwner {
    int value = 0;
};

struct UiDrawOrder {
    int layout = 0;
    int order = 0;
};

// local layout rect, optionally offset by a UiAnchorPoint.
struct UiLayoutRect {
    Rectangle bounds = {};
};

struct UiAnchorPoint {
    float x = 0.0f;
    float y = 0.0f;
};

struct UiAnchorRef {
    flecs::entity_t entity = 0;
};

struct UiContentRect {
    Rectangle bounds = {};
};

struct UiViewRect {
    Rectangle bounds = {};
};

struct UiText {
    const char* value = "";
};

struct UiTextLeft {
    const char* value = "";
};

struct UiTextRight {
    const char* value = "";
};

struct UiTitle {
    const char* value = "";
};

struct UiMessage {
    const char* value = "";
};

struct UiButtons {
    const char* value = "";
};

struct UiItemsText {
    const char* value = "";
};

struct UiItemsArray {
    const char** items = nullptr;
    int count = 0;
};

struct UiValueBool {
    bool value = false;
};

struct UiValueInt {
    int value = 0;
};

struct UiValueFloat {
    float value = 0.0f;
};

struct UiValueColor {
    Color value = {};
};

struct UiValueHsv {
    Vector3 value = {};
};

struct UiRangeInt {
    int min = 0;
    int max = 0;
};

struct UiRangeFloat {
    float min = 0.0f;
    float max = 0.0f;
};

struct UiActiveIndex {
    int value = 0;
};

struct UiScrollIndex {
    int value = 0;
};

struct UiFocusIndex {
    int value = 0;
};

struct UiEditMode {
    bool value = false;
};

struct UiSecretView {
    bool value = false;
};

struct UiScroll {
    Vector2 value = {};
};

struct UiGridSpec {
    float spacing = 10.0f;
    int subdivs = 2;
};

struct UiMouseCell {
    Vector2 value = {};
};

struct UiTextBuffer {
    char* data = nullptr;
    int size = 0;
};

struct UiResult {
    int value = 0;
};

inline const char* text_or_empty(const char* text) {
    return text ? text : "";
}

inline const char* text_or_null(const char* text) {
    return text;
}

inline void register_layout_system(flecs::world& world, flecs::entity phase) {
    world.system<const UiLayoutRect, UiRect>("ui::layout")
        .kind(phase)
        .each([](flecs::entity e, const UiLayoutRect& local, UiRect& rect) {
            float ox = 0.0f;
            float oy = 0.0f;
            if (auto anchor_ref = e.try_get<UiAnchorRef>()) {
                if (anchor_ref->entity) {
                    auto anchor = e.world().entity(anchor_ref->entity).try_get<UiAnchorPoint>();
                    if (anchor) {
                        ox = anchor->x;
                        oy = anchor->y;
                    }
                }
            }

            rect.bounds = Rectangle{
                local.bounds.x + ox,
                local.bounds.y + oy,
                local.bounds.width,
                local.bounds.height
            };
        });
}

inline void register_layout_system(flecs::world& world) {
    world.system<const UiLayoutRect, UiRect>("ui::layout")
        .each([](flecs::entity e, const UiLayoutRect& local, UiRect& rect) {
            float ox = 0.0f;
            float oy = 0.0f;
            if (auto anchor_ref = e.try_get<UiAnchorRef>()) {
                if (anchor_ref->entity) {
                    auto anchor = e.world().entity(anchor_ref->entity).try_get<UiAnchorPoint>();
                    if (anchor) {
                        ox = anchor->x;
                        oy = anchor->y;
                    }
                }
            }

            rect.bounds = Rectangle{
                local.bounds.x + ox,
                local.bounds.y + oy,
                local.bounds.width,
                local.bounds.height
            };
        });
}

inline int dispatch_control(flecs::entity e, const UiControl& control, const UiRect& rect) {
    int result = 0;
    if (control.flags & UI_CONTROL_HIDDEN) {
        return result;
    }

    int prev_state = GuiGetState();
    bool was_locked = GuiIsLocked();

    if (control.flags & UI_CONTROL_DISABLED) {
        GuiSetState(STATE_DISABLED);
    }
    if (control.flags & UI_CONTROL_LOCKED) {
        GuiLock();
    }

    switch (control.type) {
        case UI_WINDOWBOX: {
            auto title = e.try_get<UiTitle>();
            result = GuiWindowBox(rect.bounds, text_or_empty(title ? title->value : nullptr));
        } break;
        case UI_GROUPBOX: {
            auto text = e.try_get<UiText>();
            result = GuiGroupBox(rect.bounds, text_or_empty(text ? text->value : nullptr));
        } break;
        case UI_LINE: {
            auto text = e.try_get<UiText>();
            result = GuiLine(rect.bounds, text_or_empty(text ? text->value : nullptr));
        } break;
        case UI_PANEL: {
            auto text = e.try_get<UiText>();
            result = GuiPanel(rect.bounds, text_or_empty(text ? text->value : nullptr));
        } break;
        case UI_LABEL: {
            auto text = e.try_get<UiText>();
            result = GuiLabel(rect.bounds, text_or_empty(text ? text->value : nullptr));
        } break;
        case UI_BUTTON: {
            auto text = e.try_get<UiText>();
            result = GuiButton(rect.bounds, text_or_empty(text ? text->value : nullptr));
        } break;
        case UI_LABELBUTTON: {
            auto text = e.try_get<UiText>();
            result = GuiLabelButton(rect.bounds, text_or_empty(text ? text->value : nullptr));
        } break;
        case UI_TOGGLE: {
            auto text = e.try_get<UiText>();
            auto value = e.try_get_mut<UiValueBool>();
            if (value) {
                result = GuiToggle(rect.bounds, text_or_empty(text ? text->value : nullptr), &value->value);
            }
        } break;
        case UI_TOGGLEGROUP: {
            auto text = e.try_get<UiItemsText>();
            auto active = e.try_get_mut<UiActiveIndex>();
            if (text && active) {
                result = GuiToggleGroup(rect.bounds, text->value, &active->value);
            }
        } break;
        case UI_TOGGLESLIDER: {
            auto text = e.try_get<UiItemsText>();
            auto active = e.try_get_mut<UiActiveIndex>();
            if (text && active) {
                result = GuiToggleSlider(rect.bounds, text->value, &active->value);
            }
        } break;
        case UI_CHECKBOX: {
            auto text = e.try_get<UiText>();
            auto value = e.try_get_mut<UiValueBool>();
            if (value) {
                result = GuiCheckBox(rect.bounds, text_or_empty(text ? text->value : nullptr), &value->value);
            }
        } break;
        case UI_COMBOBOX: {
            auto text = e.try_get<UiItemsText>();
            auto active = e.try_get_mut<UiActiveIndex>();
            if (text && active) {
                result = GuiComboBox(rect.bounds, text->value, &active->value);
            }
        } break;
        case UI_DROPDOWNBOX: {
            auto text = e.try_get<UiItemsText>();
            auto active = e.try_get_mut<UiActiveIndex>();
            auto edit = e.try_get_mut<UiEditMode>();
            if (text && active && edit) {
                result = GuiDropdownBox(rect.bounds, text->value, &active->value, edit->value);
                if (result) {
                    edit->value = !edit->value;
                }
            }
        } break;
        case UI_SPINNER: {
            auto text = e.try_get<UiText>();
            auto value = e.try_get_mut<UiValueInt>();
            auto range = e.try_get<UiRangeInt>();
            auto edit = e.try_get_mut<UiEditMode>();
            if (value && range && edit) {
                result = GuiSpinner(rect.bounds, text_or_null(text ? text->value : nullptr), &value->value,
                    range->min, range->max, edit->value);
                if (result) {
                    edit->value = !edit->value;
                }
            }
        } break;
        case UI_VALUEBOX: {
            auto text = e.try_get<UiText>();
            auto value = e.try_get_mut<UiValueInt>();
            auto range = e.try_get<UiRangeInt>();
            auto edit = e.try_get_mut<UiEditMode>();
            if (value && range && edit) {
                result = GuiValueBox(rect.bounds, text_or_null(text ? text->value : nullptr), &value->value,
                    range->min, range->max, edit->value);
                if (result) {
                    edit->value = !edit->value;
                }
            }
        } break;
        case UI_VALUEBOX_FLOAT: {
            auto text = e.try_get<UiText>();
            auto buffer = e.try_get_mut<UiTextBuffer>();
            auto value = e.try_get_mut<UiValueFloat>();
            auto edit = e.try_get_mut<UiEditMode>();
            if (buffer && value && edit && buffer->data && buffer->size > 0) {
                result = GuiValueBoxFloat(rect.bounds, text_or_null(text ? text->value : nullptr),
                    buffer->data, &value->value, edit->value);
                if (result) {
                    edit->value = !edit->value;
                }
            }
        } break;
        case UI_TEXTBOX: {
            auto buffer = e.try_get_mut<UiTextBuffer>();
            auto edit = e.try_get_mut<UiEditMode>();
            if (buffer && edit && buffer->data && buffer->size > 0) {
                result = GuiTextBox(rect.bounds, buffer->data, buffer->size, edit->value);
                if (result) {
                    edit->value = !edit->value;
                }
            }
        } break;
        case UI_TEXTBOX_MULTI: {
            auto buffer = e.try_get_mut<UiTextBuffer>();
            auto edit = e.try_get_mut<UiEditMode>();
            if (buffer && edit && buffer->data && buffer->size > 0) {
                int prev_align = GuiGetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL);
                int prev_wrap = GuiGetStyle(DEFAULT, TEXT_WRAP_MODE);
                GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_TOP);
                GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, TEXT_WRAP_WORD);
                result = GuiTextBox(rect.bounds, buffer->data, buffer->size, edit->value);
                GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, prev_wrap);
                GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, prev_align);
                if (result) {
                    edit->value = !edit->value;
                }
            }
        } break;
        case UI_SLIDER: {
            auto left = e.try_get<UiTextLeft>();
            auto right = e.try_get<UiTextRight>();
            auto value = e.try_get_mut<UiValueFloat>();
            auto range = e.try_get<UiRangeFloat>();
            if (value && range) {
                result = GuiSlider(rect.bounds, text_or_null(left ? left->value : nullptr),
                    text_or_null(right ? right->value : nullptr), &value->value, range->min, range->max);
            }
        } break;
        case UI_SLIDERBAR: {
            auto left = e.try_get<UiTextLeft>();
            auto right = e.try_get<UiTextRight>();
            auto value = e.try_get_mut<UiValueFloat>();
            auto range = e.try_get<UiRangeFloat>();
            if (value && range) {
                result = GuiSliderBar(rect.bounds, text_or_null(left ? left->value : nullptr),
                    text_or_null(right ? right->value : nullptr), &value->value, range->min, range->max);
            }
        } break;
        case UI_PROGRESSBAR: {
            auto left = e.try_get<UiTextLeft>();
            auto right = e.try_get<UiTextRight>();
            auto value = e.try_get_mut<UiValueFloat>();
            auto range = e.try_get<UiRangeFloat>();
            if (value && range) {
                result = GuiProgressBar(rect.bounds, text_or_null(left ? left->value : nullptr),
                    text_or_null(right ? right->value : nullptr), &value->value, range->min, range->max);
            }
        } break;
        case UI_STATUSBAR: {
            auto text = e.try_get<UiText>();
            result = GuiStatusBar(rect.bounds, text_or_empty(text ? text->value : nullptr));
        } break;
        case UI_DUMMYREC: {
            auto text = e.try_get<UiText>();
            result = GuiDummyRec(rect.bounds, text_or_empty(text ? text->value : nullptr));
        } break;
        case UI_GRID: {
            auto text = e.try_get<UiText>();
            auto grid = e.try_get<UiGridSpec>();
            auto cell = e.try_get_mut<UiMouseCell>();
            if (grid) {
                Vector2 fallback = {};
                Vector2* cell_ptr = cell ? &cell->value : &fallback;
                result = GuiGrid(rect.bounds, text_or_null(text ? text->value : nullptr),
                    grid->spacing, grid->subdivs, cell_ptr);
            }
        } break;
        case UI_LISTVIEW: {
            auto text = e.try_get<UiItemsText>();
            auto scroll = e.try_get_mut<UiScrollIndex>();
            auto active = e.try_get_mut<UiActiveIndex>();
            if (text && scroll && active) {
                result = GuiListView(rect.bounds, text->value, &scroll->value, &active->value);
            }
        } break;
        case UI_LISTVIEW_EX: {
            auto items = e.try_get<UiItemsArray>();
            auto scroll = e.try_get_mut<UiScrollIndex>();
            auto active = e.try_get_mut<UiActiveIndex>();
            auto focus = e.try_get_mut<UiFocusIndex>();
            if (items && items->items && items->count > 0 && scroll && active && focus) {
                result = GuiListViewEx(rect.bounds, items->items, items->count,
                    &scroll->value, &active->value, &focus->value);
            }
        } break;
        case UI_TABBAR: {
            auto items = e.try_get<UiItemsArray>();
            auto active = e.try_get_mut<UiActiveIndex>();
            if (items && items->items && items->count > 0 && active) {
                result = GuiTabBar(rect.bounds, items->items, items->count, &active->value);
            }
        } break;
        case UI_SCROLLPANEL: {
            auto text = e.try_get<UiText>();
            auto content = e.try_get<UiContentRect>();
            auto scroll = e.try_get_mut<UiScroll>();
            if (content && scroll) {
                auto view = e.try_get_mut<UiViewRect>();
                Rectangle fallback = {};
                Rectangle* view_ptr = view ? &view->bounds : &fallback;
                result = GuiScrollPanel(rect.bounds, text_or_null(text ? text->value : nullptr),
                    content->bounds, &scroll->value, view_ptr);
            }
        } break;
        case UI_MESSAGEBOX: {
            auto title = e.try_get<UiTitle>();
            auto message = e.try_get<UiMessage>();
            auto buttons = e.try_get<UiButtons>();
            result = GuiMessageBox(rect.bounds,
                text_or_empty(title ? title->value : nullptr),
                text_or_empty(message ? message->value : nullptr),
                text_or_empty(buttons ? buttons->value : nullptr));
        } break;
        case UI_TEXTINPUTBOX: {
            auto title = e.try_get<UiTitle>();
            auto message = e.try_get<UiMessage>();
            auto buttons = e.try_get<UiButtons>();
            auto buffer = e.try_get_mut<UiTextBuffer>();
            auto secret = e.try_get_mut<UiSecretView>();
            if (buffer && buffer->data && buffer->size > 0) {
                result = GuiTextInputBox(rect.bounds,
                    text_or_empty(title ? title->value : nullptr),
                    text_or_empty(message ? message->value : nullptr),
                    text_or_empty(buttons ? buttons->value : nullptr),
                    buffer->data, buffer->size, secret ? &secret->value : nullptr);
            }
        } break;
        case UI_COLORPICKER: {
            auto text = e.try_get<UiText>();
            auto color = e.try_get_mut<UiValueColor>();
            if (color) {
                result = GuiColorPicker(rect.bounds, text_or_null(text ? text->value : nullptr), &color->value);
            }
        } break;
        case UI_COLORPANEL: {
            auto text = e.try_get<UiText>();
            auto color = e.try_get_mut<UiValueColor>();
            if (color) {
                result = GuiColorPanel(rect.bounds, text_or_null(text ? text->value : nullptr), &color->value);
            }
        } break;
        case UI_COLORBAR_ALPHA: {
            auto text = e.try_get<UiText>();
            auto value = e.try_get_mut<UiValueFloat>();
            if (value) {
                result = GuiColorBarAlpha(rect.bounds, text_or_null(text ? text->value : nullptr), &value->value);
            }
        } break;
        case UI_COLORBAR_HUE: {
            auto text = e.try_get<UiText>();
            auto value = e.try_get_mut<UiValueFloat>();
            if (value) {
                result = GuiColorBarHue(rect.bounds, text_or_null(text ? text->value : nullptr), &value->value);
            }
        } break;
        case UI_COLORPICKER_HSV: {
            auto text = e.try_get<UiText>();
            auto value = e.try_get_mut<UiValueHsv>();
            if (value) {
                result = GuiColorPickerHSV(rect.bounds, text_or_null(text ? text->value : nullptr), &value->value);
            }
        } break;
        case UI_COLORPANEL_HSV: {
            auto text = e.try_get<UiText>();
            auto value = e.try_get_mut<UiValueHsv>();
            if (value) {
                result = GuiColorPanelHSV(rect.bounds, text_or_null(text ? text->value : nullptr), &value->value);
            }
        } break;
        default:
            break;
    }

    if (control.flags & UI_CONTROL_LOCKED) {
        if (!was_locked) {
            GuiUnlock();
        }
    }
    if (control.flags & UI_CONTROL_DISABLED) {
        GuiSetState(prev_state);
    }

    auto out = e.try_get_mut<UiResult>();
    if (out) {
        out->value = result;
    }

    return result;
}

inline void register_dispatch_system(flecs::world& world, flecs::entity phase) {
    world.system<const UiControl, const UiRect>("ui::dispatch")
        .kind(phase)
        .each([](flecs::entity e, const UiControl& control, const UiRect& rect) {
            dispatch_control(e, control, rect);
        });
}

inline void register_dispatch_system(flecs::world& world) {
    world.system<const UiControl, const UiRect>("ui::dispatch")
        .each([](flecs::entity e, const UiControl& control, const UiRect& rect) {
            dispatch_control(e, control, rect);
        });
}

inline int compare_draw_order(flecs::entity_t e1, const UiDrawOrder* o1, flecs::entity_t e2, const UiDrawOrder* o2) {
    int layout_order = o1->layout - o2->layout;
    if (layout_order != 0) {
        return layout_order;
    }

    int order = o1->order - o2->order;
    if (order != 0) {
        return order;
    }

    return (int)(e1 - e2);
}

inline void register_dispatch_system_ordered(flecs::world& world, flecs::entity phase) {
    world.system<const UiControl, const UiRect, const UiDrawOrder>("ui::dispatch_ordered")
        .kind(phase)
        .order_by<UiDrawOrder>(compare_draw_order)
        .each([](flecs::entity e, const UiControl& control, const UiRect& rect, const UiDrawOrder&) {
            dispatch_control(e, control, rect);
        });
}

inline void register_dispatch_system_ordered(flecs::world& world) {
    world.system<const UiControl, const UiRect, const UiDrawOrder>("ui::dispatch_ordered")
        .order_by<UiDrawOrder>(compare_draw_order)
        .each([](flecs::entity e, const UiControl& control, const UiRect& rect, const UiDrawOrder&) {
            dispatch_control(e, control, rect);
        });
}

} // namespace ui
