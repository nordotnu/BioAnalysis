
#include <AppUI.h>
#include <GLWindow.h>

int main()
{

  GLWindow glWindow;
  if (!glWindow.create(900, 745, "BioAnalysis"))
    return 1;
  int status = -1;
  bool connected = false;
  SerialDataReceiver sdr(B921600, &status);
  EMGFilter filter(&sdr, &connected);
  AppUI ui(glWindow.window, glWindow.glsl_version, &sdr, &filter, &status, &connected);


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
