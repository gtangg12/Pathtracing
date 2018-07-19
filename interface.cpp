#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
#include "objects.cpp"

cv::Mat image(600, 600, CV_8UC3, cv::Scalar(210, 160, 30)); //cv::Scalar(211, 211, 211));
Vec3f background(210, 160, 30);
vector<Object*> objects;
vector<Light*> lights;
Vec3f eye(-1.0, 0.5, 0.5);
// Phong constants
int PN = 64; float Ka = 0.6; float Kd = 0.35; float Ks = 0.05;

int total = 0;
int cnt = 0;

void init(string name) {
   ifstream fin(name);
   int N;
   fin >> N;
   for (int i=0; i<N; i++) {
      char type;
      float a, b, c, o;
      fin >> type;
      Vec3f color;
      fin >> a; fin >> b; fin >> c;
      if (type!='L' && type!='D') fin >> o;
      color = Vec3f(a, b, c);
      switch (type) {
         case 'D': {
            float x, y, z, i;
            fin >> x; fin >> y; fin >> z, fin >> i;
            Vec3f direction(x, y, z);
            lights.push_back(new DistanceLight(i, color, direction));
            break;
         }
         case 'L': {
            float x, y, z, i;
            fin >> x; fin >> y; fin >> z, fin >> i;
            Vec3f position(x, y, z);
            lights.push_back(new PointLight(i, color, position));
            break;
         }
         case 'S': {
            float x, y, z, r;
            fin >> x; fin >> y; fin >> z, fin >> r;
            Vec3f center(x, y, z);
            objects.push_back(new Sphere(o, r, color, center));
            break;
         }
      }
   }
}

float trace(Object *&obj, const Vec3f &src, const Vec3f &ray, float range) {
   obj = NULL;
   float t_min = range;
   for (int k=0; k<objects.size(); k++) {
      float t = objects[k]->intersect(ray, src);
      if (t<t_min) {
         t_min = t;
         obj = objects[k];
      }
   }
   return t_min;
}

Vec3f castRay(const Vec3f &src, const Vec3f &ray, int depth) {
   total++;
   if (depth==0)
      return Vec3f();
   Object *obj, *temp;
   float t_min = trace(obj, src, ray, FLT_MAX);
   if (obj == NULL)
      return background;
   Vec3f hit, norm;
   obj->surfaceProp(t_min, src, ray, hit, norm);
   // reflection
   Vec3f ambient = obj->albedo*castRay(hit, reflect(ray, norm), depth-1)+(1.f-obj->albedo)*obj->color;
   // Phong Model & shadows
   bool vis = false;
   Vec3f diffuse = Vec3f(), specular = Vec3f();
   for (int k=0; k<lights.size(); k++) {
      Vec3f lightDir, shade;
      float distance = lights[k]->illuminate(ray, hit, norm, lightDir, shade);
      trace(temp, hit, lightDir, distance);
      vis = temp == NULL ? true:vis;
      diffuse = diffuse + vis*max(0.f, dot(lightDir, norm))*shade;
      specular = specular + vis*(float)pow(max(0.f, dot(reflect(-lightDir, norm), -ray)), PN)*shade;
   }
   return Ka*ambient+Kd*obj->albedo*diffuse+Ks*specular;
}

void render(const Vec3f &eye) {
   int amt = 1;
   for (int i=0; i<image.rows; i+=amt)
      for (int j=0; j<image.cols; j+=amt) {
         // camera rays
         Vec3f pxl(0, (float)(j)/image.rows, 1.0-(float)i/image.rows);
         Vec3f ray = unit(pxl - eye);
         Vec3f paint = castRay(eye, ray, 20);
         for (int a=0; a<amt; a++)
            for (int b=0; b<amt; b++) {
               cv::Vec3b& color = image.at<cv::Vec3b>(i+a, j+b);
               color[0] = min(255, (int)(paint.x+0.5));
               color[1] = min(255, (int)(paint.y+0.5));
               color[2] = min(255, (int)(paint.z+0.5));
         }
      }
}

int main() {
   init("data2.txt");
   render(eye);
   //cv::blur(image, image, cv::Size(2, 2), cv::Point(-1, -1));
   cv::imshow("Data", image);
   cv::imwrite("Test.jpg", image);
   cv::waitKey(0);
   cout << cnt << " out of " << total << ' ' << total-cnt << '\n';
   return 0;
}
