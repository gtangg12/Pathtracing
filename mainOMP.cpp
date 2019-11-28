/**
  mainOMP.cpp is the parallel implementation of main.cpp. It supports rendering
  of multiple positions of a scene, each at multiple viewing angles (training
  data for Precision Graphics), as well as parallel implementation of free exploration.
  Does not support buffers.

  Note: use mpirun ./mainOMP to run.
*/
#include "util/utilities.cpp"
#include "util/polygonMesh.cpp"
#include "util/lighting.cpp"

/**
  Stuff needed for raytracing. Initialized by reader.cpp.
*/
cv::Mat image(512, 512, CV_8UC3, cv::Scalar(210, 160, 30));
Vec3d eye(0), background(1); //background(0.118, 0.627, 0.824);
vector<PolygonMesh> obj;
vector<Light> light;
double step, astep, angle, zoom;
int MAX_DEPTH = 1;
string name; // name of scene

#include "util/reader.cpp"
#include "util/kdtree.cpp"
KDNode* tree = new KDNode();

Vec3d temp2;
// render multiple, global illumnination, ray-bundle tracing options, post-processing
bool rm = false, gi = false, rb = false, pp = false;

/**
  Recursive algorithm for ray casting.
  @param depth depth of recursion
  @param mclr, mnrm, mhit color at, normal at, and coordinates of intersection
*/
Vec3d castRay(Ray &ray, const int depth, Vec3d &mclr, Vec3d &mnrm, Vec3d &mhit) {
   pii tind; pdd uv;
   double tmin = FLT_MAX;
   if (!search(tree, ray, tind, tmin, uv)) {
      if (depth == 0) {
         mhit = Vec3d(-99999);
         return background;
      }
      return Vec3d(0);
   }
   Vec3d hit, nrm, txt;
   int reft;
   obj[tind.first].surfaceProperties(tind.second, ray, uv, nrm, txt, reft);
   hit = ray.src + tmin*ray.dir + 0.0001*nrm;
   Vec3d direct = Vec3d();
   Vec3d ref = Vec3d();
   // Direct Lighting
   pii temp;
   for (int k=0; k<light.size(); k++) {
      Vec3d ldir, shade;
      double dis;
      light[k].illuminate(hit, ldir, shade, dis);
      Ray lray(hit, ldir);
      bool vis = !search(tree, ray, tind, tmin, uv);
      direct = direct + vis*max(0.0, dot(ldir, nrm))*shade;
   }
   // GI Primary
   if (depth == 0) {
      mclr = txt*obj[tind.first].albedo;
      mnrm = nrm;
      mhit = hit;
      // reflection
      if (reft) {
         Ray rray = Ray(hit, reflect(ray.dir, nrm));
         ref = ref+castRay(rray, -1, temp2, temp2, temp2);
      }
   }
   if (depth == -1)
      return txt*obj[tind.first].albedo*direct/M_PI;
   return 0.6*txt*obj[tind.first].albedo*direct/M_PI+0.4*ref;
   //return txt*obj[tind.first].albedo*direct/M_PI;
}

/**
  Compute global illumination via Monte Carlo sampling.
  @param N num samples
  @param nrm, hit normal at and position of intersection
*/
Vec3d globalIllumination(const int N, Vec3d &nrm, Vec3d &hit) {
   Vec3d Nt, Nb;
   Vec3d indirect = Vec3d(0);
   if (hit.x == -99999 || N == 0)
      return Vec3d(0);
   createCoordSystem(nrm, Nt, Nb);
   for (int i=0; i<N; i++) {
      double r1, r2;
      Vec3d sample = uniformSampleHemisphere(r1, r2);
      Vec3d world(sample.x * Nb.x + sample.y * nrm.x + sample.z * Nt.x,
                  sample.x * Nb.y + sample.y * nrm.y + sample.z * Nt.y,
                  sample.x * Nb.z + sample.y * nrm.z + sample.z * Nt.z);
      Ray iray(hit, world);
      indirect = indirect + r1*castRay(iray, -1, temp2, temp2, temp2);
   }
   return indirect;
}

/**
  Render the image at pixel (i, j). Computes direct lighting and other buffers;
  then computes global illumination and writes result to image. Option for
  ray-bundle tracing.
  @param src ray source (e.g. eye)
  @param zoom, angle same as camera param; determine view
  @param i, j coordinates of pixel in image plane
*/
Vec3d render(const Vec3d &src, const double zoom, const double angle, int i, int j) {
   Vec3d paint(0);
   // ray-bundle tracing
   double x = 0, y = 0;
   Vec3d mclr, mnrm, mhit;
   int cnt = 0;
   for (y=-0.1; y<=0.1; y+=0.1)
      for (x=-0.1; x<=0.1; x+=0.1) {
         if (!rb && (x!=0 || y!=0)) continue;
         cnt++;
         Vec3d pxl(0.5-(double)(j+x)/image.rows, 0.5-(double)(i+y)/image.rows, zoom);
         Vec3d snk = Vec3d(src.x+sin(angle)*pxl.x+cos(angle)*pxl.z, src.y+pxl.y, src.z-cos(angle)*pxl.x+sin(angle)*pxl.z);
         Ray ray(src, unit(snk - src));
         if (x == 0 && y == 0)
            paint = paint + castRay(ray, 0, mclr, mnrm, mhit);
         else
            paint = paint + castRay(ray, 0, temp2, temp2, temp2);
      }
   Vec3d direct = paint;
   if (rb) direct = direct/9.0;
   // Global Illumination
   int Nsamples = 16;
   paint = direct;
   if (gi) paint = paint + 2.0*mclr*globalIllumination(Nsamples, mnrm, mhit)/((double)Nsamples);
   return paint;
}

/**
  Parallel implementation using Open_MPI, which creates seperate copies of the
  program and distributes it to the cores.
*/
#include <mpi.h>
#define MAX_CORES 512+5 // max num of cores, +5 for extra storage
#define WORKERS 5 // num working cores-1
int world_size, world_rank; // num processes, rank of process
#define NUM_ANGLES 16 // num of viewing angles per point on path
double angles[NUM_ANGLES] = {0, 22.5, 45, 67.5, 90, 112.5, 135, 157.5, 180, 202.5, 225, 247.5, 270, 292.5, 315, 337.5};

/**
  Create rendering in parallel given parameters
  @param a, b, c position of rendering in worlld
  @param view_angle idx of angle used in angles[]
*/
void renderParallel(double a, double b, double c, int view_angle) {
  int process[MAX_CORES]; // processes (rows) assigned to a core; process[0] is num tasks
  double red[MAX_CORES]; // color buffers updated per process
  double green[MAX_CORES];
  double blue[MAX_CORES];
  double camera[4]; // (x, y, z, angle)
  if (world_rank == 0) {
    //cout << "PING: MASTER (0)" << endl;
    vector<int> parr[MAX_CORES]; // arr for process allocation
    if (view_angle!=-1) { // for simulateParallel
      angle = -acos(-unit(Vec3d(a, 0, c)).x);
      angle += M_PI/180.0*angles[view_angle];
    }
    camera[0] = a; camera[1] = b; camera[2] = c; camera[3] = angle;
    int pc = 0; // = 24 // start node (tj cluster issue)
    for (int I=0; I<image.rows; I++) {
      parr[pc].push_back(I);
      pc++;
      pc%=WORKERS;
      //if (pc == 0) pc = 24; //skip cores on same node as master (tj cluster issue)
    }
    // send processes
   for (int A=0; A<WORKERS; A++) {
      int total = parr[A].size(); // number of processes
      process[0] = total;
      for (int B=1; B<=total; B++)
         process[B] = parr[A][B-1];
      MPI_Send(&camera, 4, MPI_DOUBLE, A+1, 0, MPI_COMM_WORLD);
      MPI_Send(&process, MAX_CORES, MPI_INT, A+1, 0, MPI_COMM_WORLD);
    }
    // recieve processes results
    for (int B=0; B<MAX_CORES; B++) { // update color buffers for every task; have to swap b/c MPI_Recv waits on unfinished node
      for (int A=0; A<WORKERS; A++) {
         if (B >= parr[A].size()) continue;
         MPI_Recv(&red, MAX_CORES, MPI_DOUBLE, A+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         MPI_Recv(&green, MAX_CORES, MPI_DOUBLE, A+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         MPI_Recv(&blue, MAX_CORES, MPI_DOUBLE, A+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         for (int k=0; k<image.cols; k++) {
            cv::Vec3b& color = image.at<cv::Vec3b>(parr[A][B], k);
            // OpenCV uses BGR
            color[0] = min(255, (int)(255.0*blue[k]));
            color[1] = min(255, (int)(255.0*green[k]));
            color[2] = min(255, (int)(255.0*red[k]));
          }
      }
    }
  }
  else {
    MPI_Recv(&camera, 4, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&process, MAX_CORES, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //cout << "PING: WORKER (" << world_rank << ")" << endl;
    Vec3d eye = Vec3d(camera[0], camera[1], camera[2]);
    angle = camera[3];
    for (int A=0; A<process[0]; A++) {
      int row = process[A+1];
      for (int J=0; J<image.cols; J++) {
          Vec3d paint = render(eye, zoom, angle, row, J);
          red[J] = paint.x;
          green[J] = paint.y;
          blue[J] = paint.z;
      }
      MPI_Send(&red, MAX_CORES, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
      MPI_Send(&green, MAX_CORES, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
      MPI_Send(&blue, MAX_CORES, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }
  }
}

/**
  Render all positions of scene specified in path.txt
*/
void renderMultiple() {
  ifstream fin("scenes/paths/"+name+"path.txt");
  int N; // num points in path specified in path.txt
  fin >> N;
  string line;
  for (int i=0; i<N; i++) {
    double a, b, c; // position specified in path.txt
    fin >> a >> b >> c;
    vector<string> tkns;
    boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);
    for (int j=0; j<NUM_ANGLES; j++) {
      auto start = std::chrono::high_resolution_clock::now();
      renderParallel(a, b, c, j);
      if (world_rank == 0) {
        auto finish = std::chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish - start;
        cout << "Elapsed time: " << elapsed.count() << " s\n";
        cv::imshow("Frame", image); // change to imwrite and remove waitKey for en masse renderings
        cv::waitKey(0);
      }
    }
  }
}

/**
  Libs to execute python script from c++ code. Used for executing post-processing.
*/
#include <stdio.h>
#include <Python.h>

/**
  Implementation of free exploration in parallel
*/
void simulateParallel() {
  if (pp) Py_Initialize();
  while(true) {
    if (pp) { // with post-processing
      auto start = std::chrono::high_resolution_clock::now();
      // render and save diffuse as auxiliary buffer for denoising model
      gi = false;
      renderParallel(eye.x, eye.y, eye.z, -1);
      if (world_rank == 0) cv::imwrite("demo/diffuse.png", image);
      gi = true;
      renderParallel(eye.x, eye.y, eye.z, -1);
      if (world_rank == 0) {
        cv::imwrite("demo/noisy_"+name+".png", image);
        auto finish = std::chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish - start;
        // run ml models
        char filename[] = "demo.py";
	      FILE* fp;
	      fp = _Py_fopen(filename, "r"); // denoise and apply superresolution
        PyRun_SimpleFile(fp, filename);
        cout << "Elapsed time: " << elapsed.count() << " s\n";
        cv::Mat display = cv::imread("demo/res.png");
        cv::imshow("Frame", display);
      }
    }
    else { // traditional parallel free exploration
      auto start = std::chrono::high_resolution_clock::now();
      renderParallel(eye.x, eye.y, eye.z, -1);
      if (world_rank == 0) {
        auto finish = std::chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish - start;
        cout << "Elapsed time: " << elapsed.count() << " s\n";
        cv::imshow("Frame", image);
        cv::waitKey(0);
      }
    }
    // movement
    if (world_rank == 0) {
      int c = cv::waitKey(0);
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
           angle-=astep;
           break;
        }
        case 3: { // RIGHT
           angle+=astep;
           break;
        }
      }
    }
  }
  if (pp) Py_Finalize();
}

int main(int argc, char** argv) {
  // process terminal args
  for (int i=1; i<argc; i++) {
    string in = string(argv[i]);
    if (in=="-rm") rm = true;
    if (in=="-gi") gi = true;
    if (in=="-rb") rb = true;
    if (in=="-pp") pp = true;
    if (in[0]!='-') name = in;
  }
  init("scenes/"+name);
  buildTree(tree);
  int c = 0, sumT = 0;
  for (int i=0; i<obj.size(); i++)
    sumT+=obj[i].tris.size();
  // MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  if (world_rank == 0) {
    cout << "Real Triangles: " << sumT << "; Naive: " << sum << "; Optimized: " << sum2 << endl;
    cout << "World Size: " << world_size << endl;
  }
  if (rm) renderMultiple();
  else simulateParallel();
  MPI_Finalize();
}
