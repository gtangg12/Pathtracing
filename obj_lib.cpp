// Constants
#define MINB 1e-8

// Objects
struct Triangle {
public:
   // Vertex, Normal, Texture
   vector<Vec3i> specs;
};

class PolygonMesh {
public:
   Vec3f albedo;
   vector<Vec3f> vertex;
   vector<Vec3f> normal;
   vector<Triangle> triangle;

   bool intersect(const int ind, const Vec3f &src, const Vec3f &ray, float &t) {
      Vec3i vert = triangle[ind].specs[0];
      Vec3f ba = vertex[vert.y] - vertex[vert.x];
      Vec3f ca = vertex[vert.z] - vertex[vert.x];
      Vec3f P = cross(ray, ca);
      float det = dot(P, ba);
      // Backface culling
      if (det<MINB)
         return FLT_MAX;
      Vec3f T = src - vertex[vert.x];
      float u = dot(P, T)/det;
      if (u < 0.0 || u > 1.0)
         return FLT_MAX;
      Vec3f Q = cross(T, ba);
      float v = dot(Q, ray)/det;
      if (v < 0.0 || u+v > 1.0)
         return FLT_MAX;
      t = dot(Q, ca)/det;
      return true;
   }

   void surfaceProperties(const int ind, const Vec3f &ray, Vec3f &nrm) {
      Vec3i vert = triangle[ind].specs[0];
      Vec3f ba = vertex[vert.y] - vertex[vert.x];
      Vec3f ca = vertex[vert.z] - vertex[vert.x];
      nrm = unit(cross(ca, ba));
   }
};

// Acceleration Structure
class KDNode {
public:
   Vec3f vmin, vmax;
   int axis;
   float pos;
   KDNode *left, *right;
   // Mesh, Triangle
   vector<pii> &objects;

   bool intersect(const Vec3f &ray) {

   }

   KDNode* build(const vector<pii> &arr, const int depth) {

   }

   bool search(const pii &obj, const Vec3f &ray, float &tmin) {

   }
};
