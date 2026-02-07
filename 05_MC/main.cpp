#include <GLApp.h>
#include <Mat4.h>
#include <ArcBall.h>

#include "QVis.h"
#include "MC.h"

class MarchingCubes : public GLApp {
public:
  std::vector<float> data;
  QVis q{"bonsai.dat"};
  uint8_t isovalue{40};
  float eye{2.0f};
  bool wireframe{false};
  bool surfaceChanged{true};
  ArcBall arcball{{512, 512}};
  Mat4 rotation;
  bool leftMouseDown{false};

  MarchingCubes(const std::vector<std::string>& args) :
  GLApp{800,800,1,"Marching Cubes demo", true, false, false, args} {}

  virtual void init() override {
    extractIsosurface();
    GL(glDisable(GL_CULL_FACE));
    GL(glEnable(GL_DEPTH_TEST));
  }
  
  void extractIsosurface() {
    surfaceChanged = true;
    Isosurface s{q.volume,isovalue};
    data.clear();
    for (const Vertex& v : s.vertices) {
      data.push_back(v.position[0]);
      data.push_back(v.position[1]);
      data.push_back(v.position[2]);
       
      data.push_back(v.position[0]+0.5f);
      data.push_back(v.position[1]+0.5f);
      data.push_back(v.position[2]+0.5f);
      data.push_back(1.0f);

      data.push_back(v.normal[0]);
      data.push_back(v.normal[1]);
      data.push_back(v.normal[2]);
    }
  }
  
  virtual void draw() override {
    setDrawProjection(Mat4::perspective(45, glEnv.getFramebufferSize().aspect(), 0.0001f, 100));
    setDrawTransform(Mat4::lookAt({0,0,eye},{0,0,0},{0,1,0}) * rotation);

    if (surfaceChanged) {
      drawTriangles(data, TrisDrawType::LIST, wireframe, true);
      surfaceChanged = false;
    } else {
      redrawTriangles(wireframe);
    }
  }
  
  virtual void keyboard(int key, int scancode, int action, int mods) override {

    if (action == GLENV_PRESS) {
      switch (key) {
        case GLENV_KEY_ESCAPE:
          closeWindow();
          break;
        case GLENV_KEY_W:
          wireframe = !wireframe;
          surfaceChanged = true;
          std::cout << "wireframe is now " << wireframe << std::endl;
          break;
      }
    }
    switch (key) {
      case GLENV_KEY_UP:
        eye *= 0.9f;
        break;
      case GLENV_KEY_DOWN:
        eye /= 0.9f;
        break;
      case GLENV_KEY_LEFT:
        isovalue++;
        extractIsosurface();
        break;
      case GLENV_KEY_RIGHT:
        isovalue--;
        extractIsosurface();
        break;
    }
  }

  virtual void mouseMove(double xPosition, double yPosition) override {
    if (leftMouseDown) {
      const Quaternion q = arcball.drag({uint32_t(xPosition),uint32_t(yPosition)});
      arcball.click({uint32_t(xPosition),uint32_t(yPosition)});
      rotation = q.computeRotation() * rotation;
    }
  }
  virtual void mouseButton(int button, int state, int mods, double xPosition, double yPosition) override {
    if (button == GLENV_MOUSE_BUTTON_LEFT) {
      leftMouseDown = state == GLENV_MOUSE_PRESS;
      arcball.click({uint32_t(xPosition),uint32_t(yPosition)});
    }
  }

  virtual void resize(const Dimensions winDim, const Dimensions fbDim) override {
    GLApp::resize(winDim, fbDim);
    arcball.setWindowSize({winDim.width,winDim.height});
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
    MarchingCubes app{args};
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
