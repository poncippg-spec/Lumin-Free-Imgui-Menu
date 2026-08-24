#pragma once
#include "includes.h"

#define IMGUI_DEFINE_MATH_OPERATORS

struct custom_slider_t
{
    bool held;
    c_rect rect;
    c_rect active;
    c_vec2 circle_pos;
};

enum title_status_icon
{
    title_status_none = 0,
    title_status_danger,
    title_status_warning,
    title_status_safe
};

enum keybind_mode
{
    keybind_mode_hold = 0,
    keybind_mode_toggle,
    keybind_mode_always
};

struct keybind_t
{
    int key = ImGuiKey_None;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool super = false;
    int mode = keybind_mode_hold;
    bool capturing = false;
};

class c_widgets
{
public:
    bool tab_button(std::string name, std::string icon, int tab);
    bool sub_tab_button(std::string name, std::string icon, int tab, float width = 0.f);
    void brand_header(std::string name, std::string subtitle);
    bool profile_dropdown(std::string name, std::string status);
    bool primary_button(std::string name, std::string icon = "");
    bool action_button(std::string name, std::string icon = "");
    bool card_button(std::string name, std::string description, std::string button_text = "Run");
    bool category_button(std::string name, std::string description, bool* callback);
    bool child(std::string name);
    void end_child();
    bool checkbox(std::string name, std::string description, bool* callback, title_status_icon status = title_status_none);
    bool keybind(std::string name, std::string description, keybind_t* bind);
    bool dropdown(std::string name, std::string description, int* callback, std::vector<std::string> variants);
    bool slider(std::string name, std::string description, float* callback, float vmin, float vmax, std::string format = "%.1f");
    custom_slider_t custom_slider(std::string name, float* callback, float vmin, float vmax, float width);
    bool color_picker(std::string name, c_vec4* color);
    bool text_field(std::string name, char* buf, int buf_size, bool password = false, std::string icon = "");
    bool search_field(std::string name, char* buf, int buf_size, const c_vec2& size);
};

inline std::unique_ptr<c_widgets> widgets = std::make_unique<c_widgets>();

enum notify_type
{
    success = 0,
    warning = 1,
    error = 2
};

struct notify_state
{
    int notify_id;
    std::string title;
    std::string text;
    notify_type type{ success };

    ImVec2 window_size{ 0, 0 };
    float notify_alpha{ 0 };
    bool active_notify{ true };
    float notify_timer{ 0 };
    float notify_pos{ 0 };
    float notify_slide{ 28 };
};

class c_notify
{
public:
    void setup_notify();

    void add_notify(std::string title, std::string text, notify_type type);

private:
    ImVec2 render_notify(float notify_alpha, float notify_percentage, float notify_pos, float notify_slide, const std::string& title, const std::string& text, notify_type type);

    float notify_time{ 2.65f };
    int notify_count{ 0 };

    float notify_spacing{ 8 };
    ImVec2 notify_padding{ 16, 16 };

    std::vector<notify_state> notifications;

};

inline std::unique_ptr<c_notify> notify = std::make_unique<c_notify>();
