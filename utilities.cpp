// Headers
// C
#include <stdlib.h>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <ctime>

// C++
#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace std;

// Types
typedef long long ll;
typedef unsigned char uc;
typedef pair<int, int> pii;
typedef pair<long, long> pll;
typedef pair<ll, ll> plll;
typedef pair<float, float> pff;
typedef pair<double, double> pdd;

// Structures
template<typename T> struct Vec3 {
   T x, y, z;
   Vec3(): x(0), y(0), z(0) {}
   Vec3(T x): x(x), y(x), z(x) {}
   Vec3(T x, T y, T z): x(x), y(y), z(z) {}
};

// Custom Types
typedef Vec3<uc> Vec3b;
typedef Vec3<short> Vec3s;
typedef Vec3<int> Vec3i;
typedef Vec3<long> Vec3l;
typedef Vec3<ll> Vec3ll;
typedef Vec3<float> Vec3f;
typedef Vec3<double> Vec3d;

// Algebra
template<typename T> Vec3<T> operator+(const Vec3<T> &u, const Vec3<T> &v) {
   return Vec3<T>(u.x + v.x, u.y + v.y, u.z + v.z);
}

template<typename T> Vec3<T> operator-(const Vec3<T> &u, const Vec3<T> &v) {
   return Vec3<T>(u.x - v.x, u.y - v.y, u.z - v.z);
}

template<typename T> Vec3<T> operator*(const T k, const Vec3<T> &u) {
   return Vec3<T>(k*u.x, k*u.y, k*u.z);
}

template<typename T> Vec3<T> operator*(const Vec3<T> &u, const Vec3<T> &v) {
   return Vec3<T>(u.x*v.x, u.y*v.y, u.z*v.z);
}

template<typename T> Vec3<T> operator/(const Vec3<T> &u, const T k) {
   return Vec3<T>(u.x/k, u.y/k, u.z/k);
}

template<typename T> Vec3<T> operator/(const Vec3<T> &u, const Vec3<T> &v) {
   return Vec3<T>(u.x/v.x, u.y/v.y, u.z/v.z);
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

// Physics
template<typename T> Vec3<T> reflect(const Vec3<T> &u, const Vec3<T> &n) {
   return u - 2*dot(u, n)*n;
}

// UNTESTED
template<typename T> Vec3<T> refract(const Vec3<T> &u, const Vec3<T> &n, const float r) {
   float c1 = dot(u, n);
   float e = 1.0/r;
   if (c1<0) c1 = -c1; else e = -r;
   float c2 = 1.0 - e*e*(1.0 - c1*c1);
   return c2 < 0 ? Vec3f() : e*u + (e*c1 - sqrt(c2))*n;
}

template<typename T> void print(const Vec3<T> &u) {
   cout << u.x << ' ' << u.y << ' ' << u.z << '\n';
}
