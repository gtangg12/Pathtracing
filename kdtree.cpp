#define LEAF 5
#define pms ind[i].first
#define trg ind[i].second

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

   double surfaceArea() {
      double l = bnds[1].x - bnds[0].x;
      double w = bnds[1].y - bnds[0].y;
      double h = bnds[1].z - bnds[0].z;
      return 2.0*(l*w+w*h+l*h);
   }
};

// Stats
int tris = 0;
int sum = 0;

// SAH
double trav = 7.0;
double inst = 5.0;

class KDNode {
public:
   Box box;
   int axis;
   double splt;
   bool leaf;
   KDNode *left, *right;
   vector<pii> ind; // Mesh, Triangle
   KDNode(): leaf(false) {}

   void build(const int depth) {
      if (ind.size()<=LEAF) {
         sum+=ind.size();
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
      // Naive Median
      //splt = pnts[pnts.size()/2][axis];
      // Update bounding box
      copy(begin(box.bnds), end(box.bnds), begin(left->box.bnds));
      copy(begin(box.bnds), end(box.bnds), begin(right->box.bnds));
      // SA Heuristic
      double minv = DBL_MAX;
      double curr, cost;
      double sa = box.surfaceArea();
      for (int i=0; i<pnts.size(); i++) {
         curr = pnts[i][axis];
         left->box.bnds[1][axis] = curr;
         right->box.bnds[0][axis] = curr;
         double lsa = left->box.surfaceArea();
         double rsa = right->box.surfaceArea();
         cost = trav + lsa/sa*(i+1)*inst + rsa/sa*(pnts.size()-i-1)*inst;
         if (cost < minv) {
            minv = cost;
            splt = curr;
         }
      }
      left->box.bnds[1][axis] = splt;
      right->box.bnds[0][axis] = splt;
      // Assign triangles based on locations of vert relative to splt
      for (int i=0; i<ind.size(); i++) {
         Vec3i V = obj[pms].tris[trg].vi;
         bool l = false, r = false;
         for (int j=0; j<3; j++) {
            Vec3d pnt = obj[pms].vert[V[j]];
            l = pnt[axis] <= splt ? true : l;
            r = pnt[axis] >= splt ? true : r;
         }
         if (l) left->ind.push_back(ind[i]);
         if (r) right->ind.push_back(ind[i]);
      }

      // if centers are the same
      if (max(left->ind.size(), right->ind.size()) == ind.size()) {
         sum += ind.size();
         leaf = true;
         free(left);
         free(right);
         return;
      }

      left->build(depth+1);
      right->build(depth+1);
   }

   bool search(const Ray &ray, pii &tind, double &tmin, pdd &uv) {
      if (!box.intersect(ray))
         return false;
      if (leaf) {
         double t;
         bool found = false;
         pdd bay;
         for (int i=0; i<ind.size(); i++)
            if (obj[pms].intersect(trg, ray, t, bay) && t>0 && t<tmin) {
               Vec3d hit = ray.src+t*ray.dir;
               if (!(hit>=box.bnds[0] && hit<=box.bnds[1]))
                  continue;
               found = true;
               tmin = t;
               uv = bay;
               tind = pii(pms, trg);
            }
         return found;
      }
      Vec3d temp = ray.src;
      if (temp[axis] <= splt)
         return left->search(ray, tind, tmin, uv) || right->search(ray, tind, tmin, uv);
      return right->search(ray, tind, tmin, uv) || left->search(ray, tind, tmin, uv);
   }
};

void buildTree(KDNode* root) {
   vector<pii> v;
   for (int i=0; i<obj.size(); i++) {
      tris += obj[i].tris.size();
      for (int j=0; j<obj[i].tris.size(); j++)
         v.push_back(pii(i, j));
   }
   // Determine inital bounds
   Vec3d vmin(0), vmax(0);
   for (int i=0; i<obj.size(); i++)
      for (int j=0; j<obj[i].vert.size(); j++)
         for (int k=0; k<3; k++) {
            vmin[k] = min(vmin[k], obj[i].vert[j][k]);
            vmax[k] = max(vmax[k], obj[i].vert[j][k]);
         }
   root->box.bnds[0] = vmin-Vec3d(0.01);
   root->box.bnds[1] = vmax+Vec3d(0.01);
   root->ind = v;
   root->build(0);
}
