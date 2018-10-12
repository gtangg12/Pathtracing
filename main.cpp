#include "utilities.cpp"
#include "polygonMesh.cpp"
#include "lighting.cpp"

cv::Mat image(512, 512, CV_8UC3, cv::Scalar(210, 160, 30));
//Vec3d background(0.118, 0.627, 0.824);
Vec3d background(1);
vector<PolygonMesh> obj;
vector<Light> light;
double step, astep, angle, zoom;
Vec3d eye(0);

void init(string scene) {
   // Scene File Reader
   ifstream reader(scene+"/master.txt");
   string input;
   vector<string> files;
   unordered_map<string, string> tarr;
   while(getline(reader, input)) {
      vector<string> data;
      boost::split(data, input, boost::is_any_of(" "), boost::token_compress_on);
      char type = data[0][0];
      if (type == '#')
         continue;
      else if (type == '%') // force quit
         break;
      else if (type == 'C') {
         double ud = 0.01745329251;
         eye = Vec3d(stod(data[1]), stod(data[2]), stod(data[3]));
         step = stod(data[4]);
         astep = ud*stod(data[5]);
         angle = ud*stod(data[6]);
         zoom = stod(data[7]);
      }
      else if (type == 'T') {
         tarr[data[1]] = data[2];
      }
      else if (type == 'L') {
         light.push_back(Light(stod(data[8]),
                         Vec3d(stod(data[2]), stod(data[3]), stod(data[4])),
                         Vec3d(stod(data[5]), stod(data[6]), stod(data[7])),
                         data[1][0]));
      }
      else if (type == 'O') {
         files.push_back(data[1]);
      }
   }
   // OBJ File Reader
   for (int i=0; i<files.size(); i++) {
      PolygonMesh mesh(Vec3d(1));
      ifstream meshReader(scene+"/"+files[i]);
      string line;
      int tcnt = 0;
      int tval;
      while(getline(meshReader, line)) {
         vector<string> tkns;
         boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);
         char type = tkns[0][tkns[0].size()-1];
         switch(type) {
            case '%': {
               tcnt = -99999;
               break;
            }
            case 'g': {
               tval = -99999;
               if (tarr.find(tkns[1]) != tarr.end()) {
                  cv::Mat tmap = cv::imread(scene+"/"+tarr[tkns[1]]+".jpg", CV_LOAD_IMAGE_COLOR);
                  mesh.tmaps.push_back(tmap);
                  tval = tcnt++;
                  // interpolate by point
                  if (tkns.size()>2 && tkns[2][0] == '?')
                     tval=-tval-1;
               }
               break;
            }
            case 'v': {
               mesh.vert.push_back(Vec3d(stod(tkns[1]), stod(tkns[2]), stod(tkns[3])));
               break;
            }
            case 'n': {
               mesh.norm.push_back(Vec3d(stod(tkns[1]), stod(tkns[2]), stod(tkns[3])));
               break;
            }
            case 't': {
               mesh.text.push_back(pdd( max(0.0, min(0.99, stod(tkns[1]))) , max(0.0, min(0.99, stod(tkns[2]))) ));
               break;
            }
            case 'f': {
               vector<int> data;
               int num;
               int sz = tkns.size() - 1;
               for (int j=1; j<=sz; j++) {
                  vector<string> spc;
                  boost::split(spc, tkns[j], boost::is_any_of("/"), boost::token_compress_on);
                  for (int k=0; k<spc.size(); k++)
                     data.push_back(stoi(spc[k]));
                  num = spc.size();
               }
               // face, normal, texture
               for (int j=1; j<sz-1; j++) {
                  Triangle tri(Vec3i(data[0]-1, data[j*num]-1, data[(j+1)*num]-1), Vec3i(-1), Vec3i(-1), tval);
                  if (num > 2) {
                     tri.ti = Vec3i(data[1]-1, data[j*num+1]-1, data[(j+1)*num+1]-1);
                     tri.ni = Vec3i(data[2]-1, data[j*num+2]-1, data[(j+1)*num+2]-1);
                  }
                  else
                     tri.ni = Vec3i(data[1]-1, data[j*num+1]-1, data[(j+1)*num+1]-1);
                  mesh.tris.push_back(tri);
                  mesh.cent.push_back((mesh.vert[tri.vi.x]+mesh.vert[tri.vi.y]+mesh.vert[tri.vi.z])/3.0);
               }
               break;
            }
            default: {
                continue;
            }
         }
         if (tcnt == -99999) // continue force quit
            break;
      }
      obj.push_back(mesh);
   }
}

// KDTree
#include "kdtree.cpp"
KDNode* tree = new KDNode();
int MAX_DEPTH = 1;

Vec3d castRay(const Ray &ray, const int depth, Vec3d &mclr, Vec3d &mnrm, Vec3d &mhit) {
   pii tind; pdd uv;
   double tmin = FLT_MAX;
   if (!tree->search(ray, tind, tmin, uv)) {
      if (depth == 0) {
         mhit = Vec3d(-99999);
         return background;
      }
      return Vec3d(0);
   }
   Vec3d hit, nrm, txt;
   obj[tind.first].surfaceProperties(tind.second, ray, uv, nrm, txt);
   hit = ray.src + tmin*ray.dir + 0.0001*nrm;
   Vec3d direct = Vec3d();
   // Direct Lighting
   pii temp;
   for (int k=0; k<light.size(); k++) {
      Vec3d ldir, shade;
      double dis;
      light[k].illuminate(hit, ldir, shade, dis);
      Ray lray(hit, ldir);
      bool vis = !tree->search(lray, temp, dis, uv);
      direct = direct + vis*max(0.0, dot(ldir, nrm))*shade;
   }
   // GI Primary
   if (depth == 0) {
      mclr = txt*obj[tind.first].albedo;
      mnrm = nrm;
      mhit = hit;
   }
   return txt*obj[tind.first].albedo*direct/M_PI;
}

Vec3d temp;
// Global Illumination
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
      indirect = indirect + r1*castRay(iray, -1, temp, temp, temp);
   }
   return indirect;
}

Vec3d render(const Vec3d &src, const double zoom, const double angle, int i, int j) {
   Vec3d paint(0);
   // ray-bundle tracing
   double x = 0, y = 0;
   Vec3d mclr, mnrm, mhit;
   for (y=-0.1; y<=0.1; y+=0.1)
      for (x=-0.1; x<=0.1; x+=0.1) {
         if ((x == 0) ^ (y == 0))
            continue;
         Vec3d pxl(0.5-(double)(j+x)/image.rows, 0.5-(double)(i+y)/image.rows, zoom);
         Vec3d snk = Vec3d(src.x+sin(angle)*pxl.x+cos(angle)*pxl.z, src.y+pxl.y, src.z-cos(angle)*pxl.x+sin(angle)*pxl.z);
         Ray ray(src, unit(snk - src));
         if (x == 0  && y == 0)
            castRay(ray, 0, mclr, mnrm, mhit);
         else
            paint = paint + castRay(ray, 0, temp, temp, temp);
      }
   Vec3d direct = paint/4.0;
   // Global Illumination
   int Nsamples = 4;
   paint = direct+2.0*mclr*globalIllumination(Nsamples, mnrm, mhit)/((double)Nsamples);
   return paint;
}

// Parallel
#include <mpi.h>

int main(int argc, char** argv) {
   string name = "car";
   init(name);
   buildTree(tree);
   int c = 0, sumT = 0;
   for (int i=0; i<obj.size(); i++)
      sumT+=obj[i].tris.size();

   MPI_Init(NULL, NULL);
   int world_size, world_rank;
   MPI_Comm_size(MPI_COMM_WORLD, &world_size);
   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

   cout << "Process "+to_string(world_rank)+" read triangles: " << sumT << endl;

   double red[512];
   double green[512];
   double blue[512];
   int process[513]; // 1 extra for size
   int camera[4];
   int slaves = 1;

   if (world_rank == 0) {
      ifstream fin("path.txt");
      int N;
      fin >> N;
      int cnt = 0;
      string line;
      double angles[16] = {0, 10, 20, 30, 45, 60, 75, 90, 180, -90, -75, -60, -45, -30, -20, -10};
      for (int i=0; i<N; i++) {
         double a, b, c;
         fin >> a >> b >> c;
         vector<string> tkns;
         boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);
         for (int j=0; j<16; j++) {
            // arr for process allocation
            vector<int> parr[512];
            angle = -acos(-unit(Vec3d(a, 0, c)).x);
            angle += 0.01745329251*angles[j];
            camera[0] = a; camera[1] = b; camera[2] = c; camera[3] = angle;
            int pc = 0;
            // allocate work by rows
            for (int I=0; I<image.rows; I++) {
               parr[pc++].push_back(I);
               pc%=slaves;
            }
            // send work
            for (int i=0; i<slaves; i++) {
               int total = parr[i].size();
               // first one denotes size
               process[0] = total;
               for (int j=1; j<=total; j++)
                  process[j] = parr[i][j-1]; // j starts at 1 but parr vector starts at 0
               MPI_Send(&camera, 4, MPI_DOUBLE, i+1, 0, MPI_COMM_WORLD);
               MPI_Send(&process, 513, MPI_INT, i+1, 0, MPI_COMM_WORLD);
            }
            // recieve work
            for (int i=0; i<slaves; i++) {
               for (int j=0; j<parr[i].size(); j++) { // update row buffers according to parr (processes)
                  MPI_Recv(&red, 512, MPI_DOUBLE, i+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                  MPI_Recv(&blue, 512, MPI_DOUBLE, i+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                  MPI_Recv(&green, 512, MPI_DOUBLE, i+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                  for (int k=0; k<image.cols; k++) {
                     cv::Vec3b& color = image.at<cv::Vec3b>(parr[i][j], k);
                     // OpenCV uses BGR
                     color[0] = min(255, (int)(255.0*blue[k]));
                     color[1] = min(255, (int)(255.0*green[k]));
                     color[2] = min(255, (int)(255.0*red[k]));
                  }
               }
            }
            cv::imshow("Test", image);
            cv::waitKey(0);
            //cv::imwrite("CARDATA2/"+name+to_string(cnt)+".jpg", image);
            cnt++;
         }
      }
   }
   else {
      MPI_Recv(&camera, 4, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&process, 513, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      Vec3d eye = Vec3d(camera[0], camera[1], camera[2]);
      angle = camera[3];
      for (int i=0; i<process[0]; i++) {
         int row = process[i+1];
         for (int j=0; j<image.cols; j++) {
            //cout << row << ' ' << j << endl;
            Vec3d paint = render(eye, zoom, angle, row, j);
            red[j] = paint.x;
            blue[j] = paint.y;
            green[j] = paint.z;
         }
         MPI_Send(&red, 512, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
         MPI_Send(&blue, 512, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
         MPI_Send(&green, 512, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
      }
   }
   MPI_Finalize();
   return 0;
}
