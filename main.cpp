#include "util_lib.cpp"
#include "obj_lib.cpp"
#include <opencv2/opencv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

// Scene
cv::Mat image(400, 400, CV_8UC3, cv::Scalar(210, 160, 30));
Vec3f background(0.824, 0.627, 0.118);
vector<PolygonMesh> objects;
float zoom = 0.5;
Vec3f eye(10.5, 4.5, -15.0);
float sc = 1.0;

// KDTree
KDNode* tree;

// Initialization
void init(string scene) {
   ifstream nameReader(scene+".txt");
   vector<string> files;
   while(nameReader) {
      string name;
      nameReader >> name;
      if (name=="")
          break;
      files.push_back(name);
   }
   for (int i=1; i<files.size(); i++) {
      ifstream meshReader(files[0]+"/"+files[i]);
      PolygonMesh mesh;
      string line;
      while(getline(meshReader, line)) {
         if (line == "")
            continue;
         vector<string> tokens;
         boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
         char type = tokens[0][tokens[0].size()-1];
         switch(type) {
            case 'v': {
               mesh.vertex.push_back(Vec3f(stof(tokens[1]), stof(tokens[2]), stof(tokens[3])));
               break;
            }
            case 'n': {
               mesh.normal.push_back(unit(Vec3f(stof(tokens[1]), stof(tokens[2]), stof(tokens[3]))));
               break;
            }
            case 't': {
               break;
            }
            case 'f': {
               vector<int> data;
               int num;
               int sz = tokens.size() - 1;
               for (int j=1; j<=sz; j++) {
                  vector<string> spc;
                  boost::split(spc, tokens[j], boost::is_any_of("/"), boost::token_compress_on);
                  for (int k=0; k<spc.size(); k++)
                     data.push_back(stoi(spc[k]));
                  num = spc.size();
               }
               for (int j=0; j<sz; j++) {
                  Triangle t;
                  for (int k=0; k<num; k++)
                     t.specs.push_back(Vec3i(data[(j*num+k)]-1, data[((j+1)%sz)*num+k]-1, data[((j+2)%sz)*num+k]-1));
                  mesh.triangle.push_back(t);
               }
               break;
            }
            default: {
                continue;
            }
         }
      }
      // black color
      mesh.albedo = Vec3f(1.0);
      objects.push_back(mesh);
   }
}

// NAIVE
bool trace(const Vec3f &src, const Vec3f& ray, int &mesh, int &tri, float &tmin) {
   bool hit = false;
   float t = FLT_MAX;
   for (int i=0; i<objects.size(); i++)
      for (int j=0; j<objects[i].triangle.size(); j++)
         if (objects[i].intersect(j, src, ray, t) && t<tmin) {
            hit = true;
            tmin = t;
            mesh = i;
            tri = j;
         }
   return hit;
}

// Rendering
Vec3f castRay(const Vec3f &src, const Vec3f& ray, const int depth) {
   int mesh, tri;
   float tmin = FLT_MAX;
   if (!trace(src, ray, mesh, tri, tmin))
      return background;
   Vec3f hit, nrm;
   hit = src+tmin*ray;
   objects[mesh].surfaceProperties(tri, ray, nrm);
   float ratio = max(0.f, dot(nrm, ray));
   return ratio*objects[mesh].albedo;
}

void render() {
   for (int i=0; i<image.rows; i++)
      for (int j=0; j<image.cols; j++) {
         // camera rays
         cout << i <<  ' ' << j << endl;
         Vec3f pxl(eye.x+0.5-(float)j/image.rows, eye.y+0.5-(float)i/image.rows, eye.z+zoom);
         Vec3f ray = unit(pxl - eye);
         Vec3f paint = castRay(eye, ray, 1);
         cv::Vec3b& color = image.at<cv::Vec3b>(i, j);
         color[0] = min(255, (int)(255.0*paint.x));
         color[1] = min(255, (int)(255.0*paint.y));
         color[2] = min(255, (int)(255.0*paint.z));
      }
}

int main() {
   init("scene");
   render();
   cv::imshow("Scene", image);
   cv::imwrite("fruit.jpg", image);
   cv::waitKey(0);
}
