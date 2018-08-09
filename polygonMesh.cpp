#define MINB 1e-8

class Ray {
public:
   Vec3d src, dir, inv;
   bool sign[3]; // (+) if < 0 else (-)
   Ray(const Vec3d &src, const Vec3d &dir): src(src), dir(dir) {
      inv = 1.0/dir;
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
   Vec3d albedo;
   vector<Vec3d> vert;
   vector<Vec3d> norm;
   vector<pff> text;
   vector<Vec3d> cent;
   vector<Triangle> tris;
   PolygonMesh(const Vec3d &albedo): albedo(albedo) {}

   bool intersect(const int ind, const Ray &ray, double &t, pdd &uv) {
      Vec3i V = tris[ind].vi;
      Vec3d ba = vert[V.y] - vert[V.x];
      Vec3d ca = vert[V.z] - vert[V.x];
      Vec3d P = cross(ray.dir, ca);
      double det = dot(P, ba);
      // Backface culling
      // if (det<MINB)
         // return false;
      Vec3d T = ray.src - vert[V.x];
      double u = dot(P, T)/det;
      if (u < 0.0 || u > 1.0)
         return false;
      Vec3d Q = cross(T, ba);
      double v = dot(Q, ray.dir)/det;
      if (v < 0.0 || u + v > 1.0)
         return false;
      t = dot(Q, ca)/det;
      uv = pdd(u, v);
      return true;
   }

   void surfaceProperties(const int ind, const Ray &ray, const pdd &uv, Vec3d &nrm) {
      /*Vec3i N = tris[ind].ni;
      nrm = unit((1.0-uv.first-uv.second)*norm[N.x] + uv.first*norm[N.y] + uv.second*norm[N.z]);
      */
      Vec3i V = tris[ind].vi;
      Vec3d ba = vert[V.y] - vert[V.x];
      Vec3d ca = vert[V.z] - vert[V.x];
      nrm = unit(cross(ca, ba));
   }
};
