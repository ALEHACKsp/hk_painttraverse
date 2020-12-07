#include "includes.hpp"

i_panel* panelmove = nullptr;
i_surface* surface = nullptr;
using paint_traverse_fn = void(__thiscall*)(i_panel*, unsigned int, bool, bool);
paint_traverse_fn original_panel;

enum class interface_type { index, bruteforce };
template <typename ret, interface_type type>
ret* get_interface(const std::string& module_name, const std::string& interface_name) {
	using create_interface_fn = void* (*)(const char*, int*);
	const auto fn = reinterpret_cast<create_interface_fn>(GetProcAddress(GetModuleHandle(module_name.c_str()), "CreateInterface"));

	if (fn) {
		void* result = nullptr;

		switch (type) {
		case interface_type::index:
			result = fn(interface_name.c_str(), nullptr);

			break;
		case interface_type::bruteforce:
			char buf[128];

			for (uint32_t i = 0; i <= 100; ++i) {
				memset(static_cast<void*>(buf), 0, sizeof buf);

				result = fn(interface_name.c_str(), nullptr);

				if (result)
					break;
			}

			break;
		}
		return static_cast<ret*>(result);
	}
}

inline unsigned int get_virtual(void* _class, unsigned int index) { 
	return static_cast<unsigned int>((*static_cast<int**>(_class))[index]);
}

void __stdcall hk_paint_traverse(unsigned int panel, bool force_repaint, bool allow_force) {
	std::string panel_name = panelmove->get_panel_name(panel);
	static unsigned int _panel = 0;
	static bool once = false;

	if (!once) {
		PCHAR panel_char = (PCHAR)panelmove->get_panel_name(panel);
		if (strstr(panel_char, "MatSystemTopPanel")) {
			_panel = panel;
			once = true;
		}
	}

	else if (_panel == panel) { // drawing

		surface->set_drawing_color(0, 0, 0, 155);
		surface->draw_filled_rectangle(5, 5, 32, 32);
	}

	original_panel(panelmove, panel, force_repaint, allow_force);
}

void interfaces() {
	panelmove = get_interface<i_panel, interface_type::index>("vgui2.dll", "VGUI_Panel009");
	surface = get_interface<i_surface, interface_type::index>("vguimatsurface.dll", "VGUI_Surface031");

	std::cout << " " << std::endl;
	std::cout << " [+]: interfaces initialized" << std::endl;
}

void hooks() {
	const auto panel_hook = reinterpret_cast<void*>(get_virtual(panelmove, 41)); // index

	if (MH_Initialize() != MH_OK)
		throw std::runtime_error(" [MH]: failed to initialize MH_Initialize.");


	if (MH_CreateHook(panel_hook, &hk_paint_traverse, reinterpret_cast<void**>(&original_panel)) != MH_OK)
		throw std::runtime_error(" [MH]: failed to initialize hk_paint_traverse.");


	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
		throw std::runtime_error(" [MH]: failed to enable hooks.");

	std::cout << " [+]: hook initialized" << std::endl;
}

unsigned long __stdcall initialize(void* instance) {
	while (!GetModuleHandleA("serverbrowser.dll"))
		Sleep(200);

	try {
		interfaces();
		hooks();
	}

	catch (const std::runtime_error& error) {
		MessageBoxA(nullptr, error.what(), " [!]", MB_OK | MB_ICONERROR);
		FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);
	}
}

unsigned long __stdcall deinitialize() {
	MH_Uninitialize();
	MH_DisableHook(MH_ALL_HOOKS);

	return TRUE;
}

int32_t __stdcall DllMain(const HMODULE instance, const unsigned long reason, const void* reserved) {
	DisableThreadLibraryCalls(instance);

	switch (reason) {
	case DLL_PROCESS_ATTACH: {
		AllocConsole();
		freopen("conin$", "r", stdin);
		freopen("conout$", "w", stdout);
		freopen("conout$", "w", stderr);

		if (auto handle = CreateThread(nullptr, NULL, initialize, instance, NULL, nullptr))
			CloseHandle(handle);

		break;
	}
	case DLL_PROCESS_DETACH: {
		deinitialize();
		break;
	}
	}
	return true;
}