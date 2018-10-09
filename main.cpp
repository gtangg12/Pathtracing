#include "utilities.cpp"
#include "polygonMesh.cpp"
#include "lighting.cpp"

cv::Mat image(500, 500, CV_8UC3, cv::Scalar(210, 160, 30));
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
                  if (tkns[2][0] == '?')
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

// Maps: diffuse, albedo*txt, normal, hit
Vec3d mpnt[1024][1024];
Vec3d mclr[1024][1024];
Vec3d mnrm[1024][1024];
Vec3d mhit[1024][1024];

Vec3d castRay(const Ray &ray, const int depth, const int r, const int c) {
   pii tind; pdd uv;
   double tmin = FLT_MAX;
   if (!tree->search(ray, tind, tmin, uv)) {
      if (depth == 0) {
         mhit[r][c] = Vec3d(-99999);
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
   // primary ray
   if (depth == 0) {
      mclr[r][c] = txt*obj[tind.first].albedo;
      mhit[r][c] = hit;
      mnrm[r][c] = nrm;
   }
   return txt*obj[tind.first].albedo*direct/M_PI;
}

// Global Illumination
Vec3d globalIllumination(const int N, const int r, const int c) {
   Vec3d Nt, Nb;
   Vec3d indirect = Vec3d(0), nrm = mnrm[r][c], hit = mhit[r][c];
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
      indirect = indirect + r1*castRay(iray, -1, -1, -1);
   }
   return 2.0*mclr[r][c]*indirect/((double)N);
}

void render(const Vec3d &src, const double zoom, const double angle) {
   //#pragma omp parallel for
   for (int i=0; i<image.rows; i++) {
      for (int j=0; j<image.cols; j++) {
         Vec3d paint(0);
         // ray-bundle tracing
         double x = 0, y = 0;
         //for (y=-0.1; y<=0.1; y+=0.2)
            //for (x=-0.1; x<=0.1; x+=0.2) {
               Vec3d pxl(0.5-(double)(j+x)/image.rows, 0.5-(double)(i+y)/image.rows, zoom);
               Vec3d snk = Vec3d(src.x+sin(angle)*pxl.x+cos(angle)*pxl.z,  src.y+pxl.y, src.z-cos(angle)*pxl.x+sin(angle)*pxl.z);
               Ray ray(src, unit(snk - src));
               paint = paint + castRay(ray, 0, i, j);
            //}
         //paint = paint/4.0;
         mpnt[i][j] = paint;
      }
   }
   cout << "Indirect Lighting" << endl;
   // Global Illumination
   int Nsamples = 8;
   //#pragma omp parallel for
   for (int i=0; i<image.rows; i++) {
      for (int j=0; j<image.cols; j++) {
         mpnt[i][j] = mpnt[i][j] + globalIllumination(Nsamples, i, j);
      }
   }
   // Set Color
   //#pragma omp parallel for
   for (int i=0; i<image.rows; i++) {
      for (int j=0; j<image.cols; j++) {
         cv::Vec3b& color = image.at<cv::Vec3b>(i, j);
         // OpenCV uses BGR
         color[0] = min(255, (int)(255.0*mpnt[i][j].z));
         color[1] = min(255, (int)(255.0*mpnt[i][j].y));
         color[2] = min(255, (int)(255.0*mpnt[i][j].x));
      }
   }
}

int main() {
   string name = "car";
   init(name);
   buildTree(tree);
   int c = 0, sumT = 0;
   for (int i=0; i<obj.size(); i++)
      sumT+=obj[i].tris.size();
   cout << "Triangles: " << sumT << endl;
   while(true) {
      println(eye);
      cout << angle << endl;
      auto start = std::chrono::high_resolution_clock::now();
      render(eye, zoom, angle);
      auto finish = std::chrono::high_resolution_clock::now();
      chrono::duration<double> elapsed = finish - start;
      cout << "Elapsed time: " << elapsed.count() << " s\n";
      cv::imshow("Scene", image);
      //cv::imwrite("Images/"+name+".jpg", image);
      c = cv::waitKey(0);
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
