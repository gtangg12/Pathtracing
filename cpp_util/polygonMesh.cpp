/**
   polygonMesh.cpp defines the objects used in raytracing

   @author George Tang
 */

/**
   Ray contains the info of a cast ray
 */
class Ray {
public:
	Vec3d src, dir, inv; // source, unit direction, and <1/dir.x, 1/dir.y, 1/dir.x>
	bool sign[3]; // sign of each component of dir; (+) if < 0 else (-)
	Ray(const Vec3d &src, const Vec3d &dir) : src(src), dir(dir) {
		inv = 1.0/dir;
		sign[0] = signbit(dir.x);
		sign[1] = signbit(dir.y);
		sign[2] = signbit(dir.z);
	}
};

/**
   Triangle is the building block of a PolygonMesh
 */
class Triangle {
public:
	Vec3i vi, ni, ti; // vertex idx, normal idx, texture idx
	int tm, reft; // texture map index, 0/1 is reflective?
	Triangle(const Vec3i &vi, const Vec3i &ni, const Vec3i &ti, const int tm, const int reft) :
        vi(vi), ni(ni), ti(ti), tm(tm), reft(reft) {}
};

/**
   PolygonMesh defines one object, including its mesh structure, texture, and
   other properties
 */
class PolygonMesh {
public:
	Vec3d albedo; // [0, 1]; how much light reflects off surface
	vector<Vec3d> vert; // verticies
	vector<Vec3d> norm; // normals
	vector<pdd> text; // texture coordinates
	vector<cv::Mat> tmaps; // texture map images (must be square)
	vector<Vec3d> cent; // centers of triangles; used in KDBuild
	vector<Triangle> tris; // triangles in the mesh
	PolygonMesh(const Vec3d &albedo) : albedo(albedo) {}

	/**
	   Moller–Trumbore fast ray-triangle intersection algorithm
	   @param ind idx of triangles
	   @param t distance to intersection
	   @param uv barycentric coordinates
	 */
	bool intersect(const int ind, const Ray &ray, double &t, pdd &uv) {
		Vec3i V = tris[ind].vi;
		Vec3d ba = vert[V.y] - vert[V.x];
		Vec3d ca = vert[V.z] - vert[V.x];
		Vec3d P = cross(ray.dir, ca);
		double det = dot(P, ba);
		// Backface culling
		// if (det < 1e-8)
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

	/**
	   Computes shading and texturing at point defined by barycentric coordinates on triangle
	   @param ind idx of triangle
	   @param uv, nrm, txt, reft barycentric coordinates, normal, and texture, is reflective?
	 */
	void surfaceProperties(const int ind, const Ray &ray, const pdd &uv, Vec3d &nrm, Vec3d &txt, int &reft) {
		// Normal
		Vec3i N = tris[ind].ni;
		double w = 1.0-uv.first-uv.second;
		nrm = unit(w*norm[N.x] + uv.first*norm[N.y] + uv.second*norm[N.z]);
		// No texture option
		int mn = tris[ind].tm;
		reft = tris[ind].reft;
		if (mn == -99999) {
			txt = Vec3d(1);
			return;
		}
		// Texture
		Vec3i T = tris[ind].ti;
		// Interpolate by Coordinate
		if (mn < 0) {
			mn = -(mn+1);
			cv::Mat &tref = tmaps[mn];
			int r = (text[T[0]].first*w + text[T[1]].first*uv.first + text[T[2]].first*uv.second)*tref.rows;
			int c = (text[T[0]].second*w + text[T[1]].second*uv.first + text[T[2]].second*uv.second)*tref.rows;
			cv::Vec3b temp = tref.at<cv::Vec3b>(cv::Point(r, c));
			txt = Vec3d(temp[2], temp[1], temp[0])/256.0;
		}
		else { // Interpolate by Color
			Vec3d color[3];
			cv::Mat &tref = tmaps[mn];
			for (int i = 0; i < 3; i++) {
				int r = 0, c = 0;
				if (T[i] != -1) {
					r = (int)(text[T[i]].first*(tref.rows));
					c = (int)(text[T[i]].second*(tref.cols));
				}
				cv::Vec3b temp = tref.at<cv::Vec3b>(cv::Point(min(max(0,r), tref.rows), min(max(0,c), tref.cols)));
				color[i] = Vec3d(temp[2], temp[1], temp[0]); // BGR
			}
			txt = (w*color[0] + uv.first*color[1] + uv.second*color[2])/256.0;
		}
	}
};
