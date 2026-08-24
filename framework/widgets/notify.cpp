#include "../headers/functions.h"
#include "../headers/widgets.h"

static constexpr float notify_visual_scale = 0.81f;

static float notify_s(float value)
{
    return s_(value * notify_visual_scale);
}

static c_vec2 notify_s(float x, float y)
{
    return s_(x * notify_visual_scale, y * notify_visual_scale);
}

static c_vec4 notify_color(notify_type type)
{
    if (type == error)
        return c_col(245, 70, 86).Value;

    return type == warning ? clr->text.Value : clr->accent.Value;
}

static const char* notify_badge_text(notify_type type)
{
    if (type == warning)
        return "DISABLED";
    if (type == error)
        return "ERROR";

    return "ENABLED";
}

static void draw_notify_check_icon(ImDrawList* draw_list, const c_vec2& center, ImU32 color)
{
    if ((color & IM_COL32_A_MASK) == 0)
        return;

    const c_vec2 points[] =
    {
        center + notify_s(-6.2f, -0.4f),
        center + notify_s(-4.7f, -1.9f),
        center + notify_s(-1.7f, 1.0f),
        center + notify_s(5.5f, -6.3f),
        center + notify_s(7.0f, -4.8f),
        center + notify_s(-1.5f, 4.1f),
    };
    draw_list->AddConcavePolyFilled(points, IM_ARRAYSIZE(points), color);
}

static void draw_notify_minus_icon(ImDrawList* draw_list, const c_vec2& center, ImU32 color)
{
    const float half_width = ImMax(2.0f, notify_s(5.8f));
    const float half_height = ImMax(1.35f, notify_s(1.15f));
    draw->rect_filled(draw_list, center - c_vec2(half_width, half_height), center + c_vec2(half_width, half_height), color, half_height);
}

void c_notify::add_notify(std::string title, std::string text, notify_type type)
{
    if (!notifications.empty())
    {
        notify_state& last = notifications.back();
        if (last.title == title && last.text == text && last.type == type && last.notify_timer < 0.22f)
            return;
    }

    notifications.push_back({ notify_count++, std::move(title), std::move(text), type });
}

void c_notify::setup_notify()
{
    float accumulated_height = notify_padding.y;

    for (int i = 0; i < static_cast<int>(notifications.size()); )
    {
        notify_state& notification = notifications[i];

        if (notification.active_notify)
            notification.notify_timer += ImGui::GetIO().DeltaTime;

        if (notification.notify_timer >= notify_time)
            notification.active_notify = false;

        gui->easing(notification.notify_alpha, notification.active_notify ? 1.f : 0.f, 14.f, dynamic_easing);
        gui->easing(notification.notify_slide, notification.active_notify ? 0.f : 28.f, 18.f, dynamic_easing);
        gui->easing(notification.notify_pos, accumulated_height, 14.f, dynamic_easing);

        if (!notification.active_notify && notification.notify_alpha <= 0.015f)
        {
            notifications.erase(notifications.begin() + i);
            continue;
        }

        if (notification.notify_alpha > 0.01f)
        {
            ImVec2 window_size = render_notify(notification.notify_alpha, notification.notify_timer / notify_time,
                notification.notify_pos, notification.notify_slide, notification.title, notification.text, notification.type);
            accumulated_height += window_size.y + notify_spacing;
        }

        i++;
    }
}

ImVec2 c_notify::render_notify(float notify_alpha, float notify_percentage, float notify_pos, float notify_slide, const std::string& title, const std::string& text, notify_type type)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = gui->foreground_drawlist();

    const c_vec4 status_color = notify_color(type);
    const c_vec4 accent = clr->accent.Value;
    const bool disabled = type == warning;
    const c_vec2 size = notify_s(288, 76);
    const c_vec2 pos = c_vec2(io.DisplaySize.x - notify_padding.x - size.x + notify_s(notify_slide), notify_pos);
    const c_rect rect(pos, pos + size);
    const float alpha = notify_alpha;
    const float rounding = notify_s(12);
    const float progress = ImSaturate(1.f - notify_percentage);

    draw->shadow_rect(draw_list, rect.Min, rect.Max, draw->get_clr(clr->black, 0.30f * alpha), notify_s(20), notify_s(0, 8), 0, rounding);
    draw->rect_filled(draw_list, rect.Min, rect.Max, draw->get_clr(clr->child, 0.98f * alpha), rounding);
    draw->rect_filled_multi_color(draw_list, rect.Min, rect.Max,
        draw->get_clr(clr->white, 0.040f * alpha), draw->get_clr(accent, 0.030f * alpha),
        draw->get_clr(clr->black, 0.035f * alpha), draw->get_clr(clr->black, 0.035f * alpha), rounding);
    draw->rect(draw_list, rect.Min, rect.Max, draw->get_clr(clr->border, 0.96f * alpha), rounding, 0, notify_s(1));

    const c_rect badge(rect.Min + notify_s(13, 15), rect.Min + notify_s(49, 51));
    draw->shadow_rect(draw_list, badge.Min + notify_s(1, 2), badge.Max - notify_s(1, 0), draw->get_clr(status_color, disabled ? 0.08f * alpha : 0.16f * alpha), notify_s(12), c_vec2(0, 0), 0, notify_s(9));
    draw->rect_filled(draw_list, badge.Min, badge.Max, draw->get_clr(clr->widget, 0.94f * alpha), notify_s(9));
    draw->rect_filled_multi_color(draw_list, badge.Min, badge.Max,
        draw->get_clr(clr->white, 0.060f * alpha), draw->get_clr(status_color, disabled ? 0.020f * alpha : 0.070f * alpha),
        draw->get_clr(clr->black, 0.025f * alpha), draw->get_clr(clr->black, 0.025f * alpha), notify_s(9));
    draw->rect(draw_list, badge.Min, badge.Max, draw->get_clr(status_color, disabled ? 0.30f * alpha : 0.44f * alpha), notify_s(9), 0, notify_s(1));

    const c_vec2 mark_center = badge.GetCenter() + notify_s(0, -0.5f);
    if (disabled)
    {
        draw_notify_minus_icon(draw_list, mark_center, draw->get_clr(status_color, 0.95f * alpha));
    }
    else
    {
        draw_notify_check_icon(draw_list, mark_center, draw->get_clr(status_color, alpha));
    }

    const c_rect pill(rect.Max - notify_s(84, 60), rect.Max - notify_s(13, 42));
    draw->rect_filled(draw_list, pill.Min, pill.Max, draw->get_clr(status_color, disabled ? 0.075f * alpha : 0.12f * alpha), notify_s(999));
    draw->rect(draw_list, pill.Min, pill.Max, draw->get_clr(status_color, disabled ? 0.22f * alpha : 0.36f * alpha), notify_s(999), 0, notify_s(1));
    draw->text_clipped(draw_list, font->get(inter_semibold, 8.f * notify_visual_scale), pill.Min, pill.Max, draw->get_clr(status_color, alpha),
        notify_badge_text(type), 0, 0, { 0.5f, 0.5f });

    draw->text_clipped(draw_list, font->get(inter_semibold, 12.f * notify_visual_scale), rect.Min + notify_s(60, 14), rect.Max - notify_s(91, 0),
        draw->get_clr(clr->white, alpha), title.data(), 0, 0, { 0, 0 });
    draw->text_clipped(draw_list, font->get(inter_medium, 10.f * notify_visual_scale), rect.Min + notify_s(60, 33), rect.Max - notify_s(15, 0),
        draw->get_clr(clr->text, 0.96f * alpha), text.data(), 0, 0, { 0, 0 });

    draw->line(draw_list, rect.Min + notify_s(60, 54), rect.Max - notify_s(15, 22), draw->get_clr(clr->border, 0.55f * alpha));

    const c_rect track(rect.Min + notify_s(13, 64), rect.Max - notify_s(13, 8));
    draw->rect_filled(draw_list, track.Min, track.Max, draw->get_clr(clr->widget, 0.78f * alpha), notify_s(999));
    draw->rect_filled(draw_list, track.Min, c_vec2(track.Min.x + track.GetWidth() * progress, track.Max.y),
        draw->get_clr(status_color, disabled ? 0.40f * alpha : 0.88f * alpha), notify_s(999));

    return size;
}
