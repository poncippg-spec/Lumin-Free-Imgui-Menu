#pragma once
#include <string>
#include <vector>
#include "imgui.h"
#include "../headers/flags.h"


class c_variables
{
public:

	struct
	{
		float dpi = 1.5f;
		int stored_dpi = 150;
		bool dpi_changed = true;

		int tab = 0;
		int tab_stored = 0;
		float tab_alpha = 1.f;

		int sub_tab = 1;
		int sub_tab_stored = 1;

		bool login_loading = false;
		float login_loading_timer = 0.f;
		float login_loading_alpha = 0.f;
		std::string login_loading_description = "Please wait, account verification";

		bool license_invalid = false;
		float license_invalid_timer = 0.f;
		std::string license_error_message = "";

		bool compact_layout = false;
		bool sidebar_glass = false;
		float window_rounding = 12.f;
		bool strict_license = true;
		bool verification_animation = true;
		char profile_name[64] = "Active user";
		char feature_search[96] = "";
	} gui;

	gui_style style;

};

inline std::unique_ptr<c_variables> var = std::make_unique<c_variables>();
