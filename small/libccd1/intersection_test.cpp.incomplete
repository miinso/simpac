  #include <ccd/ccd.h>
  #include <ccd/quat.h> // for work with quaternions

  /** Support function for box */
  void support(const void *obj, const ccd_vec3_t *dir, ccd_vec3_t *vec)
  {
      // assume that obj_t is user-defined structure that holds info about
      // object (in this case box: x, y, z, pos, quat - dimensions of box,
      // position and rotation)
      obj_t *obj = (obj_t *)_obj;
      ccd_vec3_t dir;
      ccd_quat_t qinv;

      // apply rotation on direction vector
      ccdVec3Copy(&dir, _dir);
      ccdQuatInvert2(&qinv, &obj->quat);
      ccdQuatRotVec(&dir, &qinv);

      // compute support point in specified direction
      ccdVec3Set(v, ccdSign(ccdVec3X(&dir)) * box->x * CCD_REAL(0.5),
                    ccdSign(ccdVec3Y(&dir)) * box->y * CCD_REAL(0.5),
                    ccdSign(ccdVec3Z(&dir)) * box->z * CCD_REAL(0.5));

      // transform support point according to position and rotation of object
      ccdQuatRotVec(v, &obj->quat);
      ccdVec3Add(v, &obj->pos);
  }

  int main(int argc, char *argv[])
  {
      ...

      ccd_t ccd;
      CCD_INIT(&ccd); // initialize ccd_t struct

      // set up ccd_t struct
      ccd.support1       = support; // support function for first object
      ccd.support2       = support; // support function for second object
      ccd.max_iterations = 100;     // maximal number of iterations

      int intersect = ccdGJKIntersect(obj1, obj2, &ccd);
      // now intersect holds true if obj1 and obj2 intersect, false otherwise
  }