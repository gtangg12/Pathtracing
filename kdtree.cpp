// Acceleration Structure
class KDNode {
public:
   Vec3f bnds[2];
   int axis;
   float pos;
   KDNode *left, *right;
   // Mesh, Triangle
   vector<pii> objects;

   KDNode() {}
   //UNTESED

   bool intersect(const Ray &ray) {

      float tmin, tmax, ymin, ymax, zmin, zmax;
      tmin = (bnds[ray.sign[0]].x - ray.src.x)*ray.inv.x;
      tmax = (bnds[1-ray.sign[0]].x - ray.src.x)*ray.inv.x;
      ymin = (bnds[ray.sign[1]].y - ray.src.y)*ray.inv.y;
      ymax = (bnds[1-ray.sign[1]].y - ray.src.y)*ray.inv.y;
      if ((tmin > ymax) || (ymin > tmax))
         return false;
      if (ymin > tmin)
         tmin = ymin;
      if (ymax < tmax)
         tmax = ymax;
      zmin = (bnds[ray.sign[2]].z - ray.src.z) * ray.inv.z;
      zmax = (bnds[1-ray.sign[2]].z - ray.src.z) * ray.inv.z;
      if ((tmin > zmax) || (zmin > tmax))
         return false;
      return true;
   }

   KDNode* build(const vector<pii> &arr, const int depth) {
      
   }

   bool search(const pii &obj, const Vec3f &ray, float &tmin) {

   }
};
