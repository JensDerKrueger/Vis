#include <GLApp.h>
#include <Tesselation.h>
#include <ArcBall.h>
#include "Clipper.h"

#include "QVis.h"

class Raycaster : public GLApp {
public:
    
  Raycaster() : GLApp(512, 512, 1, "Raycaster") {}

  virtual void animate(double animationTime) override {
  }

  void updateMatrices() {
    clipBoxSize = Vec3::maxV({0,0,0},Vec3::minV({1,1,1}, clipBoxSize));
    clipBoxShift = Vec3::maxV((Vec3{1,1,1}-clipBoxSize)*-0.5,Vec3::minV((Vec3{1,1,1}-clipBoxSize)*0.5, clipBoxShift));
    clipBox = Mat4::translation(clipBoxShift) * Mat4::scaling(clipBoxSize);
    minBounds = clipBox * Vec3{-0.5,-0.5,-0.5} + 0.5f;
    maxBounds = clipBox * Vec3{ 0.5, 0.5, 0.5} + 0.5f;
    model = Mat4::translation(0,0,zoom) * rotation * Mat4::scaling(volumeExtend);
    modelViewProjection = projection * view * model * clipBox;
    viewToTexture = Mat4::translation({0.5f,0.5f,0.5f}) * Mat4::inverse(view * model);
    meshNeedsUpdte = true;
  }

  void clipCubeToNearplane() {
    if (!meshNeedsUpdte) return;
    meshNeedsUpdte = false;
    // transpose( inverse( inverse(view*model) ) ) -> transpose(view*model)
    const Vec4 objectSpaceNearPlane{Mat4::transpose(view*model)*Vec4{0,0,1.0f,near+0.01f}};
    std::vector<float> verts = Clipper::meshPlane(cube.getVertices(),
                                                  objectSpaceNearPlane.xyz,
                                                  objectSpaceNearPlane.w);
    vertCount = verts.size()/3;
    vbCube.setData(verts, 3);
  }

  void loadVolume() {
    volume = QVis{filenames[currentFile]}.volume;
    voxelCount = Vec3{float(volume.width),float(volume.height),float(volume.depth)};
    volumeExtend = volume.scale*voxelCount/float(volume.maxSize);

    volumeTex.setData(volume.data,
                      uint32_t(volume.width),
                      uint32_t(volume.height),
                      uint32_t(volume.depth), 1);

  }

  virtual void init() override {
    loadVolume();

    vertCount = cube.getVertices().size();
    cubeArray.bind();
    vbCube.setData(cube.getVertices(), 3);
    cubeArray.connectVertexAttrib(vbCube, cubeProgram, "vPos", 3);

    GL(glClearColor(0,0,0.5,1));
    GL(glClearDepth(1.0f));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_CULL_FACE));
    GL(glCullFace(GL_BACK));
    GL(glEnable(GL_BLEND));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glBlendEquation(GL_FUNC_ADD));
  }
      
  virtual void resize(int width, int height) override {
    projection = Mat4{ Mat4::perspective(45, glEnv.getFramebufferSize().aspect(), near, 100) };
    const Dimensions dim = glEnv.getWindowSize();
    arcball.setWindowSize({dim.width,dim.height});

    GL(glViewport(0, 0, GLsizei(width), GLsizei(height)));
    updateMatrices();
  }

  virtual void draw() override {
    clipCubeToNearplane();

    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    cubeProgram.enable();
    cubeProgram.setTexture("volume",volumeTex,0);
    cubeProgram.setUniform("modelViewProjection", modelViewProjection);
    cubeProgram.setUniform("clip", clipBox);
    cubeProgram.setUniform("minBounds", minBounds);
    cubeProgram.setUniform("maxBounds", maxBounds);
    cubeProgram.setUniform("cameraPosInTextureSpace", (viewToTexture * Vec4{0,0,0,1}).xyz);

    cubeProgram.setUniform("voxelCount", voxelCount);
    cubeProgram.setUniform("oversampling", oversampling);
    cubeProgram.setUniform("smoothStepStart", stepStart);
    cubeProgram.setUniform("smoothStepWidth", stepWidth);

    GL(glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertCount)));
  }
  
  virtual void keyboard(int key, int scancode, int action, int mods) override {
    std::stringstream ss;
    if (action == GLENV_PRESS) {
      switch (key) {
        case GLENV_KEY_ESCAPE :
          closeWindow();
          break;
        case GLENV_KEY_V:
          currentFile = (currentFile + 1) % filenames.size();
          loadVolume();
          updateMatrices();
          break;
        case GLENV_KEY_Q:
          oversampling *= 2;
          ss << "Raycaster (" << oversampling << " x oversampling)";
          glEnv.setTitle(ss.str());
          break;
        case GLENV_KEY_W:
          oversampling /= 2;
          ss << "Raycaster (" << oversampling << " x oversampling)";
          glEnv.setTitle(ss.str());
          break;
        case GLENV_KEY_R:
          rotation = Mat4{};
          stepStart = 0.12f;
          stepWidth = 0.1f;
          zoom = 0.0f;
          oversampling = 2.0f;
          clipBoxSize = Vec3{1,1,1};
          clipBoxShift = Vec3{0,0,0};
          updateMatrices();
          break;
        case GLENV_KEY_UP:
          zoom += 0.1f;
          updateMatrices();
          break;
        case GLENV_KEY_DOWN:
          zoom -= 0.1f;
          updateMatrices();
          break;
      }
    }
    switch (key) {
      case GLENV_KEY_1:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxSize.x += 0.01f;
        else
          clipBoxSize.x -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_2:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxSize.y += 0.01f;
        else
          clipBoxSize.y -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_3:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxSize.z += 0.01f;
        else
          clipBoxSize.z -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_4:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxShift.x += 0.01f;
        else
          clipBoxShift.x -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_5:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxShift.y += 0.01f;
        else
          clipBoxShift.y -= 0.01f;
        updateMatrices();
        break;
      case GLENV_KEY_6:
        if (mods & GLENV_MOD_SHIFT)
          clipBoxShift.z += 0.01f;
        else
          clipBoxShift.z -= 0.01f;
        updateMatrices();
        break;
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
      updateMatrices();
    }
  }
  
  virtual void mouseButton(int button, int state, int mods, double xPosition, double yPosition) override {
    if (button == GLENV_MOUSE_BUTTON_RIGHT) {
      rightMouseDown = state == GLENV_MOUSE_PRESS;
      xPositionStart = xPosition;
      yPositionStart = yPosition;
    }
    
    if (button == GLENV_MOUSE_BUTTON_LEFT) {
      leftMouseDown = state == GLENV_MOUSE_PRESS;
      arcball.click({uint32_t(xPosition),uint32_t(yPosition)});
    }
  }

private:
  Tesselation cube{Tesselation::genBrick({0, 0, 0}, {1, 1, 1}).unpack()};
  GLBuffer vbCube{GL_ARRAY_BUFFER};
  GLArray cubeArray;
  GLProgram cubeProgram{GLProgram::createFromFile("cubeVS.glsl", "cubeFS.glsl")};
  size_t vertCount;
  Volume volume;
  Vec3 voxelCount;
  Vec3 volumeExtend;
  GLTexture3D volumeTex{GL_LINEAR, GL_LINEAR,GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE};

  ArcBall arcball{{512, 512}};
  Mat4 rotation;
  Mat4 clipBox;
  Mat4 modelViewProjection;
  Mat4 viewToTexture;
  Mat4 model;
  Mat4 view{Mat4::lookAt({ 0, 0, 2 }, { 0, 0, 0 }, { 0, 1, 0 })};
  Mat4 projection;

  Vec3 minBounds;
  Vec3 maxBounds;
  Vec3 clipBoxSize{1,1,1};
  Vec3 clipBoxShift{0,0,0};

  float oversampling{2.0f};
  float near{0.1f};
  float zoom{0.0f};

  bool meshNeedsUpdte{true};

  std::vector<std::string> filenames{"c60.dat","bonsai.dat"};
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
    Raycaster raycaster;
    raycaster.run();
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
