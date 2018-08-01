#include "utilities.cpp"
#include "objects.cpp"
#include <opencv2/opencv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

// Scene
cv::Mat image(500, 500, CV_8UC3, cv::Scalar(210, 160, 30));
Vec3d background(0.824, 0.627, 0.118);
vector<PolygonMesh> obj;
float zoom = -1.0;
Vec3d eye(-0.025, 0.125, 0.25);
double sc = 1.0;

// KDTree
#include "kdtree.cpp"
KDNode* tree;

// Initialization
void init(string scene) {
   ifstream nameReader(scene+".txt");
   vector<string> files;
   while(nameReader) {
      string name;
      nameReader >> name;
      if (name == "")
          break;
      files.push_back(name);
   }
   for (int i=1; i<files.size(); i++) {
      ifstream meshReader("OBJ/"+files[0]+"/"+files[i]);
      PolygonMesh mesh(Vec3d(1.0));
      string line;
      while(getline(meshReader, line)) {
         if (line == "")
            break;
         vector<string> tkns;
         boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);
         char type = tkns[0][tkns[0].size()-1];
         switch(type) {
            case 'v': {
               mesh.vert.push_back(Vec3d(stof(tkns[1]), stof(tkns[2]), stof(tkns[3])));
               break;
            }
            case 'n': {
               mesh.norm.push_back(unit(Vec3d(stof(tkns[1]), stof(tkns[2]), stof(tkns[3]))));
               break;
            }
            case 't': {
               mesh.text.push_back(pff(stof(tkns[1]), stof(tkns[2])));
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
}

bool trace(const Ray &ray, pii &tind, double &tmin) {
   bool hit = false;
   double t;
   for (int i=0; i<obj.size(); i++)
      for (int j=0; j<obj[i].tris.size(); j++)
         if (obj[i].intersect(j, ray, t) && t<tmin) {
            hit = true;
            tmin = t;
            tind = pii(i, j);
         }
   return hit;
}

void buildTree() {
   vector<pii> v;
   for (int i=0; i<obj.size(); i++)
      for (int j=0; j<obj[i].tris.size(); j++)
         v.push_back(pii(i, j));
   tree = new KDNode();
   tree->box.bnds[0] = Vec3d(-100.0, -100.0, -100.0);
   tree->box.bnds[1] = Vec3d(100.0, 100.0, 100.0);
   tree->ind = v;
   tree->build(0);
}

// Rendering
Vec3d castRay(const Ray &ray, const int depth) {
   pii tind;
   double tmin = FLT_MAX;
   if (!tree->search(ray, tind, tmin))
   //if (!trace(ray, tind, tmin))
      return background;
   Vec3d hit, nrm;
   hit = ray.src+tmin*ray.dir;
   obj[tind.first].surfaceProperties(tind.second, ray, nrm);
   double ratio = max(0.0, dot(nrm, ray.dir));
   return ratio*obj[tind.first].albedo;
}

void render() {
   for (int i=0; i<image.rows; i++)
      for (int j=0; j<image.cols; j++) {
         // camera rays
         cout << i <<  ' ' << j << endl;
         Vec3d pxl(eye.x+0.5-(double)j/image.rows, eye.y+0.5-(double)i/image.rows, eye.z+zoom);
         Ray ray(eye, unit(pxl - eye));
         Vec3d paint = castRay(ray, 1);
         cv::Vec3b& color = image.at<cv::Vec3b>(i, j);
         color[0] = min(255, (int)(255.0*paint.x));
         color[1] = min(255, (int)(255.0*paint.y));
         color[2] = min(255, (int)(255.0*paint.z));
      }
}

int main() {
   string name = "bunny";
   init(name);
   buildTree();
   render();
   cv::imshow("Scene", image);
   cv::imwrite(name+".jpg", image);
   cv::waitKey(0);
}
