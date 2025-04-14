#include <GLApp.h>
#include <Mat4.h>
#include <ArcBall.h>

#include "Flowfield.h"

class MyGLApp : public GLApp {
public:
  size_t particleCount{1000};
  double lastAnimationTime{0};
  std::vector<Vec3> particlePositions;
  std::vector<float> data;
  Flowfield flow = Flowfield::genDemo(64, DemoType::SATTLE);
  ArcBall arcball{{512, 512}};
  Mat4 rotation;
  bool leftMouseDown{false};

  virtual void init() override {
    glEnv.setTitle("Flow Vis Demo 1 (Particle Tracing)");
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


} myApp;

int main(int argc, char ** argv) {
  myApp.run();
  return EXIT_SUCCESS;
}  
