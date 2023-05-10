#include <GLApp.h>
#include <bmp.h>
#include "MS.h"

class MyGLApp : public GLApp {
public:
  std::vector<float> data;
  std::vector<float> grid;
  Image image = BMP::load("image.bmp");
  std::vector<uint8_t> isovalues{128};
  bool useAsymptoticDecider{true};
  bool doLinearSampling{true};
  bool drawGridLines{false};
  
  virtual void init() override {
    glEnv.setTitle("Marching Squares demo");
    GL(glDisable(GL_CULL_FACE));
    GL(glDisable(GL_DEPTH_TEST));
    GL(glClearColor(0,0,0,0));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glBlendEquation(GL_FUNC_ADD));
    GL(glEnable(GL_BLEND));
    extractIsoline();
    generateGridLines();
  }

  void generateGridLines() {
    for (size_t y = 0;y<image.height-1;++y) {
      const float fy = y/float(image.height-1)*2-1;
      grid.push_back(-1); grid.push_back(fy); grid.push_back(0);
      grid.push_back(1.0f); grid.push_back(1.0f);
      grid.push_back(0.0f); grid.push_back(0.2f);

      grid.push_back(1); grid.push_back(fy); grid.push_back(0);
      grid.push_back(1.0f); grid.push_back(1.0f);
      grid.push_back(0.0f); grid.push_back(0.2f);
    }
    for (size_t x = 0;x<image.width-1;++x) {
      const float fx = x/float(image.width-1)*2-1;
      grid.push_back(fx); grid.push_back(-1); grid.push_back(0);
      grid.push_back(1.0f); grid.push_back(1.0f);
      grid.push_back(0.0f); grid.push_back(0.2f);

      grid.push_back(fx); grid.push_back(1);grid.push_back(0);
      grid.push_back(1.0f); grid.push_back(1.0f);
      grid.push_back(0.0f); grid.push_back(0.2f);
    }
  }

  void extractIsoline() {
    data.clear();
    for (const uint8_t isovalue : isovalues) {
      Isoline s{image, isovalue, useAsymptoticDecider};
      for (const Vec2& v : s.vertices) {
        data.push_back(v[0]);
        data.push_back(v[1]);
        data.push_back(0);
         
        data.push_back(0.0f);
        data.push_back(0.0f);
        data.push_back(1.0f);
        data.push_back(1.0f);
      }
    }
  }
  
  virtual void draw() override {
    GL(glClear(GL_COLOR_BUFFER_BIT));
    drawImage(image,{-1,-1},{1,1},true);
    if (drawGridLines) drawLines(grid, LineDrawType::LIST, 2);
    drawLines(data, LineDrawType::LIST, 2);
  }
  
  virtual void keyboard(int key, int scancode, int action, int mods) override {
    if (action == GLFW_PRESS) {
      switch (key) {
        case GLFW_KEY_ESCAPE:
          closeWindow();
          break;
        case GLFW_KEY_D:
          useAsymptoticDecider = ! useAsymptoticDecider;
          std::cout << "Asymptotic Decider is " << (useAsymptoticDecider ? "enabled" : "disabled") << std::endl;
          extractIsoline();
          break;
        case GLFW_KEY_G:
          drawGridLines = ! drawGridLines;
          break;
        case GLFW_KEY_F:
          doLinearSampling = ! doLinearSampling;
          setImageFilter(doLinearSampling ? GL_LINEAR : GL_NEAREST,
                         doLinearSampling ? GL_LINEAR : GL_NEAREST);
          break;
        case GLFW_KEY_ENTER:
          isovalues.push_back(isovalues.back());
          extractIsoline();
          break;
        case GLFW_KEY_C:
          isovalues.clear();
          isovalues.push_back(128);
          extractIsoline();
          break;

      }
    }
    switch (key) {
      case GLFW_KEY_UP:
        isovalues[isovalues.size()-1]++;
        extractIsoline();
        break;
      case GLFW_KEY_DOWN:
        isovalues[isovalues.size()-1]--;
        extractIsoline();
        break;
    }
  }
} myApp;

int main(int argc, char ** argv) {
  myApp.run();
  return EXIT_SUCCESS;
}  
