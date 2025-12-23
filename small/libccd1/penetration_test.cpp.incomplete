  #include <ccd/ccd.h>
  #include <ccd/quat.h> // for work with quaternions

  /** Support function is same as in previous case */

  int main(int argc, char *argv[])
  {
      ccd_t ccd;
      CCD_INIT(&ccd); // initialize ccd_t struct

      // set up ccd_t struct
      ccd.support1       = support; // support function for first object
      ccd.support2       = support; // support function for second object
      ccd.max_iterations = 100;     // maximal number of iterations
      ccd.epa_tolerance  = 0.0001;  // maximal tolerance fro EPA part

      ccd_real_t depth;
      ccd_vec3_t dir, pos;
      int intersect = ccdGJKPenetration(obj1, obj2, &ccd, &depth, &dir, &pos);
      // now intersect holds 0 if obj1 and obj2 intersect, -1 otherwise
      // in depth, dir and pos is stored penetration depth, direction of
      // separation vector and position in global coordinate system
  }