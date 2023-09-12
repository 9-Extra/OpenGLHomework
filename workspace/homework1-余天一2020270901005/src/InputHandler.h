#pragma once
#include "CGmath.h"
#include <assert.h>
#include "winapi.h"

#define KEYCODE_MAX 0xff

class InputHandler {
private:
	Vector2f mouse_position;
	bool mouse_lb;
	bool mouse_rb;
	bool keyboard[KEYCODE_MAX];

	friend LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	void clear_keyboard_state() {
		for (unsigned int i = 0; i < KEYCODE_MAX; i++) {
			keyboard[i] = false;
		}
	}

	void clear_mouse_state() {
		mouse_lb = false;
		mouse_rb = false;
		mouse_position = Vector2f(0.5, 0.5);
	}

	void handle_mouse_move(float x, float y, bool lb, bool rb) {
		//LOG_DEBUG("Real mouse handle called!");
		mouse_position = Vector2f(x, y);
		mouse_lb = lb;
		mouse_rb = rb;
	}

	void key_down(unsigned long long vk) {
		if (vk < KEYCODE_MAX) {
			keyboard[vk] = true;
		}
	}

	void key_up(unsigned long long vk) {
		if (vk < KEYCODE_MAX) {
			keyboard[vk] = false;
		}
	}

public:
	InputHandler() {
		clear_keyboard_state();
		clear_mouse_state();
	}

	inline Vector2f get_mouse_position() const {
		return mouse_position;
	}

	inline bool is_left_button_down()  const {
		return mouse_lb;
	}

	inline bool is_right_button_down()  const {
		return mouse_rb;
	}

	inline bool is_keydown(unsigned int key)  const {
		assert(key < KEYCODE_MAX);
		return keyboard[key];
	}

};