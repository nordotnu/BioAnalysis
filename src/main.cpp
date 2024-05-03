
#include <AppUI.h>
#include <GLWindow.h>

int main()
{

  GLWindow glWindow;
  if (!glWindow.create(900, 750, "BioAnalysis"))
    return 1;
  AppUI ui(glWindow.window, glWindow.glsl_version);


  while (!glWindow.closed())
  {

    ui.newFrame();
    ui.update();
    ui.render();
    glWindow.swap();
  }

  ui.shutdown();
  glWindow.destroy();


  return 0;
}
