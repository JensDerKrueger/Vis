#include <GLApp.h>
#include <Mat4.h>
#include <ArcBall.h>

#include "Flowfield.h"

class ParticleTracer : public GLApp {
public:
  size_t particleCount{1000};
  double lastAnimationTime{0};
  std::vector<Vec3> particlePositions;
  std::vector<float> data;
  Flowfield flow = Flowfield::genDemo(64, DemoType::SATTLE);
  ArcBall arcball{{512, 512}};
  Mat4 rotation;
  bool leftMouseDown{false};

  ParticleTracer(const std::vector<std::string>& args) :
  GLApp{800,800,1,"Flow Vis Demo 1 (Particle Tracing)", true, false, false, args} {}

  virtual void init() override {
    GL(glDisable(GL_CULL_FACE));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glClearColor(0,0,0,0));
    initParticles();
  }

  void initParticles() {
    particlePositions.resize(particleCount);
    for (size_t i = 0;i<particlePositions.size();++i) {
      particlePositions[i] = Vec3::random();
    }
    data.resize(particlePositions.size()*7);
  }
  
  void advect(double deltaT) {
    // TODO: advect the particles
  }

  void particlePositionsToRenderData() {
    for (size_t i = 0;i<particlePositions.size();++i) {
      data[i*7+0] = particlePositions[i].x*2-1;
      data[i*7+1] = particlePositions[i].y*2-1;
      data[i*7+2] = particlePositions[i].z*2-1;
      
      data[i*7+3] = particlePositions[i].x;
      data[i*7+4] = particlePositions[i].y;
      data[i*7+5] = particlePositions[i].z;
      data[i*7+6] = 1.0f;
    }
  }
  
  virtual void animate(double animationTime) override {
    const double deltaT = animationTime - lastAnimationTime;
    lastAnimationTime = animationTime;
    advect(deltaT*10);
    particlePositionsToRenderData();
  }
  
  virtual void draw() override {
    GL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
    setDrawProjection(Mat4::perspective(45, glEnv.getFramebufferSize().aspect(), 0.0001f, 100));
    setDrawTransform(Mat4::lookAt({0,0,5},{0,0,0},{0,1,0}) * rotation);
    
    drawPoints(data, 4, false);
  }
  
  virtual void resize(int width, int height) override {
    const Dimensions dim = glEnv.getWindowSize();
    arcball.setWindowSize({dim.width,dim.height});
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

  virtual void keyboard(int key, int scancode, int action, int mods) override {
    if (action == GLENV_PRESS) {
      switch (key) {
        case GLENV_KEY_ESCAPE:
          closeWindow();
          break;
        case GLENV_KEY_I:
          initParticles();
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
    ParticleTracer app{args};
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
