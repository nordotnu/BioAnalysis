#pragma once
#include <vector>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
class Keyboard
{
private:
  std::vector<int> keyCodes;
  Display *display;
  int last;

public:
  std::vector<int> keyMappings;
  std::vector<const char *> keys;
  Keyboard();
  void sendKey(int key);
  ~Keyboard();
};
