#include <flecs.h>
#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "ui.h"

#include "graphics.h"

#include "styles/amber/style_amber.h"
#include "styles/ashes/style_ashes.h"
#include "styles/bluish/style_bluish.h"
#include "styles/candy/style_candy.h"
#include "styles/cherry/style_cherry.h"
#include "styles/cyber/style_cyber.h"
#include "styles/dark/style_dark.h"
#include "styles/enefete/style_enefete.h"
#include "styles/jungle/style_jungle.h"
#include "styles/lavanda/style_lavanda.h"
#include "styles/sunny/style_sunny.h"
#include "styles/terminal/style_terminal.h"

struct DemoState {
    bool show_message_box;
    bool show_text_input_box;
    int prev_visual_style;
    float alpha;
};

static void apply_style(int style) {
    GuiLoadStyleDefault();
    switch (style) {
        case 1:
            GuiLoadStyleJungle();
            break;
        case 2:
            GuiLoadStyleLavanda();
            break;
        case 3:
            GuiLoadStyleDark();
            break;
        case 4:
            GuiLoadStyleBluish();
            break;
        case 5:
            GuiLoadStyleCyber();
            break;
        case 6:
            GuiLoadStyleTerminal();
            break;
        case 7:
            GuiLoadStyleCandy();
            break;
        case 8:
            GuiLoadStyleCherry();
            break;
        case 9:
            GuiLoadStyleAshes();
            break;
        case 10:
            GuiLoadStyleEnefete();
            break;
        case 11:
            GuiLoadStyleSunny();
            break;
        case 12:
            GuiLoadStyleAmber();
            break;
        default:
            break;
    }

    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
}

int main() {
    flecs::world world;

    graphics::init(world);
    graphics::init_window(960, 560, "raygui - controls test suite");
    SetExitKey(0);

    static char text_box_text[64] = "Text box";
    static char text_box_multi_text[1024] =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore "
        "et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
        "aliquip ex ea commodo consequat.\n\nDuis aute irure dolor in reprehenderit in voluptate velit esse "
        "cillum dolore eu fugiat nulla pariatur.\n\n"
        "Thisisastringlongerthanexpectedwithoutspacestotestcharbreaksforthosecases,checkingifworkingasexpected.\n\n"
        "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est "
        "laborum.";
    static char text_input[256] = {0};
    static char text_input_file_name[256] = {0};

    static const char* list_view_ex_list[8] = {
        "This", "is", "a", "list view", "with", "disable", "elements", "amazing!"
    };

    world.set<DemoState>({false, false, 0, 1.0f});

    int layout_id = 0;
    int layout_order = 0;
    auto make_control = [&](const char* name, int type, Rectangle rect) {
        return world.entity(name)
            .set(ui::UiControl{type})
            .set(ui::UiLayoutRect{rect})
            .set(ui::UiRect{})
            .set(ui::UiLayoutOwner{layout_id})
            .set(ui::UiDrawOrder{layout_id, layout_order++});
    };

    auto dropdown_main = make_control("Dropdown Main", ui::UI_DROPDOWNBOX, Rectangle{25, 25, 125, 30})
        .set(ui::UiItemsText{"ONE;TWO;THREE"})
        .set(ui::UiActiveIndex{0})
        .set(ui::UiEditMode{false});

    auto dropdown_icons = make_control("Dropdown Icons", ui::UI_DROPDOWNBOX, Rectangle{25, 65, 125, 30})
        .set(ui::UiItemsText{"#01#ONE;#02#TWO;#03#THREE;#04#FOUR"})
        .set(ui::UiActiveIndex{0})
        .set(ui::UiEditMode{false});

    auto force_check = make_control("Force Check", ui::UI_CHECKBOX, Rectangle{25, 108, 15, 15})
        .set(ui::UiText{"FORCE CHECK!"})
        .set(ui::UiValueBool{false});

    auto spinner = make_control("Spinner", ui::UI_SPINNER, Rectangle{25, 135, 125, 30})
        .set(ui::UiText{nullptr})
        .set(ui::UiValueInt{0})
        .set(ui::UiRangeInt{0, 100})
        .set(ui::UiEditMode{false});

    auto value_box = make_control("Value Box", ui::UI_VALUEBOX, Rectangle{25, 175, 125, 30})
        .set(ui::UiText{nullptr})
        .set(ui::UiValueInt{0})
        .set(ui::UiRangeInt{0, 100})
        .set(ui::UiEditMode{false});

    auto text_box = make_control("Text Box", ui::UI_TEXTBOX, Rectangle{25, 215, 125, 30})
        .set(ui::UiTextBuffer{text_box_text, (int)sizeof(text_box_text)})
        .set(ui::UiEditMode{false});

    auto save_button = make_control("Save Button", ui::UI_BUTTON, Rectangle{25, 255, 125, 30})
        .set(ui::UiText{""});

    auto group_states = make_control("States Group", ui::UI_GROUPBOX, Rectangle{25, 310, 125, 150})
        .set(ui::UiText{"STATES"});

    auto state_normal = make_control("State Normal", ui::UI_BUTTON, Rectangle{30, 320, 115, 30})
        .set(ui::UiText{"NORMAL"});

    auto state_focused = make_control("State Focused", ui::UI_BUTTON, Rectangle{30, 355, 115, 30})
        .set(ui::UiText{"FOCUSED"});

    auto state_pressed = make_control("State Pressed", ui::UI_BUTTON, Rectangle{30, 390, 115, 30})
        .set(ui::UiText{"#15#PRESSED"});

    auto state_disabled = make_control("State Disabled", ui::UI_BUTTON, Rectangle{30, 425, 115, 30})
        .set(ui::UiText{"DISABLED"});

    auto style_combo = make_control("Style Combo", ui::UI_COMBOBOX, Rectangle{25, 480, 125, 30})
        .set(ui::UiItemsText{
            "default;Jungle;Lavanda;Dark;Bluish;Cyber;Terminal;Candy;Cherry;Ashes;Enefete;Sunny;Amber"})
        .set(ui::UiActiveIndex{0});

    auto list_view = make_control("List View", ui::UI_LISTVIEW, Rectangle{165, 25, 140, 124})
        .set(ui::UiItemsText{"Charmander;Bulbasaur;#18#Squirtel;Pikachu;Eevee;Pidgey"})
        .set(ui::UiScrollIndex{0})
        .set(ui::UiActiveIndex{-1});

    auto list_view_ex = make_control("List View Ex", ui::UI_LISTVIEW_EX, Rectangle{165, 162, 140, 184})
        .set(ui::UiItemsArray{list_view_ex_list, 8})
        .set(ui::UiScrollIndex{0})
        .set(ui::UiActiveIndex{2})
        .set(ui::UiFocusIndex{-1});

    auto toggle_group = make_control("Toggle Group", ui::UI_TOGGLEGROUP, Rectangle{165, 360, 140, 24})
        .set(ui::UiItemsText{"#1#ONE\n#3#TWO\n#8#THREE\n#23#"})
        .set(ui::UiActiveIndex{0});

    auto toggle_slider = make_control("Toggle Slider", ui::UI_TOGGLESLIDER, Rectangle{165, 480, 140, 30})
        .set(ui::UiItemsText{"ON;OFF"})
        .set(ui::UiActiveIndex{0});

    auto panel_info = make_control("Panel Info", ui::UI_PANEL, Rectangle{320, 25, 225, 140})
        .set(ui::UiText{"Panel Info"});

    auto color_picker = make_control("Color Picker", ui::UI_COLORPICKER, Rectangle{320, 185, 196, 192})
        .set(ui::UiText{nullptr})
        .set(ui::UiValueColor{RED});

    auto slider = make_control("Slider", ui::UI_SLIDER, Rectangle{355, 400, 165, 20})
        .set(ui::UiTextLeft{"TEST"})
        .set(ui::UiTextRight{""})
        .set(ui::UiValueFloat{50.0f})
        .set(ui::UiRangeFloat{-50.0f, 100.0f});

    auto slider_bar = make_control("Slider Bar", ui::UI_SLIDERBAR, Rectangle{320, 430, 200, 20})
        .set(ui::UiTextLeft{nullptr})
        .set(ui::UiTextRight{""})
        .set(ui::UiValueFloat{60.0f})
        .set(ui::UiRangeFloat{0.0f, 100.0f});

    auto progress_bar = make_control("Progress Bar", ui::UI_PROGRESSBAR, Rectangle{320, 460, 200, 20})
        .set(ui::UiTextLeft{nullptr})
        .set(ui::UiTextRight{""})
        .set(ui::UiValueFloat{0.1f})
        .set(ui::UiRangeFloat{0.0f, 1.0f});

    auto color_bar_alpha = make_control("Color Bar Alpha", ui::UI_COLORBAR_ALPHA, Rectangle{320, 490, 200, 30})
        .set(ui::UiText{nullptr})
        .set(ui::UiValueFloat{0.5f});

    auto scroll_panel = make_control("Scroll Panel", ui::UI_SCROLLPANEL, Rectangle{560, 25, 102, 354})
        .set(ui::UiText{nullptr})
        .set(ui::UiContentRect{Rectangle{560, 25, 300, 1200}})
        .set(ui::UiScroll{Vector2{0.0f, 0.0f}})
        .set(ui::UiViewRect{});

    auto grid = make_control("Grid", ui::UI_GRID, Rectangle{560, 400, 100, 120})
        .set(ui::UiText{nullptr})
        .set(ui::UiGridSpec{20.0f, 3})
        .set(ui::UiMouseCell{});

    auto text_box_multi = make_control("Text Box Multi", ui::UI_TEXTBOX_MULTI, Rectangle{678, 25, 258, 492})
        .set(ui::UiTextBuffer{text_box_multi_text, (int)sizeof(text_box_multi_text)})
        .set(ui::UiEditMode{false});

    auto status_bar = make_control("Status Bar", ui::UI_STATUSBAR, Rectangle{0, 540, 960, 20})
        .set(ui::UiText{"This is a status bar"});

    auto message_box = make_control("Message Box", ui::UI_MESSAGEBOX, Rectangle{0, 0, 250, 100})
        .set(ui::UiTitle{""})
        .set(ui::UiMessage{"Do you really want to exit?"})
        .set(ui::UiButtons{"Yes;No"})
        .set(ui::UiResult{});

    auto text_input_box = make_control("Text Input Box", ui::UI_TEXTINPUTBOX, Rectangle{0, 0, 240, 140})
        .set(ui::UiTitle{""})
        .set(ui::UiMessage{"Introduce output file name:"})
        .set(ui::UiButtons{"Ok;Cancel"})
        .set(ui::UiTextBuffer{text_input, (int)sizeof(text_input)})
        .set(ui::UiResult{});

    ui::register_layout_system(world, graphics::phase_pre_render);

    world.system("UpdateDemoState")
        .kind(flecs::PostUpdate)
        .run([=](flecs::iter& it) {
            auto& state = it.world().get_mut<DemoState>();

            if (IsKeyPressed(KEY_ESCAPE)) {
                state.show_message_box = !state.show_message_box;
            }

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
                state.show_text_input_box = true;
            }

            if (IsFileDropped()) {
                FilePathList dropped_files = LoadDroppedFiles();
                if (dropped_files.count > 0 &&
                    IsFileExtension(dropped_files.paths[0], ".rgs")) {
                    GuiLoadStyle(dropped_files.paths[0]);
                }
                UnloadDroppedFiles(dropped_files);
            }

            if (IsKeyPressed(KEY_SPACE)) {
                state.alpha = 1.0f;
            }
            if (state.alpha < 0.0f) {
                state.alpha = 0.0f;
            }
            GuiSetAlpha(state.alpha);

            auto& progress = progress_bar.get_mut<ui::UiValueFloat>();
            if (IsKeyPressed(KEY_LEFT)) {
                progress.value -= 0.1f;
            } else if (IsKeyPressed(KEY_RIGHT)) {
                progress.value += 0.1f;
            }
            if (progress.value > 1.0f) {
                progress.value = 1.0f;
            } else if (progress.value < 0.0f) {
                progress.value = 0.0f;
            }

            const auto& style = style_combo.get<ui::UiActiveIndex>();
            if (style.value != state.prev_visual_style) {
                apply_style(style.value);
                state.prev_visual_style = style.value;
            }

            float width = (float)GetScreenWidth();
            float height = (float)GetScreenHeight();

            auto& status_rect = status_bar.get_mut<ui::UiLayoutRect>();
            status_rect.bounds = Rectangle{0.0f, height - 20.0f, width, 20.0f};

            auto& message_rect = message_box.get_mut<ui::UiLayoutRect>();
            message_rect.bounds = Rectangle{width * 0.5f - 125.0f, height * 0.5f - 50.0f, 250.0f, 100.0f};

            auto& input_rect = text_input_box.get_mut<ui::UiLayoutRect>();
            input_rect.bounds = Rectangle{width * 0.5f - 120.0f, height * 0.5f - 60.0f, 240.0f, 140.0f};
        });

    world.system("DrawDemoUi")
        .kind(graphics::phase_post_render)
        .run([=](flecs::iter& it) {
            auto& state = it.world().get_mut<DemoState>();

            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            auto dispatch = [&](flecs::entity e) {
                const auto& control = e.get<ui::UiControl>();
                const auto& rect = e.get<ui::UiRect>();
                return ui::dispatch_control(e, control, rect);
            };

            auto dispatch_with_state = [&](flecs::entity e, int gui_state) {
                int prev_state = GuiGetState();
                GuiSetState(gui_state);
                int result = dispatch(e);
                GuiSetState(prev_state);
                return result;
            };

            bool drop_main_edit = false;
            bool drop_icons_edit = false;
            const auto& main_edit = dropdown_main.get<ui::UiEditMode>();
            drop_main_edit = main_edit.value;
            const auto& icons_edit = dropdown_icons.get<ui::UiEditMode>();
            drop_icons_edit = icons_edit.value;

            if (drop_main_edit || drop_icons_edit) {
                GuiLock();
            }
            if (state.show_text_input_box) {
                GuiLock();
            }

            dispatch(force_check);

            GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
            dispatch(spinner);
            dispatch(value_box);
            GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            dispatch(text_box);

            GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
            auto& save_text = save_button.get_mut<ui::UiText>();
            save_text.value = GuiIconText(ICON_FILE_SAVE, "Save File");
            if (dispatch(save_button)) {
                state.show_text_input_box = true;
            }

            dispatch(group_states);
            dispatch_with_state(state_normal, STATE_NORMAL);
            dispatch_with_state(state_focused, STATE_FOCUSED);
            dispatch_with_state(state_pressed, STATE_PRESSED);
            dispatch_with_state(state_disabled, STATE_DISABLED);
            GuiSetState(STATE_NORMAL);

            dispatch(style_combo);

            if (drop_main_edit || drop_icons_edit) {
                GuiUnlock();
            }
            if (state.show_text_input_box) {
                GuiLock();
            }

            GuiSetStyle(DROPDOWNBOX, TEXT_PADDING, 4);
            GuiSetStyle(DROPDOWNBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            dispatch(dropdown_icons);
            GuiSetStyle(DROPDOWNBOX, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
            GuiSetStyle(DROPDOWNBOX, TEXT_PADDING, 0);
            dispatch(dropdown_main);

            dispatch(list_view);
            dispatch(list_view_ex);
            GuiSetStyle(LISTVIEW, LIST_ITEMS_BORDER_NORMAL, 0);

            dispatch(toggle_group);
            GuiSetStyle(SLIDER, SLIDER_PADDING, 2);
            dispatch(toggle_slider);
            GuiSetStyle(SLIDER, SLIDER_PADDING, 0);

            dispatch(panel_info);
            dispatch(color_picker);

            auto& slider_value = slider.get_mut<ui::UiValueFloat>();
            auto& slider_text = slider.get_mut<ui::UiTextRight>();
            slider_text.value = TextFormat("%2.2f", slider_value.value);
            dispatch(slider);

            auto& slider_bar_value = slider_bar.get_mut<ui::UiValueFloat>();
            auto& slider_bar_text = slider_bar.get_mut<ui::UiTextRight>();
            slider_bar_text.value = TextFormat("%i", (int)slider_bar_value.value);
            dispatch(slider_bar);

            auto& progress_value = progress_bar.get_mut<ui::UiValueFloat>();
            auto& progress_text = progress_bar.get_mut<ui::UiTextRight>();
            progress_text.value = TextFormat("%i%%", (int)(progress_value.value * 100.0f));
            dispatch(progress_bar);
            GuiEnable();

            dispatch(scroll_panel);
            dispatch(grid);
            dispatch(color_bar_alpha);

            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_TOP);
            GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, TEXT_WRAP_WORD);
            dispatch(text_box_multi);
            GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, TEXT_WRAP_NONE);
            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_MIDDLE);

            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            dispatch(status_bar);
            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

            if (state.show_message_box) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RAYWHITE, 0.8f));
                auto& title = message_box.get_mut<ui::UiTitle>();
                title.value = GuiIconText(ICON_EXIT, "Close Window");
                int result = dispatch(message_box);
                if (result == 0 || result == 2) {
                    state.show_message_box = false;
                } else if (result == 1) {
                    CloseWindow();
                }
            }

            if (state.show_text_input_box) {
                GuiUnlock();
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RAYWHITE, 0.8f));
                auto& title = text_input_box.get_mut<ui::UiTitle>();
                title.value = GuiIconText(ICON_FILE_SAVE, "Save file as...");
                int result = dispatch(text_input_box);
                if (result == 1) {
                    TextCopy(text_input_file_name, text_input);
                }
                if (result == 0 || result == 1 || result == 2) {
                    state.show_text_input_box = false;
                    TextCopy(text_input, "\0");
                }
            }
        });

    world.app().run();
    return 0;
}
