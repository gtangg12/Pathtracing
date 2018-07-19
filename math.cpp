#include <cmath>
#include <cfloat>
#include <cstdlib>

#define PI 3.1415926
#define E 2.71828

template<typename T> struct Vec3 {

   T x, y, z;
   Vec3(): x(0), y(0), z(0) {}
   Vec3(T x, T y, T z): x(x), y(y), z(z) {}
};

typedef Vec3<float> Vec3f;

template<typename T> Vec3<T> operator+(const Vec3<T> &u, const Vec3<T> &v) {
   return Vec3<T>(u.x + v.x, u.y + v.y, u.z + v.z);
}

template<typename T> Vec3<T> operator-(const Vec3<T> &u, const Vec3<T> &v) {
   return Vec3<T>(u.x - v.x, u.y - v.y, u.z - v.z);
}

template<typename T> Vec3<T> operator*(T k, const Vec3<T> &u) {
   return Vec3<T>(k*u.x, k*u.y, k*u.z);
}

template<typename T> Vec3<T> operator/(const Vec3<T> &u, T k) {
   return Vec3<T>(u.x/k, u.y/k, u.z/k);
}

template<typename T> Vec3<T> operator-(const Vec3<T> &u) {
   return Vec3<T>(-u.x, -u.y, -u.z);
}

template<typename T> float len(const Vec3<T> &u) {
   return sqrt(u.x*u.x + u.y*u.y + u.z*u.z);
}

template<typename T> Vec3<T> unit(const Vec3<T> &u) {
   float m = len(u);
   return Vec3<T>(u.x/m, u.y/m, u.z/m);
}

template<typename T> T dot(const Vec3<T> &u, const Vec3<T> &v) {
   return u.x*v.x + u.y*v.y + u.z*v.z;
}

template<typename T> Vec3<T> cross(const Vec3<T> &u, const Vec3<T> &v) {
   return Vec3<T>(u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z, u.x*v.y-u.y*v.x);
}

template<typename T> Vec3<T> reflect(const Vec3<T> &u, Vec3<T> &n) {
   return u - 2*dot(u, n)*n;
}
