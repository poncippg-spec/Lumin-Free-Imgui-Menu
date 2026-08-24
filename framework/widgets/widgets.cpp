#include "../headers/includes.h"

#include <Windows.h>
#include <array>

// https://discord.gg/jxqVWub6Dk
struct color_picker_state
{
	bool init_val;
	bool open;
	float h, s, v;
	float a;
	float display_h, display_s, display_v;
	float display_a;
	float popup_alpha;
	float hover_alpha;
	c_vec4 last_color;
};

static float color_unit(float value)
{
	return ImClamp(value, 0.f, 1.f);
}

static int color_byte(float value)
{
	return static_cast<int>(color_unit(value) * 255.f + 0.5f);
}

static c_vec4 clamped_color(c_vec4 color)
{
	color.x = color_unit(color.x);
	color.y = color_unit(color.y);
	color.z = color_unit(color.z);
	color.w = color_unit(color.w);
	return color;
}

static bool color_changed_externally(const c_vec4& a, const c_vec4& b)
{
	return ImFabs(a.x - b.x) > 0.001f || ImFabs(a.y - b.y) > 0.001f || ImFabs(a.z - b.z) > 0.001f || ImFabs(a.w - b.w) > 0.001f;
}

static void sync_color_picker_state(color_picker_state* state, const c_vec4& color)
{
	const float previous_h = state->h;
	const bool had_value = state->init_val;
	ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, state->h, state->s, state->v);

	if (had_value && state->s <= 0.0001f)
		state->h = previous_h;

	state->a = color.w;
	state->display_h = state->h;
	state->display_s = state->s;
	state->display_v = state->v;
	state->display_a = state->a;
	state->last_color = color;
	state->init_val = true;
}

static c_vec4 color_from_picker_state(const color_picker_state* state)
{
	c_vec4 result;
	ImGui::ColorConvertHSVtoRGB(state->h, state->s, state->v, result.x, result.y, result.z);
	result.w = state->a;
	return clamped_color(result);
}

static void format_color_hex(char* buffer, int buffer_size, const c_vec4& color, bool include_alpha)
{
	if (include_alpha)
	{
		ImFormatString(buffer, buffer_size, "#%02X%02X%02X%02X",
			color_byte(color.x), color_byte(color.y), color_byte(color.z), color_byte(color.w));
		return;
	}

	ImFormatString(buffer, buffer_size, "#%02X%02X%02X",
		color_byte(color.x), color_byte(color.y), color_byte(color.z));
}

static void draw_alpha_base(ImDrawList* draw_list, const c_rect& rect, float rounding)
{
	draw->rect_filled(draw_list, rect.Min, rect.Max, draw->get_clr(clr->widget), rounding);
	draw->rect_filled_multi_color(draw_list, rect.Min, rect.Max,
		draw->get_clr(clr->white, 0.035f), draw->get_clr(clr->black, 0.045f), draw->get_clr(clr->black, 0.045f), draw->get_clr(clr->white, 0.035f), rounding);
}

static void draw_color_swatch(ImDrawList* draw_list, const c_rect& rect, const c_vec4& color, float rounding)
{
	draw_alpha_base(draw_list, rect, rounding);
	draw->rect_filled(draw_list, rect.Min, rect.Max, draw->get_clr(color), rounding);
}

static void draw_hue_bar(ImDrawList* draw_list, const c_rect& rect, float rounding)
{
	const int segment_count = 6;

	for (int i = 0; i < segment_count; i++)
	{
		const float y0 = rect.Min.y + rect.GetHeight() * (static_cast<float>(i) / static_cast<float>(segment_count));
		const float y1 = rect.Min.y + rect.GetHeight() * (static_cast<float>(i + 1) / static_cast<float>(segment_count));
		const c_vec4 top = ImColor::HSV(static_cast<float>(i) / static_cast<float>(segment_count), 1.f, 1.f).Value;
		const c_vec4 bottom = ImColor::HSV(static_cast<float>(i + 1) / static_cast<float>(segment_count), 1.f, 1.f).Value;
		const draw_flags flags = i == 0 ? draw_flags_round_corners_top : i == segment_count - 1 ? draw_flags_round_corners_bottom : draw_flags_round_corners_none;
		const float segment_rounding = i == 0 || i == segment_count - 1 ? rounding : 0.f;

		draw->rect_filled_multi_color(draw_list, c_vec2(rect.Min.x, y0), c_vec2(rect.Max.x, y1),
			draw->get_clr(top), draw->get_clr(top), draw->get_clr(bottom), draw->get_clr(bottom), segment_rounding, flags);
	}

}

static void draw_color_bar_handle(ImDrawList* draw_list, const c_rect& rect)
{
	draw->shadow_rect(draw_list, rect.Min, rect.Max, draw->get_clr(clr->black, 0.22f), s_(5), c_vec2(0, 0), 0, s_(999));
	draw->rect_filled(draw_list, rect.Min, rect.Max, draw->get_clr(clr->white, 0.96f), s_(999));
	draw->rect_filled(draw_list, rect.Min + s_(2, 2), rect.Max - s_(2, 2), draw->get_clr(clr->black, 0.10f), s_(999));
}

static bool color_rect_button(const c_rect& rect, c_id id, bool* held)
{
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	return gui->button_behavior(rect, id, &hovered, held);
}

bool c_widgets::color_picker(std::string name, c_vec4* color)
{
	if (!color)
		return false;

	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	color_picker_state* state = gui->anim_container<color_picker_state>(id);

	*color = clamped_color(*color);
	if (!state->init_val || color_changed_externally(*color, state->last_color))
		sync_color_picker_state(state, *color);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(50));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(0, 8), rect.Max - s_(0, 8));
	c_rect swatch = c_rect(c_vec2(inner.Max.x - s_(44), inner.GetCenter().y - s_(11)), c_vec2(inner.Max.x, inner.GetCenter().y + s_(11)));

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool held = false;
	bool pressed = gui->button_behavior(rect, id, &hovered, &held);
	if (pressed)
		state->open = !state->open;

	gui->easing(state->hover_alpha, hovered || state->open ? 1.f : 0.f, 10.f, static_easing);

	char hex[16];
	format_color_hex(hex, IM_ARRAYSIZE(hex), *color, color->w < 0.995f);

	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), inner.Min, inner.Max - s_(64, 0),
		draw->get_clr(hovered || state->open ? clr->white.Value : clr->text.Value), name.data(), 0, 0, { 0, 0 });
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), inner.Min + s_(0, 17), inner.Max - s_(64, 0),
		draw->get_clr(clr->text), hex, 0, 0, { 0, 0 });

	draw->shadow_rect(window->DrawList, swatch.Min + s_(2, 3), swatch.Max - s_(2, 0), draw->get_clr(clr->black, 0.24f * state->hover_alpha), s_(9), c_vec2(0, 0), 0, s_(8));
	draw->rect_filled(window->DrawList, swatch.Min - s_(2, 2), swatch.Max + s_(2, 2), draw->get_clr(clr->widget), s_(999));
	draw_color_swatch(window->DrawList, swatch, *color, s_(999));

	if (gui->content_avail().y > 0)
	{
		draw->line(window->DrawList, rect.GetBL(), rect.GetBR(), draw->get_clr(clr->border));
	}

	gui->easing(state->popup_alpha, state->open ? 1.f : 0.f, 11.f, dynamic_easing);

	bool changed = false;
	if (state->open || state->popup_alpha > 0.f)
	{
		const float popup_width = ImClamp(rect.GetWidth(), s_(220), s_(286));
		const c_vec2 popup_size = c_vec2(popup_width, s_(208));
		const c_vec2 popup_pos = gui->adjust_window_pos(rect.GetBL() + s_(0, 6) - s_(0, 8) * (1.f - state->popup_alpha), popup_size);
		const std::string popup_name = name + "##color_popup";

		gui->push_var(style_var_alpha, state->popup_alpha);
		gui->push_var(style_var_window_padding, s_(0, 0));
		gui->push_var(style_var_item_spacing, s_(0, 0));
		gui->set_next_window_pos(popup_pos);
		gui->set_next_window_size(popup_size);
		window_flags popup_flags = window_flags_no_decoration | window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_move | window_flags_no_background | window_flags_no_focus_on_appearing;
		if (!state->open)
			popup_flags |= window_flags_no_inputs;

		gui->begin(popup_name, nullptr, popup_flags);
		{
			if (state->open)
				gui->set_window_focus();

			c_window* popup = gui->get_window();
			const c_rect popup_rect = popup->Rect();
			const c_vec4 live_color = color_from_picker_state(state);

			draw->shadow_rect(popup->DrawList, popup_rect.Min, popup_rect.Max, draw->get_clr(clr->black, 0.24f), s_(16), s_(0, 7), 0, s_(16));
			draw->rect_filled(popup->DrawList, popup_rect.Min, popup_rect.Max, draw->get_clr(clr->child), s_(16));

			c_rect header = c_rect(popup_rect.Min + s_(10, 8), popup_rect.Max - s_(10, 0));
			header.Max.y = header.Min.y + s_(28);
			const c_rect preview = c_rect(header.Min, header.Min + s_(32, 22));

			char live_hex[16];
			char rgba[64];
			format_color_hex(live_hex, IM_ARRAYSIZE(live_hex), live_color, live_color.w < 0.995f);
			ImFormatString(rgba, IM_ARRAYSIZE(rgba), "R %d  G %d  B %d  A %d%%",
				color_byte(live_color.x), color_byte(live_color.y), color_byte(live_color.z), static_cast<int>(color_unit(live_color.w) * 100.f + 0.5f));

			draw_color_swatch(popup->DrawList, preview, live_color, s_(999));
			draw->text_clipped(popup->DrawList, font->get(inter_semibold, 11), header.Min + s_(41, 0), header.Max,
				draw->get_clr(clr->white), live_hex, 0, 0, { 0, 0 });
			draw->text_clipped(popup->DrawList, font->get(inter_medium, 10), header.Min + s_(41, 14), header.Max,
				draw->get_clr(clr->text), rgba, 0, 0, { 0, 0 });

			const float pad = s_(10);
			const float gap = s_(8);
			const float hue_width = s_(14);
			const float picker_height = s_(94);
			const c_rect sv_rect = c_rect(popup_rect.Min + c_vec2(pad, s_(43)), c_vec2(popup_rect.Max.x - pad - hue_width - gap, popup_rect.Min.y + s_(43) + picker_height));
			const c_rect hue_rect = c_rect(c_vec2(sv_rect.Max.x + gap, sv_rect.Min.y), c_vec2(sv_rect.Max.x + gap + hue_width, sv_rect.Max.y));
			const c_rect alpha_rect = c_rect(c_vec2(sv_rect.Min.x, sv_rect.Max.y + s_(10)), c_vec2(hue_rect.Max.x, sv_rect.Max.y + s_(23)));

			const c_vec4 hue_color = ImColor::HSV(state->h, 1.f, 1.f).Value;
			const float sv_rounding = s_(12);
			const c_rect sv_fill = sv_rect;
			draw->rect_filled_multi_color(popup->DrawList, sv_fill.Min, sv_fill.Max,
				draw->get_clr(clr->white), draw->get_clr(hue_color), draw->get_clr(clr->black), draw->get_clr(clr->black), sv_rounding);

			bool sv_held = false;
			const bool sv_pressed = color_rect_button(sv_rect, popup->GetID("sv"), &sv_held);
			if (sv_held || sv_pressed)
			{
				state->s = color_unit((gui->mouse_pos().x - sv_rect.Min.x) / sv_rect.GetWidth());
				state->v = 1.f - color_unit((gui->mouse_pos().y - sv_rect.Min.y) / sv_rect.GetHeight());
				changed = true;
			}

			draw_hue_bar(popup->DrawList, hue_rect, s_(999));
			bool hue_held = false;
			const bool hue_pressed = color_rect_button(hue_rect, popup->GetID("hue"), &hue_held);
			if (hue_held || hue_pressed)
			{
				state->h = color_unit((gui->mouse_pos().y - hue_rect.Min.y) / hue_rect.GetHeight());
				changed = true;
			}

			draw_alpha_base(popup->DrawList, alpha_rect, s_(999));
			draw->rect_filled_multi_color(popup->DrawList, alpha_rect.Min, alpha_rect.Max,
				draw->get_clr(c_vec4(live_color.x, live_color.y, live_color.z, 0.f)), draw->get_clr(c_vec4(live_color.x, live_color.y, live_color.z, 1.f)),
				draw->get_clr(c_vec4(live_color.x, live_color.y, live_color.z, 1.f)), draw->get_clr(c_vec4(live_color.x, live_color.y, live_color.z, 0.f)), s_(999));

			bool alpha_held = false;
			const bool alpha_pressed = color_rect_button(alpha_rect, popup->GetID("alpha"), &alpha_held);
			if (alpha_held || alpha_pressed)
			{
				state->a = color_unit((gui->mouse_pos().x - alpha_rect.Min.x) / alpha_rect.GetWidth());
				changed = true;
			}

			static const c_vec4 presets[] =
			{
				c_vec4(1.00f, 0.26f, 0.32f, 1.f),
				c_vec4(1.00f, 0.58f, 0.22f, 1.f),
				c_vec4(1.00f, 0.88f, 0.25f, 1.f),
				c_vec4(0.34f, 0.86f, 0.55f, 1.f),
				c_vec4(0.26f, 0.76f, 0.94f, 1.f),
				c_vec4(0.42f, 0.55f, 1.00f, 1.f),
				c_vec4(0.72f, 0.44f, 1.00f, 1.f),
				c_vec4(1.00f, 1.00f, 1.00f, 1.f),
			};

			const int preset_count = IM_ARRAYSIZE(presets);
			const float chip_gap = s_(5);
			const float chip_height = s_(13);
			const float chip_width = (alpha_rect.GetWidth() - chip_gap * static_cast<float>(preset_count - 1)) / static_cast<float>(preset_count);
			const float chip_y = alpha_rect.Max.y + s_(13);

			for (int i = 0; i < preset_count; i++)
			{
				const c_rect chip = c_rect(c_vec2(alpha_rect.Min.x + (chip_width + chip_gap) * static_cast<float>(i), chip_y), c_vec2(alpha_rect.Min.x + (chip_width + chip_gap) * static_cast<float>(i) + chip_width, chip_y + chip_height));
				const c_id chip_id = popup->GetID(1000 + i);
				const bool selected = ImFabs(live_color.x - presets[i].x) + ImFabs(live_color.y - presets[i].y) + ImFabs(live_color.z - presets[i].z) < 0.10f;

				bool chip_held = false;
				const bool chip_pressed = color_rect_button(chip, chip_id, &chip_held);

				draw_color_swatch(popup->DrawList, chip, presets[i], s_(999));
				if (selected || chip_held)
				{
					draw->rect_filled(popup->DrawList, chip.Min - s_(2, 2), chip.Max + s_(2, 2),
						draw->get_clr(chip_held ? clr->white.Value : clr->accent.Value, 0.18f), s_(999));
					draw_color_swatch(popup->DrawList, chip, presets[i], s_(999));
				}

				if (chip_pressed)
				{
					ImGui::ColorConvertRGBtoHSV(presets[i].x, presets[i].y, presets[i].z, state->h, state->s, state->v);
					changed = true;
				}
			}

			gui->easing(state->display_s, state->s, 28.f, dynamic_easing);
			gui->easing(state->display_v, state->v, 28.f, dynamic_easing);
			gui->easing(state->display_h, state->h, 28.f, dynamic_easing);
			gui->easing(state->display_a, state->a, 28.f, dynamic_easing);

			const float sv_cursor_radius = s_(5.5f);
			const float sv_cursor_inset = sv_cursor_radius + s_(1);
			const c_vec2 raw_sv_cursor = c_vec2(sv_rect.Min.x + sv_rect.GetWidth() * state->display_s, sv_rect.Min.y + sv_rect.GetHeight() * (1.f - state->display_v));
			const c_vec2 sv_cursor = c_vec2(
				ImClamp(raw_sv_cursor.x, sv_fill.Min.x + sv_cursor_inset, sv_fill.Max.x - sv_cursor_inset),
				ImClamp(raw_sv_cursor.y, sv_fill.Min.y + sv_cursor_inset, sv_fill.Max.y - sv_cursor_inset));
			draw->shadow_circle(popup->DrawList, sv_cursor, s_(6), draw->get_clr(clr->black, 0.30f), s_(4), c_vec2(0, 0));
			draw->circle_filled(popup->DrawList, sv_cursor, sv_cursor_radius, draw->get_clr(clr->white, 0.98f), 32);
			draw->circle_filled(popup->DrawList, sv_cursor, s_(3.1f), draw->get_clr(clr->black, 0.13f), 32);

			const float hue_y = hue_rect.Min.y + hue_rect.GetHeight() * state->display_h;
			draw_color_bar_handle(popup->DrawList, c_rect(c_vec2(hue_rect.Min.x - s_(3), hue_y - s_(4)), c_vec2(hue_rect.Max.x + s_(3), hue_y + s_(4))));

			const float alpha_x = alpha_rect.Min.x + alpha_rect.GetWidth() * state->display_a;
			draw_color_bar_handle(popup->DrawList, c_rect(c_vec2(alpha_x - s_(4), alpha_rect.Min.y - s_(3)), c_vec2(alpha_x + s_(4), alpha_rect.Max.y + s_(3))));

			if (state->open && !popup_rect.Contains(gui->mouse_pos()) && !rect.Contains(gui->mouse_pos()) && gui->mouse_clicked(0))
				state->open = false;
		}
		gui->end();
		gui->pop_var(3);
	}

	if (changed)
	{
		*color = color_from_picker_state(state);
		state->last_color = *color;
	}

	return changed;
}

struct slider_state
{
	float normalized;
	float hover_alpha;
	c_vec4 text;
	c_vec4 value;
	c_vec4 track;
};

custom_slider_t c_widgets::custom_slider(std::string name, float* callback, float vmin, float vmax, float width)
{
	custom_slider_t data;

	c_window* window = gui->get_window();
	if (window->SkipItems)
		return data;

	c_id id = window->GetID(name.data());
	slider_state* state = gui->anim_container<slider_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(width, s_(12));
	c_rect rect = c_rect(pos, pos + size);
	data.rect = rect;

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return data;

	bool held, pressed = gui->button_behavior(rect, id, nullptr, &held);
	data.held = held;

	const float normalized = ImSaturate((gui->mouse_pos().x - rect.Min.x) / rect.GetWidth());

	if (held)
	{
		*callback = vmin + normalized * (vmax - vmin);
	}

	gui->easing(state->normalized, ImSaturate((*callback - vmin) / (vmax - vmin)), 18.f, dynamic_easing);

	float x = rect.GetWidth() * state->normalized;
	x = ImClamp(x, s_(12), rect.GetWidth());

	data.active = c_rect(rect.Min, rect.Min + c_vec2(x, rect.GetHeight()));
	data.circle_pos = c_vec2(rect.Min.x + x - s_(6), rect.GetCenter().y);

	return data;
}

bool c_widgets::slider(std::string name, std::string description, float* callback, float vmin, float vmax, std::string format)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	slider_state* state = gui->anim_container<slider_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(68));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(0, 10), rect.Max - s_(0, 10));
	c_rect button = c_rect(inner.GetBL() - s_(0, 12), inner.GetBR());

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool held, pressed = gui->button_behavior(button, id, nullptr, &held);

	const float normalized = ImSaturate((gui->mouse_pos().x - button.Min.x) / button.GetWidth());

	if (held) 
	{
		*callback = vmin + normalized * (vmax - vmin);
	}

	gui->easing(state->normalized, ImSaturate((*callback - vmin) / (vmax - vmin)), 18.f, dynamic_easing);

	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), inner.Min, inner.Max, draw->get_clr(clr->white),
		name.data(), 0, 0, { 0, 0 });
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), inner.Min + s_(0, 17), inner.Max, draw->get_clr(clr->text),
		description.data(), 0, 0, { 0, 0 });

	char value_buf[64];
	gui->get_fmt(value_buf, callback, format);
	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), inner.Min, inner.Max, draw->get_clr(clr->white),
		value_buf, 0, 0, { 1, 0 });

	draw->rect_filled(window->DrawList, button.Min, button.Max, draw->get_clr(clr->widget), s_(999));
	{
		float x = button.GetWidth() * state->normalized;
		x = ImClamp(x, s_(12), button.GetWidth());

		draw->rect_filled(window->DrawList, button.Min, button.Min + c_vec2(x, button.GetHeight()),
			draw->get_clr(clr->accent), s_(999));

		draw->circle_filled(window->DrawList, c_vec2(button.Min.x + x - s_(6), button.GetCenter().y), s_(4), draw->get_clr(clr->black), s_(999));
	}

	if (gui->content_avail().y > 0)
	{
		draw->line(window->DrawList, rect.GetBL(), rect.GetBR(), draw->get_clr(clr->border));
	}

	return held;
}

struct selectable_state
{
	float alpha;
	c_vec4 text;
};

bool selectable_ex(std::string name, bool is_selected)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	selectable_state* state = gui->anim_container< selectable_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(29));
	c_rect rect = c_rect(pos, pos + size);

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool pressed = gui->button_behavior(rect, id, &hovered, nullptr);

	gui->easing(state->text, is_selected ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);
	gui->easing(state->alpha, is_selected ? 1.f : 0.f, 7.f, static_easing);

	if (gui->content_avail().y > 0)
	{
		draw->line(window->DrawList, rect.GetBL() + s_(10, 0), rect.GetBR() - s_(10, 0), draw->get_clr(clr->border));
	}

	//draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->cirlce, state->alpha), s_(2));

	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), rect.Min + s_(10, 0), rect.Max, draw->get_clr(state->text),
		name.data(), 0, 0, { 0, 0.5 });


	return pressed;
}

struct dropdown_state
{
	bool open;
	float alpha;
	float hover_alpha;
	c_vec4 background;
	c_vec4 text;
	c_vec4 arrow;
};

static void draw_dropdown_chevron(ImDrawList* draw_list, c_vec2 center, bool open, ImU32 color)
{
	const c_rect icon_rect(center - s_(8, 8), center + s_(8, 8));
	ImFont* chevron_font = font->get_file("C:\\Windows\\Fonts\\segmdl2.ttf", 11.f, true);

	if (chevron_font)
	{
		draw->text_clipped(draw_list, chevron_font, icon_rect.Min, icon_rect.Max, color,
			open ? "\xEE\x9C\x8E" : "\xEE\x9C\x8D", 0, 0, { 0.5f, 0.5f });
		return;
	}

	draw->text_clipped(draw_list, font->get(inter_semibold, 12), icon_rect.Min, icon_rect.Max, color,
		open ? "^" : "v", 0, 0, { 0.5f, 0.5f });
}

bool dropdown_ex(std::string name, std::string description, std::string preview)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	dropdown_state* state = gui->anim_container<dropdown_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(84));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(0, 10), rect.Max - s_(0, 10));
	c_rect button = c_rect(inner.GetBL() - s_(0, 29), inner.GetBR());

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool held = false;
	bool pressed = gui->button_behavior(button, id, &hovered, &held);

	if (pressed)
		state->open = true;

	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), inner.Min, inner.Max, draw->get_clr(clr->white),
		name.data(), 0, 0, { 0, 0 });
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), inner.Min + s_(0, 17), inner.Max, draw->get_clr(clr->text),
		description.data(), 0, 0, { 0, 0 });

	draw->rect_filled(window->DrawList, button.Min, button.Max, draw->get_clr(clr->widget), s_(8));
	draw->line(window->DrawList, c_vec2(button.Max.x - s_(28), button.Min.y + s_(8)), c_vec2(button.Max.x - s_(28), button.Max.y - s_(8)),
		draw->get_clr(clr->border, 0.9f));
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), button.Min + s_(10, 0), button.Max - s_(28, 0), draw->get_clr(clr->white),
		preview.data(), 0, 0, { 0, 0.5 });

	const c_vec2 arrow_center(button.Max.x - s_(14), button.GetCenter().y);
	const ImU32 arrow_color = state->open ? draw->get_clr(clr->accent) : draw->get_clr((hovered || held) ? clr->white.Value : clr->text.Value, 0.96f);
	draw_dropdown_chevron(window->DrawList, arrow_center, state->open, arrow_color);

	if (gui->content_avail().y > 0)
	{
		draw->line(window->DrawList, rect.GetBL(), rect.GetBR(), draw->get_clr(clr->border));
	}

	gui->easing(state->alpha, state->open ? 1.f : 0.f, 11.f, dynamic_easing);

	if (state->alpha > 0)
	{
		gui->push_var(style_var_alpha, state->alpha);
		gui->push_var(style_var_window_padding, s_(0, 0));
		gui->push_var(style_var_item_spacing, s_(0, 0));

		gui->set_next_window_pos(button.Min + s_(0, 10) * (1.f - state->alpha) + c_vec2(0, button.GetHeight() + s_(10)));
		gui->set_next_window_size(c_vec2(button.GetWidth(), 0));

		window_flags popup_flags = window_flags_always_auto_resize | window_flags_no_decoration | window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_move | window_flags_no_background;
		if (!state->open)
			popup_flags |= window_flags_no_inputs;

		gui->begin(name, nullptr, popup_flags);
		{

			if (state->open)
				gui->set_window_focus();

			window = gui->get_window();
			rect = window->Rect();

			draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->widget), s_(8));

			if (state->open && !rect.Contains(gui->mouse_pos()) && gui->mouse_clicked(0))
				state->open = false;
		}

		return true;
	}

	return false;
}

void end_dropdown_ex()
{
	gui->end();
	gui->pop_var(3);
}

bool c_widgets::dropdown(std::string name, std::string description, int* callback, std::vector<std::string> variants)
{
	bool changed = false;

	if (dropdown_ex(name, description, variants[*callback]))
	{
		for (int i = 0; i < variants.size(); i++)
		{
			if (selectable_ex(variants[i], i == *callback))
			{
				*callback = i;
				changed = true;
			}
		}

		end_dropdown_ex();
	}

	return changed;
}

struct profile_dropdown_state
{
	bool open;
	float alpha;
	c_vec4 background;
	c_vec4 text;
	c_vec4 status;
	c_vec4 arrow;
};

struct profile_dropdown_row_state
{
	float hover_alpha;
	c_vec4 text;
};

bool profile_dropdown_row(std::string icon, std::string label, bool active)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(label.data());
	profile_dropdown_row_state* state = gui->anim_container<profile_dropdown_row_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(24));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(9, 4), rect.Max - s_(9, 4));

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool pressed = gui->button_behavior(rect, id, &hovered, nullptr);

	gui->easing(state->hover_alpha, hovered || active ? 1.f : 0.f, 10.f, static_easing);
	gui->easing(state->text, active ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);

	if (state->hover_alpha > 0.f)
	{
		const bool active_row = active && !hovered;
		draw->rect_filled(window->DrawList, rect.Min, rect.Max,
			active_row ? draw->get_clr(clr->accent.Value, state->hover_alpha * 0.16f) : draw->get_clr(clr->widget, state->hover_alpha),
			s_(7));
	}

	draw->text_clipped(window->DrawList, font->get(icon_font, 14), inner.Min, inner.Min + s_(16, 16),
		draw->get_clr(active ? clr->accent.Value : state->text), icon.data(), 0, 0, { 0.5f, 0.5f });
	draw->text_clipped(window->DrawList, font->get(inter_semibold, 11), inner.Min + s_(20, 0), inner.Max,
		draw->get_clr(active ? clr->white.Value : state->text), label.data(), 0, 0, { 0, 0.5f });

	return pressed;
}

void c_widgets::brand_header(std::string name, std::string subtitle)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return;

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(50));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(10, 8), rect.Max - s_(10, 8));
	c_rect mark = c_rect(inner.Min, inner.Min + s_(34, 34));

	gui->item_size(rect);
	if (!gui->item_add(rect, window->GetID("brand_header")))
		return;

	const bool glass = var->gui.sidebar_glass;
	draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->child, glass ? 0.24f : 1.f), s_(12));
	if (glass)
		draw->rect(window->DrawList, rect.Min + s_(0.5f, 0.5f), rect.Max - s_(0.5f, 0.5f), draw->get_clr(clr->border, 0.36f), s_(12));
	draw->shadow_rect(window->DrawList, mark.Min + s_(2, 2), mark.Max - s_(2, 2), draw->get_clr(clr->accent, glass ? 0.14f : 0.2f), s_(14), c_vec2(0, 0), 0, s_(10));
	draw->rect_filled(window->DrawList, mark.Min, mark.Max, draw->get_clr(clr->widget, glass ? 0.48f : 1.f), s_(10));
	draw->rect_filled(window->DrawList, mark.Min + s_(3, 3), mark.Max - s_(3, 3), draw->get_clr(clr->accent, 0.12f), s_(8));

	const c_vec2 center = mark.GetCenter();
	draw->circle_filled(window->DrawList, center, s_(12), draw->get_clr(clr->accent, 0.08f), s_(999));
	draw->line(window->DrawList, center + s_(0, -13), center + s_(0, 12), draw->get_clr(clr->accent), s_(2));
	draw->circle_filled(window->DrawList, center + s_(0, -3), s_(5), draw->get_clr(clr->accent), s_(999));
	draw->circle_filled(window->DrawList, center + s_(0, -3), s_(2), draw->get_clr(clr->black, 0.38f), s_(999));
	draw->line(window->DrawList, center + s_(0, 3), center + s_(-8, 13), draw->get_clr(clr->accent, 0.85f), s_(1.5f));
	draw->line(window->DrawList, center + s_(0, 3), center + s_(8, 13), draw->get_clr(clr->accent, 0.85f), s_(1.5f));
	draw->rect_filled(window->DrawList, mark.Min + s_(6, 23), mark.Min + s_(9, 29), draw->get_clr(clr->white, 0.18f), s_(1));
	draw->rect_filled(window->DrawList, mark.Min + s_(11, 20), mark.Min + s_(14, 29), draw->get_clr(clr->white, 0.18f), s_(1));
	draw->rect_filled(window->DrawList, mark.Min + s_(20, 21), mark.Min + s_(23, 29), draw->get_clr(clr->white, 0.18f), s_(1));
	draw->rect_filled(window->DrawList, mark.Min + s_(25, 24), mark.Min + s_(28, 29), draw->get_clr(clr->white, 0.18f), s_(1));
	draw->text_clipped(window->DrawList, font->get(inter_semibold, 13), inner.Min + s_(44, 2), inner.Max,
		draw->get_clr(clr->white), name.data(), 0, 0, { 0, 0 });
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), inner.Min + s_(44, 18), inner.Max,
		draw->get_clr(clr->text), subtitle.data(), 0, 0, { 0, 0 });
}

bool c_widgets::profile_dropdown(std::string name, std::string status)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	profile_dropdown_state* state = gui->anim_container<profile_dropdown_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(58));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(10, 9), rect.Max - s_(10, 9));
	c_rect avatar = c_rect(inner.Min, inner.Min + s_(40, 40));

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool pressed = gui->button_behavior(rect, id, &hovered, nullptr);

	if (pressed)
		state->open = !state->open;

	gui->easing(state->alpha, state->open ? 1.f : 0.f, 11.f, dynamic_easing);
	gui->easing(state->background, (hovered || state->open) ? clr->widget.Value : clr->child.Value, 18.f, dynamic_easing);
	gui->easing(state->text, (hovered || state->open) ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);
	gui->easing(state->status, state->open ? clr->accent.Value : clr->text.Value, 18.f, dynamic_easing);
	gui->easing(state->arrow, state->open ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);

	draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(state->background), s_(12));
	draw->circle_filled(window->DrawList, avatar.GetCenter(), avatar.GetWidth() * 0.5f, draw->get_clr(clr->accent, 0.22f), s_(999));

	draw->text_clipped(window->DrawList, font->get(inter_semibold, 16), avatar.Min, avatar.Max,
		draw->get_clr(clr->white), "D", 0, 0, { 0.5f, 0.5f });

	draw->circle_filled(window->DrawList, avatar.Max - s_(5, 5), s_(7), draw->get_clr(clr->layout), s_(999));
	draw->circle_filled(window->DrawList, avatar.Max - s_(5, 5), s_(4), draw->get_clr(clr->accent), s_(999));
	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), inner.Min + s_(50, 4), inner.Max - s_(18, 0),
		draw->get_clr(clr->white), name.data(), 0, 0, { 0, 0 });
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), inner.Min + s_(50, 20), inner.Max - s_(18, 0),
		draw->get_clr(state->status), status.data(), 0, 0, { 0, 0 });
	draw_dropdown_chevron(window->DrawList, c_vec2(rect.Max.x - s_(18), rect.GetCenter().y), state->open, draw->get_clr(state->arrow));

	if (state->alpha > 0.f)
	{
		const c_vec2 popup_size = c_vec2(rect.GetWidth(), s_(104));
		const c_vec2 popup_pos = rect.GetTL() - c_vec2(0, popup_size.y + s_(5) * state->alpha);

		gui->push_var(style_var_alpha, state->alpha);
		gui->push_var(style_var_window_padding, s_(5, 5));
		gui->push_var(style_var_item_spacing, s_(0, 1));
		gui->set_next_window_pos(popup_pos);
		gui->set_next_window_size(popup_size);

		window_flags popup_flags = window_flags_no_decoration | window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_move | window_flags_no_background | window_flags_no_focus_on_appearing;
		if (!state->open)
			popup_flags |= window_flags_no_inputs;

		gui->begin("profile_dropdown_popup", nullptr, popup_flags);
		{
			if (state->open)
				gui->set_window_focus();
			c_window* popup = gui->get_window();
			c_rect popup_rect = popup->Rect();

			draw->shadow_rect(popup->DrawList, popup_rect.Min, popup_rect.Max, draw->get_clr(clr->black, 0.26f), s_(18), s_(0, 8), 0, s_(12));
			draw->rect_filled(popup->DrawList, popup_rect.Min, popup_rect.Max, draw->get_clr(clr->child), s_(12));
			draw->rect(popup->DrawList, popup_rect.Min, popup_rect.Max, draw->get_clr(clr->border), s_(12));

			draw->text_clipped(popup->DrawList, font->get(inter_semibold, 11), popup_rect.Min + s_(9, 5), popup_rect.Max - s_(9, 0),
				draw->get_clr(clr->white), "Quick settings", 0, 0, { 0, 0 });
			gui->dummy(s_(0, 18));

			if (profile_dropdown_row("C", "Settings", true))
				state->open = false;
			if (profile_dropdown_row("F", "Appearance", false))
				state->open = false;
			if (profile_dropdown_row("B", "Tools", false))
				state->open = false;

			if (state->open && !popup_rect.Contains(gui->mouse_pos()) && !rect.Contains(gui->mouse_pos()) && gui->mouse_clicked(0))
				state->open = false;
		}
		gui->end();
		gui->pop_var(3);
	}

	return pressed;
}

struct primary_button_state
{
	c_vec4 background;
	c_vec4 text;
	float shadow;
	float radius;
	float circle_alpha;
	float icon_spacing;
	bool clicked;
};

static void draw_active_icon(ImDrawList* draw_list, c_vec2 center, ImU32 color)
{
	draw->circle(draw_list, center, s_(7), color, s_(24), s_(1.3f));
	// https://discord.gg/jxqVWub6Dk
	draw->line(draw_list, center + s_(-3, 0), center + s_(-1, 3), color, s_(1.5f));
	draw->line(draw_list, center + s_(-1, 3), center + s_(4, -3), color, s_(1.5f));
}

bool c_widgets::primary_button(std::string name, std::string icon)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	primary_button_state* state = gui->anim_container<primary_button_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(42));
	c_rect rect = c_rect(pos, pos + size);
	const bool has_icon = !icon.empty();

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool held = false;
	bool pressed = gui->button_behavior(rect, id, &hovered, &held);

	if (pressed)
		state->clicked = true;

	gui->easing(state->background, held ? clr->white.Value : clr->accent.Value, 18.f, dynamic_easing);
	gui->easing(state->text, held ? clr->black.Value : clr->black.Value, 18.f, dynamic_easing);
	gui->easing(state->shadow, hovered || held ? 1.f : 0.45f, 10.f, static_easing);
	gui->easing(state->icon_spacing, hovered || held ? 14.f : 7.f, 18.f, dynamic_easing);

	state->radius = ImClamp(state->radius + gui->fixed_speed(rect.GetWidth() * 7.f) * (state->clicked ? 1.f : -1.f), 0.f, rect.GetWidth() * 0.5f + s_(8));
	state->circle_alpha = ImClamp(state->circle_alpha + gui->fixed_speed(3.5f) * (state->radius > rect.GetWidth() * 0.5f - s_(1) ? 1.f : -1.f), 0.f, 1.f);

	if (state->circle_alpha > 0.95f)
	{
		state->radius = 0.f;
		state->circle_alpha = 0.f;
		state->clicked = false;
	}

	draw->shadow_rect(window->DrawList, rect.Min + s_(8, 8), rect.Max - s_(8, 2), draw->get_clr(clr->accent, 0.22f * state->shadow), s_(18), c_vec2(0, 0), 0, s_(11));
	draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(state->background), s_(11));

	draw->push_clip_rect(window->DrawList, rect.Min, rect.Max, true);
	draw->circle_filled(window->DrawList, rect.GetCenter(), state->radius, draw->get_clr(clr->black, 0.28f * (1.f - state->circle_alpha)), s_(48));
	draw->pop_clip_rect(window->DrawList);

	if (has_icon)
	{
		const bool active_icon = icon == "active";
		c_vec2 icon_size = active_icon ? s_(14, 14) : gui->text_size(font->get(icon_font, 15), icon.data());
		c_vec2 name_size = gui->text_size(font->get(inter_semibold, 12), name.data());
		float total_width = icon_size.x + s_(state->icon_spacing) + name_size.x;
		c_vec2 start = c_vec2(rect.GetCenter().x - total_width * 0.5f, rect.Min.y);

		if (active_icon)
			draw_active_icon(window->DrawList, c_vec2(start.x + icon_size.x * 0.5f, rect.GetCenter().y), draw->get_clr(state->text));
		else
			draw->text_clipped(window->DrawList, font->get(icon_font, 15), start, start + c_vec2(icon_size.x, rect.GetHeight()),
				draw->get_clr(state->text), icon.data(), 0, 0, { 0, 0.5f });
		draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), start + c_vec2(icon_size.x + s_(state->icon_spacing), 0), rect.Max,
			draw->get_clr(state->text), name.data(), 0, 0, { 0, 0.5f });
	}
	else
	{
		draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), rect.Min, rect.Max,
			draw->get_clr(state->text), name.data(), 0, 0, { 0.5f, 0.5f });
	}

	return pressed;
}

struct action_button_state
{
	c_vec4 background;
	c_vec4 text;
	c_vec4 icon;
	float accent_alpha;
	float arrow_offset;
};

static void draw_action_icon(ImDrawList* draw_list, std::string icon, c_vec2 center, ImU32 color)
{
	const float thickness = s_(1.4f);

	if (icon == "copy")
	{
		draw->rect(draw_list, center + s_(-5, -4), center + s_(3, 4), color, s_(2), 0, thickness);
		draw->rect(draw_list, center + s_(-2, -7), center + s_(6, 1), color, s_(2), 0, thickness);
		return;
	}

	if (icon == "back")
	{
		draw->line(draw_list, center + s_(-5, 0), center + s_(6, 0), color, thickness);
		draw->line(draw_list, center + s_(-5, 0), center + s_(0, -5), color, thickness);
		draw->line(draw_list, center + s_(-5, 0), center + s_(0, 5), color, thickness);
		return;
	}

	if (icon == "reset")
	{
		draw->circle(draw_list, center, s_(6), color, s_(28), thickness);
		draw->triangle_filled(draw_list, center + s_(2, -8), center + s_(8, -6), center + s_(5, -2), color);
		return;
	}

	if (!icon.empty())
	{
		draw->text_clipped(draw_list, font->get(icon_font, 15), center - s_(8, 8), center + s_(8, 8),
			color, icon.data(), 0, 0, { 0.5f, 0.5f });
	}
}

bool c_widgets::action_button(std::string name, std::string icon)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	action_button_state* state = gui->anim_container<action_button_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(44));
	c_rect rect = c_rect(pos, pos + size);
	c_rect button = c_rect(rect.Min + s_(0, 3), rect.Max - s_(0, 3));
	c_rect icon_rect = c_rect(button.Min + s_(9, 8), button.Min + s_(31, 30));

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool held = false;
	bool pressed = gui->button_behavior(button, id, &hovered, &held);

	gui->easing(state->background, held ? clr->cirlce.Value : clr->widget.Value, 18.f, dynamic_easing);
	gui->easing(state->text, hovered || held ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);
	gui->easing(state->icon, hovered || held ? clr->accent.Value : clr->text.Value, 18.f, dynamic_easing);
	gui->easing(state->arrow_offset, hovered || held ? 2.f : 0.f, 18.f, dynamic_easing);
	state->accent_alpha = ImClamp(state->accent_alpha + gui->fixed_speed(9.f) * (hovered || held ? 1.f : -1.f), 0.f, 1.f);

	draw->rect_filled(window->DrawList, button.Min, button.Max, draw->get_clr(state->background), s_(8));
	draw->rect(window->DrawList, button.Min, button.Max, draw->get_clr(clr->border, 0.9f), s_(8));
	draw->rect_filled(window->DrawList, button.Min, button.Min + c_vec2(s_(3), button.GetHeight()),
		draw->get_clr(clr->accent, state->accent_alpha), s_(8));

	draw->rect_filled(window->DrawList, icon_rect.Min, icon_rect.Max,
		draw->get_clr(clr->accent, 0.08f + state->accent_alpha * 0.12f), s_(6));
	draw_action_icon(window->DrawList, icon, icon_rect.GetCenter(), draw->get_clr(state->icon));

	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), button.Min + s_(40, 0), button.Max - s_(28, 0),
		draw->get_clr(state->text), name.data(), 0, 0, { 0, 0.5f });

	const c_vec2 arrow = c_vec2(button.Max.x - s_(17) + s_(state->arrow_offset), button.GetCenter().y);
	draw->line(window->DrawList, arrow + s_(-2, -4), arrow + s_(2, 0), draw->get_clr(state->icon), s_(1.3f));
	draw->line(window->DrawList, arrow + s_(2, 0), arrow + s_(-2, 4), draw->get_clr(state->icon), s_(1.3f));

	return pressed;
}

struct card_button_state
{
	c_vec4 text;
	c_vec4 background;
	c_vec4 button_text;
};

bool c_widgets::card_button(std::string name, std::string description, std::string button_text)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	card_button_state* state = gui->anim_container<card_button_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(82));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(elements->padding), rect.Max - s_(elements->padding));
	c_rect button = c_rect(inner.GetBL() - s_(0, 27), inner.GetBR());

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool held = false;
	bool pressed = gui->button_behavior(button, id, &hovered, &held);

	gui->easing(state->text, hovered || held ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);
	gui->easing(state->background, hovered || held ? clr->accent.Value : clr->widget.Value, 18.f, dynamic_easing);
	gui->easing(state->button_text, hovered || held ? clr->black.Value : clr->white.Value, 18.f, dynamic_easing);

	draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->child), s_(2));

	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), inner.Min, inner.Max, draw->get_clr(state->text),
		name.data(), 0, 0, { 0, 0 });
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), inner.Min + s_(0, 16), inner.Max, draw->get_clr(clr->text),
		description.data(), 0, 0, { 0, 0 });

	draw->rect_filled(window->DrawList, button.Min, button.Max, draw->get_clr(state->background), s_(2));
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), button.Min, button.Max, draw->get_clr(state->button_text),
		button_text.data(), 0, 0, { 0.5, 0.5 });

	return pressed;
}

struct checkbox_state
{
	c_vec4 text;
	c_vec4 background;
	c_vec4 circle;
	float pos;
	float hover_alpha;
	float glow_alpha;
	float tooltip_alpha;
};

static c_vec4 title_status_color(title_status_icon status)
{
	c_vec4 color = c_col(75, 220, 145).Value;

	if (status == title_status_danger)
		color = c_col(245, 70, 86).Value;
	else if (status == title_status_warning)
		color = c_col(255, 190, 75).Value;

	return color;
}

static const char* title_status_label(title_status_icon status)
{
	if (status == title_status_danger)
		return "DANGER";
	if (status == title_status_warning)
		return "WARNING";
	if (status == title_status_safe)
		return "SAFE";

	return "";
}

static const char* title_status_hint(title_status_icon status)
{
	if (status == title_status_danger)
		return "High signal item. Use only when this detail is worth the extra exposure.";
	if (status == title_status_warning)
		return "Medium signal item. Useful, but it can add visual noise fast.";
	if (status == title_status_safe)
		return "Low signal item. Stable for normal use and clean defaults.";

	return "";
}

static bool draw_title_status_icon(ImDrawList* draw_list, const c_vec2& center, title_status_icon status, c_rect* out_rect)
{
	if (status == title_status_none)
		return false;

	const c_vec4 color = title_status_color(status);

	const c_rect icon_rect(center - s_(6, 8), center + s_(6, 8));
	if (out_rect)
		*out_rect = icon_rect;

	draw->text_clipped(draw_list, font->get(inter_semibold, 18), icon_rect.Min, icon_rect.Max, draw->get_clr(color),
		"*", 0, 0, { 0.5f, 0.5f });

	return icon_rect.Contains(gui->mouse_pos());
}

static void draw_title_status_tooltip(const c_rect& anchor, title_status_icon status, const std::string& title, const std::string& description, float alpha)
{
	if (status == title_status_none || alpha <= 0.01f)
		return;

	const c_vec4 color = title_status_color(status);
	const char* label = title_status_label(status);
	const char* hint = title_status_hint(status);
	ImDrawList* draw_list = ImGui::GetForegroundDrawList();

	const float compact = 0.90f;
	const c_vec2 size = s_(286.f * compact, 104.f * compact);
	c_vec2 pos = gui->adjust_window_pos(anchor.Max + s_(12.f * compact, -16.f * compact) - s_(0, 6.f * compact) * (1.f - alpha), size);
	const c_rect rect(pos, pos + size);

	const float rounding = s_(10.f * compact);
	draw->shadow_rect(draw_list, rect.Min, rect.Max, draw->get_clr(clr->black, 0.28f * alpha), s_(18.f * compact), s_(0, 8.f * compact), 0, rounding);
	draw->rect_filled(draw_list, rect.Min, rect.Max, draw->get_clr(clr->child, 0.98f * alpha), rounding);
	draw->rect_filled_multi_color(draw_list, rect.Min, rect.Max,
		draw->get_clr(clr->white, 0.035f * alpha), draw->get_clr(color, 0.055f * alpha),
		draw->get_clr(clr->black, 0.02f * alpha), draw->get_clr(clr->black, 0.02f * alpha), rounding);
	draw->rect(draw_list, rect.Min, rect.Max, draw->get_clr(color, 0.30f * alpha), rounding, 0, s_(1));
	draw->rect_filled(draw_list, rect.Min + s_(0, 11.f * compact), rect.Min + s_(3.f * compact, 46.f * compact), draw->get_clr(color, alpha), s_(999));

	const c_rect star_rect(rect.Min + s_(13.f * compact, 10.f * compact), rect.Min + s_(29.f * compact, 30.f * compact));
	draw->text_clipped(draw_list, font->get(inter_semibold, 18), star_rect.Min, star_rect.Max,
		draw->get_clr(color, alpha), "*", 0, 0, { 0.5f, 0.5f });

	draw->text_clipped(draw_list, font->get(inter_semibold, 10), rect.Min + s_(34.f * compact, 11.f * compact), rect.Min + s_(160.f * compact, 27.f * compact),
		draw->get_clr(color, alpha), label, 0, 0, { 0, 0.5f });

	const c_rect tag(rect.Max - s_(69.f * compact, 93.f * compact), rect.Max - s_(12.f * compact, 75.f * compact));
	draw->rect_filled(draw_list, tag.Min, tag.Max, draw->get_clr(color, 0.12f * alpha), s_(999));
	draw->rect(draw_list, tag.Min, tag.Max, draw->get_clr(color, 0.38f * alpha), s_(999), 0, s_(1));
	draw->text_clipped(draw_list, font->get(inter_semibold, 10), tag.Min, tag.Max,
		draw->get_clr(color, alpha), "STATUS", 0, 0, { 0.5f, 0.5f });

	draw->text_clipped(draw_list, font->get(inter_semibold, 12), rect.Min + s_(13.f * compact, 36.f * compact), rect.Max - s_(13.f * compact, 0),
		draw->get_clr(clr->white, alpha), title.data(), 0, 0, { 0, 0 });
	draw->text_clipped(draw_list, font->get(inter_medium, 10), rect.Min + s_(13.f * compact, 54.f * compact), rect.Max - s_(13.f * compact, 0),
		draw->get_clr(clr->text, alpha), description.data(), 0, 0, { 0, 0 });
	draw->line(draw_list, rect.Min + s_(13.f * compact, 75.f * compact), rect.Max - s_(13.f * compact, 29.f * compact), draw->get_clr(clr->border, 0.92f * alpha));
	draw->text_clipped(draw_list, font->get(inter_medium, 10), rect.Min + s_(13.f * compact, 82.f * compact), rect.Max - s_(13.f * compact, 8.f * compact),
		draw->get_clr(clr->text, 0.92f * alpha), hint, 0, 0, { 0, 0 });
}

struct keybind_widget_state
{
	bool listening;
	int listen_started_frame;
	float bind_hover_alpha;
	float listen_alpha;
	c_vec4 bind_background;
	c_vec4 title;
};

static bool keybind_is_ctrl(ImGuiKey key)
{
	return key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl;
}

static bool keybind_is_shift(ImGuiKey key)
{
	return key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift;
}

static bool keybind_is_alt(ImGuiKey key)
{
	return key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt;
}

static bool keybind_is_super(ImGuiKey key)
{
	return key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper;
}

static bool keybind_is_mouse_key(ImGuiKey key)
{
	return key >= ImGuiKey_MouseLeft && key <= ImGuiKey_MouseX2;
}

static bool keybind_is_wheel_key(ImGuiKey key)
{
	return key == ImGuiKey_MouseWheelX || key == ImGuiKey_MouseWheelY;
}

static void keybind_assign(keybind_t* bind, ImGuiKey key);
static void keybind_clear(keybind_t* bind);

static bool keybind_is_modifier(ImGuiKey key)
{
	return keybind_is_ctrl(key) || keybind_is_shift(key) || keybind_is_alt(key) || keybind_is_super(key);
}

static std::array<bool, 256> g_keybind_previous_vk_down{};

static bool keybind_vk_down(int vk)
{
	return vk > 0 && vk < static_cast<int>(g_keybind_previous_vk_down.size()) && (GetAsyncKeyState(vk) & 0x8000) != 0;
}

static bool keybind_vk_pressed(int vk)
{
	if (vk <= 0 || vk >= static_cast<int>(g_keybind_previous_vk_down.size()))
		return false;

	const bool down = keybind_vk_down(vk);
	const bool pressed = down && !g_keybind_previous_vk_down[vk];
	g_keybind_previous_vk_down[vk] = down;
	return pressed;
}

static void keybind_prime_physical_input()
{
	for (int vk = 0; vk < static_cast<int>(g_keybind_previous_vk_down.size()); vk++)
		g_keybind_previous_vk_down[vk] = keybind_vk_down(vk);
}

static ImGuiKey keybind_key_from_vk(int vk)
{
	if (vk >= '0' && vk <= '9')
		return static_cast<ImGuiKey>(ImGuiKey_0 + (vk - '0'));
	if (vk >= 'A' && vk <= 'Z')
		return static_cast<ImGuiKey>(ImGuiKey_A + (vk - 'A'));
	if (vk >= VK_F1 && vk <= VK_F24)
		return static_cast<ImGuiKey>(ImGuiKey_F1 + (vk - VK_F1));
	if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
		return static_cast<ImGuiKey>(ImGuiKey_Keypad0 + (vk - VK_NUMPAD0));

	switch (vk)
	{
	case VK_TAB: return ImGuiKey_Tab;
	case VK_LEFT: return ImGuiKey_LeftArrow;
	case VK_RIGHT: return ImGuiKey_RightArrow;
	case VK_UP: return ImGuiKey_UpArrow;
	case VK_DOWN: return ImGuiKey_DownArrow;
	case VK_PRIOR: return ImGuiKey_PageUp;
	case VK_NEXT: return ImGuiKey_PageDown;
	case VK_HOME: return ImGuiKey_Home;
	case VK_END: return ImGuiKey_End;
	case VK_INSERT: return ImGuiKey_Insert;
	case VK_DELETE: return ImGuiKey_Delete;
	case VK_BACK: return ImGuiKey_Backspace;
	case VK_SPACE: return ImGuiKey_Space;
	case VK_RETURN: return ImGuiKey_Enter;
	case VK_ESCAPE: return ImGuiKey_Escape;
	case VK_LCONTROL: return ImGuiKey_LeftCtrl;
	case VK_RCONTROL: return ImGuiKey_RightCtrl;
	case VK_LSHIFT: return ImGuiKey_LeftShift;
	case VK_RSHIFT: return ImGuiKey_RightShift;
	case VK_LMENU: return ImGuiKey_LeftAlt;
	case VK_RMENU: return ImGuiKey_RightAlt;
	case VK_LWIN: return ImGuiKey_LeftSuper;
	case VK_RWIN: return ImGuiKey_RightSuper;
	case VK_APPS: return ImGuiKey_Menu;
	case VK_OEM_7: return ImGuiKey_Apostrophe;
	case VK_OEM_COMMA: return ImGuiKey_Comma;
	case VK_OEM_MINUS: return ImGuiKey_Minus;
	case VK_OEM_PERIOD: return ImGuiKey_Period;
	case VK_OEM_2: return ImGuiKey_Slash;
	case VK_OEM_1: return ImGuiKey_Semicolon;
	case VK_OEM_PLUS: return ImGuiKey_Equal;
	case VK_OEM_4: return ImGuiKey_LeftBracket;
	case VK_OEM_5: return ImGuiKey_Backslash;
	case VK_OEM_6: return ImGuiKey_RightBracket;
	case VK_OEM_3: return ImGuiKey_GraveAccent;
	case VK_CAPITAL: return ImGuiKey_CapsLock;
	case VK_SCROLL: return ImGuiKey_ScrollLock;
	case VK_NUMLOCK: return ImGuiKey_NumLock;
	case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
	case VK_PAUSE: return ImGuiKey_Pause;
	case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
	case VK_DIVIDE: return ImGuiKey_KeypadDivide;
	case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
	case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
	case VK_ADD: return ImGuiKey_KeypadAdd;
	default: return ImGuiKey_None;
	}
}

static bool keybind_capture_physical_input(keybind_t* bind)
{
	if (keybind_vk_pressed(VK_ESCAPE))
	{
		keybind_clear(bind);
		return true;
	}

	struct mouse_key_map
	{
		int vk;
		ImGuiKey key;
	};

	static const mouse_key_map mouse_keys[] =
	{
		{ VK_LBUTTON, ImGuiKey_MouseLeft },
		{ VK_RBUTTON, ImGuiKey_MouseRight },
		{ VK_MBUTTON, ImGuiKey_MouseMiddle },
		{ VK_XBUTTON1, ImGuiKey_MouseX1 },
		{ VK_XBUTTON2, ImGuiKey_MouseX2 },
	};

	for (const mouse_key_map& mouse_key : mouse_keys)
	{
		if (keybind_vk_pressed(mouse_key.vk))
		{
			keybind_assign(bind, mouse_key.key);
			return true;
		}
	}

	for (int vk = 1; vk < static_cast<int>(g_keybind_previous_vk_down.size()); vk++)
	{
		const ImGuiKey key = keybind_key_from_vk(vk);
		if (key == ImGuiKey_None || keybind_is_modifier(key))
			continue;

		if (keybind_vk_pressed(vk))
		{
			keybind_assign(bind, key);
			return true;
		}
	}

	return false;
}

static std::string keybind_display_name(const keybind_t& bind)
{
	if (bind.key == ImGuiKey_None)
		return "Unbound";

	const ImGuiKey key = static_cast<ImGuiKey>(bind.key);
	const char* key_name = ImGui::GetKeyName(key);
	std::string display;

	if (bind.ctrl)
		display += "Ctrl + ";
	if (bind.shift)
		display += "Shift + ";
	if (bind.alt)
		display += "Alt + ";
	if (bind.super)
		display += "Super + ";

	if (key_name && key_name[0] != '\0')
		display += key_name;
	else
		display += "Key " + std::to_string(bind.key);

	return display;
}

static void keybind_assign(keybind_t* bind, ImGuiKey key)
{
	ImGuiIO& io = ImGui::GetIO();

	bind->key = key;
	bind->ctrl = (io.KeyCtrl || keybind_vk_down(VK_CONTROL)) && !keybind_is_ctrl(key);
	bind->shift = (io.KeyShift || keybind_vk_down(VK_SHIFT)) && !keybind_is_shift(key);
	bind->alt = (io.KeyAlt || keybind_vk_down(VK_MENU)) && !keybind_is_alt(key);
	bind->super = (io.KeySuper || keybind_vk_down(VK_LWIN) || keybind_vk_down(VK_RWIN)) && !keybind_is_super(key);
}

static void keybind_clear(keybind_t* bind)
{
	bind->key = ImGuiKey_None;
	bind->ctrl = false;
	bind->shift = false;
	bind->alt = false;
	bind->super = false;
	bind->capturing = false;
}

static bool keybind_capture_input(keybind_t* bind)
{
	if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
	{
		keybind_clear(bind);
		return true;
	}

	for (int button = 0; button < ImGuiMouseButton_COUNT; button++)
	{
		if (ImGui::IsMouseClicked(button, false))
		{
			keybind_assign(bind, static_cast<ImGuiKey>(ImGuiKey_MouseLeft + button));
			return true;
		}
	}

	for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
	{
		const ImGuiKey imgui_key = static_cast<ImGuiKey>(key);
		if (keybind_is_mouse_key(imgui_key) || keybind_is_wheel_key(imgui_key) || keybind_is_modifier(imgui_key))
			continue;

		if (ImGui::IsKeyPressed(imgui_key, false))
		{
			keybind_assign(bind, imgui_key);
			return true;
		}
	}

	return keybind_capture_physical_input(bind);
}

bool c_widgets::keybind(std::string name, std::string description, keybind_t* bind)
{
	if (!bind)
		return false;

	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	bind->mode = ImClamp(bind->mode, static_cast<int>(keybind_mode_hold), static_cast<int>(keybind_mode_always));

	c_id id = window->GetID(name.data());
	keybind_widget_state* state = gui->anim_container<keybind_widget_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(45));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(0, 9), rect.Max - s_(0, 9));

	const float bind_width = ImClamp(inner.GetWidth() * 0.30f, s_(82), s_(108));
	const c_rect bind_button = c_rect(c_vec2(inner.Max.x - bind_width, inner.GetCenter().y - s_(12)), c_vec2(inner.Max.x, inner.GetCenter().y + s_(12)));
	const c_rect text_rect = c_rect(inner.Min, c_vec2(bind_button.Min.x - s_(14), inner.Max.y));

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool row_hovered = false;
	bool row_held = false;
	const bool bind_pressed = gui->button_behavior(rect, id, &row_hovered, &row_held);
	const bool bind_hovered = row_hovered && bind_button.Contains(gui->mouse_pos());
	const bool bind_held = row_held && bind_button.Contains(gui->mouse_pos());

	bool changed = false;
	if (bind_pressed)
	{
		state->listening = true;
		bind->capturing = true;
		state->listen_started_frame = ImGui::GetFrameCount();
		keybind_prime_physical_input();
	}

	if (state->listening && ImGui::GetFrameCount() > state->listen_started_frame && keybind_capture_input(bind))
	{
		state->listening = false;
		bind->capturing = false;
		changed = true;
	}
	bind->capturing = state->listening;

	gui->easing(state->bind_hover_alpha, bind_hovered || bind_held || state->listening ? 1.f : 0.f, 12.f, dynamic_easing);
	gui->easing(state->listen_alpha, state->listening ? 1.f : 0.f, 14.f, dynamic_easing);
	gui->easing(state->bind_background, state->listening ? clr->accent.Value : clr->widget.Value, 18.f, dynamic_easing);
	gui->easing(state->title, state->listening ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);

	const float pulse = state->listening ? 0.5f + 0.5f * ImSin(static_cast<float>(ImGui::GetTime()) * 5.2f) : 0.f;
	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), text_rect.Min, text_rect.Max,
		draw->get_clr(state->title), name.data(), 0, 0, { 0, 0 });
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), text_rect.Min + s_(0, 16), text_rect.Max,
		draw->get_clr(clr->text), description.data(), 0, 0, { 0, 0 });

	if (state->listening)
		draw->shadow_rect(window->DrawList, bind_button.Min + s_(3, 4), bind_button.Max - s_(3, 1), draw->get_clr(clr->accent, 0.16f + pulse * 0.06f), s_(12), c_vec2(0, 0), 0, s_(7));

	draw->rect_filled(window->DrawList, bind_button.Min, bind_button.Max, draw->get_clr(state->bind_background), s_(7));
	draw->rect_filled_multi_color(window->DrawList, bind_button.Min, bind_button.Max,
		draw->get_clr(clr->white, state->listening ? 0.12f : 0.018f * state->bind_hover_alpha), draw->get_clr(clr->white, state->listening ? 0.04f : 0.012f),
		draw->get_clr(clr->black, 0.05f), draw->get_clr(clr->black, 0.07f), s_(7));
	draw->rect(window->DrawList, bind_button.Min, bind_button.Max,
		draw->get_clr(state->listening ? clr->accent.Value : clr->border.Value, state->listening ? 0.80f : 0.92f), s_(7));

	const std::string bind_text = state->listening ? "Press key" : keybind_display_name(*bind);
	draw->text_clipped(window->DrawList, font->get(inter_semibold, 10), bind_button.Min + s_(9, 0), bind_button.Max - s_(9, 0),
		draw->get_clr(state->listening ? clr->black.Value : (bind_hovered ? clr->white.Value : clr->text.Value)), bind_text.data(), 0, 0, { 0.5f, 0.5f });

	if (gui->content_avail().y > 0)
	{
		draw->line(window->DrawList, rect.GetBL(), rect.GetBR(), draw->get_clr(clr->border));
	}

	return changed;
}

bool c_widgets::checkbox(std::string name, std::string description, bool* callback, title_status_icon status)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	checkbox_state* state = gui->anim_container<checkbox_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(gui->content_avail().x, s_(40));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(0, 6), rect.Max - s_(0, 6));
	c_vec2 switch_size = s_(36.3f, 20.9f);
	c_rect button = c_rect(c_vec2(inner.Max.x - switch_size.x, inner.GetCenter().y - switch_size.y * 0.5f), c_vec2(inner.Max.x, inner.GetCenter().y + switch_size.y * 0.5f));

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool held = false;
	bool pressed = gui->button_behavior(inner, id, &hovered, &held);

	if (pressed)
	{
		*callback = !*callback;
		notify->add_notify(*callback ? "Function enabled" : "Function disabled", name, *callback ? success : warning);
	}

	const c_vec4 off_track = clr->widget.Value;
	const c_vec4 on_track = c_col(38, 37, 50).Value;
	const c_vec4 on_knob = clr->accent.Value;
	const c_vec4 off_knob = c_col(96, 88, 142).Value;
	const c_vec4 on_knob_inner = c_col(28, 28, 33).Value;
	const c_vec4 off_knob_inner = c_col(31, 31, 39).Value;

	gui->easing(state->pos, *callback ? 1.f : 0.f, 20.f, dynamic_easing);
	gui->easing(state->hover_alpha, hovered || held ? 1.f : 0.f, 14.f, dynamic_easing);
	gui->easing(state->glow_alpha, *callback ? 1.f : 0.f, 18.f, dynamic_easing);
	gui->easing(state->text, *callback ? clr->white.Value : c_vec4(clr->text.Value.x * 0.82f, clr->text.Value.y * 0.82f, clr->text.Value.z * 0.82f, clr->text.Value.w), 18.f, dynamic_easing);
	gui->easing(state->background, *callback ? on_track : off_track, 18.f, dynamic_easing);
	gui->easing(state->circle, *callback ? on_knob : off_knob, 18.f, dynamic_easing);

	const c_vec2 title_min = inner.Min;
	c_rect status_rect;
	bool status_hovered = false;

	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), title_min, c_vec2(button.Min.x - s_(12), inner.Max.y), draw->get_clr(state->text),
		name.data(), 0, 0, { 0, 0.5f });

	if (status != title_status_none)
	{
		const c_vec2 title_size = gui->text_size(font->get(inter_semibold, 12), name.data());
		const float max_x = button.Min.x - s_(14);
		const float icon_x = ImMin(title_min.x + title_size.x + s_(11), max_x);
		status_hovered = draw_title_status_icon(window->DrawList, c_vec2(icon_x, inner.GetCenter().y), status, &status_rect);
	}

	gui->easing(state->tooltip_alpha, status_hovered ? 1.f : 0.f, 18.f, dynamic_easing);
	draw_title_status_tooltip(status_rect, status, name, description, state->tooltip_alpha);

	const float switch_rounding = s_(999);
	const c_vec2 knob_center = c_vec2(ImLerp(button.Min.x + s_(10.45f), button.Max.x - s_(10.45f), state->pos), button.GetCenter().y);
	const float marker_x = ImLerp(button.Max.x - s_(9.98f), button.Min.x + s_(9.98f), state->pos);
	const c_rect marker(c_vec2(marker_x - s_(1.35f), button.GetCenter().y - s_(4.35f)), c_vec2(marker_x + s_(1.35f), button.GetCenter().y + s_(4.35f)));

	draw->rect_filled(window->DrawList, button.Min, button.Max, draw->get_clr(state->background), switch_rounding);

	const ImU32 marker_col = draw->get_clr(*callback ? clr->accent.Value : c_col(132, 113, 198).Value, *callback ? 0.92f : 0.58f);
	window->DrawList->AddRectFilled(marker.Min, marker.Max, marker_col, marker.GetWidth() * 0.5f, ImDrawFlags_RoundCornersAll);

	draw->circle_filled(window->DrawList, knob_center, s_(7.0f), draw->get_clr(state->circle), 32);
	draw->circle_filled(window->DrawList, knob_center, s_(3.65f), draw->get_clr(*callback ? on_knob_inner : off_knob_inner, *callback ? 1.0f : 0.95f), 32);

	if (gui->content_avail().y > 0)
	{
		const float y = IM_FLOOR(rect.Max.y - 0.5f) + 0.5f;
		window->DrawList->AddLine(c_vec2(rect.Min.x, y), c_vec2(rect.Max.x, y), draw->get_clr(c_col(35, 35, 44).Value, 0.72f), 1.0f);
	}

	return pressed;
}

bool c_widgets::child(std::string name)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());

	gui->begin_content(name, c_vec2(elements->child_width, 0), s_(10, 0), s_(0, 0), window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_move, child_flags_always_auto_resize | child_flags_auto_resize_y);
	{
		window = gui->get_window();

		draw->rect_filled(window->DrawList, window->Rect().Min, window->Rect().Max, draw->get_clr(clr->child), s_(10));
	}
	
	return true;
}

void c_widgets::end_child()
{
	gui->end_content();
}

struct tab_button_state
{
	float width;
	c_vec4 icon;
	c_vec4 background;
	float alpha;
};

c_vec4 g_pill_selected_rect = c_vec4(0, 0, 0, 0);
c_vec4 g_sidebar_selected_rect = c_vec4(0, 0, 0, 0);

static void append_utf8(std::string& output, unsigned int codepoint)
{
	if (codepoint <= 0x7F)
	{
		output.push_back(static_cast<char>(codepoint));
		return;
	}

	if (codepoint <= 0x7FF)
	{
		output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
		output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
		return;
	}

	if (codepoint <= 0xFFFF)
	{
		output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
		output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
		output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
		return;
	}

	output.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
	output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
	output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
	output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
}

static unsigned int tab_icon_codepoint(const std::string& icon)
{
	if (icon == "player") return 0xFEA0;   // fi-rr-user
	if (icon == "shapes") return 0xF71C;   // fi-rr-grid
	if (icon == "world") return 0xFF21;    // fi-rr-world
	if (icon == "polish") return 0xF8E1;   // fi-rr-magic-wand
	if (icon == "combat") return 0xFD32;   // fi-rr-sword
	if (icon == "aim") return 0xFD5F;      // fi-rr-target
	if (icon == "loot") return 0xFE19;     // fi-rr-treasure-chest
	if (icon == "movement") return 0xFB7C; // fi-rr-running
	if (icon == "visuals") return 0xF5F8;  // fi-rr-eye
	if (icon == "hud") return 0xF87D;      // fi-rr-layout-fluid
	if (icon == "profile") return 0xFEA0;  // fi-rr-user
	if (icon == "loadout") return 0xF1E7;  // fi-rr-backpack
	if (icon == "match") return 0xF309;    // fi-rr-bullseye
	if (icon == "squad") return 0xFEA6;    // fi-rr-users
	if (icon == "stats") return 0xF3A2;    // fi-rr-chart-histogram
	return 0;
}

static std::string tab_icon_glyph(const std::string& icon)
{
	const unsigned int codepoint = tab_icon_codepoint(icon);
	if (!codepoint)
		return icon;

	std::string glyph;
	append_utf8(glyph, codepoint);
	return glyph;
}

static void draw_tab_icon(ImDrawList* draw_list, const c_rect& rect, const std::string& icon, const c_vec4& color, float size, ImFont* fallback_font)
{
	const unsigned int codepoint = tab_icon_codepoint(icon);
	const std::string glyph = codepoint ? tab_icon_glyph(icon) : icon;
	ImFont* icon_font = codepoint ? font->get_file(flaticon_uicons_regular_rounded_path, size, true) : fallback_font;

	if (!icon_font)
		icon_font = fallback_font;

	draw->text_clipped(draw_list, icon_font, rect.Min, rect.Max,
		draw->get_clr(color), glyph.c_str(), 0, 0, { 0.5, 0.5 });
}

bool c_widgets::tab_button(std::string name, std::string icon, int tab)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	tab_button_state* state = gui->anim_container<tab_button_state>(id);

	bool is_selected = tab == var->gui.tab_stored;
	const float tab_height = 32.f;
	const float icon_slot = 32.f;
	float target_width = ImMax(gui->content_avail().x, s_(tab_height));
	if (state->width <= 0.f)
		state->width = target_width;
	gui->easing(state->width, target_width, 18.f, dynamic_easing);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(round(state->width), s_(tab_height));
	c_rect rect = c_rect(pos, pos + size);

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool pressed = gui->button_behavior(rect, id, &hovered, nullptr);

	if (pressed)
	{
		var->gui.tab_stored = tab;
	}

	if (is_selected)
		g_sidebar_selected_rect = c_vec4(rect.Min.x, rect.Min.y, rect.Max.x, rect.Max.y);

	gui->easing(state->alpha, (is_selected || hovered) ? 1.f : 0.58f, 18.f, dynamic_easing);
	gui->easing(state->icon, (is_selected || hovered) ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);

	c_vec4 icon_color = state->icon;
	icon_color.w *= state->alpha;
	draw_tab_icon(window->DrawList, c_rect(rect.Min, rect.Min + s_(icon_slot, tab_height)), icon, icon_color, 12.5f, font->get(icon_font, 14));
	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), rect.Min + s_(icon_slot, 0), rect.Max - s_(11, 0),
		draw->get_clr(clr->white, state->alpha), name.data(), 0, 0, { 0, 0.5 });

	return pressed;
}

bool c_widgets::sub_tab_button(std::string name, std::string icon, int tab, float width)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	tab_button_state* state = gui->anim_container<tab_button_state>(id);

	bool is_selected = tab == var->gui.sub_tab_stored;
	c_vec2 name_size = gui->text_size(font->get(inter_semibold, 11), name.data());
	const float sub_tab_height = 32.f;
	const float icon_slot = 32.f;
	const float target_width = width > 0.f ? width : (s_(42.f) + name_size.x);
	if (state->width <= 0.f)
		state->width = target_width;
	gui->easing(state->width, target_width, 18.f, dynamic_easing);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(round(state->width), s_(sub_tab_height));
	c_rect rect = c_rect(pos, pos + size);

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool hovered = false;
	bool pressed = gui->button_behavior(rect, id, &hovered, nullptr);

	if (pressed)
	{
		var->gui.sub_tab_stored = tab;
	}

	if (is_selected)
		g_pill_selected_rect = c_vec4(rect.Min.x, rect.Min.y, rect.Max.x, rect.Max.y);

	gui->easing(state->alpha, (is_selected || hovered) ? 1.f : 0.58f, 18.f, dynamic_easing);
	gui->easing(state->icon, (is_selected || hovered) ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);

	c_vec4 icon_color = state->icon;
	icon_color.w *= state->alpha;
	draw_tab_icon(window->DrawList, c_rect(rect.Min, rect.Min + s_(icon_slot, sub_tab_height)), icon, icon_color, 12.f, font->get(icon_font, 13));
	draw->text_clipped(window->DrawList, font->get(inter_semibold, 11), rect.Min + s_(icon_slot, 0), rect.Max - s_(10, 0),
		draw->get_clr(clr->white, state->alpha), name.data(), 0, 0, { 0, 0.5 });

	return pressed;
}

struct category_button_state
{
	c_vec4 text;
	c_vec4 background;
	c_vec4 button_text;
};

bool c_widgets::category_button(std::string name, std::string description, bool* callback)
{
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return false;

	c_id id = window->GetID(name.data());
	category_button_state* state = gui->anim_container<category_button_state>(id);

	c_vec2 pos = window->DC.CursorPos;
	c_vec2 size = c_vec2(elements->tab_window_width, s_(82));
	c_rect rect = c_rect(pos, pos + size);
	c_rect inner = c_rect(rect.Min + s_(elements->padding), rect.Max - s_(elements->padding));
	c_rect button = c_rect(inner.GetBL() - s_(0, 27), inner.GetBR());

	gui->item_size(rect);
	if (!gui->item_add(rect, id))
		return false;

	bool pressed = gui->button_behavior(button, id, nullptr, nullptr);

	if (pressed)
	{
		*callback = !*callback;
	}

	gui->easing(state->text, *callback ? clr->white.Value : clr->text.Value, 18.f, dynamic_easing);
	gui->easing(state->background, *callback ? clr->accent.Value : clr->widget.Value, 18.f, dynamic_easing);
	gui->easing(state->button_text, *callback ? clr->black.Value : clr->white.Value, 18.f, dynamic_easing);

	draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->child), s_(10));

	draw->text_clipped(window->DrawList, font->get(inter_semibold, 12), inner.Min, inner.Max, draw->get_clr(state->text),
		name.data(), 0, 0, { 0, 0 });
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), inner.Min + s_(0, 16), inner.Max, draw->get_clr(clr->text),
		description.data(), 0, 0, { 0, 0 });

	draw->rect_filled(window->DrawList, button.Min, button.Max, draw->get_clr(state->background), s_(8));
	draw->text_clipped(window->DrawList, font->get(inter_medium, 11), button.Min, button.Max, draw->get_clr(state->button_text),
		*callback ? "Enabled" : "Disabled", 0, 0, {0.5, 0.5});

	return pressed;
}
