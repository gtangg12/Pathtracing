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
   int tm;
   Triangle(const Vec3i &vi, const Vec3i &ni, const Vec3i &ti, const int tm): vi(vi), ni(ni), ti(ti), tm(tm) {}
};

class PolygonMesh {
public:
   Vec3d albedo;
   vector<Vec3d> vert;
   vector<Vec3d> norm;
   vector<pdd> text;
   vector<cv::Mat> tmaps;
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
      //if (det<MINB)
         //return false;
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

   void surfaceProperties(const int ind, const Ray &ray, const pdd &uv, Vec3d &nrm, Vec3d &txt) {
      // Normal
      Vec3i N = tris[ind].ni;
      double w = 1.0-uv.first-uv.second;
      nrm = unit(w*norm[N.x] + uv.first*norm[N.y] + uv.second*norm[N.z]);
      // No texture option
      if (tris[ind].tm == -1) {
         txt = Vec3d(1);
         return;
      }
      // Texture
      Vec3i T = tris[ind].ti;
      Vec3d color[3];
      for (int i=0; i<3; i++) {
         cv::Mat &tref = tmaps[tris[ind].tm];
         // must be square
         int r = 0, c = 0;
         //cout << "HI" << endl;
         if (T[i] != -1) {
            r = (int)(text[T[i]].first*tref.rows);
            c = (int)(text[T[i]].second*tref.rows);
         }
         cv::Vec3b temp = tref.at<cv::Vec3b>(cv::Point(r, c));
         //cout << "BYE" << endl;
         color[i] = Vec3d(temp[2], temp[1], temp[0]); // BGR
      }
      txt = (w*color[0] + uv.first*color[1] + uv.second*color[2])/256.0;
   }
};
