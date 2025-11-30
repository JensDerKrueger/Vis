#include <GLApp.h>
#include <Mat4.h>
#include <ArcBall.h>

#include "Flowfield.h"

class LineVisualizer : public GLApp {
public:
  ArcBall arcball{{512, 512}};
  Mat4 rotation;
  bool leftMouseDown{false};

  size_t lineCount{200};
  size_t lineLength{300};
  double angle{0};
  std::vector<float> data;
  Flowfield flow = Flowfield::genDemo(128, DemoType::SATTLE);
  
  LineVisualizer(const std::vector<std::string>& args) :
  GLApp{800,800,1,"Flow Vis Demo 2 (Integral Curves)", true, false, false, args} {}

  virtual void init() override {
    initLines();
  }

  void initLines() {
    std::vector<Vec3> linePoints;
    linePoints.resize(lineCount*lineLength);

    // TODO compute integral curves here

    linePointsToRenderData(linePoints);
  }
  

  void linePointsToRenderData(const std::vector<Vec3>& linePoints) {
    // 2 -> line start and line end point
    // 7 -> x,y,z coord plus r,g,b,a color components
    data.resize(lineCount*(lineLength-1)*2*7);

    size_t i = 0;
    for (size_t l = 0;l<lineCount;++l) {
      for (size_t s = 0;s<lineLength-1;++s) {
        size_t j = l*lineLength + s;
        
        if (linePoints[j] == linePoints[j+1]) break;
        
        data[i++] = linePoints[j].x*2-1;
        data[i++] = linePoints[j].y*2-1;
        data[i++] = linePoints[j].z*2-1;
        
        data[i++] = linePoints[j].x;
        data[i++] = linePoints[j].y;
        data[i++] = linePoints[j].z;
        data[i++] = 1.0f;

        j++;
        
        data[i++] = linePoints[j].x*2-1;
        data[i++] = linePoints[j].y*2-1;
        data[i++] = linePoints[j].z*2-1;
        
        data[i++] = linePoints[j].x;
        data[i++] = linePoints[j].y;
        data[i++] = linePoints[j].z;
        data[i++] = 1.0f;
      }
    }
  }
  
  virtual void draw() override {
    GL(glDisable(GL_CULL_FACE));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glClearColor(0,0,0,1));
    GL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
    setDrawProjection(Mat4::perspective(45, glEnv.getFramebufferSize().aspect(), 0.0001f, 100));
    setDrawTransform(Mat4::lookAt({0,0,5},{0,0,0},{0,1,0}) * rotation);
    drawLines(data, LineDrawType::LIST, 1.0f);
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

  virtual void resize(int width, int height) override {
    const Dimensions dim = glEnv.getWindowSize();
    arcball.setWindowSize({dim.width,dim.height});
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
    LineVisualizer app{args};
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
