/**
   kdtree.cpp defines KDNode, geometrically a Box, and contains pointers to two
   KDNode children as well as other supplemental info. It also defines build and
   search methods, implemented with our proposed optimizations.

   @author George Tang
 */

/**
   @const LEAF max amount of triangles in leaf node
   pms macro for polygonMesh idx
   trg macro for triangle idx in pms
 */
#define LEAF 3
#define pms ind[i].first
#define trg ind[i].second

/**
   Box is the geometric representation of KDNode
 */
class Box {
public:
	Vec3d bnds[2];         // two points define a volume

/**
   Fast ray-box intersection test
   @param ent enter distance
   @param ext exit distance
 */
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

/**
   Computes and returns box surface area
 */
	double surfaceArea() {
		double l = bnds[1].x - bnds[0].x;
		double w = bnds[1].y - bnds[0].y;
		double h = bnds[1].z - bnds[0].z;
		return 2.0*(l*w+w*h+l*h);
	}
};

// Stats for proposed optimization
int tris = 0; // num triangles
int sum = 0; // naive num build/search triangles
int sum2 = 0; // optimized num build/search triangles

// Surface Area Heuristic (SAH) Parameters
double trav = 5.0;
double inst = 5.0;

/**
   KD Node
 */
class KDNode {
public:
	Box box;         // geometric representation
	int axis;         // split axis
	double splt;         // split coordinate
	bool leaf;         // is leaf?
	KDNode *left, *right;         // children
	vector<pii> ind;         // vector of (Mesh idx, Triangle idx)
	KDNode() : leaf(false) {}

/**
   Recursive build is implemented with SAH and proposed optimization
   @param depth is current depth of build dfs
 */
	void build(const int depth) {
		if (ind.size() <= LEAF) {
			sum += ind.size();
			leaf = true;
			return;
		}
		left = new KDNode();
		right = new KDNode();
		axis = depth%3;
		// Find median NlogN
		vector<Vec3d> pnts;
		for (int i = 0; i < ind.size(); i++)
			pnts.push_back(obj[pms].cent[trg]);
		sort(pnts.begin(), pnts.end(), func[axis]);
		// Update bounding box
		copy(begin(box.bnds), end(box.bnds), begin(left->box.bnds));
		copy(begin(box.bnds), end(box.bnds), begin(right->box.bnds));
		// SAH implementation
		double minv = DBL_MAX;
		double curr, cost;
		double sa = box.surfaceArea();
		for (int i = 0; i < pnts.size(); i++) {
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
		if (inst*ind.size() <= minv) {
			sum += ind.size();
			leaf = true;
			free(left);
			free(right);
			return;
		}
		left->box.bnds[1][axis] = splt;
		right->box.bnds[0][axis] = splt;
		// Assign triangles based on locations of vertex relative to splt
		for (int i = 0; i < ind.size(); i++) {
			Vec3i V = obj[pms].tris[trg].vi;
			bool l = false, r = false;
			for (int j = 0; j < 3; j++) {
				Vec3d pnt = obj[pms].vert[V[j]];
				l = pnt[axis] <= splt ? true : l;
				r = pnt[axis] >= splt ? true : r;
			}
			if (l) left->ind.push_back(ind[i]);
			if (r) right->ind.push_back(ind[i]);
		}
		// if centers are the same, merge
		if (max(left->ind.size(), right->ind.size()) == ind.size()) {
			sum += ind.size();
			leaf = true;
			free(left);
			free(right);
			return;
		}
		left->build(depth+1);
		right->build(depth+1);

		// pull up (proposed optimization component): merge common triangles between children
		vector<pii> ls;
		set<pii> rs;
		for (int i = 0; i < left->ind.size(); i++)
			ls.push_back(left->ind[i]);
		for (int i = 0; i < right->ind.size(); i++)
			rs.insert(right->ind[i]);
		ind.clear();
		left->ind.clear();
		right->ind.clear();
		for (int i = 0; i < ls.size(); i++)
			if (rs.find(ls[i]) != rs.end()) {
				rs.erase(ls[i]);
				ind.push_back(ls[i]);
			}
			else
				left->ind.push_back(ls[i]);
		right->ind = vector<pii>(rs.begin(), rs.end());
		sum2 += left->ind.size() + right->ind.size();
	}

/**
   Search for closest intersecting triangle in KDNode
   @param tind optimal triangle
   @param tmin distance to optimal triangle
   @param barycentric coordinates of intersection
 */
	bool searchNode(Ray &ray, pii &tind, double &tmin, pdd &uv) {
		bool found = false;
		if (ind.size() > 0) {
			double t;
			pdd bay;
			for (int i = 0; i < ind.size(); i++)
				if (obj[pms].intersect(trg, ray, t, bay) && t > 0 && t < tmin) {
					Vec3d hit = ray.src+t*ray.dir;
					if (!(hit >= box.bnds[0] && hit <= box.bnds[1]))
						continue;
					found = true;
					tmin = t;
					uv = bay;
					tind = pii(pms, trg);
				}
		}
		return found;
	}
};

/**
   Block datatype used for search dfs. Contains reference to KDNode and ent, ext distances.
 */
class Block {
public:
	KDNode *node;
	double ent, ext;
	Block(KDNode *node, const double ent, const double ext) : node(node), ent(ent), ext(ext) {}
};

/**
   Recursive search performs a dfs through KDNodes to find node with optimal triangle.
   Implements early break (proposed optimization component): if triangle already
   found is better than dfs branch we entered, return
   @param root current node
 */
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
		if (ent >= tmin) break; // comment
		while(!curr->leaf) {
			found |= curr->searchNode(ray, tind, tmin, uv); // comment
			s = curr->splt;
			a = curr->axis;
			tdir = ray.dir[a] == 0 ? 0.0000001 : ray.dir[a];
			tmid = (s-ray.src[a])/tdir;
			near = curr->left; far = curr->right;
			if (ray.src[a] > s)
				swap(near, far);
			if (tmid >= ext || tmid < 0)
				curr = near;
			else if (tmid <= ent && tmid <= tmin) // comment after &&
				curr = far;
			else {
				if (tmid <= tmin) // comment
					stk.push(Block(far, tmid, ext));
				curr = near;
				ext = tmid;
			}
		}
		bool temp = curr->searchNode(ray, tind, tmin, uv);
		if (temp) return true; // proposed algorithm in project.pdf ends here
		// if (found) return true; // error; note other nodes later in dfs can have optimal solution; please verify
	}
	return found;
}

/**
   Build root KDNode
 */
void buildTree(KDNode* root) {
	vector<pii> v;
	for (int i = 0; i < obj.size(); i++) {
		tris += obj[i].tris.size();
		for (int j = 0; j < obj[i].tris.size(); j++)
			v.push_back(pii(i, j));
	}
	// Determine inital bounds
	Vec3d vmin(0), vmax(0);
	for (int i = 0; i < obj.size(); i++)
		for (int j = 0; j < obj[i].vert.size(); j++)
			for (int k = 0; k < 3; k++) {
				vmin[k] = min(vmin[k], obj[i].vert[j][k]);
				vmax[k] = max(vmax[k], obj[i].vert[j][k]);
			}
	root->box.bnds[0] = vmin-Vec3d(0.01);
	root->box.bnds[1] = vmax+Vec3d(0.01);
	root->ind = v;
	root->build(0);
}
