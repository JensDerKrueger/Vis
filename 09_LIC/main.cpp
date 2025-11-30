#include <GLApp.h>
#include <bmp.h>

#include "Flowfield.h"

class LICApp : public GLApp {
public:
  Flowfield flow = Flowfield::genDemo(256, DemoType::SATTLE);
  // this field may be a better start for debugging
  //Flowfield flow = Flowfield::fromFile("four_sector_128.txt");
  Image inputImage = BMP::load("noise.bmp");
  Image licImage{uint32_t(flow.getSizeX()),uint32_t(flow.getSizeY()),3};

  LICApp(const std::vector<std::string>& args) :
  GLApp{800,800,1,"LIC demo", true, false, false, args} {}

  virtual void init() override {
    GL(glDisable(GL_CULL_FACE));
    GL(glDisable(GL_DEPTH_TEST));
    GL(glClearColor(0,0,0,0));
    computeLIC();
  }
  
  void computeLIC() {
    // TODO: perform LIC here and store output in licImage
    licImage = inputImage;
  }
  
  virtual void draw() override {
    GL(glClear(GL_COLOR_BUFFER_BIT));
    drawImage(licImage);
  }
  
  virtual void keyboard(int key, int scancode, int action, int mods) override {
    if (action == GLENV_PRESS) {
      switch (key) {
        case GLENV_KEY_ESCAPE:
          closeWindow();
          break;
      }
    }
  }
};

#ifdef _WIN32
#include <Windows.h>

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
  std::vector<std::string> args = getArgsWindows();
#else
int main(int argc, char** argv) {
  std::vector<std::string> args{argv + 1, argv + argc};
#endif
  try {
    LICApp app{args};
    app.run();
  }
  catch (const GLException& e) {
    std::stringstream ss;
    ss << "Insufficient OpenGL Support " << e.what();
#ifndef _WIN32
    std::cerr << ss.str().c_str() << std::endl;
#else
    MessageBoxA(
                NULL,
                ss.str().c_str(),
                "OpenGL Error",
                MB_ICONERROR | MB_OK
                );
#endif
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
