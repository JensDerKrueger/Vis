#include "Clipper.h"
#include <algorithm>
#include <functional>


static bool epsilonEqual(float a, float b) {
  return fabs(a-b) <= std::numeric_limits<float>::epsilon();
}

// Calculates the intersection point of a line segment lb->la which crosses the
// plane with normal 'n'.
static bool rayPlaneIntersection(const Vec3 &la,
                                 const Vec3 &lb,
                                 const Vec3 &n, const float D,
                                 Vec3 &hit)
{
  const float denom = Vec3::dot(n, (la - lb));

  if(epsilonEqual(denom, 0.0f)) {
    return false;
  }

  const float t = (Vec3::dot(n, la) + D) / denom;
  hit = la + (lb - la)*t;
  return true;
}


// Splits a triangle along a plane with the given normal.
// Assumes: plane's D == 0.
//          triangle does span the plane.
static void splitTriangle(Vec3 a,
                   Vec3 b,
                   Vec3 c,
                   float fa, 
                   float fb,
                   float fc,
                   const Vec3 &normal,
                   const float D,
                   std::vector<Vec3>& out,
                   std::vector<Vec3>& newVerts)
{
  // rotation / mirroring.
  //            c
  //           o          Push `c' to be alone on one side of the plane, making
  //          / \         `a' and `b' on the other.  Later we'll be able to
  // plane ---------      assume that there will be an intersection with the
  //        /     \       clip plane along the lines `ac' and `bc'.  This
  //       o-------o      reduces the number of cases below.
  //      a         b

  // if fa*fc is non-negative, both have the same sign -- and thus are on the
  // same side of the plane.
  if(fa*fc >= 0) {
    std::swap(fb, fc);
    std::swap(b, c);
    std::swap(fa, fb);
    std::swap(a, b);
  } else if(fb*fc >= 0) {
    std::swap(fa, fc);
    std::swap(a, c);
    std::swap(fa, fb);
    std::swap(a, b);
  }

  // Find the intersection points.
  Vec3 A, B;
  rayPlaneIntersection(a,c, normal,D, A);
  rayPlaneIntersection(b,c, normal,D, B);

  if(fc >= 0) {
    out.push_back(a); out.push_back(b); out.push_back(A);
    out.push_back(b); out.push_back(B); out.push_back(A);
  } else {
    out.push_back(A); out.push_back(B); out.push_back(c);
  }
  newVerts.push_back(A);
  newVerts.push_back(B);
}


std::vector<Vec3> Clipper::triPlane(std::vector<Vec3>& posData, const Vec3 &normal, const float D) {
  std::vector<Vec3> newVertices;
  std::vector<Vec3> out;
  if (posData.size() % 3 != 0) return newVertices;

  for(auto iter = posData.begin(); iter < (posData.end()-2); iter += 3) {

    const Vec3 &a = (*iter);
    const Vec3 &b = (*(iter+1));
    const Vec3 &c = (*(iter+2));

    float fa = Vec3::dot(normal, a) + D;
    float fb = Vec3::dot(normal, b) + D;
    float fc = Vec3::dot(normal, c) + D;
    if(fabs(fa) < (2 * std::numeric_limits<float>::epsilon())) { fa = 0; }
    if(fabs(fb) < (2 * std::numeric_limits<float>::epsilon())) { fb = 0; }
    if(fabs(fc) < (2 * std::numeric_limits<float>::epsilon())) { fc = 0; }
    if(fa >= 0 && fb >= 0 && fc >= 0) {        // trivial reject
      // discard -- i.e. do nothing / ignore tri.
      continue;
    } else if(fa <= 0 && fb <= 0 && fc <= 0) { // trivial accept
      out.push_back(a);
      out.push_back(b);
      out.push_back(c);
    } else { // triangle spans plane -- must be split.
      std::vector<Vec3> tris, newVerts;
      splitTriangle(a,b,c, fa,fb,fc,normal,D, tris, newVerts);

      // append triangles and vertices to lists
      out.insert(out.end(), tris.begin(), tris. end());
      newVertices.insert(newVertices.end(), newVerts.begin(), newVerts. end());
    }
  }

  posData = out;
  return newVertices;
}

struct compSorter {
  bool operator() (const Vec3& i, const Vec3& j) const {
    return i.x < j.x || (i.x == j.x && i.y < j.y) ||
           (i.x == j.x && i.y == j.y && i.z < j.z);
  }
};

static bool angleSorter(const Vec3& i, const Vec3& j, const Vec3& center, const Vec3& refVec, const Vec3& normal) {
  Vec3 vecI = Vec3::normalize(i-center);
  float cosI = Vec3::dot(refVec, vecI);
  float sinI = Vec3::dot(Vec3::cross(vecI, refVec), normal);

  Vec3 vecJ = Vec3::normalize(j-center);
  float cosJ = Vec3::dot(refVec, vecJ);
  float sinJ = Vec3::dot(Vec3::cross(vecJ, refVec), normal);

  float acI = atan2(sinI, cosI);
  float acJ = atan2(sinJ, cosJ);

  return acI > acJ;
}

static std::vector<Vec3> rawToVec3(const std::vector<float>& posData) {
  std::vector<Vec3> v(posData.size()/3);
  for (size_t i = 0;i<posData.size()/3;++i) {
    v[i] = Vec3{posData[i*3+0],posData[i*3+1],posData[i*3+2]};
  }
  return v;
}

static std::vector<float> vec3toRaw(const std::vector<Vec3>& posData) {
  std::vector<float> v(posData.size()*3);
  for (size_t i = 0;i<posData.size();++i) {
    v[i*3+0] = posData[i].x;
    v[i*3+1] = posData[i].y;
    v[i*3+2] = posData[i].z;
  }
  return v;
}


void Clipper::boxPlane(std::vector<Vec3>& posData, const Vec3 &normal, const float D) {

  std::vector<Vec3> newVertices = Clipper::triPlane(posData, normal, D);

  if (newVertices.size() < 3) return;
  
  // remove duplicate vertices
  std::sort(newVertices.begin(), newVertices.end(), compSorter());
  newVertices.erase(std::unique(newVertices.begin(), newVertices.end()),
                    newVertices.end());

  // sort counter clockwise
  using namespace std::placeholders;

  Vec3 center;
  for (const Vec3& vertex : newVertices) {
    center = center + vertex;
  }
  center = center / float(newVertices.size());

  std::sort(newVertices.begin(), newVertices.end(), std::bind(angleSorter, _1, _2, center, Vec3::normalize(newVertices[0]-center), normal ));

  // create a triangle fan with the newly created vertices to close the polytope
  for (auto vertex = newVertices.begin()+2;vertex<newVertices.end();++vertex) {
    posData.push_back(newVertices[0]);
    posData.push_back(*(vertex-1));
    posData.push_back(*(vertex));
  }

}

std::vector<float> Clipper::boxPlane(std::vector<float> posData,
                                     const Vec3 &normal, const float D) {
  std::vector<Vec3> vecPos = rawToVec3(posData);
  boxPlane(vecPos, normal, D);
  return vec3toRaw(vecPos);
}
