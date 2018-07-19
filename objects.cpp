#include <algorithm>
#include <vector>
#include "math.cpp"

using namespace std;

// lights
class Light {
public:
   float intensity;
   Vec3f color;

   virtual float illuminate(const Vec3f &, const Vec3f &, const Vec3f &, Vec3f &, Vec3f &) = 0;
};

class DistanceLight: public Light {
public:
   Vec3f direction;

   DistanceLight(float intensity, const Vec3f &color, const Vec3f &direction) {
      this->intensity = intensity;
      this->color = color;
      this->direction = direction;
   }

   float illuminate(const Vec3f &ray, const Vec3f &hit, const Vec3f &norm, Vec3f &lightDir, Vec3f &shade) {
      lightDir = unit(direction);
      shade = intensity*color;
      return FLT_MAX;
   }
};

class PointLight: public Light {
public:
   Vec3f position;

   PointLight(float intensity, const Vec3f &color, const Vec3f &position) {
      this->intensity = intensity;
      this->color = color;
      this->position = position;
   }

   float illuminate(const Vec3f &ray, const Vec3f &hit, const Vec3f &norm, Vec3f &lightDir, Vec3f &shade) {
      lightDir = position - hit;
      float distance = len(lightDir);
      lightDir = unit(lightDir);
      shade = intensity*color/(float)(4.0*distance*distance*PI);
      return distance;
   }
};

// rendered objects
class Object {
public:
   float albedo;
   Vec3f color;

   virtual float intersect(const Vec3f &, const Vec3f &) = 0;
   virtual void surfaceProp(float, const Vec3f &, const Vec3f &, Vec3f &, Vec3f &) = 0;
};

class Sphere: public Object {
public:
   float radius;
   Vec3f center;

   Sphere(float albedo, float radius, const Vec3f &color, const Vec3f &center) {
      this->albedo = albedo;
      this->radius = radius;
      this->color = color;
      this->center = center;
   }

   float intersect(const Vec3f &ray, const Vec3f &orig) {
      Vec3f v = center - orig;
      float proj = dot(v, ray);
      if (proj < 0)
         return FLT_MAX;
      float l2 = dot(v, v) - proj*proj;
      if (l2 > radius*radius)
         return FLT_MAX;
      return proj - sqrt(radius*radius - l2);
   }

   void surfaceProp(float t_min, const Vec3f &src, const Vec3f &ray, Vec3f &hit, Vec3f &norm) {
      hit = src+t_min*ray;
      norm = unit(hit-center);
      hit = hit + (float)0.01*norm;
   }
};
