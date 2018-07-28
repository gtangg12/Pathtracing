// Constants
#define MINB 1e-8

// Objects
class Ray {
public:
   Vec3f src, dir, inv;
   bool sign[3]; // (+) if < 0 else (-)

   Ray(const Vec3f &src, const Vec3f &dir): src(src), dir(dir) {
      inv = 1.f/dir;
      sign[0] = signbit(dir.x);
      sign[1] = signbit(dir.y);
      sign[2] = signbit(dir.z);
   }
};

class Triangle {
public:
   Vec3i vi, ni, ti;

   Triangle(const Vec3i &vi, const Vec3i &ni, const Vec3i &ti): vi(vi), ni(ni), ti(ti) {}
};

class PolygonMesh {
public:
   Vec3f albedo;
   vector<Vec3f> vert;
   vector<Vec3f> norm;
   vector<pff> text;
   vector<Vec3f> cent;
   vector<Triangle> tris;

   PolygonMesh(const Vec3f &albedo): albedo(albedo) {}

   bool intersect(const int ind, const Ray &ray, float &t) {
      Vec3i V = tris[ind].vi;
      Vec3f ba = vert[V.y] - vert[V.x];
      Vec3f ca = vert[V.z] - vert[V.x];
      Vec3f P = cross(ray.dir, ca);
      float det = dot(P, ba);
      // Backface culling
      if (det<MINB)
         return false;
      Vec3f T = ray.src - vert[V.x];
      float u = dot(P, T)/det;
      if (u < 0.0 || u > 1.0)
         return false;
      Vec3f Q = cross(T, ba);
      float v = dot(Q, ray.dir)/det;
      if (v < 0.0 || u+v > 1.0)
         return false;
      t = dot(Q, ca)/det;
      return true;
   }

   void surfaceProperties(const int ind, const Ray &ray, Vec3f &nrm) {
      Vec3i V = tris[ind].vi;
      Vec3f ba = vert[V.y] - vert[V.x];
      Vec3f ca = vert[V.z] - vert[V.x];
      nrm = unit(cross(ca, ba));
   }
};
