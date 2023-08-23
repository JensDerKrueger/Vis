#include <GLApp.h>
#include <Tesselation.h>
#include <ArcBall.h>

#include "QVis.h"

class GLIPApp : public GLApp {
public:
    
  GLIPApp() : GLApp(512, 512, 1, "Raycaster") {}

  virtual void animate(double animationTime) override {
    const Mat4 m{ rotation * Mat4::scaling(volumeExtend)};
    mvp = p * v * m;
    v2t = Mat4::translation({0.5f,0.5f,0.5f}) * Mat4::inverse(v * m);
  }

  void loadVolume() {
    volume = QVis{filenames[currentFile]}.volume;
    voxelCount = Vec3{float(volume.width),float(volume.height),float(volume.depth)};
    volumeExtend = volume.scale*voxelCount/volume.maxSize;

    volumeTex.setData(volume.data,
                      uint32_t(volume.width),
                      uint32_t(volume.height),
                      uint32_t(volume.depth), 1);
  }

  virtual void init() override {
    loadVolume();

    cubeArray.bind();
    vbCube.setData(cube.getVertices(), 3);
    ibCube.setData(cube.getIndices());
    cubeArray.connectVertexAttrib(vbCube, cubeProgram, "vPos", 3);
    cubeArray.connectIndexBuffer(ibCube);
              
    Tesselation fullScreenQuad{Tesselation::genRectangle({0,0,0},2,2)};

    GL(glClearDepth(1.0f));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_CULL_FACE));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glBlendEquation(GL_FUNC_ADD));
  }
      
  virtual void resize(int width, int height) override {
    p = Mat4{ Mat4::perspective(45, glEnv.getFramebufferSize().aspect(), 0.1f, 100) };
    
    const Dimensions dim = glEnv.getWindowSize();
    arcball.setWindowSize({dim.width,dim.height});
  }

  virtual void draw() override {
    GL(glCullFace(GL_FRONT));
    GL(glEnable(GL_BLEND));

    const Dimensions dim = glEnv.getFramebufferSize();
    GL(glViewport(0, 0, GLsizei(dim.width), GLsizei(dim.height)));
    GL(glClearColor(0,0,0.5,0));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    cubeProgram.enable();
    cubeProgram.setTexture("volume",volumeTex,0);
    cubeProgram.setUniform("mvp", mvp);
    cubeProgram.setUniform("v2t", v2t);
    cubeProgram.setUniform("voxelCount", voxelCount);
    cubeProgram.setUniform("oversampling", oversampling);
    cubeProgram.setUniform("smoothStepStart", stepStart);
    cubeProgram.setUniform("smoothStepWidth", stepWidth);

    const GLsizei indexCount = GLsizei(cube.getIndices().size());
    GL(glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)0));
    cubeProgram.unsetTexture3D(0);

    GL(glDisable(GL_BLEND));
  }
  
  virtual void keyboard(int key, int scancode, int action, int mods) override {
    if (action == GLFW_PRESS) {
      switch (key) {
        case GLFW_KEY_ESCAPE :
          closeWindow();
          break;
        case GLFW_KEY_V:
          currentFile = (currentFile + 1) % filenames.size();
          loadVolume();
          break;
        case GLFW_KEY_R:
          rotation = Mat4{};
          stepStart = 0.12f;
          stepWidth = 0.1f;
          break;
      }
    }
  }
  
  virtual void mouseMove(double xPosition, double yPosition) override {
    if (rightMouseDown) {
      const Dimensions dim = glEnv.getFramebufferSize();
      const double xDelta = xPositionStart - xPosition;
      const double yDelta = yPositionStart - yPosition;
      xPositionStart = xPosition;
      yPositionStart = yPosition;
      
      stepStart += float(xDelta/dim.width);
      stepWidth += float(yDelta/dim.height);
    }
    
    if (leftMouseDown) {
      const Quaternion q = arcball.drag({uint32_t(xPosition),uint32_t(yPosition)});
      arcball.click({uint32_t(xPosition),uint32_t(yPosition)});
      rotation = q.computeRotation() * rotation;
    }
  }
  
  virtual void mouseButton(int button, int state, int mods, double xPosition, double yPosition) override {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      rightMouseDown = state == GLFW_PRESS;
      xPositionStart = xPosition;
      yPositionStart = yPosition;
    }
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      leftMouseDown = state == GLFW_PRESS;
      arcball.click({uint32_t(xPosition),uint32_t(yPosition)});
    }
  }

private:
  Tesselation cube{Tesselation::genBrick({0, 0, 0}, {1, 1, 1})};
  GLBuffer vbCube{GL_ARRAY_BUFFER};
  GLBuffer ibCube{GL_ELEMENT_ARRAY_BUFFER};
  GLArray cubeArray;
  GLProgram cubeProgram{GLProgram::createFromFile("cubeVS.glsl", "cubeFS.glsl")};
  Volume volume;
  Vec3 voxelCount;
  Vec3 volumeExtend;
  GLTexture3D volumeTex{GL_LINEAR, GL_LINEAR,GL_CLAMP_TO_BORDER,GL_CLAMP_TO_BORDER,GL_CLAMP_TO_BORDER};
  
  ArcBall arcball{{512, 512}};
  Mat4 rotation;
  Mat4 mvp;
  Mat4 v2t;
  Mat4 p;
  Mat4 v{Mat4::lookAt({ 0, 0, 2 }, { 0, 0, 0 }, { 0, 1, 0 })};

  float oversampling{10};

  std::vector<std::string> filenames{"c60.dat","bonsai.dat","Engine.dat"};
  size_t currentFile{0};
  float stepStart{0.12f};
  float stepWidth{0.1f};
  bool leftMouseDown{false};
  bool rightMouseDown{false};
  double xPositionStart{0};
  double yPositionStart{0};

};

#ifdef _WIN32
#include <Windows.h>

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
#else
int main(int argc, char** argv) {
#endif
  try {
    GLIPApp imageProcessing;
    imageProcessing.run();
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
