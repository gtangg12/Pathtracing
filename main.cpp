#include "utilities.cpp"
#include "polygonMesh.cpp"
#include "lighting.cpp"

cv::Mat image(300, 300, CV_8UC3, cv::Scalar(210, 160, 30));
Vec3d background(0.118, 0.627, 0.824);
vector<PolygonMesh> obj;
vector<Light> light;
double zoom = -3.0;
Vec3d eye(-2.0, 3.0, 27.5);

void init(string scene) {
   // Scene File Reader
   ifstream reader("Scenes/"+scene+".txt");
   string input;
   while(getline(reader, input)) {
      vector<string> data;
      boost::split(data, input, boost::is_any_of(" "), boost::token_compress_on);
      char type = data[0][0];
      if (type == '#')
         continue;
      else if (type == 'O') {
         string name = data[1];
         cv::Mat tmap = cv::imread("TXT/"+data[5], CV_LOAD_IMAGE_COLOR);
         PolygonMesh mesh(Vec3d(1), tmap);
         // OBJ File Reader
         ifstream meshReader("OBJ/"+name);
         string line;
         while(getline(meshReader, line)) {
            vector<string> tkns;
            boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);
            char type = tkns[0][tkns[0].size()-1];
            switch(type) {
               case 'v': {
                  mesh.vert.push_back(Vec3d(stod(tkns[1]), stod(tkns[2]), stod(tkns[3])));
                  break;
               }
               case 'n': {
                  mesh.norm.push_back(Vec3d(stod(tkns[1]), stod(tkns[2]), stod(tkns[3])));
                  break;
               }
               case 't': {
                  mesh.text.push_back(pdd(stod(tkns[1]), stod(tkns[2])));
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
                  for (int j=1; j<sz-1; j++) {
                     Triangle tri(Vec3i(data[0]-1, data[j*num]-1, data[(j+1)*num]-1), Vec3i(-1), Vec3i(-1));
                     if (data.size()>1)
                        tri.ti = Vec3i(data[1]-1, data[j*num+1]-1, data[(j+1)*num+1]-1);
                     if (data.size()>2)
                        tri.ni = Vec3i(data[2]-1, data[j*num+2]-1, data[(j+1)*num+2]-1);
                     mesh.tris.push_back(tri);
                     //mesh.cent.push_back(mesh.vert[tri.vi.x]);
                     mesh.cent.push_back((mesh.vert[tri.vi.x]+mesh.vert[tri.vi.y]+mesh.vert[tri.vi.z])/3.0);
                  }
                  break;
               }
               default: {
                   continue;
               }
            }
         }
         obj.push_back(mesh);
      }
      else if (type == 'L') {
         light.push_back(Light(stod(data[8]),
                         Vec3d(stod(data[2]), stod(data[3]), stod(data[4])),
                         Vec3d(stod(data[5]), stod(data[6]), stod(data[7])),
                         data[1][0]));
      }
   }
}
bool trace(const Ray &ray, pii &tind, double &tmin, pdd &uv) {
   double t;
   bool found = false;
   pdd bay;
   for (int i=0; i<obj.size(); i++)
      for (int j=0; j<obj[i].tris.size(); j++)
         if (obj[i].intersect(j, ray, t, bay) && t>0 && t<tmin) {
            found = true;
            tmin = t;
            uv = bay;
            tind = pii(i, j);
         }
   return found;
}
// KDTree
#include "KDTree.cpp"
KDNode* tree = new KDNode();
int MAX_DEPTH = 1;

Vec3d castRay(const Ray &ray, const int depth) {
   pii tind; pdd uv;
   double tmin = FLT_MAX;
   if (!tree->search(ray, tind, tmin, uv)) {
   //if (!trace(ray, tind, tmin, uv)) {
      if (depth == 0)
         return background;
      return Vec3d(0);
   }
   Vec3d hit, nrm, txt;
   obj[tind.first].surfaceProperties(tind.second, ray, uv, nrm, txt);
   hit = ray.src + tmin*ray.dir + 0.001*nrm;
   Vec3d direct = Vec3d(), indirect = Vec3d();
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
   /*
   // Global Illumination
   if (depth < MAX_DEPTH) {
      // Indirect Lighting
      Vec3d Nt, Nb;
      createCoordSystem(nrm, Nt, Nb);
      for (int i=0; i<Nsamples; i++) {
         double r1, r2;
         Vec3d sample = uniformSampleHemisphere(r1, r2);
         Vec3d world(sample.x * Nb.x + sample.y * nrm.x + sample.z * Nt.x,
                     sample.x * Nb.y + sample.y * nrm.y + sample.z * Nt.y,
                     sample.x * Nb.z + sample.y * nrm.z + sample.z * Nt.z);
         Ray iray(hit, world);
         indirect = indirect + r1*castRay(iray, depth+1);
      }
   }*/
   return txt*obj[tind.first].albedo*(direct/M_PI + 2.0*indirect/(double)Nsamples);
}

void render() {
   for (int i=0; i<image.rows; i++)
      for (int j=0; j<image.cols; j++) {
         //cout << i <<  ' ' << j << endl;
         Vec3d pxl(eye.x+0.5-(double)j/image.rows, eye.y+0.5-(double)i/image.rows, eye.z+zoom);
         Ray ray(eye, unit(pxl - eye));
         Vec3d paint = castRay(ray, 0);
         cv::Vec3b& color = image.at<cv::Vec3b>(i, j);
         // OpenCV uses BGR
         color[0] = min(255, (int)(255.0*paint.z));
         color[1] = min(255, (int)(255.0*paint.y));
         color[2] = min(255, (int)(255.0*paint.x));
      }
}

int main() {
   string name = "fruit";
   cout << "READ" << endl;
   init(name);
   cout << "BUILD" << endl;
   buildTree(tree);
   cout << "RENDER" << endl;
   // Record start time
   auto start = std::chrono::high_resolution_clock::now();
   render();
   auto finish = std::chrono::high_resolution_clock::now();
   //cv::resize(image, image, cv::Size(400, 400), 2, 2, cv::INTER_CUBIC);
   cv::imshow("Scene", image);
   // Record end time
   chrono::duration<double> elapsed = finish - start;
   cout << "Elapsed time: " << elapsed.count() << " s\n";
   cv::imwrite("Images/"+name+".bmp", image);
   cv::waitKey(0);
}
