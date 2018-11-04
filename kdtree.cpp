#define LEAF 3
#define pms ind[i].first
#define trg ind[i].second

class Box {
public:
   Vec3d bnds[2];

   bool intersect(Ray &ray, double &ent, double &ext) {
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
      ent = tmin;
      ext = tmax;
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
int sum2 = 0;

// SAH
double trav = 5.0;
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

      // pull up
      vector<pii> ls;
      set<pii> rs;
      for (int i=0; i<left->ind.size(); i++)
         ls.push_back(left->ind[i]);
      for (int i=0; i<right->ind.size(); i++)
         rs.insert(right->ind[i]);
      ind.clear();
      left->ind.clear();
      right->ind.clear();
      for (int i=0; i<ls.size(); i++)
         if (rs.find(ls[i]) != rs.end()) {
            rs.erase(ls[i]);
            ind.push_back(ls[i]);
         }
         else
            left->ind.push_back(ls[i]);
      right->ind = vector<pii>(rs.begin(), rs.end());
      sum2 += left->ind.size() + right->ind.size();
   }

   bool searchNode(Ray &ray, pii &tind, double &tmin, pdd &uv) {
      bool found = false;
      if (ind.size() > 0) {
         double t;
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
      }
      return found;
   }

   bool search(Ray &ray, pii &tind, double &tmin, pdd &uv) {
      double ent, ext;
      if (!box.intersect(ray, ent, ext))
         return false;
      bool found = searchNode(ray, tind, tmin, uv);
      if (leaf)
         return found;
      Vec3d hit = ray.src+tmin*ray.dir;
      if (ray.src[axis] <= splt) {
         if (found && hit>=left->box.bnds[0] && hit<=left->box.bnds[1])
            return left->search(ray, tind, tmin, uv) || found;
         return left->search(ray, tind, tmin, uv) || right->search(ray, tind, tmin, uv) || found;
      }
      if (found && hit>=right->box.bnds[0] && hit<=right->box.bnds[1])
         return right->search(ray, tind, tmin, uv) || found;
      return right->search(ray, tind, tmin, uv) || left->search(ray, tind, tmin, uv) || found;
      /*
      if (leaf)
         return searchNode(ray, tind, tmin, uv);
      if (ray.src[axis] <= splt)
         return left->search(ray, tind, tmin, uv) || right->search(ray, tind, tmin, uv);
      return right->search(ray, tind, tmin, uv) || left->search(ray, tind, tmin, uv);
      */
   }
};

// Recursive Algorithm to minimize ray-box intersections
class Block {
public:
   KDNode *node;
   double ent, ext;
   Block(KDNode *node, const double ent, const double ext): node(node), ent(ent), ext(ext) {}
};

bool search(KDNode *root, Ray &ray, pii &tind, double &tmin, pdd &uv) {
   double ent, ext, s, tdir, tmid;
   int a;
   KDNode *curr = root;
   KDNode *near, *far;
   if (!root->box.intersect(ray, ent, ext))
      return false;
   stack<Block> stk;
   stk.push(Block(root, ent, ext));
   bool found = false;
   while (!stk.empty()) {
      Block b = stk.top();
      stk.pop();
      curr = b.node;
      ent = b.ent; ext = b.ext;
      while(!curr->leaf) {
         found |= curr->searchNode(ray, tind, tmin, uv);
         s = curr->splt;
         a = curr->axis;
         tdir = ray.dir[a] == 0 ? 0.0000001 : ray.dir[a];
         tmid = (s-ray.src[a])/tdir;
         near = curr->left; far = curr->right;
         if (ray.src[a] > s)
            swap(near, far);
         if (tmid >= ext || tmid < 0)
            curr = near;
         else if (tmid <= ent && tmid <= tmin)
            curr = far;
         else {
            if (tmid <= tmin)
               stk.push(Block(far, tmid, ext));
            curr = near;
            ext = tmid;
         }
      }
      bool temp = curr->searchNode(ray, tind, tmin, uv);
      if (temp) return true;
      //if (found) return true;
   }
   return found;
}

// Construct Tree
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
