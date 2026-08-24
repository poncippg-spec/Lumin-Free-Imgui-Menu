#pragma once
#include "includes.h"

inline constexpr const char* flaticon_uicons_regular_rounded_path = "..\\..\\..\\..\\..\\framework\\data\\uicons\\uicons-regular-rounded.ttf";

class c_font
{
public:
    void update();

    ImFont* get(const std::vector<unsigned char>& font_data, float size);
    ImFont* get_file(const std::string& file_path, float size, bool private_glyphs = false);

private:
    struct font_data
    {
        std::vector<unsigned char> data;
        std::string file_path;
        const void* source;
        float size;
        ImFont* font;
        bool file;
        bool private_glyphs;
    };

    void add(const std::vector<unsigned char>& font_data, float size);
    void add_file(const std::string& file_path, float size, bool private_glyphs);

    std::vector<font_data> data;
};

inline std::unique_ptr<c_font> font = std::make_unique<c_font>();
