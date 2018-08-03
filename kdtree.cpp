// Constants
#define LEAF 3
#define pms ind[i].first
#define trg ind[i].second

// Bounding Box
class Box {
public:
   Vec3d bnds[2];

   bool intersect(const Ray &ray) {
      double tmin, tmax, ymin, ymax, zmin, zmax;
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
};

int sum = 0;
// Acceleration Structure
class KDNode {
public:
   Box box;
   int axis;
   double splt;
   bool leaf;
   KDNode *left, *right;
   vector<pii> ind; // Mesh, Triangle

   KDNode() {}

   void build(const int depth) {
      leaf = false;
      if (ind.size()<=LEAF) {
         leaf = true;
         return;
      }
      left = new KDNode();
      right = new KDNode();
      axis = depth%3;
      // Find median NlogN
      vector<Vec3d> pnts;
      for (int i=0; i<ind.size(); i++)
         pnts.push_back(obj[pms].cent[trg]);
      sort(pnts.begin(), pnts.end(), func[axis]);
      splt = pnts[pnts.size()/2][axis];
      // Assign triangles
      for (int i=0; i<ind.size(); i++) {
         if (obj[pms].cent[trg][axis] <= splt)
            left->ind.push_back(ind[i]);
         else
            right->ind.push_back(ind[i]);
      }
      // Centers are the same
      if (max(left->ind.size(), right->ind.size()) == ind.size()) {
         leaf = true;
         return;
      }
      // Update bounding box
      copy(begin(box.bnds), end(box.bnds), begin(left->box.bnds));
      copy(begin(box.bnds), end(box.bnds), begin(right->box.bnds));
      left->box.bnds[1][axis] = splt;
      right->box.bnds[0][axis] = splt;
      // Expand boxes
      Triangle* temp;
      for (int i=0; i<left->ind.size(); i++) {
         temp = &obj[left->pms].tris[left->trg];
         for (int j=0; j<3; j++)
            left->box.bnds[1][axis] = max(left->box.bnds[1][axis], obj[left->pms].vert[temp->vi[j]][axis]);
      }
      for (int i=0; i<right->ind.size(); i++) {
         temp = &obj[right->pms].tris[right->trg];
         for (int j=0; j<3; j++)
            right->box.bnds[0][axis] = min(right->box.bnds[0][axis], obj[right->pms].vert[temp->vi[j]][axis]);
      }
      left->build(depth+1);
      right->build(depth+1);
   }

   bool search(const Ray &ray, pii &tind, double &tmin, pdd &uv, int depth) {
      if (!box.intersect(ray))
         return false;
      if (leaf) {
         // Check all triangles in leaf node
         double t = FLT_MAX;
         pdd bay;
         for (int i=0; i<ind.size(); i++)
            if (obj[pms].intersect(trg, ray, t, bay) && t<tmin) {
               tmin = t;
               uv = bay;
               tind = pii(pms, trg);
            }
         return t != FLT_MAX;
      }
      Vec3d temp = ray.src;
      if (temp[axis] <= splt)
         return left->search(ray, tind, tmin, uv, depth+1) || right->search(ray, tind, tmin, uv, depth+1);
      return right->search(ray, tind, tmin, uv, depth+1) || left->search(ray, tind, tmin, uv, depth+1);
   }
};
