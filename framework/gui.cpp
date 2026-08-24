#include "headers/includes.h"

#include <Windows.h>

// https://discord.gg/jxqVWub6Dk
static bool license_key_matches(const char* key)
{
	return std::string(key) == "demo";
}

static void sync_layout_preferences()
{
	elements->padding = var->gui.compact_layout ? c_vec2(8, 8) : c_vec2(10, 10);
	elements->window.rounding = var->gui.window_rounding;
}

static void draw_login_loading(const c_rect& rect)
{
	if (!var->gui.login_loading && var->gui.login_loading_alpha <= 0.f)
		return;

	const float dt = ImGui::GetIO().DeltaTime;
	gui->easing(var->gui.login_loading_alpha, var->gui.login_loading ? 1.f : 0.f, 16.f, dynamic_easing);

	ImDrawList* draw_list = gui->foreground_drawlist();
	const float alpha = var->gui.login_loading_alpha;
	const c_vec2 center = rect.GetCenter();

	draw->rect_filled(draw_list, rect.Min, rect.Max, draw->get_clr(clr->black, 0.68f * alpha), s_(elements->window.rounding));

	static float pulse = 0.f;
	pulse += dt * 7.f;

	const float bar_width = s_(8);
	const float bar_spacing = s_(18);
	const float base_height = s_(20);

	for (int i = 0; i < 3; i++)
	{
		const float wave = (sinf(pulse - i * 0.75f) + 1.f) * 0.5f;
		const float height = base_height + s_(18) * wave;
		const c_vec2 bar_center = center + c_vec2((i - 1) * bar_spacing, s_(-12));
		const c_rect bar_rect(c_vec2(bar_center.x - bar_width * 0.5f, bar_center.y - height * 0.5f), c_vec2(bar_center.x + bar_width * 0.5f, bar_center.y + height * 0.5f));

		draw->rect_filled(draw_list, bar_rect.Min, bar_rect.Max, draw->get_clr(clr->accent, alpha), s_(999));
		draw->rect_filled(draw_list, bar_rect.Min, bar_rect.Max, draw->get_clr(clr->black, 0.22f * (1.f - wave) * alpha), s_(999));
	}

	draw->text_clipped(draw_list, font->get(inter_medium, 11), rect.Min, rect.Max - s_(0, 34),
		draw->get_clr(clr->white, alpha), var->gui.login_loading_description.data(), 0, 0, { 0.5f, 0.78f });

	if (var->gui.login_loading)
	{
		var->gui.login_loading_timer += dt;
		if (var->gui.login_loading_timer >= 1.45f)
		{
			var->gui.login_loading = false;
			var->gui.login_loading_timer = 0.f;
			var->gui.login_loading_alpha = 0.f;
			var->gui.tab = 1;
			var->gui.tab_stored = 1;
			var->gui.tab_alpha = 0.f;
			pulse = 0.f;
		}
	}
}

static constexpr float visual_window_width = 900.f;
static constexpr float visual_window_height = 527.f;
static constexpr float visual_sidebar_width = 162.5f;
static constexpr float visual_outer_padding = 10.f;
static constexpr float visual_column_gap = 14.f;
static constexpr float visual_panel_scale = 1.0f;
static constexpr float visual_section_width_scale = 0.98f;
static constexpr float visual_panel_4_content_offset_x = -2.f;
static constexpr float visual_panel_5_content_offset_x = 2.f;
static constexpr float visual_feature_padding_x = 0.f;
static constexpr float visual_feature_padding_y = 0.f;

struct visual_panel_density_state
{
	float dpi;
	float font_scale;
};

static float unscale_visual(float value)
{
	return value / ImMax(var->gui.dpi, 0.01f);
}

static float visual_section_height(int section_count)
{
	const float content_height = unscale_visual(gui->content_avail().y);
	const float gaps = visual_column_gap * static_cast<float>(ImMax(section_count - 1, 0));
	return ImMax(220.f, (content_height - gaps) / static_cast<float>(ImMax(section_count, 1)));
}

extern c_vec4 g_pill_selected_rect;
extern c_vec4 g_sidebar_selected_rect;

static int g_panel_number = 0;

static char visual_search_lower(char value)
{
	return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
}

static std::string visual_search_query()
{
	std::string query = var->gui.feature_search;
	size_t first = 0;
	while (first < query.size() && (query[first] == ' ' || query[first] == '\t'))
		first++;

	size_t last = query.size();
	while (last > first && (query[last - 1] == ' ' || query[last - 1] == '\t'))
		last--;

	return query.substr(first, last - first);
}

static bool visual_search_has_query()
{
	return !visual_search_query().empty();
}

static bool visual_contains_ci(std::string_view text, std::string_view query)
{
	if (query.empty())
		return true;

	if (query.size() > text.size())
		return false;

	for (size_t i = 0; i <= text.size() - query.size(); i++)
	{
		size_t j = 0;
		while (j < query.size() && visual_search_lower(text[i + j]) == visual_search_lower(query[j]))
			j++;

		if (j == query.size())
			return true;
	}

	return false;
}

static bool visual_search_matches(std::string_view name, std::string_view description = {})
{
	const std::string query = visual_search_query();
	if (query.empty())
		return true;

	return visual_contains_ci(name, query) || visual_contains_ci(description, query);
}

static bool visual_search_matches_any(std::string_view name, std::string_view description, const std::vector<std::string>& values)
{
	if (visual_search_matches(name, description))
		return true;

	const std::string query = visual_search_query();
	for (const std::string& value : values)
		if (visual_contains_ci(value, query))
			return true;

	return false;
}

struct visual_widget_filter
{
	bool checkbox(std::string name, std::string description, bool* callback, title_status_icon status = title_status_none) const
	{
		if (!visual_search_matches(name, description))
			return false;

		return ::widgets->checkbox(name, description, callback, status);
	}

	bool keybind(std::string name, std::string description, keybind_t* bind) const
	{
		if (!visual_search_matches(name, description))
			return false;

		return ::widgets->keybind(name, description, bind);
	}

	bool dropdown(std::string name, std::string description, int* callback, const std::vector<std::string>& variants) const
	{
		if (!visual_search_matches_any(name, description, variants))
			return false;

		return ::widgets->dropdown(name, description, callback, variants);
	}

	bool slider(std::string name, std::string description, float* callback, float vmin, float vmax, std::string format = "%.1f") const
	{
		if (!visual_search_matches(name, description))
			return false;

		return ::widgets->slider(name, description, callback, vmin, vmax, format);
	}

	bool color_picker(std::string name, c_vec4* color) const
	{
		if (!visual_search_matches(name, "color picker"))
			return false;

		return ::widgets->color_picker(name, color);
	}

	bool primary_button(std::string name, std::string icon = "") const
	{
		if (!visual_search_matches(name, icon))
			return false;

		return ::widgets->primary_button(name, icon);
	}

	bool action_button(std::string name, std::string icon = "") const
	{
		if (!visual_search_matches(name, icon))
			return false;

		return ::widgets->action_button(name, icon);
	}

	bool card_button(std::string name, std::string description, std::string button_text = "Run") const
	{
		if (!visual_search_matches(name, description))
			return false;

		return ::widgets->card_button(name, description, button_text);
	}
};

static void begin_visual_section(std::string_view name, float height)
{
	const int panel_number = g_panel_number + 1;
	const float section_width = elements->child_width * visual_section_width_scale;
	const float content_offset_x = panel_number == 4 ? visual_panel_4_content_offset_x : panel_number == 5 ? visual_panel_5_content_offset_x : 0.f;
	const float content_padding_x = ImMax(0.f, section_width * 0.05f + content_offset_x);
	gui->begin_content(name, c_vec2(section_width, s_(height)), c_vec2(content_padding_x, s_(5)), s_(0, 0), window_flags_no_move | window_flags_no_scrollbar, child_flags_none);
	c_window* window = gui->get_window();
	draw->rect_filled(window->DrawList, window->Rect().Min, window->Rect().Max, draw->get_clr(clr->child), s_(14));

	g_panel_number = panel_number;
}

static void end_visual_section()
{
	gui->end_content();
}

struct visual_details_anim
{
	float amount;
	float cached_height;
};

struct visual_details_frame
{
	bool* trigger;
	float start_y;
	bool clipped;
	bool sentinel;
};

static std::unordered_map<bool*, visual_details_anim> g_details_states;
static std::vector<visual_details_frame> g_details_stack;

static c_vec2 begin_visual_details(bool* trigger = nullptr)
{
	float amount = 1.f;
	float cached_height = 0.f;
	const bool search_active = visual_search_has_query();

	if (trigger)
	{
		visual_details_anim& s = g_details_states[trigger];
		if (search_active)
			s.amount = 1.f;
		else
			gui->easing(s.amount, *trigger ? 1.f : 0.f, 12.f, dynamic_easing);

		if (s.amount <= 0.001f)
		{
			return c_vec2(-1.f, -1.f);
		}
		amount = s.amount;
		cached_height = s.cached_height;
	}

	c_window* window = gui->get_window();
	const c_vec2 start_pos = window->DC.CursorPos;

	bool clipped = false;
	if (trigger && cached_height > 0.f && !search_active)
	{
		ImGui::PushClipRect(
			ImVec2(window->Rect().Min.x, start_pos.y),
			ImVec2(window->Rect().Max.x, start_pos.y + cached_height * amount),
			true);
		clipped = true;
	}

	g_details_stack.push_back({ trigger, start_pos.y, clipped, false });

	gui->push_var(style_var_alpha, amount);

	const float original_indent = window->DC.Indent.x;
	window->DC.Indent.x += s_(12);
	gui->set_screen_pos(start_pos + s_(12, 0), pos_all);
	return c_vec2(start_pos.x, original_indent);
}

static void end_visual_details(c_vec2 original_state)
{
	if (g_details_stack.empty()) return;

	visual_details_frame frame = g_details_stack.back();
	g_details_stack.pop_back();

	if (frame.sentinel) return;

	c_window* window = gui->get_window();
	window->DC.Indent.x = original_state.y;

	gui->pop_var();

	if (frame.clipped)
		ImGui::PopClipRect();

	const float actual_height = window->DC.CursorPos.y - frame.start_y;
	const bool search_active = visual_search_has_query();
	if (frame.trigger && actual_height > 0.f && !search_active)
		g_details_states[frame.trigger].cached_height = actual_height;

	float new_y;
	if (frame.trigger && search_active)
	{
		new_y = actual_height > 0.f ? window->DC.CursorPos.y + s_(3) : frame.start_y;
	}
	else if (frame.trigger)
	{
		const visual_details_anim& s = g_details_states[frame.trigger];
		new_y = frame.start_y + s.cached_height * s.amount + s_(3);
	}
	else
	{
		new_y = window->DC.CursorPos.y + s_(3);
	}

	gui->set_screen_pos(c_vec2(original_state.x, new_y), pos_all);
}

static visual_panel_density_state begin_visual_panel_density()
{
	c_window* window = gui->get_window();
	visual_panel_density_state state{ var->gui.dpi, window->FontWindowScale };

	var->gui.dpi = ImMax(0.01f, state.dpi * visual_panel_scale);
	ImGui::SetWindowFontScale(state.font_scale * visual_panel_scale);
	return state;
}

static void end_visual_panel_density(visual_panel_density_state state)
{
	ImGui::SetWindowFontScale(state.font_scale);
	var->gui.dpi = state.dpi;
}

static void draw_visual_sidebar_glass(const c_rect& rect)
{
	ImDrawList* draw_list = gui->get_window()->DrawList;
	const float rounding = s_(14);

	draw->rect_filled(draw_list, rect.Min, rect.Max, draw->get_clr(clr->child), rounding);
}

struct visual_player_state
{
	bool labels = true;
	bool name_nation = true;
	bool uid = false;
	bool team_id = true;
	bool team_color = true;
	bool bot_detection = true;
	float label_scale = 100.f;
	float label_x_offset = 0.f;
	float label_y_offset = -6.f;
	float label_spacing = 3.f;
	float label_opacity = 100.f;
	int label_layout = 1;
	c_vec4 name_color = c_col(245, 245, 255).Value;
	c_vec4 nation_color = c_col(135, 185, 255).Value;
	c_vec4 uid_color = c_col(155, 160, 180).Value;
	c_vec4 team_color_value = c_col(100, 180, 255).Value;
	c_vec4 bot_color = c_col(255, 190, 95).Value;

	bool health_bar = true;
	bool health_values = true;
	bool knock_state = true;
	// https://discord.gg/jxqVWub6Dk
	bool kills = true;
	bool distance = true;
	float max_distance = 450.f;
	float health_width = 46.f;
	float health_height = 4.f;
	float health_y_offset = 3.f;
	float distance_y_offset = 18.f;
	int health_style = 2;
	c_vec4 health_full_color = c_col(75, 225, 145).Value;
	c_vec4 health_mid_color = c_col(255, 205, 80).Value;
	c_vec4 low_health_color = c_col(255, 85, 95).Value;
	c_vec4 distance_color = c_col(205, 210, 225).Value;
	c_vec4 knock_color = c_col(255, 120, 95).Value;

	bool weapon_name = true;
	bool weapon_icon = true;
	bool ammo = true;
	bool fire_mode = true;
	bool ads = true;
	bool firing = true;
	int weapon_layout = 1;
	float weapon_icon_size = 100.f;
	float weapon_y_offset = 24.f;
	float ammo_spacing = 4.f;
	float weapon_opacity = 100.f;
	c_vec4 weapon_text_color = c_col(245, 245, 255).Value;
	c_vec4 weapon_icon_color = c_col(176, 180, 255).Value;
	c_vec4 ammo_color = c_col(115, 210, 185).Value;
	c_vec4 fire_mode_color = c_col(255, 205, 80).Value;

	bool pose = true;
	bool parachute = true;
	bool fall_speed = true;
	bool velocity = true;
	bool alive_state = true;
	bool last_known = false;
	bool vehicle_tag = true;
	float movement_scale = 100.f;
	float movement_y_offset = 38.f;
	float velocity_arrow_size = 18.f;
	float tag_opacity = 92.f;
	c_vec4 pose_color = c_col(176, 180, 255).Value;
	c_vec4 parachute_color = c_col(95, 210, 185).Value;
	c_vec4 velocity_color = c_col(255, 205, 80).Value;
	c_vec4 vehicle_tag_color = c_col(255, 155, 105).Value;
};

struct visual_shape_state
{
	bool skeleton = true;
	bool bone_full = true;
	bool bone_thickness_distance = true;
	bool bone_health_color = true;
	bool visible_only = false;
	bool head_circle = true;
	bool predicted_bones = false;
	float bone_thickness = 1.6f;
	float skeleton_alpha = 100.f;
	float head_radius = 6.f;
	float joint_size = 2.4f;
	c_vec4 skeleton_color = c_col(235, 235, 255).Value;
	c_vec4 head_circle_color = c_col(255, 255, 255).Value;
	c_vec4 predicted_bone_color = c_col(176, 180, 255, 120).Value;

	bool boxes = true;
	int box_variant = 2;
	bool filled_box = true;
	bool outline_combo = false;
	bool mesh_bounds = true;
	float box_thickness = 1.4f;
	float box_rounding = 4.f;
	float box_padding = 2.f;
	float fill_alpha = 28.f;
	c_vec4 box_color = c_col(176, 180, 255).Value;
	c_vec4 fill_color = c_col(80, 100, 255, 95).Value;
	c_vec4 outline_color = c_col(0, 0, 0, 180).Value;
	c_vec4 corner_color = c_col(255, 255, 255).Value;

	bool snaplines = true;
	bool bottom_lines = true;
	bool crosshair_lines = false;
	bool local_lines = false;
	bool head_lines = true;
	bool line_stats = true;
	float line_thickness = 1.2f;
	float line_alpha = 90.f;
	float line_origin_offset = 0.f;
	c_vec4 line_color = c_col(255, 255, 255, 190).Value;
	c_vec4 line_shadow_color = c_col(0, 0, 0, 160).Value;
	c_vec4 line_text_color = c_col(245, 245, 255).Value;

	bool visibility_coloring = true;
	bool distance_fade = true;
	bool fov_count = true;
	int sorting = 0;
	float fade_distance = 650.f;
	float visible_outline = 1.2f;
	float fov_counter_size = 12.f;
	c_vec4 visible_color = c_col(255, 85, 95).Value;
	c_vec4 occluded_color = c_col(255, 205, 80).Value;
	c_vec4 fade_color = c_col(110, 110, 129, 130).Value;
	c_vec4 fov_count_color = c_col(115, 210, 185).Value;
};

struct visual_world_state
{
	bool vehicle_name = true;
	bool vehicle_hp = true;
	bool vehicle_fuel = true;
	bool engine = true;
	bool speed = true;
	bool boost = true;
	bool occupants = true;
	bool direction_line = true;
	float vehicle_distance = 700.f;
	float vehicle_label_scale = 100.f;
	float vehicle_line_thickness = 1.4f;
	c_vec4 vehicle_color = c_col(95, 210, 185).Value;
	c_vec4 vehicle_hp_color = c_col(75, 225, 145).Value;
	c_vec4 vehicle_fuel_color = c_col(255, 205, 80).Value;
	c_vec4 vehicle_direction_color = c_col(176, 180, 255).Value;

	bool pickup_list = true;
	bool rarity = true;
	bool loot_distance = true;
	bool throwables = true;
	bool airdrop = true;
	bool filtered_loot = true;
	float loot_distance_limit = 160.f;
	float rarity_glow = 45.f;
	float loot_label_scale = 100.f;
	float loot_icon_size = 14.f;
	int loot_filter = 1;
	c_vec4 crate_color = c_col(255, 195, 75).Value;
	c_vec4 weapon_loot_color = c_col(176, 180, 255).Value;
	c_vec4 med_loot_color = c_col(75, 225, 145).Value;
	c_vec4 throwable_color = c_col(255, 155, 105).Value;
	c_vec4 rare_loot_color = c_col(255, 205, 80).Value;

	bool radar = true;
	bool head_spheres = false;
	bool velocity_lines = true;
	bool aim_spheres = false;
	float radar_radius = 120.f;
	float radar_scale = 60.f;
	float radar_point_size = 5.f;
	float radar_opacity = 82.f;
	float radar_border = 1.2f;
	c_vec4 radar_color = c_col(176, 180, 255).Value;
	c_vec4 radar_background = c_col(18, 18, 22, 185).Value;
	c_vec4 radar_enemy_color = c_col(255, 95, 95).Value;
	c_vec4 radar_team_color = c_col(95, 210, 185).Value;

	bool alive_counts = true;
	bool team_counts = true;
	bool zone_timer = true;
	bool game_time = true;
	bool phase = true;
	bool perspective = true;
	bool local_info = true;
	bool circle_counts = true;
	float overlay_opacity = 85.f;
	float overlay_x_offset = 12.f;
	float overlay_y_offset = 12.f;
	c_vec4 overlay_text_color = c_col(245, 245, 255).Value;
	c_vec4 zone_color = c_col(95, 155, 255).Value;
	c_vec4 local_info_color = c_col(115, 210, 185).Value;
};

struct visual_polish_state
{
	bool stacked_text = true;
	bool multilayer_panel = false;
	bool bone_name_position = true;
	bool dynamic_text = true;
	bool clean_replay = true;
	float text_scale = 100.f;
	float row_spacing = 3.f;
	float text_shadow = 70.f;
	float text_outline = 1.2f;
	c_vec4 text_color = c_col(245, 245, 255).Value;
	c_vec4 text_shadow_color = c_col(0, 0, 0, 185).Value;
	c_vec4 panel_background = c_col(18, 18, 22, 160).Value;

	bool health_above = true;
	bool health_inside = false;
	bool segmented_health = true;
	bool gradient_health = true;
	float bar_width = 42.f;
	float bar_height = 4.f;
	float bar_rounding = 3.f;
	float segment_gap = 2.f;
	c_vec4 health_full = c_col(75, 225, 145).Value;
	c_vec4 health_empty = c_col(35, 36, 40).Value;
	c_vec4 health_damage = c_col(255, 85, 95).Value;

	bool threat_count = true;
	bool closest_first = true;
	bool vehicle_combo = true;
	bool spectator_clean = true;
	int priority_mode = 1;
	float threat_emphasis = 55.f;
	float panel_opacity = 78.f;
	float priority_glow = 12.f;
	float priority_border = 1.4f;
	c_vec4 priority_color = c_col(255, 95, 95).Value;
	c_vec4 priority_text_color = c_col(245, 245, 255).Value;
	c_vec4 priority_panel_color = c_col(35, 36, 40, 190).Value;

	bool loot_glow = true;
	bool rarity_border = false;
	bool parachute_line = true;
	bool fall_prediction = true;
	bool team_markers = true;
	float glow_size = 12.f;
	float marker_size = 6.f;
	float trajectory_thickness = 1.5f;
	float marker_opacity = 88.f;
	c_vec4 polish_accent = c_col(176, 180, 255).Value;
	c_vec4 loot_glow_color = c_col(255, 205, 80, 145).Value;
	c_vec4 trajectory_color = c_col(95, 210, 185).Value;
	c_vec4 team_marker_color = c_col(115, 180, 255).Value;
};

struct keybind_settings_state
{
	bool player_labels_details_open = true;
	bool player_labels_keybind_enabled = false;
	keybind_t player_labels{ ImGuiKey_F1, false, false, false, false, keybind_mode_toggle };
};

static visual_player_state visual_player;
static visual_shape_state visual_shape;
static visual_world_state visual_world;
static visual_polish_state visual_polish;
static keybind_settings_state keybind_settings;

static void set_gui_scale_percent(float scale)
{
	const int next_dpi = static_cast<int>(ImClamp(scale, 100.f, 200.f) + 0.5f);
	if (next_dpi == var->gui.stored_dpi)
		return;

	var->gui.stored_dpi = next_dpi;
	var->gui.dpi_changed = true;
}

static void handle_gui_scale_wheel()
{
	ImGuiIO& io = ImGui::GetIO();
	if (!io.KeyCtrl || io.MouseWheel == 0.f)
		return;

	set_gui_scale_percent(static_cast<float>(var->gui.stored_dpi) + io.MouseWheel * 5.f);
}

static bool keybind_modifiers_match(const keybind_t& bind)
{
	const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	const bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
	const bool super = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
	const ImGuiKey key = static_cast<ImGuiKey>(bind.key);

	const bool ctrl_matches = key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl || bind.ctrl == ctrl;
	const bool shift_matches = key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift || bind.shift == shift;
	const bool alt_matches = key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt || bind.alt == alt;
	const bool super_matches = key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper || bind.super == super;

	return ctrl_matches && shift_matches && alt_matches && super_matches;
}

static int keybind_virtual_key(ImGuiKey key)
{
	if (key >= ImGuiKey_0 && key <= ImGuiKey_9)
		return '0' + static_cast<int>(key - ImGuiKey_0);
	if (key >= ImGuiKey_A && key <= ImGuiKey_Z)
		return 'A' + static_cast<int>(key - ImGuiKey_A);
	if (key >= ImGuiKey_F1 && key <= ImGuiKey_F24)
		return VK_F1 + static_cast<int>(key - ImGuiKey_F1);
	if (key >= ImGuiKey_Keypad0 && key <= ImGuiKey_Keypad9)
		return VK_NUMPAD0 + static_cast<int>(key - ImGuiKey_Keypad0);

	switch (key)
	{
	case ImGuiKey_Tab: return VK_TAB;
	case ImGuiKey_LeftArrow: return VK_LEFT;
	case ImGuiKey_RightArrow: return VK_RIGHT;
	case ImGuiKey_UpArrow: return VK_UP;
	case ImGuiKey_DownArrow: return VK_DOWN;
	case ImGuiKey_PageUp: return VK_PRIOR;
	case ImGuiKey_PageDown: return VK_NEXT;
	case ImGuiKey_Home: return VK_HOME;
	case ImGuiKey_End: return VK_END;
	case ImGuiKey_Insert: return VK_INSERT;
	case ImGuiKey_Delete: return VK_DELETE;
	case ImGuiKey_Backspace: return VK_BACK;
	case ImGuiKey_Space: return VK_SPACE;
	case ImGuiKey_Enter: return VK_RETURN;
	case ImGuiKey_Escape: return VK_ESCAPE;
	case ImGuiKey_LeftCtrl: return VK_LCONTROL;
	case ImGuiKey_LeftShift: return VK_LSHIFT;
	case ImGuiKey_LeftAlt: return VK_LMENU;
	case ImGuiKey_LeftSuper: return VK_LWIN;
	case ImGuiKey_RightCtrl: return VK_RCONTROL;
	case ImGuiKey_RightShift: return VK_RSHIFT;
	case ImGuiKey_RightAlt: return VK_RMENU;
	case ImGuiKey_RightSuper: return VK_RWIN;
	case ImGuiKey_Menu: return VK_APPS;
	case ImGuiKey_Apostrophe: return VK_OEM_7;
	case ImGuiKey_Comma: return VK_OEM_COMMA;
	case ImGuiKey_Minus: return VK_OEM_MINUS;
	case ImGuiKey_Period: return VK_OEM_PERIOD;
	case ImGuiKey_Slash: return VK_OEM_2;
	case ImGuiKey_Semicolon: return VK_OEM_1;
	case ImGuiKey_Equal: return VK_OEM_PLUS;
	case ImGuiKey_LeftBracket: return VK_OEM_4;
	case ImGuiKey_Backslash: return VK_OEM_5;
	case ImGuiKey_RightBracket: return VK_OEM_6;
	case ImGuiKey_GraveAccent: return VK_OEM_3;
	case ImGuiKey_CapsLock: return VK_CAPITAL;
	case ImGuiKey_ScrollLock: return VK_SCROLL;
	case ImGuiKey_NumLock: return VK_NUMLOCK;
	case ImGuiKey_PrintScreen: return VK_SNAPSHOT;
	case ImGuiKey_Pause: return VK_PAUSE;
	case ImGuiKey_KeypadDecimal: return VK_DECIMAL;
	case ImGuiKey_KeypadDivide: return VK_DIVIDE;
	case ImGuiKey_KeypadMultiply: return VK_MULTIPLY;
	case ImGuiKey_KeypadSubtract: return VK_SUBTRACT;
	case ImGuiKey_KeypadAdd: return VK_ADD;
	case ImGuiKey_KeypadEnter: return VK_RETURN;
	case ImGuiKey_MouseLeft: return VK_LBUTTON;
	case ImGuiKey_MouseRight: return VK_RBUTTON;
	case ImGuiKey_MouseMiddle: return VK_MBUTTON;
	case ImGuiKey_MouseX1: return VK_XBUTTON1;
	case ImGuiKey_MouseX2: return VK_XBUTTON2;
	default: return 0;
	}
}

static bool keybind_pressed(const keybind_t& bind)
{
	if (bind.key == ImGuiKey_None || bind.capturing || !keybind_modifiers_match(bind))
		return false;

	const int virtual_key = keybind_virtual_key(static_cast<ImGuiKey>(bind.key));
	if (virtual_key == 0)
		return false;

	const int state_key = bind.key | (bind.ctrl ? 1 << 20 : 0) | (bind.shift ? 1 << 21 : 0) | (bind.alt ? 1 << 22 : 0) | (bind.super ? 1 << 23 : 0);
	static std::unordered_map<int, bool> previous_key_down;
	const bool down = (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
	const bool pressed = down && !previous_key_down[state_key];
	previous_key_down[state_key] = down;
	return pressed;
}

static void handle_feature_keybinds()
{
	if (!keybind_settings.player_labels_keybind_enabled)
		return;

	if (keybind_pressed(keybind_settings.player_labels))
	{
		visual_player.labels = !visual_player.labels;
		notify->add_notify(visual_player.labels ? "Function enabled" : "Function disabled", "Player labels", visual_player.labels ? success : warning);
	}
}

static void render_gui_scale_slider()
{
	float gui_scale = static_cast<float>(var->gui.stored_dpi);
	const visual_widget_filter visual_widgets;

	if (visual_widgets.slider("GUI scale", "Menu size multiplier", &gui_scale, 100.f, 200.f, "%.0f%%"))
		set_gui_scale_percent(gui_scale);
}

static void render_profile_button_showcase()
{
	const visual_widget_filter visual_widgets;

	if (visual_widgets.primary_button("Activate profile", "active"))
		notify->add_notify("Profile activated", "Active profile", success);
	if (visual_widgets.action_button("Copy profile ID", "copy"))
		notify->add_notify("Copied to clipboard", "Profile ID", success);
	if (visual_widgets.action_button("Reset profile", "reset"))
		notify->add_notify("Profile reset queued", "Profile controls", warning);
	if (visual_widgets.card_button("Tournament preset", "Apply tuned labels, weapon row, and radar defaults", "Apply"))
		notify->add_notify("Preset applied", "Tournament profile", success);
}

void c_gui::render()
{
	gui->initialize();
	handle_gui_scale_wheel();
	handle_feature_keybinds();
	sync_layout_preferences();

	gui->easing(var->gui.tab_alpha, var->gui.tab != var->gui.tab_stored ? 0.f : 1.f, 7.f, static_easing);
	if (var->gui.tab_alpha == 0)
		var->gui.tab = var->gui.tab_stored;

	c_vec2 size = c_vec2(0, 0);

	if (var->gui.tab_stored == 0)
	{
		size.x = s_(380);
		size.y = s_(236);
	}
	else
	{
		size.x = s_(visual_window_width);
		size.y = s_(visual_window_height);
	}

	gui->easing(elements->window.size.x, size.x, 24.f, dynamic_easing);
	gui->easing(elements->window.size.y, size.y, 24.f, dynamic_easing);

	gui->set_next_window_size(elements->window.size);
	gui->set_next_window_pos(c_vec2(0, 0));

	gui->begin(elements->window.name, nullptr, window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_bring_to_front_on_focus | window_flags_no_focus_on_appearing | window_flags_no_background | window_flags_no_decoration | window_flags_no_move);
	{
		gui->set_style();
		gui->draw_decorations();

		if (var->gui.tab == 0)
		{
			const c_vec4 accent_backup = clr->accent.Value;
			static c_vec4 login_accent = accent_backup;
			const c_vec4 fail_accent = c_col(250, 75, 85).Value;
			gui->easing(login_accent, var->gui.license_invalid ? fail_accent : accent_backup, 18.f, dynamic_easing);
			clr->accent.Value = login_accent;

			if (var->gui.license_invalid)
			{
				var->gui.license_invalid_timer += ImGui::GetIO().DeltaTime;
				if (var->gui.license_invalid_timer >= 0.8f)
				{
					var->gui.license_invalid = false;
					var->gui.license_invalid_timer = 0.f;
				}
			}

			gui->set_pos(c_vec2(0, 0), pos_all);
			gui->begin_content("Login", size, s_(14, 14), s_(0, 0), 0, child_flags_none);
			{
				c_window* login_window = gui->get_window();
				const c_rect login_rect = login_window->Rect();
				draw->rect_filled(login_window->DrawList, login_rect.Min, login_rect.Max, draw->get_clr(clr->child), s_(12));

				widgets->brand_header("Lumin", "License access");

				gui->dummy(s_(0, 14));

				static char license_key[96];
				if (widgets->text_field("License key", license_key, sizeof(license_key), false, "key"))
				{
					var->gui.license_invalid = false;
					var->gui.license_invalid_timer = 0.f;
					var->gui.license_error_message.clear();
				}

				if (!var->gui.license_error_message.empty())
				{
					c_vec2 error_pos = login_window->DC.CursorPos;
					c_vec2 error_size = c_vec2(gui->content_avail().x, s_(22));
					c_rect error_rect(error_pos, error_pos + error_size);
					gui->dummy(error_size);

					const c_vec2 key_center = error_rect.Min + s_(8, 11);
					const ImU32 key_color = draw->get_clr(clr->accent);
					draw->circle(login_window->DrawList, key_center + s_(-3, 0), s_(3), key_color, s_(24), s_(1.3f));
					draw->line(login_window->DrawList, key_center, key_center + s_(8, 0), key_color, s_(1.3f));
					draw->line(login_window->DrawList, key_center + s_(5, 0), key_center + s_(5, 3), key_color, s_(1.3f));
					draw->text_clipped(login_window->DrawList, font->get(inter_medium, 11), error_rect.Min + s_(22, 0), error_rect.Max,
						draw->get_clr(clr->text), var->gui.license_error_message.data(), 0, 0, { 0, 0.5f });
				}
				else
				{
					gui->dummy(s_(0, 10));
				}

				const c_vec2 button_pos = gui->get_pos();
				const float shake = var->gui.license_invalid ? sinf(var->gui.license_invalid_timer * 48.f) * s_(5.f) * (1.f - ImSaturate(var->gui.license_invalid_timer / 0.8f)) : 0.f;
				gui->set_pos(button_pos + c_vec2(shake, 0), pos_all);

				// https://discord.gg/jxqVWub6Dk
				if (!var->gui.login_loading && widgets->primary_button("Activate", "active"))
				{
					if (license_key[0] == '\0')
					{
						var->gui.license_invalid = true;
						var->gui.license_invalid_timer = 0.f;
						var->gui.license_error_message = "License key required";
					}
					else if (var->gui.strict_license && !license_key_matches(license_key))
					{
						var->gui.license_invalid = true;
						var->gui.license_invalid_timer = 0.f;
						var->gui.license_error_message = "Invalid license key";
					}
					else
					{
						var->gui.license_invalid = false;
						var->gui.license_error_message.clear();

						if (var->gui.verification_animation)
						{
							var->gui.login_loading = true;
							var->gui.login_loading_timer = 0.f;
							var->gui.login_loading_description = "Please wait, account verification";
						}
						else
						{
							var->gui.login_loading = false;
							var->gui.login_loading_timer = 0.f;
							var->gui.login_loading_alpha = 0.f;
							var->gui.tab = 1;
							var->gui.tab_stored = 1;
							var->gui.tab_alpha = 0.f;
						}
					}
				}

				draw_login_loading(login_rect);
			}
			gui->end_content();
			clr->accent.Value = accent_backup;
			gui->end();
			return;
		}

		const float top_row_y = visual_outer_padding;
		const float top_row_height = 50.f;
		const float top_row_gap = 2.f;
		const float bottom_row_y = top_row_y + top_row_height + top_row_gap;
		const float bottom_row_height = visual_window_height - bottom_row_y - visual_outer_padding;
		const float sidebar_tabs_y = top_row_y + top_row_height;
		const float sidebar_tabs_height = visual_window_height - sidebar_tabs_y - visual_outer_padding;

		draw_visual_sidebar_glass(c_rect(c_vec2(s_(visual_outer_padding), s_(top_row_y)), c_vec2(s_(visual_outer_padding + visual_sidebar_width), s_(visual_window_height - visual_outer_padding))));

		gui->set_pos(c_vec2(s_(visual_outer_padding), s_(top_row_y)), pos_all);
		gui->begin_content("TopBrand", c_vec2(s_(visual_sidebar_width), s_(top_row_height)), s_(0, 0), s_(0, 0), window_flags_no_scrollbar | window_flags_no_background, child_flags_none);
		{
			widgets->brand_header("Lumin", var->gui.profile_name[0] != '\0' ? var->gui.profile_name : "Control panel");
		}
		gui->end_content();

		gui->set_pos(c_vec2(s_(visual_outer_padding), s_(sidebar_tabs_y)), pos_all);
		gui->begin_content("Tabs", c_vec2(s_(visual_sidebar_width), s_(sidebar_tabs_height)), s_(10, 10), s_(0, 4), window_flags_no_scrollbar | window_flags_no_background, child_flags_none);
		{
			var->gui.sidebar_glass = true;

			{
				c_window* tabs_inner_bg = gui->get_window();
				static c_vec4 sidebar_overlay = c_vec4(0, 0, 0, 0);
				gui->easing(sidebar_overlay, g_sidebar_selected_rect, 18.f, dynamic_easing);
				if (sidebar_overlay.z > sidebar_overlay.x + 1.f)
				{
					draw->rect_filled(tabs_inner_bg->DrawList,
						c_vec2(sidebar_overlay.x, sidebar_overlay.y), c_vec2(sidebar_overlay.z, sidebar_overlay.w),
						draw->get_clr(clr->widget), s_(9.1f));
				}
			}

			widgets->tab_button("Player", "player", 1);
			widgets->tab_button("Shapes", "shapes", 2);
			widgets->tab_button("World", "world", 3);
			widgets->tab_button("Polish", "polish", 4);
			widgets->tab_button("Combat", "combat", 5);
			widgets->tab_button("Aim", "aim", 6);
			widgets->tab_button("Loot", "loot", 7);
			widgets->tab_button("Movement", "movement", 8);
			widgets->tab_button("Visuals", "visuals", 9);
			widgets->tab_button("HUD", "hud", 10);

			{
				static c_vec4 sidebar_indicator = c_vec4(0, 0, 0, 0);
				gui->easing(sidebar_indicator, g_sidebar_selected_rect, 18.f, dynamic_easing);
				if (sidebar_indicator.w > sidebar_indicator.y + 1.f)
				{
					c_window* tabs_inner = gui->get_window();
					const float bar_w = s_(2);
					const float bar_inset_y = s_(6);
					const float bar_x = sidebar_indicator.x + s_(2);
					const c_vec2 bar_min = c_vec2(bar_x, sidebar_indicator.y + bar_inset_y);
					const c_vec2 bar_max = c_vec2(bar_x + bar_w, sidebar_indicator.w - bar_inset_y);
					draw->rect_filled(tabs_inner->DrawList, bar_min, bar_max,
						draw->get_clr(clr->accent), bar_w * 0.5f);
				}
			}

			gui->easing(elements->tab_window_width, gui->get_window()->Size.x, 24.f, dynamic_easing);
			var->gui.sidebar_glass = false;
		}
		gui->end_content();
		
		gui->push_var(style_var_alpha, var->gui.tab_alpha);

		if (var->gui.tab != 0)
		{
			const float feature_x = visual_outer_padding + visual_sidebar_width + visual_outer_padding;
			const float feature_width = visual_window_width - feature_x - visual_outer_padding;
			const float feature_header_height = top_row_height * 0.8f;
			const float feature_header_y = top_row_y;

			gui->set_pos(c_vec2(s_(feature_x), s_(feature_header_y) + s_(10) * (1.f - var->gui.tab_alpha)), pos_all);
			{
				const c_vec2 pill_pos = gui->get_window()->DC.CursorPos;
				draw->rect_filled(gui->get_window()->DrawList, pill_pos, pill_pos + c_vec2(s_(feature_width), s_(feature_header_height)), draw->get_clr(clr->child), s_(14));

				gui->begin_content("FeatureHeader", c_vec2(s_(feature_width), s_(feature_header_height)), s_(8, 4), s_(8, 0), window_flags_no_scrollbar | window_flags_no_background, child_flags_none);
				{
					c_window* hdr_win = gui->get_window();

					static c_vec4 pill_overlay = c_vec4(0, 0, 0, 0);
					gui->easing(pill_overlay, g_pill_selected_rect, 18.f, dynamic_easing);
					if (pill_overlay.z > pill_overlay.x + 1.f)
					{
						draw->rect_filled(hdr_win->DrawList,
							c_vec2(pill_overlay.x, pill_overlay.y), c_vec2(pill_overlay.z, pill_overlay.w),
						draw->get_clr(clr->widget), s_(9.1f));
					}

					const c_vec2 row_pos = hdr_win->DC.CursorPos;
					const float search_width = s_(178.f);
					widgets->search_field("Search", var->gui.feature_search, IM_ARRAYSIZE(var->gui.feature_search), c_vec2(search_width, s_(32.f)));

					ImFont* tab_font = font->get(inter_semibold, 11);
					const float tab_gap = s_(8.f);
					const float profile_width = s_(42.f) + gui->text_size(tab_font, "Profile").x;
					const float loadout_width = s_(42.f) + gui->text_size(tab_font, "Loadout").x;
					const float match_width = s_(42.f) + gui->text_size(tab_font, "Match").x;
					const float squad_width = s_(42.f) + gui->text_size(tab_font, "Squad").x;
					const float stats_width = s_(42.f) + gui->text_size(tab_font, "Stats").x;
					const float tabs_x = row_pos.x + search_width + s_(8.f);
					const float tabs_y = row_pos.y;

					gui->set_screen_pos(c_vec2(tabs_x, tabs_y), pos_all);
					widgets->sub_tab_button("Profile", "profile", 1, profile_width);
					gui->sameline(0.f, tab_gap);
					widgets->sub_tab_button("Loadout", "loadout", 2, loadout_width);
					gui->sameline(0.f, tab_gap);
					widgets->sub_tab_button("Match", "match", 3, match_width);
					gui->sameline(0.f, tab_gap);
					widgets->sub_tab_button("Squad", "squad", 4, squad_width);
					gui->sameline(0.f, tab_gap);
					widgets->sub_tab_button("Stats", "stats", 5, stats_width);
				}
				gui->end_content();
			}

			gui->set_pos(c_vec2(s_(feature_x), s_(bottom_row_y) + s_(10) * (1.f - var->gui.tab_alpha)), pos_all);
			gui->begin_content("Features", c_vec2(s_(feature_width), s_(bottom_row_height)), s_(visual_feature_padding_x, visual_feature_padding_y), c_vec2(s_(visual_column_gap), 0.f), window_flags_no_scrollbar);
			{
				const visual_panel_density_state density_state = begin_visual_panel_density();

				g_panel_number = 3;

				gui->easing(elements->child_width, (gui->content_avail().x - s_(visual_column_gap)) / 2, 24.f, dynamic_easing);
				const float half_section_height = visual_section_height(1);
				const visual_widget_filter visual_widgets;

#define widgets (&visual_widgets)

				if (var->gui.tab == 1)
				{
					gui->begin_group();
					{
						{
							begin_visual_section("Player identity", half_section_height);
							render_gui_scale_slider();
							if (var->gui.sub_tab_stored == 1)
								render_profile_button_showcase();
							if (widgets->checkbox("Player labels", "Master visual label group", &visual_player.labels, title_status_safe))
								keybind_settings.player_labels_details_open = visual_player.labels;
							if (const c_vec2 details_pos = begin_visual_details(&keybind_settings.player_labels_details_open); details_pos.x >= 0.f)
							{
								widgets->checkbox("Use keybind", "Allow a key to toggle this group", &keybind_settings.player_labels_keybind_enabled);
								if (const c_vec2 bind_details = begin_visual_details(&keybind_settings.player_labels_keybind_enabled); bind_details.x >= 0.f)
								{
									widgets->keybind("Toggle key", "Turns player labels on or off", &keybind_settings.player_labels);
									end_visual_details(bind_details);
								}
								widgets->dropdown("Label layout", "Stacking style", &visual_player.label_layout, { "Compact", "Stacked", "Detailed", "Tournament" });
								widgets->slider("Label scale", "Text size multiplier", &visual_player.label_scale, 70.f, 150.f, "%.0f%%");
								widgets->slider("Label X offset", "Horizontal label position", &visual_player.label_x_offset, -80.f, 80.f, "%.0fpx");
								widgets->slider("Label Y offset", "Vertical label position", &visual_player.label_y_offset, -80.f, 80.f, "%.0fpx");
								widgets->slider("Label spacing", "Gap between label rows", &visual_player.label_spacing, 0.f, 12.f, "%.1fpx");
								widgets->slider("Label opacity", "Overall label alpha", &visual_player.label_opacity, 0.f, 100.f, "%.0f%%");
								widgets->color_picker("Name color", &visual_player.name_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Name + nation", "Name, nation, and player region", &visual_player.name_nation, title_status_warning);
							if (const c_vec2 details_pos = begin_visual_details(&visual_player.name_nation); details_pos.x >= 0.f)
							{
								widgets->color_picker("Nation color", &visual_player.nation_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Player UID", "Optional identifier text", &visual_player.uid, title_status_danger);
							if (const c_vec2 details_pos = begin_visual_details(&visual_player.uid); details_pos.x >= 0.f)
							{
								widgets->color_picker("UID color", &visual_player.uid_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Team ID", "Team number display", &visual_player.team_id);
							widgets->checkbox("Team color coding", "Color labels by team", &visual_player.team_color);
							if (const c_vec2 details_pos = begin_visual_details(&visual_player.team_color); details_pos.x >= 0.f)
							{
								widgets->color_picker("Team color", &visual_player.team_color_value);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Bot detection", "Real player vs bot badge", &visual_player.bot_detection);
							if (const c_vec2 details_pos = begin_visual_details(&visual_player.bot_detection); details_pos.x >= 0.f)
							{
								widgets->color_picker("Bot badge color", &visual_player.bot_color);
								end_visual_details(details_pos);
							}
							end_visual_section();
						}

					}
					gui->end_group();

					gui->sameline();

					gui->begin_group();
					{
						{
							begin_visual_section("Weapon row", half_section_height);
							widgets->checkbox("Weapon name", "Current weapon display", &visual_player.weapon_name, title_status_safe);
							if (const c_vec2 details_pos = begin_visual_details(&visual_player.weapon_name); details_pos.x >= 0.f)
							{
								widgets->dropdown("Weapon layout", "Row composition", &visual_player.weapon_layout, { "Text only", "Icon + text", "Ammo focused", "Full row" });
								widgets->slider("Weapon Y offset", "Weapon row anchor offset", &visual_player.weapon_y_offset, -20.f, 80.f, "%.0fpx");
								widgets->slider("Weapon opacity", "Weapon row alpha", &visual_player.weapon_opacity, 0.f, 100.f, "%.0f%%");
								widgets->color_picker("Weapon text color", &visual_player.weapon_text_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Weapon icon", "Small icon beside name", &visual_player.weapon_icon, title_status_warning);
							if (const c_vec2 details_pos = begin_visual_details(&visual_player.weapon_icon); details_pos.x >= 0.f)
							{
								widgets->slider("Icon size", "Weapon icon scaling", &visual_player.weapon_icon_size, 70.f, 140.f, "%.0f%%");
								widgets->color_picker("Weapon icon color", &visual_player.weapon_icon_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Ammo clip/reserve", "Clip and reserve numbers", &visual_player.ammo, title_status_danger);
							if (const c_vec2 details_pos = begin_visual_details(&visual_player.ammo); details_pos.x >= 0.f)
							{
								widgets->slider("Ammo spacing", "Gap around ammo text", &visual_player.ammo_spacing, 0.f, 14.f, "%.1fpx");
								widgets->color_picker("Ammo color", &visual_player.ammo_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Fire mode", "Single, auto, burst tag", &visual_player.fire_mode);
							if (const c_vec2 details_pos = begin_visual_details(&visual_player.fire_mode); details_pos.x >= 0.f)
							{
								widgets->color_picker("Fire mode color", &visual_player.fire_mode_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("ADS indicator", "Scoped/aim state badge", &visual_player.ads);
							widgets->checkbox("Firing indicator", "Active weapon fire state", &visual_player.firing);
							end_visual_section();
						}

					}
					gui->end_group();

				}

				if (var->gui.tab == 2)
				{
					gui->begin_group();
					{
						{
							begin_visual_section("Skeleton ESP", half_section_height);
							widgets->checkbox("Skeleton master", "Full bone visual group", &visual_shape.skeleton);
							if (const c_vec2 details_pos = begin_visual_details(&visual_shape.skeleton); details_pos.x >= 0.f)
							{
								widgets->slider("Bone thickness", "Skeleton stroke width", &visual_shape.bone_thickness, 0.5f, 5.f, "%.1fpx");
								widgets->slider("Skeleton alpha", "Skeleton opacity", &visual_shape.skeleton_alpha, 0.f, 100.f, "%.0f%%");
								widgets->slider("Joint size", "Bone joint dot size", &visual_shape.joint_size, 0.f, 8.f, "%.1fpx");
								widgets->color_picker("Skeleton color", &visual_shape.skeleton_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Full bone skeleton", "Head, spine, limbs, hands, feet", &visual_shape.bone_full);
							widgets->checkbox("Distance thickness", "Scale lines by distance", &visual_shape.bone_thickness_distance);
							widgets->checkbox("Health coloring", "Color bones by health", &visual_shape.bone_health_color);
							widgets->checkbox("Visible only", "Velocity-based visibility style", &visual_shape.visible_only);
							widgets->checkbox("Head/neck circle", "Round highlight at upper bones", &visual_shape.head_circle);
							if (const c_vec2 details_pos = begin_visual_details(&visual_shape.head_circle); details_pos.x >= 0.f)
							{
								widgets->slider("Head radius", "Head circle radius", &visual_shape.head_radius, 2.f, 22.f, "%.1fpx");
								widgets->color_picker("Head circle color", &visual_shape.head_circle_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Predicted bones", "Future-position ghost bones", &visual_shape.predicted_bones);
							if (const c_vec2 details_pos = begin_visual_details(&visual_shape.predicted_bones); details_pos.x >= 0.f)
							{
								widgets->color_picker("Predicted bone color", &visual_shape.predicted_bone_color);
								end_visual_details(details_pos);
							}
							end_visual_section();
						}

					}
					gui->end_group();

					gui->sameline();

					gui->begin_group();
					{
						{
							begin_visual_section("Line ESP", half_section_height);
							widgets->checkbox("Snaplines master", "All line visual variants", &visual_shape.snaplines);
							if (const c_vec2 details_pos = begin_visual_details(&visual_shape.snaplines); details_pos.x >= 0.f)
							{
								widgets->slider("Line thickness", "Snapline stroke width", &visual_shape.line_thickness, 0.5f, 4.f, "%.1fpx");
								widgets->slider("Line alpha", "Snapline opacity", &visual_shape.line_alpha, 0.f, 100.f, "%.0f%%");
								widgets->slider("Origin offset", "Line start position offset", &visual_shape.line_origin_offset, -120.f, 120.f, "%.0fpx");
								widgets->color_picker("Line color", &visual_shape.line_color);
								widgets->color_picker("Line shadow color", &visual_shape.line_shadow_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Bottom snaplines", "Screen bottom to target", &visual_shape.bottom_lines);
							widgets->checkbox("Crosshair snaplines", "Center screen to target", &visual_shape.crosshair_lines);
							widgets->checkbox("Local player lines", "Local-to-enemy visual line", &visual_shape.local_lines);
							widgets->checkbox("Head-only lines", "Anchor at head position", &visual_shape.head_lines);
							widgets->checkbox("Line stats", "Distance and health on line", &visual_shape.line_stats);
							if (const c_vec2 details_pos = begin_visual_details(&visual_shape.line_stats); details_pos.x >= 0.f)
							{
								widgets->color_picker("Line text color", &visual_shape.line_text_color);
								end_visual_details(details_pos);
							}
							end_visual_section();
						}

					}
					gui->end_group();
				}

				if (var->gui.tab == 3)
				{
					gui->begin_group();
					{
						{
							begin_visual_section("Vehicle ESP", half_section_height);
							widgets->checkbox("Vehicle name/type", "Vehicle label display", &visual_world.vehicle_name);
							if (const c_vec2 details_pos = begin_visual_details(&visual_world.vehicle_name); details_pos.x >= 0.f)
							{
								widgets->slider("Vehicle distance", "Vehicle visual range", &visual_world.vehicle_distance, 50.f, 1400.f, "%.0fm");
								widgets->slider("Vehicle label scale", "Vehicle text scaling", &visual_world.vehicle_label_scale, 70.f, 150.f, "%.0f%%");
								widgets->color_picker("Vehicle color", &visual_world.vehicle_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Vehicle HP bar", "HP and max HP style bar", &visual_world.vehicle_hp);
							if (const c_vec2 details_pos = begin_visual_details(&visual_world.vehicle_hp); details_pos.x >= 0.f)
							{
								widgets->color_picker("Vehicle HP color", &visual_world.vehicle_hp_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Fuel bar", "Fuel and max fuel style bar", &visual_world.vehicle_fuel);
							if (const c_vec2 details_pos = begin_visual_details(&visual_world.vehicle_fuel); details_pos.x >= 0.f)
							{
								widgets->color_picker("Fuel color", &visual_world.vehicle_fuel_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Engine status", "On/off indicator", &visual_world.engine);
							widgets->checkbox("Current speed", "Forward speed text", &visual_world.speed);
							widgets->checkbox("Boost status", "Boost badge", &visual_world.boost);
							widgets->checkbox("Occupants list", "Driver and passengers list", &visual_world.occupants);
							widgets->checkbox("Direction line", "Velocity direction line", &visual_world.direction_line);
							if (const c_vec2 details_pos = begin_visual_details(&visual_world.direction_line); details_pos.x >= 0.f)
							{
								widgets->slider("Direction thickness", "Vehicle direction stroke width", &visual_world.vehicle_line_thickness, 0.5f, 5.f, "%.1fpx");
								widgets->color_picker("Direction color", &visual_world.vehicle_direction_color);
								end_visual_details(details_pos);
							}
							end_visual_section();
						}

					}
					gui->end_group();

					gui->sameline();

					gui->begin_group();
					{
						{
							begin_visual_section("Radar visuals", half_section_height);
							widgets->checkbox("Radar/minimap", "2D relative position display", &visual_world.radar);
							if (const c_vec2 details_pos = begin_visual_details(&visual_world.radar); details_pos.x >= 0.f)
							{
								widgets->slider("Radar radius", "Radar panel radius", &visual_world.radar_radius, 60.f, 220.f, "%.0fpx");
								widgets->slider("Radar scale", "World-to-radar scale", &visual_world.radar_scale, 20.f, 160.f, "%.0f%%");
								widgets->slider("Point size", "Radar point size", &visual_world.radar_point_size, 2.f, 12.f, "%.1fpx");
								widgets->slider("Radar opacity", "Radar panel transparency", &visual_world.radar_opacity, 0.f, 100.f, "%.0f%%");
								widgets->slider("Radar border", "Radar ring thickness", &visual_world.radar_border, 0.f, 5.f, "%.1fpx");
								widgets->color_picker("Radar color", &visual_world.radar_color);
								widgets->color_picker("Radar background", &visual_world.radar_background);
								widgets->color_picker("Radar enemy color", &visual_world.radar_enemy_color);
								widgets->color_picker("Radar team color", &visual_world.radar_team_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("3D head spheres", "Head position sphere style", &visual_world.head_spheres);
							widgets->checkbox("Velocity lines", "Speed and direction line", &visual_world.velocity_lines);
							widgets->checkbox("Aim point spheres", "Aim-point dot style", &visual_world.aim_spheres);
							end_visual_section();
						}

					}
					gui->end_group();
				}

				if (var->gui.tab == 4)
				{
					gui->begin_group();
					{
						{
							begin_visual_section("Text polish", half_section_height);
							widgets->checkbox("Stacked text", "Name, weapon, ammo stacked", &visual_polish.stacked_text);
							if (const c_vec2 details_pos = begin_visual_details(&visual_polish.stacked_text); details_pos.x >= 0.f)
							{
								widgets->slider("Row spacing", "Stacked text spacing", &visual_polish.row_spacing, 0.f, 10.f, "%.1fpx");
								widgets->slider("Text shadow", "Shadow opacity", &visual_polish.text_shadow, 0.f, 100.f, "%.0f%%");
								widgets->slider("Text outline", "Outline thickness", &visual_polish.text_outline, 0.f, 4.f, "%.1fpx");
								widgets->color_picker("Text color", &visual_polish.text_color);
								widgets->color_picker("Shadow color", &visual_polish.text_shadow_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Extra info panel", "Main box plus text panel", &visual_polish.multilayer_panel);
							if (const c_vec2 details_pos = begin_visual_details(&visual_polish.multilayer_panel); details_pos.x >= 0.f)
							{
								widgets->color_picker("Panel background", &visual_polish.panel_background);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Bone name position", "Name anchored above head bone", &visual_polish.bone_name_position);
							widgets->checkbox("Dynamic text scale", "Scale text by distance", &visual_polish.dynamic_text);
							if (const c_vec2 details_pos = begin_visual_details(&visual_polish.dynamic_text); details_pos.x >= 0.f)
							{
								widgets->slider("Text scale", "Global visual text scale", &visual_polish.text_scale, 70.f, 150.f, "%.0f%%");
								end_visual_details(details_pos);
							}
							widgets->checkbox("Replay clean overlay", "Cleaner spectator/replay display", &visual_polish.clean_replay);
							end_visual_section();
						}

					}
					gui->end_group();

					gui->sameline();

					gui->begin_group();
					{
						{
							begin_visual_section("Priority polish", half_section_height);
							widgets->checkbox("FOV enemy count", "Count enemies in visible area", &visual_polish.threat_count);
							if (const c_vec2 details_pos = begin_visual_details(&visual_polish.threat_count); details_pos.x >= 0.f)
							{
								widgets->slider("Threat emphasis", "Priority highlight strength", &visual_polish.threat_emphasis, 0.f, 100.f, "%.0f%%");
								widgets->slider("Priority glow", "Target priority glow size", &visual_polish.priority_glow, 0.f, 36.f, "%.0fpx");
								widgets->slider("Priority border", "Priority border thickness", &visual_polish.priority_border, 0.f, 5.f, "%.1fpx");
								widgets->color_picker("Priority color", &visual_polish.priority_color);
								widgets->color_picker("Priority text color", &visual_polish.priority_text_color);
								end_visual_details(details_pos);
							}
							widgets->checkbox("Closest first", "Closest visual priority", &visual_polish.closest_first);
							if (const c_vec2 details_pos = begin_visual_details(&visual_polish.closest_first); details_pos.x >= 0.f)
							{
								widgets->dropdown("Priority mode", "Ordering style", &visual_polish.priority_mode, { "Closest", "Threat", "Low health", "Vehicle" });
								end_visual_details(details_pos);
							}
							widgets->checkbox("Vehicle/player combo", "Combined riding labels", &visual_polish.vehicle_combo);
							widgets->checkbox("Spectator clean", "Clean overlay in spectator mode", &visual_polish.spectator_clean);
							if (const c_vec2 details_pos = begin_visual_details(&visual_polish.spectator_clean); details_pos.x >= 0.f)
							{
								widgets->slider("Panel opacity", "Info panel opacity", &visual_polish.panel_opacity, 0.f, 100.f, "%.0f%%");
								widgets->color_picker("Priority panel color", &visual_polish.priority_panel_color);
								end_visual_details(details_pos);
							}
							end_visual_section();
						}

					}
					gui->end_group();
				}

#undef widgets

				end_visual_panel_density(density_state);
			}
			gui->end_content();

		}
		gui->pop_var();

	}
	gui->end();
}
