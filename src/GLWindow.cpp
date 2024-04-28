#include "GLWindow.h"

static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

/// @brief Create a window using GLFW.
/// @param width The width of the window in pixels.
/// @param height The height of the window in pixels.
/// @param title The title of the window.
/// @return if the window was created.
bool GLWindow::create(int width, int height, const char *title)
{
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return false;

  glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (window == nullptr)
    return false;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  // glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
  return true;
}
/// @brief Check if a close command has been received.
/// @return If received command.
bool GLWindow::closed()
{

  bool closed = glfwWindowShouldClose(window);
  if (!closed)
  {
    glfwPollEvents();
    glClear(GL_COLOR_BUFFER_BIT);
  }
  return closed;
}

/// @brief Swap the buffers.
void GLWindow::swap()
{
  glfwSwapBuffers(window);
}

/// @brief Destroy the window.
void GLWindow::destroy()
{
  glfwDestroyWindow(window);
  glfwTerminate();
}
