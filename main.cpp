/**
   main.cpp is the non mpi-compatible version of the path tracer. Path tracing is
   implemented via buffer maps. Supports free exploration of scene.

   @author George Tang
 */
#include "util/utilities.cpp"
#include "util/polygonMesh.cpp"
#include "util/lighting.cpp"

/**
   Stuff needed for raytracing. Initialized by reader.cpp.
 */
cv::Mat image(512, 512,
    CV_8UC3, cv::Scalar(210, 160, 30)); // image to write result to
Vec3d eye(0), background(1); // background(0.118, 0.627, 0.824); // position of eye; color of background
vector<PolygonMesh> obj; // scene polygonMeshes
vector<Light> light; // scene lights
double step, astep, angle, zoom; // camera param: step of camera during movement; angle step, angle, zoom factor
int MAX_DEPTH = 1; // max depth of reflections

#include "util/reader.cpp"
#include "util/kdtree.cpp"
KDNode* tree = new KDNode();

bool gi = false, rb = false; // global illumnination, ray-bundle tracing options

/**
   Buffer maps: diffuse, albedo*txt, normal, hit
 */
Vec3d mpnt[1024][1024];
Vec3d mclr[1024][1024];
Vec3d mnrm[1024][1024];
Vec3d mhit[1024][1024];

/**
   Recursive algorithm for ray casting.
   @param depth depth of recursion
   @param r, c (row, col) of pixel in image
 */
Vec3d castRay(Ray &ray, const int depth, const int r, const int c) {
	pii tind; pdd uv;
	double tmin = FLT_MAX;
	if (!search(tree, ray, tind, tmin, uv)) {
		if (depth == 0) {
			mhit[r][c] = Vec3d(-99999);
			return background;
		}
		return Vec3d(0);
	}
	Vec3d hit, nrm, txt;
	int reft;
	obj[tind.first].surfaceProperties(tind.second, ray, uv, nrm, txt, reft);
	hit = ray.src + tmin*ray.dir + 0.0001*nrm;
	Vec3d direct, ref = Vec3d();
	// Direct Lighting
	pii temp;
	for (int k = 0; k < light.size(); k++) {
		Vec3d ldir, shade;
		double dis;
		light[k].illuminate(hit, ldir, shade, dis);
		Ray lray(hit, ldir);
		bool vis = !search(tree, lray, temp, dis, uv);
		direct = direct + vis*max(0.0, dot(ldir, nrm))*shade;
	}
	// primary ray
	if (depth == 0) {
		mclr[r][c] = txt*obj[tind.first].albedo;
		mhit[r][c] = hit;
		mnrm[r][c] = nrm;
		// reflection
		if (reft) {
			Ray rray = Ray(hit, reflect(ray.dir, nrm));
			ref = ref+castRay(rray, depth+1, r, c);
		}
	}
	return 0.6*txt*obj[tind.first].albedo*direct/M_PI+0.4*ref;
}

/**
   Compute global illumination via Monte Carlo sampling.
   @param N num samples
   @param r, c (row, col) of pixel in image
 */
Vec3d globalIllumination(const int N, const int r, const int c) {
	Vec3d Nt, Nb;
	Vec3d indirect = Vec3d(0), nrm = mnrm[r][c], hit = mhit[r][c];
	if (hit.x == -99999 || N == 0)
		return Vec3d(0);
	createCoordSystem(nrm, Nt, Nb);
	for (int i = 0; i < N; i++) {
		double r1, r2;
		Vec3d sample = uniformSampleHemisphere(r1, r2);
		Vec3d world(sample.x * Nb.x + sample.y * nrm.x + sample.z * Nt.x,
		            sample.x * Nb.y + sample.y * nrm.y + sample.z * Nt.y,
		            sample.x * Nb.z + sample.y * nrm.z + sample.z * Nt.z);
		Ray iray(hit, world);
		indirect = indirect + r1*castRay(iray, -1, -1, -1);
	}
	return indirect;
}

/**
   Render the image. Computes direct lighting and other buffers; then computes
   global illumination and writes result to image. Option for ray-bundle tracing.
   @param src ray source (e.g. eye)
   @param zoom, angle same as camera param; determine view
 */
void render(const Vec3d &src, const double zoom, const double angle) {
	// #pragma omp parallel for
	for (int i = 0; i < image.rows; i++) {
		for (int j = 0; j < image.cols; j++) {
			Vec3d paint(0);
			// ray-bundle tracing with 9rppp
			double x = 0, y = 0;
			for (y = -0.01; y <= 0.01; y += 0.01)
				for (x = -0.01; x <= 0.01; x += 0.01) {
					if (!rb && (x != 0 || y != 0)) continue;
					Vec3d pxl(0.5-(double)(j+x)/image.rows, 0.5-(double)(i+y)/image.rows, zoom);
					Vec3d snk = Vec3d(src.x+sin(angle)*pxl.x+cos(angle)*pxl.z,  src.y+pxl.y, src.z-cos(angle)*pxl.x+sin(angle)*pxl.z);
					Ray ray(src, unit(snk - src));
					paint = paint + castRay(ray, 0, i, j);
				}
			if (rb) paint = paint/9.0;
			mpnt[i][j] = paint;
		}
	}
	// Global Illumination
	int Nsamples = 4;
	// #pragma omp parallel for
	if (gi)
		for (int i = 0; i < image.rows; i++)
			for (int j = 0; j < image.cols; j++)
				mpnt[i][j] = mpnt[i][j] + 2.0*mclr[i][j]*globalIllumination(Nsamples, i, j)/((double)Nsamples);
	// Set Color
	// #pragma omp parallel for
	for (int i = 0; i < image.rows; i++) {
		for (int j = 0; j < image.cols; j++) {
			cv::Vec3b& color = image.at<cv::Vec3b>(i, j);
			// OpenCV uses BGR
			Vec3d paint = mpnt[i][j];
			color[0] = min(255, (int)(255.0*paint.z));
			color[1] = min(255, (int)(255.0*paint.y));
			color[2] = min(255, (int)(255.0*paint.x));
		}
	}
}

/**
   Allow camera to explore freely in scene.
 */
void simulate() {
	while(true) {
		auto start = std::chrono::high_resolution_clock::now();
		render(eye, zoom, angle);
		auto finish = std::chrono::high_resolution_clock::now();
		chrono::duration<double> elapsed = finish - start;
		cout << "Elapsed time: " << elapsed.count() << " s\n";
		cv::imshow("Scene", image);
		int c = cv::waitKey(0);
		// Movement
		switch(c) {
			case 119: { // W
				eye = eye+Vec3d(cos(angle)*step, 0, sin(angle)*step);
				break;
			}
			case 97: { // A
				eye = eye-Vec3d(-sin(angle)*step, 0, cos(angle)*step);
				break;
			}
			case 115: { // S
				eye = eye-Vec3d(cos(angle)*step, 0, sin(angle)*step);
				break;
			}
			case 100: { // D
				eye = eye+Vec3d(-sin(angle)*step, 0, cos(angle)*step);
				break;
			}
			case 2: { // LEFT
				angle -= astep;
				break;
			}
			case 3: { // RIGHT
				angle += astep;
				break;
			}
		}
	}
}

int main(int argc, char** argv) {
	string name = "scenes/";
	// process terminal args
	for (int i = 1; i < argc; i++) {
		string in = string(argv[i]);
		if (in == "-gi") gi = true;
		if (in == "-rb") rb = true;
		if (in[0] != '-') name += in;
	}
	init(name);
	buildTree(tree);
	int sumT = 0;
	for (int i = 0; i < obj.size(); i++) sumT += obj[i].tris.size();
	cout << "Real Triangles: " << sumT << "; Naive: " << sum << "; Optimized: " << sum2 << endl;
	simulate();
}
