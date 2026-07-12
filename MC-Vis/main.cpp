#include <GLApp.h>
#include <Mat4.h>
#include <ArcBall.h>
#include <FontRenderer.h>

#include <algorithm>

#include "MC.inl"

class MarchingCubes : public GLApp {
public:
  std::vector<float> data;
  uint8_t currectMCCase{1};
  float eye{3.0f};
  ArcBall arcball{{512, 512}};
  Mat4 rotation;
  bool showText{true};
  bool leftMouseDown{false};
  bool modifiedTable{false};

  FontRenderer fr{"helvetica_neue.bmp", "helvetica_neue.pos"};
  std::shared_ptr<FontEngine> fe{nullptr};

  MarchingCubes(const std::vector<std::string>& args) :
  GLApp{800,800,1,"Marching Cubes Case Visualizer", true, false, false, args} {
    registerCommands();
  }

  void registerCommands() {
    interpreter.registerCommand("showText",
                                [this](bool show) {
      showText = show;
    });
    interpreter.registerCommand("setModifiedTable",
                                [this](bool modified) {
      modifiedTable = modified;
    });
    interpreter.registerCommand("setCase",
                                [this](uint32_t mcCase) {
      currectMCCase = uint8_t(mcCase);
    });
    interpreter.registerCommand("setEyeDist",
                                [this](float eyeDist) {
      eye = eyeDist;
    });
  }

  virtual void init() override {
    fe = fr.generateFontEngine();
    GL(glDisable(GL_CULL_FACE));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_BLEND));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));
    GL(glBlendEquation(GL_FUNC_ADD));
    setBackground(0.2f, 0.2f, 0.2f, 0.2f);
  }

  void drawCube() {
    std::vector<float> l;
    for (const auto& line : edgeToVertexTable) {
      const Vec3 a = vertexPosTable[line[0]];
      const Vec3 b = vertexPosTable[line[1]];

      l.push_back(a.x-0.5f); l.push_back(a.y-0.5f); l.push_back(a.z-0.5f);
      l.push_back(1); l.push_back(1); l.push_back(1); l.push_back(1);
      l.push_back(b.x-0.5f); l.push_back(b.y-0.5f); l.push_back(b.z-0.5f);
      l.push_back(1); l.push_back(1); l.push_back(1); l.push_back(1);
    }
    drawLines(l, LineDrawType::LIST, 5);

    std::vector<float> p;
    int bit = 0;
    for (const auto& v : vertexPosTable) {
      p.push_back(v.x-0.5f); p.push_back(v.y-0.5f); p.push_back(v.z-0.5f);

      bool bitIsSet = (currectMCCase & (uint8_t{1} << bit)) != 0;
      p.push_back(1);
      if (bitIsSet) {
         p.push_back(1); p.push_back(1);
      } else {
        p.push_back(0); p.push_back(0);
      }
      p.push_back(1.0);
      bit++;
    }

    GL(glDepthMask(GL_FALSE));
    drawPoints(p,50, true);
    GL(glDepthMask(GL_TRUE));
  }

  uint32_t drawIsosurface() {
    const auto& trisVerts = modifiedTable ? trisTable[currectMCCase] : origTrisTable[currectMCCase];

    std::vector<float> verts;
    std::vector<float> tris;
    std::vector<float> trisOutline;

    for (const auto& e : trisVerts) {
      auto vs = edgeToVertexTable[e];
      const Vec3 v = (vertexPosTable[vs[0]] +vertexPosTable[vs[1]]) * 0.5;

      verts.push_back(v.x-0.5f);
      verts.push_back(v.y-0.5f);
      verts.push_back(v.z-0.5f);
      verts.push_back(1.0f);
      verts.push_back(1.0f);
      verts.push_back(0.0f);
      verts.push_back(1.0f);

      tris.push_back(v.x-0.5f);
      tris.push_back(v.y-0.5f);
      tris.push_back(v.z-0.5f);
      tris.push_back(1.0f);
      tris.push_back(1.0f);
      tris.push_back(0.0f);
      tris.push_back(0.5f);
    }

    for (size_t i = 0;i<tris.size()/(7);i+=3) {
      for (size_t j = 1;j<3;j++) {
        trisOutline.push_back(tris[(i+j-1)*7+0]);
        trisOutline.push_back(tris[(i+j-1)*7+1]);
        trisOutline.push_back(tris[(i+j-1)*7+2]);
        trisOutline.push_back(1.0f);
        trisOutline.push_back(1.0f);
        trisOutline.push_back(1.0f);
        trisOutline.push_back(1.0f);

        trisOutline.push_back(tris[(i+j)*7+0]);
        trisOutline.push_back(tris[(i+j)*7+1]);
        trisOutline.push_back(tris[(i+j)*7+2]);
        trisOutline.push_back(1.0f);
        trisOutline.push_back(1.0f);
        trisOutline.push_back(1.0f);
        trisOutline.push_back(1.0f);
      }
      trisOutline.push_back(tris[(i+2)*7+0]);
      trisOutline.push_back(tris[(i+2)*7+1]);
      trisOutline.push_back(tris[(i+2)*7+2]);
      trisOutline.push_back(1.0f);
      trisOutline.push_back(1.0f);
      trisOutline.push_back(1.0f);
      trisOutline.push_back(1.0f);

      trisOutline.push_back(tris[(i)*7+0]);
      trisOutline.push_back(tris[(i)*7+1]);
      trisOutline.push_back(tris[(i)*7+2]);
      trisOutline.push_back(1.0f);
      trisOutline.push_back(1.0f);
      trisOutline.push_back(1.0f);
      trisOutline.push_back(1.0f);
    }


    GL(glDepthMask(GL_FALSE));
    drawPoints(verts, 20, true);
    drawTriangles(tris, TrisDrawType::LIST, false, false);
    GL(glDepthMask(GL_TRUE));
    drawLines(trisOutline, LineDrawType::LIST, 4);

    return uint32_t(tris.size() / (7*3));
  }

  void drawText(const uint32_t trisCount) {

    const auto& trisCase = trisTable[currectMCCase];
    const auto& origTrisCase = origTrisTable[currectMCCase];
    const bool caseDiffers = trisCase.size() != origTrisCase.size() ||
                             !std::equal(trisCase.begin(), trisCase.end(), origTrisCase.begin());
    const std::string differenceCase = caseDiffers ? " *" : "";

    std::stringstream ss;
    ss << "MC Case: " << +currectMCCase << " Triangles: " << trisCount << differenceCase;
    const Dimensions dim{ glEnv.getFramebufferSize() };
    fe->render(ss.str(), dim.aspect(), 0.03f, {0,-0.9f}, Alignment::Center, {1,1,1,1});
  }

  virtual void draw() override {
    setDrawProjection(Mat4::perspective(45, glEnv.getFramebufferSize().aspect(), 0.0001f, 100));
    setDrawTransform(Mat4::lookAt({0,0,eye},{0,0,0},{0,1,0}) * rotation);

    drawCube();
    const uint32_t trisCount = drawIsosurface();

    if (showText) drawText(trisCount);
  }
  
  virtual void keyboard(int key, int scancode, int action, int mods) override {

    if (action == GLENV_PRESS) {
      switch (key) {
        case GLENV_KEY_ESCAPE:
          closeWindow();
          break;
        case GLENV_KEY_LEFT:
          currectMCCase--;
          break;
        case GLENV_KEY_RIGHT:
          currectMCCase++;
          break;
        case GLENV_KEY_M:
          modifiedTable = !modifiedTable;
          break;
        case GLENV_KEY_T:
          showText = !showText;
          break;
        case GLENV_KEY_UP:
          eye *= 0.9f;
          break;
        case GLENV_KEY_DOWN:
          eye /= 0.9f;
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
