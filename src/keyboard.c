/* 
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "joystick.h"
#include "keyboard.h"
#include "network.h"
#include "opentyr.h"
#include "video.h"
#include "video_scale.h"

#include "SDL.h"

#include "3ds.h"

JE_boolean ESCPressed;

JE_boolean newkey, newmouse, keydown, mousedown;
SDLKey lastkey_sym;
SDLMod lastkey_mod;
unsigned char lastkey_char;
Uint8 lastmouse_but;
Uint16 lastmouse_x, lastmouse_y;
JE_boolean mouse_pressed[3] = {false, false, false};
Uint16 mouse_x, mouse_y;

Uint8 keysactive[SDLK_LAST];

#ifdef NDEBUG
bool input_grab_enabled = true;
#else
bool input_grab_enabled = false;
#endif


void flush_events_buffer( void )
{
	SDL_Event ev;

	while (SDL_PollEvent(&ev));
}

void wait_input( JE_boolean keyboard, JE_boolean mouse, JE_boolean joystick )
{
	service_SDL_events(false);
	while (!((keyboard && keydown) || (mouse && mousedown) || (joystick && joydown)))
	{
		SDL_Delay(SDL_POLL_INTERVAL);
		push_joysticks_as_keyboard();
		service_SDL_events(false);
		
#ifdef WITH_NETWORK
		if (isNetworkGame)
			network_check();
#endif
	}
}

void wait_noinput( JE_boolean keyboard, JE_boolean mouse, JE_boolean joystick )
{
	service_SDL_events(false);
	while ((keyboard && keydown) || (mouse && mousedown) || (joystick && joydown))
	{
		SDL_Delay(SDL_POLL_INTERVAL);
		poll_joysticks();
		service_SDL_events(false);
		
#ifdef WITH_NETWORK
		if (isNetworkGame)
			network_check();
#endif
	}
}

void init_keyboard( void )
{
	SDL_EnableKeyRepeat(500, 60);

	newkey = newmouse = false;
	keydown = mousedown = false;

	SDL_EnableUNICODE(1);
}

void input_grab( bool enable )
{
#if defined(TARGET_GP2X) || defined(TARGET_DINGUX) || defined(_DS)
	enable = true;
#endif
	
	input_grab_enabled = enable || fullscreen_enabled;
	
	SDL_ShowCursor(input_grab_enabled ? SDL_DISABLE : SDL_ENABLE);
#ifdef NDEBUG
	SDL_WM_GrabInput(input_grab_enabled ? SDL_GRAB_ON : SDL_GRAB_OFF);
#endif
}

JE_word JE_mousePosition( JE_word *mouseX, JE_word *mouseY )
{
	service_SDL_events(false);
	*mouseX = mouse_x;
	*mouseY = mouse_y;
	return mousedown ? lastmouse_but : 0;
}

void set_mouse_position( int x, int y )
{
	if (input_grab_enabled)
	{
		SDL_WarpMouse(x * scalers[scaler].width / vga_width, y * scalers[scaler].height / vga_height);
		mouse_x = x;
		mouse_y = y;
	}
}

void service_SDL_events( JE_boolean clear_new )
{
	SDL_Event ev;
	
	if (clear_new)
		newkey = newmouse = false;
	
	//Matrix containing the name of each key. Useful for printing when a key is pressed
	char keysNames[32][32] = {
		"KEY_A", "KEY_B", "KEY_SELECT", "KEY_START",
		"KEY_DRIGHT", "KEY_DLEFT", "KEY_DUP", "KEY_DDOWN",
		"KEY_R", "KEY_L", "KEY_X", "KEY_Y",
		"", "", "KEY_ZL", "KEY_ZR",
		"", "", "", "",
		"KEY_TOUCH", "", "", "",
		"KEY_CSTICK_RIGHT", "KEY_CSTICK_LEFT", "KEY_CSTICK_UP", "KEY_CSTICK_DOWN",
		"KEY_CPAD_RIGHT", "KEY_CPAD_LEFT", "KEY_CPAD_UP", "KEY_CPAD_DOWN"
	};

    uint32_t kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame

	while (SDL_PollEvent(&ev))
	//Scan all the inputs. This should be done once for each frame
	hidScanInput();

	//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
	uint32_t kDown = hidKeysDown();
	//hidKeysHeld returns information about which buttons have are held down in this frame
	uint32_t kHeld = hidKeysHeld();
	//hidKeysUp returns information about which buttons have been just released
	uint32_t kUp = hidKeysUp();
	{
	
		if (kDown)
		{
			if (kDown & KEY_START)
			{
				ev.key.keysym.sym = SDLK_RETURN;
			}
			
			if (kDown & KEY_SELECT)
			{
				ev.key.keysym.sym = SDLK_ESCAPE;
			}
			
			if (kDown & KEY_A)
			{
				ev.key.keysym.sym = SDLK_LCTRL;
			}
			
			if (kDown & KEY_B)
			{
				ev.key.keysym.sym = SDLK_LALT;
			}
			
			if (kDown & KEY_X)
			{
				ev.key.keysym.sym = SDLK_SPACE;
			}
			
			if (kDown & KEY_Y)
			{
				//YButton = 1;
			}
			
			if (kDown & KEY_LEFT)
			{
				ev.key.keysym.sym = SDLK_LEFT;
			}
			
			if (kDown & KEY_RIGHT)
			{
				ev.key.keysym.sym = SDLK_RIGHT;
			}
			
			if (kDown & KEY_UP)
			{
				ev.key.keysym.sym = SDLK_UP;
			}
			
			if (kDown & KEY_DOWN)
			{
				ev.key.keysym.sym = SDLK_DOWN;
			}
			
			if (kDown & KEY_L)
			{
				//LeftShoulder = 1;
			}
			
			if (kDown & KEY_R)
			{
				//RightShoulder = 1;
			}

			keysactive[ev.key.keysym.sym] = 1;
			
			newkey = true;
			lastkey_sym = ev.key.keysym.sym;
			lastkey_mod = ev.key.keysym.mod;
			lastkey_char = ev.key.keysym.unicode;
			keydown = true;
		}

		if (kUp)
		{
			keysactive[ev.key.keysym.sym] = 0;
			keydown = false;
		}

		return; //Maybe needed?
	}
}

void JE_clearKeyboard( void )
{
	// /!\ Doesn't seems important. I think. D:
}

