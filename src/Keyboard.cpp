#include "Keyboard.h"

Keyboard::Keyboard()
{
  Keyboard::keyCodes = {XK_0, XK_Up, XK_Down, XK_Left, XK_Right, XK_space, XK_KP_Enter};
  Keyboard::keys = {"None", "Up", "Down", "Left", "Right", "Space", "Enter"};
  Keyboard::keyMappings = {0, 1, 4, 3, 2};
  Keyboard::display = XOpenDisplay(NULL);
  Keyboard::last = -1;
}

void Keyboard::sendKey(int key)
{
  unsigned int keycode = XKeysymToKeycode(display, keyCodes[key]);
  if (key == 0)
  {
    unsigned int code = XKeysymToKeycode(display, keyCodes[last]);
    XTestFakeKeyEvent(display, code, False, 0);
    XFlush(display);
    return;
  }
  if (last == -1)
  {
    last = key;
    XTestFakeKeyEvent(display, keycode, True, 0);
    XFlush(display);
    return;
  }

  if (key != last)
  {

    unsigned int code = XKeysymToKeycode(display, keyCodes[last]);
    XTestFakeKeyEvent(display, code, False, 0);

    XFlush(display);
    XTestFakeKeyEvent(display, keycode, True, 0);
    last = key;

    XFlush(display);
  }

  // AppUI::display = XOpenDisplay(NULL);
}

Keyboard::~Keyboard()
{
}
