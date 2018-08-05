#include "utilities.cpp"
#include "polygonMesh.cpp"
#include "lights.cpp"
#include <opencv2/opencv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

// Scene
cv::Mat image(500, 500, CV_8UC3, cv::Scalar(210, 160, 30));
Vec3d background(0.118, 0.627, 0.824);
vector<PolygonMesh> obj;
vector<Light> light;
double zoom = -1.0;
Vec3d eye(-2.0, 5.5, 27.5);

// KDTree
#include "KDTree.cpp"
KDNode* tree;
Vec3d vmin(0), vmax(0);

// Phong constants
int PN = 4; double Ka = 0.4, Kd = 0.45, Ks = 0.15;

// Initialization
void init(string scene) {
   // OBJ File Reader
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
         ifstream meshReader("OBJ/"+name);
         PolygonMesh mesh(Vec3d(stod(data[2]), stod(data[3]), stod(data[4])));
         string line;
         while(getline(meshReader, line)) {
            vector<string> tkns;
            boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);
            char type = tkns[0][tkns[0].size()-1];
            switch(type) {
               case 'v': {
                  mesh.vert.push_back(Vec3d(stod(tkns[3]), stod(tkns[2]), stod(tkns[1])));
                  break;
               }
               case 'n': {
                  mesh.norm.push_back(unit(Vec3d(stod(tkns[3]), stod(tkns[2]), stod(tkns[1]))));
                  break;
               }
               case 't': {
                  mesh.text.push_back(pff(stod(tkns[1]), stod(tkns[2])));
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
                     Triangle tri(Vec3i(data[0]-1, data[(j%sz)*num]-1, data[((j+1)%sz)*num]-1), Vec3i(-1), Vec3i(-1));
                     if (data.size()>1)
                        tri.ni = Vec3i(data[1]-1, data[(j%sz)*num+1]-1, data[((j+1)%sz)*num+1]-1);
                     if (data.size()>2)
                        tri.ti = Vec3i(data[2]-1, data[(j%sz)*num+2]-1, data[((j+1)%sz)*num+2]-1);
                     mesh.tris.push_back(tri);
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

// Naive
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

void buildTree() {
   vector<pii> v;
   for (int i=0; i<obj.size(); i++)
      for (int j=0; j<obj[i].tris.size(); j++)
         v.push_back(pii(i, j));
   for (int i=0; i<obj.size(); i++)
      for (int j=0; j<obj[i].vert.size(); j++)
         for (int k=0; k<3; k++) {
            vmin[k] = min(vmin[k], obj[i].vert[j][k]);
            vmax[k] = max(vmax[k], obj[i].vert[j][k]);
         }
   tree = new KDNode();
   tree->box.bnds[0] = vmin-Vec3d(0.01);
   tree->box.bnds[1] = vmax+Vec3d(0.01);
   tree->ind = v;
   tree->build(0);
}

Vec3d castRay(const Ray &ray, const int depth) {
   pii tind; pdd uv;
   double tmin = FLT_MAX;
   if (!tree->search(ray, tind, tmin, uv))
   //if (!trace(ray, tind, tmin, uv))
      return background;
   Vec3d hit, nrm;
   obj[tind.first].surfaceProperties(tind.second, ray, uv, nrm);
   nrm = -nrm;
   hit = ray.src + tmin*ray.dir + 0.01*nrm;

   // reflection
   // Vec3d ambient = obj->albedo*castRay(hit, reflect(ray, norm), depth-1)+(1.f-obj->albedo)*obj->color;

   Vec3d ambient = obj[tind.first].albedo;
   // Phong Model & shadows
   Vec3d diffuse = Vec3d(), specular = Vec3d();
   pii temp;
   for (int k=0; k<light.size(); k++) {
      Vec3d ldir, shade;
      double dis;
      light[k].illuminate(hit, ldir, shade, dis);
      Ray lray(hit, ldir);
      bool vis = !tree->search(lray, temp, dis, uv);
      diffuse = diffuse + vis*max(0.0, dot(ldir, nrm))*shade;
      specular = specular + vis*pow(max(0.0, dot(reflect(-ldir, nrm), -ray.dir)), PN)*shade;
   }
   return Ka*ambient+Kd*obj[tind.first].albedo*diffuse+Ks*specular;
   /*
   double ratio = max(0.0, dot(nrm, ray.dir));
   return ratio*obj[tind.first].albedo;
   */
}

void render() {
   for (int i=0; i<image.rows; i++)
      for (int j=0; j<image.cols; j++) {
         // cout << i <<  ' ' << j << endl;
         Vec3d pxl(eye.x+0.5-(double)j/image.rows, eye.y+0.5-(double)i/image.rows, eye.z+zoom);
         Ray ray(eye, unit(pxl - eye));
         Vec3d paint = castRay(ray, 1);
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
   buildTree();
   cout << "RENDER" << endl;
   render();
   //cv::imshow("Scene", image);
   //cv::imwrite("Images/"+name+".jpg", image);
   //cv::waitKey(0);
}
