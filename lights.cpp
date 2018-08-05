// Illumination
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
         shade = mag*color/(4*dis*dis*M_PI);
      }
   }
};
