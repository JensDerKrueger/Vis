#include <GLApp.h>
#include <Tessellation.h>
#include <ArcBall.h>

#include <filesystem>

#include "Clipper.h"
#include "QVis.h"
#include "TransferFunction.h"

class Raycaster : public GLApp {
public:
    
  Raycaster(const std::vector<std::string>& args) :
  GLApp(512, 512, 1, "Raycaster", true, false, false, args)
  {
  }

  void updateMatrices() {
    clipBoxSize = Vec3::maxV({0,0,0},Vec3::minV({1,1,1}, clipBoxSize));
    clipBoxShift = Vec3::maxV((Vec3{1,1,1}-clipBoxSize)*-0.5,Vec3::minV((Vec3{1,1,1}-clipBoxSize)*0.5, clipBoxShift));
    clipBox = Mat4::translation(clipBoxShift) * Mat4::scaling(clipBoxSize);
    minBounds = clipBox * Vec3{-0.5,-0.5,-0.5} + 0.5f;
    maxBounds = clipBox * Vec3{ 0.5, 0.5, 0.5} + 0.5f;
    model = Mat4::translation(0,0,zoom) * rotation * Mat4::scaling(volumeExtend);
    const Mat4 modelView = view * model * clipBox;

    modelViewProjection = projection * modelView;
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
    try {
      volume = QVis{filenames[currentFile],false}.volume;
    } catch (const QVisFileException& e) {
      std::cout << "Cannot load volme:" << e.what() << std::endl;
      return;
    }

    voxelCount = Vec3{float(volume.width),float(volume.height),float(volume.depth)};
    volumeExtend = volume.scale*voxelCount/float(volume.maxSize);
    volumeTex.setData(volume.data,
                      uint32_t(volume.width),
                      uint32_t(volume.height),
                      uint32_t(volume.depth), 1);
  }

  void updateTransferFunction() {
    transferFunction.smoothStep(TransferFunction::Channel::R, stepStart, stepWidth);
    transferFunction.smoothStep(TransferFunction::Channel::G, stepStart, stepWidth);
    transferFunction.smoothStep(TransferFunction::Channel::B, stepStart, stepWidth);
    transferFunction.smoothStep(TransferFunction::Channel::A, stepStart, stepWidth);
  }

  virtual void reset() override {
    rotation = Mat4{};
    stepStart = 0.12f;
    stepWidth = 0.1f;
    updateTransferFunction();
    zoom = 0.0f;
    oversampling = 1.0f;
    clipBoxSize = Vec3{1,1,1};
    clipBoxShift = Vec3{0,0,0};
    updateMatrices();
  }

  virtual void init() override {
    loadVolume();

    vertCount = cube.getVertices().size();
    cubeArray.bind();
    vbCube.setData(cube.getVertices(), 3);
    cubeArray.connectVertexAttrib(vbCube, program, "vPos", 3);
    setBackground(0,0,1,1);
  }

  virtual void resize(const Dimensions winDim, const Dimensions fbDim) override {
    GLApp::resize(winDim, fbDim);

    arcball.setWindowSize({winDim.width,winDim.height});
    projection = Mat4{ Mat4::perspective(45, fbDim.aspect(), near, 100) };
    updateMatrices();
  }

  void setupShader() {
    program.enable();
    program.setTexture("volume",volumeTex,0);
    program.setTexture("transfer",transferFunction.getTexture(),1);
    program.setUniform("modelViewProjection", modelViewProjection);
    program.setUniform("clip", clipBox);
    program.setUniform("minBounds", minBounds);
    program.setUniform("maxBounds", maxBounds);
    program.setUniform("voxelCount", voxelCount);
    program.setUniform("cameraPosInTextureSpace", (viewToTexture * Vec4{0,0,0,1}).xyz);
    program.setUniform("oversampling", oversampling);
  }

  virtual void animate(double animationTime) override {
    clipCubeToNearplane();
  }

  virtual void draw() override {
    GL(glEnable(GL_CULL_FACE));
    GL(glCullFace(GL_BACK));
    GL(glEnable(GL_BLEND));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glBlendEquation(GL_FUNC_ADD));
    setupShader();
    cubeArray.bind();
    GL(glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertCount)));

    if (tfEditor) drawTF();
  }

  void drawTF() {
    GL(glDisable(GL_DEPTH_TEST));
    GL(glDisable(GL_CULL_FACE));

    drawRect(Vec4{1.0f,1.0f,1.0f,0.3f},br,tl);
    const std::vector<float> frame{
      br.x, br.y, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      br.x, tl.y, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      tl.x, tl.y, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
      tl.x, br.y, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    };

    const std::vector<Vec4>& data = transferFunction.getData();

    std::vector<float> r(data.size()*7);
    std::vector<float> g(data.size()*7);
    std::vector<float> b(data.size()*7);
    std::vector<float> a(data.size()*7);

    for (size_t i = 0;i<data.size();++i) {
      const float alpha = float(i)/float(data.size());
      const float xPos = br.x + alpha * (tl.x-br.x);
      const Vec4 elem = data[i];

      r[i*7+0] = xPos; r[i*7+1] = br.y + elem.r * (tl.y-br.y); r[i*7+2] = 0;
      r[i*7+3] = 1; r[i*7+4] = 0; r[i*7+5] = 0; r[i*7+6] = 1.0;

      g[i*7+0] = xPos; g[i*7+1] = br.y + elem.g * (tl.y-br.y); g[i*7+2] = 0;
      g[i*7+3] = 0; g[i*7+4] = 1; g[i*7+5] = 0; g[i*7+6] = 1.0;

      b[i*7+0] = xPos; b[i*7+1] = br.y + elem.b * (tl.y-br.y); b[i*7+2] = 0;
      b[i*7+3] = 0; b[i*7+4] = 0; b[i*7+5] = 1; b[i*7+6] = 1.0;

      a[i*7+0] = xPos; a[i*7+1] = br.y + elem.a * (tl.y-br.y); a[i*7+2] = 0;
      a[i*7+3] = 1; a[i*7+4] = 1; a[i*7+5] = 1; a[i*7+6] = 1.0;
    }

    drawLines(r, LineDrawType::STRIP, activeChannel == TransferFunction::Channel::R ? 4 :2);
    drawLines(g, LineDrawType::STRIP, activeChannel == TransferFunction::Channel::G ? 4 :2);
    drawLines(b, LineDrawType::STRIP, activeChannel == TransferFunction::Channel::B ? 4 :2);
    drawLines(a, LineDrawType::STRIP, activeChannel == TransferFunction::Channel::A ? 4 :2);

    drawLines(frame, LineDrawType::LOOP, 4);
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
        case GLENV_KEY_T:
          tfEditor = !tfEditor;
          break;
        case GLENV_KEY_A:
          activeChannel = TransferFunction::Channel((int(activeChannel)+1)%4);
          break;
        case GLENV_KEY_P:
          std::cout << transferFunction.encodeForUrl() << std::endl;
          break;
        case GLENV_KEY_R:
          reset();
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
      updateTransferFunction();
    }
    
    if (leftMouseDown) {
      if (tfEditor) {
        const Vec2 bias  = (br)/2.0f+0.5f;
        const Vec2 scale = (tl-br)/2.0f;
        const Dimensions dim = glEnv.getWindowSize();
        const float x = (float(xPosition/dim.width)-bias.x)/scale.x;
        const float y = ((1.0f-float(yPosition/dim.height))-bias.y)/scale.y;
        transferFunction.setValue(x, y, activeChannel);
      } else {
        const Quaternion q = arcball.drag({uint32_t(xPosition),uint32_t(yPosition)});
        arcball.click({uint32_t(xPosition),uint32_t(yPosition)});
        rotation = q.computeRotation() * rotation;
        updateMatrices();
      }
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
  Tessellation cube{Tessellation::genBrick({0, 0, 0}, {1, 1, 1}).unpack()};
  GLBuffer vbCube{GL_ARRAY_BUFFER};
  GLArray cubeArray;
  GLProgram program{GLProgram::createFromFile("cubeVS.glsl", "cubeFS.glsl","",true, true)};
  size_t vertCount;
  Volume volume;
  TransferFunction transferFunction{256};
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

  bool tfEditor{false};
  const Vec2 br{-0.9f,-0.9f};
  const Vec2 tl{ 0.9f,-0.6f};
  TransferFunction::Channel activeChannel = TransferFunction::Channel::A;

  float oversampling{1.0f};
  float near{0.1f};
  float zoom{0.0f};

  bool meshNeedsUpdte{true};

  std::vector<std::string> filenames{"aneurism.dat","c60.dat","bonsai.dat"};
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
  std::vector<std::string> args = getArgsWindows();
#else
int main(int argc, char** argv) {
  std::vector<std::string> args{argv + 1, argv + argc};
#endif
  try {
    Raycaster raycaster{args};
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

