/**
  lighting.cpp defines a light sources and the procedure for global illumination
  via Monte Carlo sampling
*/

/**
  Direction/Point light source
*/
class Light {
public:
   double mag; //intensity
   Vec3d pos, color; //location and color
   char type; // direction or point light source
   Light(const float mag, const Vec3d &pos, const Vec3d &color, char type): mag(mag), pos(pos), color(color), type(type) {}

   /**
     Compute the lighting at the point of intersection
     @param hit point of intersection
     @param ldir direction light is illuminating
     @param shade lighting at point
     @param dis distance to light
   */
   void illuminate(const Vec3d &hit, Vec3d &ldir, Vec3d &shade, double &dis) {
      if (type == 'd') {
         dis = FLT_MAX;
         ldir = unit(pos);
         shade = mag*color;
      }
      else if (type == 'p') {
         ldir = pos - hit;
         dis = len(ldir);
         ldir = unit(ldir);
         shade = mag*color/(4*dis*M_PI); // linear
      }
   }
};

/**
  Create a local coordinate system at point with the normal as the vertical axis
*/
void createCoordSystem(const Vec3d &N, Vec3d &Nt, Vec3d &Nb) {
   if (fabs(N.x) > fabs(N.y))
      Nt = Vec3d(N.z, 0, -N.x)/sqrt(N.x*N.x+N.z*N.z);
   else
      Nt = Vec3d(0, -N.z, N.y)/sqrt(N.y*N.y+N.z*N.z);
   Nb = cross(N, Nt);
}

/**
  Uniformly sample a random direction in local coordinate system
*/
Vec3d uniformSampleHemisphere(double &r1, double &r2) {
   r1 = ((double)rand())/RAND_MAX;
   r2 = ((double)rand())/RAND_MAX;
   double sinTheta = sqrt(1 - r1*r1);
   double phi = 2*M_PI*r2;
   return Vec3d(sinTheta*cos(phi), r1, sinTheta*sin(phi));
}
