#pragma once
#include <GLFW/glfw3.h>
#include <stdio.h>
class GLWindow
{
public:
  GLFWwindow *window;
  const char *glsl_version;
  bool create(int width, int height, const char *title);
  bool closed();
  void swap();
  void destroy();
};