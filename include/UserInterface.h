#pragma once

#include <imgui_impl_glfw.h>
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_opengl3.h>

class UserInterface
{
public:
  virtual void init(GLFWwindow* window, const char* glsl_version);
  void newFrame();
  virtual void update();
  void render();
  virtual void shutdown();
};