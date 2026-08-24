#include "../headers/includes.h"
#include "../../thirdparty/imgui/imgui_freetype.h"
#include "imgui_impl_dx11.h"

static const ImWchar* get_private_icon_ranges()
{
    static const ImWchar ranges[] =
    {
        0x26A0, 0x26A0, // warning sign
        0x26D4, 0x26D4, // no entry
        0x2714, 0x2714, // heavy check mark
        0xE70D, 0xE70E, // Segoe MDL2 chevrons
        0xF1E7, 0xF1E7, // fi-rr-backpack
        0xF309, 0xF309, // fi-rr-bullseye
        0xF3A2, 0xF3A2, // fi-rr-chart-histogram
        0xF5F8, 0xF5F8, // fi-rr-eye
        0xF71C, 0xF71C, // fi-rr-grid
        0xF87D, 0xF87D, // fi-rr-layout-fluid
        0xF8E1, 0xF8E1, // fi-rr-magic-wand
        0xFB7C, 0xFB7C, // fi-rr-running
        0xFD32, 0xFD32, // fi-rr-sword
        0xFD5F, 0xFD5F, // fi-rr-target
        0xFE19, 0xFE19, // fi-rr-treasure-chest
        0xFEA0, 0xFEA0, // fi-rr-user
        0xFEA6, 0xFEA6, // fi-rr-users
        0xFF21, 0xFF21, // fi-rr-world
        0
    };
    return ranges;
}

void c_font::update()
{
    if (var->gui.dpi_changed)
    {
        var->gui.dpi = var->gui.stored_dpi / 100.f;

        ImGuiIO& io = ImGui::GetIO();
        ImGui_ImplDX11_InvalidateDeviceObjects();
        io.Fonts->Clear();

        for (auto& font_t : data)
        {
            ImFontConfig cfg;

            if (font_t.file)
            {
                cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
                const ImWchar* ranges = font_t.private_glyphs ? get_private_icon_ranges() : io.Fonts->GetGlyphRangesCyrillic();
                font_t.font = io.Fonts->AddFontFromFileTTF(font_t.file_path.c_str(), s_(font_t.size), &cfg, ranges);
            }
            else
            {
                cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_Bitmap;
                cfg.FontDataOwnedByAtlas = false;
                font_t.font = io.Fonts->AddFontFromMemoryTTF(font_t.data.data(), font_t.data.size(), s_(font_t.size), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            }
        }

        io.Fonts->Build();
        ImGui_ImplDX11_CreateDeviceObjects();

        var->gui.dpi_changed = false;
    }
}

ImFont* c_font::get(const std::vector<unsigned char>& font_data, float size)
{
    const void* source = font_data.empty() ? nullptr : font_data.data();

    for (auto& font_t : data)
    {
        if (!font_t.file && font_t.source == source && font_t.size == size)
        {
            return font_t.font;
        }
    }

    add(font_data, size);

    var->gui.dpi_changed = true;

    return get(font_data, size);
}

ImFont* c_font::get_file(const std::string& file_path, float size, bool private_glyphs)
{
    for (auto& font_t : data)
    {
        if (font_t.file && font_t.file_path == file_path && font_t.size == size && font_t.private_glyphs == private_glyphs)
        {
            return font_t.font;
        }
    }

    add_file(file_path, size, private_glyphs);

    var->gui.dpi_changed = true;

    return nullptr;
}

void c_font::add(const std::vector<unsigned char>& font_data, float size)
{
    const void* source = font_data.empty() ? nullptr : font_data.data();
    data.push_back({ font_data, "", source, size, nullptr, false, false });
}

void c_font::add_file(const std::string& file_path, float size, bool private_glyphs)
{
    data.push_back({ {}, file_path, nullptr, size, nullptr, true, private_glyphs });
}
