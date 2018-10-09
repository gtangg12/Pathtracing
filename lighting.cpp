class Light {
public:
   double mag;
   Vec3d pos, color;
   char type;
   Light(const float mag, const Vec3d &pos, const Vec3d &color, char type): mag(mag), pos(pos), color(color), type(type) {}

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

// Global Illumination
double pdf = 1/(2*M_PI);

void createCoordSystem(const Vec3d &N, Vec3d &Nt, Vec3d &Nb) {
   if (fabs(N.x) > fabs(N.y))
      Nt = Vec3d(N.z, 0, -N.x)/sqrt(N.x*N.x+N.z*N.z);
   else
      Nt = Vec3d(0, -N.z, N.y)/sqrt(N.y*N.y+N.z*N.z);
   Nb = cross(N, Nt);
}

Vec3d uniformSampleHemisphere(double &r1, double &r2) {
   r1 = ((double)rand())/RAND_MAX;
   r2 = ((double)rand())/RAND_MAX;
   double sinTheta = sqrt(1 - r1*r1);
   double phi = 2*M_PI*r2;
   return Vec3d(sinTheta*cos(phi), r1, sinTheta*sin(phi));
}
