#include "sgl_physics/sgl_convex.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <set>
#include <sstream>

namespace sgl {
namespace Physics {

/* structs for qsort */
struct _indexed_double { double val; int idx; };
struct _indexed_int { int val; int idx; };

/* internal functions definitions: */
int _convhull_cmp_asc_float(const void *a, const void *b) {
  _indexed_double *a1 = (_indexed_double*)a;
  _indexed_double *a2 = (_indexed_double*)b;
  if ((*a1).val < (*a2).val)return -1;
  else if ((*a1).val > (*a2).val)return 1;
  else return 0;
}
int _convhull_cmp_desc_float(const void *a, const void *b) {
  _indexed_double *a1 = (_indexed_double*)a;
  _indexed_double *a2 = (_indexed_double*)b;
  if ((*a1).val > (*a2).val)return -1;
  else if ((*a1).val < (*a2).val)return 1;
  else return 0;
}
int _convhull_cmp_asc_int(const void *a, const void *b) {
  _indexed_int *a1 = (_indexed_int*)a;
  _indexed_int *a2 = (_indexed_int*)b;
  if ((*a1).val < (*a2).val)return -1;
  else if ((*a1).val > (*a2).val)return 1;
  else return 0;
}
int _convhull_cmp_desc_int(const void *a, const void *b) {
  _indexed_int *a1 = (_indexed_int*)a;
  _indexed_int *a2 = (_indexed_int*)b;
  if ((*a1).val > (*a2).val)return -1;
  else if ((*a1).val < (*a2).val)return 1;
  else return 0;
}
void _convhull_sort_float(
  double* in_vec,      /* vector[len] to be sorted */
  double* out_vec,     /* if NULL, then in_vec is sorted "in-place" */
  int* new_idices,   /* set to NULL if you don't need them */
  int len,           /* number of elements in vectors, must be consistent with the input data */
  int descendFLAG    /* !1:ascending, 1:descending */
)
{
  int i;
  _indexed_double *data;

  data = (_indexed_double*)malloc(len * sizeof(_indexed_double));
  for (i = 0; i < len; i++) {
    data[i].val = in_vec[i];
    data[i].idx = i;
  }
  if (descendFLAG)
    qsort(data, len, sizeof(data[0]), _convhull_cmp_desc_float);
  else
    qsort(data, len, sizeof(data[0]), _convhull_cmp_asc_float);
  for (i = 0; i < len; i++) {
    if (out_vec != NULL)
      out_vec[i] = data[i].val;
    else
      in_vec[i] = data[i].val; /* overwrite input vector */
    if (new_idices != NULL)
      new_idices[i] = data[i].idx;
  }
  free(data);
}
void _convhull_sort_int
(
  int* in_vec,       /* vector[len] to be sorted */
  int* out_vec,      /* if NULL, then in_vec is sorted "in-place" */
  int* new_idices,   /* set to NULL if you don't need them */
  int len,           /* number of elements in vectors, must be consistent with the input data */
  int descendFLAG    /* !1:ascending, 1:descending */
)
{
  int i;
  _indexed_int *data;

  data = (_indexed_int*)malloc(len * sizeof(_indexed_int));
  for (i = 0; i < len; i++) {
    data[i].val = in_vec[i];
    data[i].idx = i;
  }
  if (descendFLAG)
    qsort(data, len, sizeof(data[0]), _convhull_cmp_desc_int);
  else
    qsort(data, len, sizeof(data[0]), _convhull_cmp_asc_int);
  for (i = 0; i < len; i++) {
    if (out_vec != NULL)
      out_vec[i] = data[i].val;
    else
      in_vec[i] = data[i].val; /* overwrite input vector */
    if (new_idices != NULL)
      new_idices[i] = data[i].idx;
  }
  free(data);
}
Vec3 _convhull_cross(Vec3* v1, Vec3* v2)
{
  Vec3 cross;
  cross.x = v1->y * v2->z - v1->z * v2->y;
  cross.y = v1->z * v2->x - v1->x * v2->z;
  cross.z = v1->x * v2->y - v1->y * v2->x;
  return cross;
}
double _convhull_det_4x4(double* m) {
  /* calculates the determinent of a 4x4 matrix */
  return
    m[3] * m[6] * m[9] * m[12] - m[2] * m[7] * m[9] * m[12] -
    m[3] * m[5] * m[10] * m[12] + m[1] * m[7] * m[10] * m[12] +
    m[2] * m[5] * m[11] * m[12] - m[1] * m[6] * m[11] * m[12] -
    m[3] * m[6] * m[8] * m[13] + m[2] * m[7] * m[8] * m[13] +
    m[3] * m[4] * m[10] * m[13] - m[0] * m[7] * m[10] * m[13] -
    m[2] * m[4] * m[11] * m[13] + m[0] * m[6] * m[11] * m[13] +
    m[3] * m[5] * m[8] * m[14] - m[1] * m[7] * m[8] * m[14] -
    m[3] * m[4] * m[9] * m[14] + m[0] * m[7] * m[9] * m[14] +
    m[1] * m[4] * m[11] * m[14] - m[0] * m[5] * m[11] * m[14] -
    m[2] * m[5] * m[8] * m[15] + m[1] * m[6] * m[8] * m[15] +
    m[2] * m[4] * m[9] * m[15] - m[0] * m[6] * m[9] * m[15] -
    m[1] * m[4] * m[10] * m[15] + m[0] * m[5] * m[10] * m[15];
}
void _convhull_create_sub_matrix
(/* Helper function for det_NxN()  */
  double* m,
  int N,
  int i,
  double* sub_m
)
{
  int j, k;
  for (j = N, k = 0; j < N * N; j++) {
    if (j % N != i) { /* i is the index to remove */
      sub_m[k] = m[j];
      k++;
    }
  }
}

double _convhull_det_NxN
(
  double* m,
  int d
)
{
  double sum;
  double sub_m[SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS * SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS];
  int sign;

  if (d == 0)
    return 1.0;
  sum = 0.0;
  sign = 1;
  for (int i = 0; i < d; i++) {
    _convhull_create_sub_matrix(m, d, i, sub_m);
    sum += sign * m[i] * _convhull_det_NxN(sub_m, d - 1);
    sign *= -1;
  }
  return sum;
}

/* Calculates the coefficients of the equation of a PLANE in 3D.
  * Original Copyright (c) 2014, George Papazafeiropoulos
  * Distributed under the BSD (2-clause) license
  */
void _convhull_plane_3d
(
  double* p,
  double* c,
  double* d
)
{
  int i, j, k, l;
  int r[3];
  double sign, det, norm_c;
  double pdiff[2][3], pdiff_s[2][2];

  for (i = 0; i < 2; i++)
    for (j = 0; j < 3; j++)
      pdiff[i][j] = p[(i + 1) * 3 + j] - p[i * 3 + j];
  memset(c, 0, 3 * sizeof(double));
  sign = 1.0;
  for (i = 0; i < 3; i++)
    r[i] = i;
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 2; j++) {
      for (k = 0, l = 0; k < 3; k++) {
        if (r[k] != i) {
          pdiff_s[j][l] = pdiff[j][k];
          l++;
        }
      }
    }
    det = pdiff_s[0][0] * pdiff_s[1][1] - pdiff_s[1][0] * pdiff_s[0][1];
    c[i] = sign * det;
    sign *= -1.0;
  }
  norm_c = 0.0;
  for (i = 0; i < 3; i++)
    norm_c += (pow(c[i], 2.0));
  norm_c = sqrt(norm_c);
  for (i = 0; i < 3; i++)
    c[i] /= norm_c;
  (*d) = 0.0;
  for (i = 0; i < 3; i++)
    (*d) += -p[i] * c[i];
}

/* Calculates the coefficients of the equation of a PLANE in ND.
  * Original Copyright (c) 2014, George Papazafeiropoulos
  * Distributed under the BSD (2-clause) license
  */
void _convhull_plane_nd
(
  const int Nd,
  double* p,
  double* c,
  double* d
)
{
  int i, j, k, l;
  int r[SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS];
  double sign, det, norm_c;
  double pdiff[SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS - 1][SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS], pdiff_s[(SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS - 1)*(SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS - 1)];

  if (Nd == 3) {
    _convhull_plane_3d(p, c, d);
    return;
  }

  for (i = 0; i < Nd - 1; i++)
    for (j = 0; j < Nd; j++)
      pdiff[i][j] = p[(i + 1)*Nd + j] - p[i*Nd + j];
  memset(c, 0, Nd * sizeof(double));
  sign = 1.0;
  for (i = 0; i < Nd; i++)
    r[i] = i;
  for (i = 0; i < Nd; i++) {
    for (j = 0; j < Nd - 1; j++) {
      for (k = 0, l = 0; k < Nd; k++) {
        if (r[k] != i) {
          pdiff_s[j*(Nd - 1) + l] = pdiff[j][k];
          l++;
        }
      }
    }
    /* Determinant 1 dimension lower */
    if (Nd == 3)
      det = pdiff_s[0 * (Nd - 1) + 0] * pdiff_s[1 * (Nd - 1) + 1] - pdiff_s[1 * (Nd - 1) + 0] * pdiff_s[0 * (Nd - 1) + 1];
    else if (Nd == 5)
      det = _convhull_det_4x4((double*)pdiff_s);
    else {
      det = _convhull_det_NxN((double*)pdiff_s, Nd - 1);
    }
    c[i] = sign * det;
    sign *= -1.0;
  }
  norm_c = 0.0;
  for (i = 0; i < Nd; i++)
    norm_c += (pow(c[i], 2.0));
  norm_c = sqrt(norm_c);
  for (i = 0; i < Nd; i++)
    c[i] /= norm_c;
  (*d) = 0.0;
  for (i = 0; i < Nd; i++)
    (*d) += -p[i] * c[i];
}

void _convhull_ismember
(
  int* pLeft,          /* left vector; nLeftElements x 1 */
  int* pRight,         /* right vector; nRightElements x 1 */
  int* pOut,           /* 0, unless pRight elements are present in pLeft then 1; nLeftElements x 1 */
  int nLeftElements,   /* number of elements in pLeft */
  int nRightElements   /* number of elements in pRight */
)
{
  int i, j;
  memset(pOut, 0, nLeftElements * sizeof(int));
  for (i = 0; i < nLeftElements; i++)
    for (j = 0; j < nRightElements; j++)
      if (pLeft[i] == pRight[j])
        pOut[i] = 1;
}

double _convhull_rnd(int x, int y)
{
  /*
  Reference(s):

  - Improvements to the canonical one-liner GLSL rand() for OpenGL ES 2.0
    http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
  */
  double a = double(12.9898);
  double b = double(78.233);
  double c = double(43758.5453);
  double dt = x * a + y * b;
  double sn = fmod(dt, sgl::PI_LOW_PREC);
  double intpart;
  return modf(sin(sn) * c, &intpart);
}

/* A C version of the 3D quickhull matlab implementation from here:
  * https://www.mathworks.com/matlabcentral/fileexchange/48509-computational-geometry-toolbox?focused=3851550&tab=example
  * (*out_faces) is returned as NULL, if triangulation fails *
  * Original Copyright (c) 2014, George Papazafeiropoulos
  * Distributed under the BSD (2-clause) license
  * Reference: "The Quickhull Algorithm for Convex Hull, C. Bradford Barber, David P. Dobkin
  *             and Hannu Huhdanpaa, Geometry Center Technical Report GCG53, July 30, 1993"
  */

  /*
  builds the 3-D convexhull
  */
void _convhull_3d_build(
  /* input arguments */
  const Vec3* const in_vertices,      /* vector of input vertices; nVert x 1 */
  const int nVert,                    /* number of vertices */
  /* output arguments */
  int** out_faces,                    /* & of empty int*, output face indices; flat: nOut_faces x 3 */
  int* nOut_faces                     /* & of int, number of output face indices */
) {
  int i, j, k, l, h;
  int nFaces, p, d;
  int* aVec, *faces;
  double dfi, v, max_p, min_p;
  double* points, *cf, *cfi, *df, *p_s, *span;

  if (nVert < 3 || in_vertices == NULL) {
    (*out_faces) = NULL;
    (*nOut_faces) = 0;
    return;
  }

  /* 3 dimensions. The code should theoretically work for >=2 dimensions, but "plane_3d" and "det_4x4" are hardcoded for 3,
    * so would need to be rewritten */
  d = 3;

  /* Add noise to the points */
  points = (double*)malloc(nVert*(d + 1) * sizeof(double));
  for (i = 0; i < nVert; i++) {
    for (j = 0; j < d; j++)
      points[i*(d + 1) + j] = in_vertices[i].i[j] + SGL_PHYSICS_CONVHULL_NOISE_VALUE * _convhull_rnd(i, j); /* noise mitigates duplicates */
    points[i*(d + 1) + d] = 1.0; /* add a last column of ones. Used only for determinant calculation */
  }
  /* Find the span */
  span = (double*)malloc(d * sizeof(double));
  for (j = 0; j < d; j++) {
    max_p = double(-2.23e+13); min_p = double(2.23e+13);
    for (i = 0; i < nVert; i++) {
      max_p = max(max_p, points[i*(d + 1) + j]);
      min_p = min(min_p, points[i*(d + 1) + j]);
    }
    span[j] = max_p - min_p;
    if (span[j] <= SGL_PHYSICS_CONVHULL_NOISE_VALUE) {
      /* If you hit this assertion error, then the input vertices do not span all 3 dimensions. Therefore the convex hull cannot be built.
        * In these cases, reduce the dimensionality of the points and call convhull_nd_build() instead with d<3 */
#ifdef SGL_PHYSICS_CONVHULL_VERBOSE
      printf("ERROR: input vertices do not span all 3 dimensions. "
        "Therefore the convex hull cannot be built. "
        "In these cases, reduce the dimensionality of the "
        "points and call convhull_nd_build() instead with d<3. "
        "out_faces is set to NULL.\n");
#endif
      free(points);
      free(span);
      *out_faces = NULL;
      *nOut_faces = 0;
      return;
    }
  }

  /* The initial convex hull is a simplex with (d+1) facets, where d is the number of dimensions */
  nFaces = (d + 1);
  faces = (int*)calloc(nFaces*d, sizeof(int));
  aVec = (int*)malloc(nFaces * sizeof(int));
  for (i = 0; i < nFaces; i++)
    aVec[i] = i;

  /* Each column of cf contains the coefficients of a plane */
  cf = (double*)malloc(nFaces*d * sizeof(double));
  cfi = (double*)malloc(d * sizeof(double));
  df = (double*)malloc(nFaces * sizeof(double));
  p_s = (double*)malloc(d*d * sizeof(double));
  for (i = 0; i < nFaces; i++) {
    /* Set the indices of the points defining the face  */
    for (j = 0, k = 0; j < (d + 1); j++) {
      if (aVec[j] != i) {
        faces[i*d + k] = aVec[j];
        k++;
      }
    }

    /* Calculate and store the plane coefficients of the face */
    for (j = 0; j < d; j++)
      for (k = 0; k < d; k++)
        p_s[j*d + k] = points[(faces[i*d + j])*(d + 1) + k];

    /* Calculate and store the plane coefficients of the face */
    _convhull_plane_3d(p_s, cfi, &dfi);
    for (j = 0; j < d; j++)
      cf[i*d + j] = cfi[j];
    df[i] = dfi;
  }
  double *A;
  int *bVec, *fVec, *asfVec;
  int face_tmp[2];

  /* Check to make sure that faces are correctly oriented */
  bVec = (int*)malloc(4 * sizeof(int));
  for (i = 0; i < d + 1; i++)
    bVec[i] = i;

  /* A contains the coordinates of the points forming a simplex */
  A = (double*)calloc((d + 1)*(d + 1), sizeof(double));
  fVec = (int*)malloc((d + 1) * sizeof(int));
  asfVec = (int*)malloc((d + 1) * sizeof(int));
  for (k = 0; k < (d + 1); k++) {
    /* Get the point that is not on the current face (point p) */
    for (i = 0; i < d; i++)
      fVec[i] = faces[k*d + i];
    _convhull_sort_int(fVec, NULL, NULL, d, 0); /* sort accending */
    p = k;
    for (i = 0; i < d; i++)
      for (j = 0; j < (d + 1); j++)
        A[i*(d + 1) + j] = points[(faces[k*d + i])*(d + 1) + j];
    for (; i < (d + 1); i++)
      for (j = 0; j < (d + 1); j++)
        A[i*(d + 1) + j] = points[p*(d + 1) + j];

    /* det(A) determines the orientation of the face */
    v = _convhull_det_4x4(A);

    /* Orient so that each point on the original simplex can't see the opposite face */
    if (v < 0) {
      /* Reverse the order of the last two vertices to change the volume */
      for (j = 0; j < 2; j++)
        face_tmp[j] = faces[k*d + d - j - 1];
      for (j = 0; j < 2; j++)
        faces[k*d + d - j - 1] = face_tmp[1 - j];

      /* Modify the plane coefficients of the properly oriented faces */
      for (j = 0; j < d; j++)
        cf[k*d + j] = -cf[k*d + j];
      df[k] = -df[k];
      for (i = 0; i < d; i++)
        for (j = 0; j < (d + 1); j++)
          A[i*(d + 1) + j] = points[(faces[k*d + i])*(d + 1) + j];
      for (; i < (d + 1); i++)
        for (j = 0; j < (d + 1); j++)
          A[i*(d + 1) + j] = points[p*(d + 1) + j];
    }
  }

  /* Coordinates of the center of the point set */
  double* meanp, *absdist, *reldist, *desReldist;
  meanp = (double*)calloc(d, sizeof(double));
  for (i = d + 1; i < nVert; i++)
    for (j = 0; j < d; j++)
      meanp[j] += points[i*(d + 1) + j];
  for (j = 0; j < d; j++)
    meanp[j] = meanp[j] / (double)(nVert - d - 1);

  /* Absolute distance of points from the center */
  absdist = (double*)malloc((nVert - d - 1)*d * sizeof(double));
  for (i = d + 1, k = 0; i < nVert; i++, k++)
    for (j = 0; j < d; j++)
      absdist[k*d + j] = (points[i*(d + 1) + j] - meanp[j]) / span[j];

  /* Relative distance of points from the center */
  reldist = (double*)calloc((nVert - d - 1), sizeof(double));
  desReldist = (double*)malloc((nVert - d - 1) * sizeof(double));
  for (i = 0; i < (nVert - d - 1); i++)
    for (j = 0; j < d; j++)
      reldist[i] += pow(absdist[i*d + j], 2.0);

  /* Sort from maximum to minimum relative distance */
  int num_pleft, cnt;
  int* ind, *pleft;
  ind = (int*)malloc((nVert - d - 1) * sizeof(int));
  pleft = (int*)malloc((nVert - d - 1) * sizeof(int));
  _convhull_sort_float(reldist, desReldist, ind, (nVert - d - 1), 1);

  /* Initialize the vector of points left. The points with the larger relative
    distance from the center are scanned first. */
  num_pleft = (nVert - d - 1);
  for (i = 0; i < num_pleft; i++)
    pleft[i] = ind[i] + d + 1;

  /* Loop over all remaining points that are not deleted. Deletion of points
    occurs every #iter2del# iterations of this while loop */
  memset(A, 0, (d + 1)*(d + 1) * sizeof(double));

  /* cnt is equal to the points having been selected without deletion of
    nonvisible points (i.e. points inside the current convex hull) */
  cnt = 0;

  /* The main loop for the quickhull algorithm */
  double detA;
  double* points_cf, *points_s;
  int* visible_ind, *visible, *nonvisible_faces, *f0, *face_s, *u, *gVec, *horizon, *hVec, *pp, *hVec_mem_face;
  int num_visible_ind, num_nonvisible_faces, n_newfaces, count, vis;
  int f0_sum, u_len, start, num_p, index, horizon_size1;
  int FUCKED;
  FUCKED = 0;
  u = horizon = NULL;
  nFaces = d + 1;
  visible_ind = (int*)malloc(nFaces * sizeof(int));
  points_cf = (double*)malloc(nFaces * sizeof(double));
  points_s = (double*)malloc(d * sizeof(double));
  face_s = (int*)malloc(d * sizeof(int));
  gVec = (int*)malloc(d * sizeof(int));
  while ((num_pleft > 0)) {
    /* i is the first point of the points left */
    i = pleft[0];

    /* Delete the point selected */
    for (j = 0; j < num_pleft - 1; j++)
      pleft[j] = pleft[j + 1];
    num_pleft--;
    if (num_pleft == 0)
      free(pleft);
    else
      pleft = (int*)realloc(pleft, num_pleft * sizeof(int));

    /* Update point selection counter */
    cnt++;

    /* find visible faces */
    for (j = 0; j < d; j++)
      points_s[j] = points[i*(d + 1) + j];
    points_cf = (double*)realloc(points_cf, nFaces * sizeof(double));
    visible_ind = (int*)realloc(visible_ind, nFaces * sizeof(int));
    for (j = 0; j < nFaces; j++) {
      points_cf[j] = 0;
      for (k = 0; k < d; k++)
        points_cf[j] += points_s[k] * cf[j*d + k];
    }
    num_visible_ind = 0;
    for (j = 0; j < nFaces; j++) {
      if (points_cf[j] + df[j] > 0.0) {
        num_visible_ind++; /* will sum to 0 if none are visible */
        visible_ind[j] = 1;
      }
      else
        visible_ind[j] = 0;
    }
    num_nonvisible_faces = nFaces - num_visible_ind;

    /* proceed if there are any visible faces */
    if (num_visible_ind != 0) {
      /* Find visible face indices */
      visible = (int*)malloc(num_visible_ind * sizeof(int));
      for (j = 0, k = 0; j < nFaces; j++) {
        if (visible_ind[j] == 1) {
          visible[k] = j;
          k++;
        }
      }

      /* Find nonvisible faces */
      nonvisible_faces = (int*)malloc(num_nonvisible_faces*d * sizeof(int));
      f0 = (int*)malloc(num_nonvisible_faces*d * sizeof(int));
      for (j = 0, k = 0; j < nFaces; j++) {
        if (visible_ind[j] == 0) {
          for (l = 0; l < d; l++)
            nonvisible_faces[k*d + l] = faces[j*d + l];
          k++;
        }
      }

      /* Create horizon (count is the number of the edges of the horizon) */
      count = 0;
      for (j = 0; j < num_visible_ind; j++) {
        /* visible face */
        vis = visible[j];
        for (k = 0; k < d; k++)
          face_s[k] = faces[vis*d + k];
        _convhull_sort_int(face_s, NULL, NULL, d, 0);
        _convhull_ismember(nonvisible_faces, face_s, f0, num_nonvisible_faces*d, d);
        u_len = 0;

        /* u are the nonvisible faces connected to the face v, if any */
        for (k = 0; k < num_nonvisible_faces; k++) {
          f0_sum = 0;
          for (l = 0; l < d; l++)
            f0_sum += f0[k*d + l];
          if (f0_sum == d - 1) {
            u_len++;
            if (u_len == 1)
              u = (int*)malloc(u_len * sizeof(int));
            else
              u = (int*)realloc(u, u_len * sizeof(int));
            u[u_len - 1] = k;
          }
        }
        for (k = 0; k < u_len; k++) {
          /* The boundary between the visible face v and the k(th) nonvisible face connected to the face v forms part of the horizon */
          count++;
          if (count == 1)
            horizon = (int*)malloc(count*(d - 1) * sizeof(int));
          else
            horizon = (int*)realloc(horizon, count*(d - 1) * sizeof(int));
          for (l = 0; l < d; l++)
            gVec[l] = nonvisible_faces[u[k] * d + l];
          for (l = 0, h = 0; l < d; l++) {
            if (f0[u[k] * d + l]) {
              horizon[(count - 1)*(d - 1) + h] = gVec[l];
              h++;
            }
          }
        }
        if (u_len != 0)
          free(u);
      }
      horizon_size1 = count;
      for (j = 0, l = 0; j < nFaces; j++) {
        if (!visible_ind[j]) {
          /* Delete visible faces */
          for (k = 0; k < d; k++)
            faces[l*d + k] = faces[j*d + k];

          /* Delete the corresponding plane coefficients of the faces */
          for (k = 0; k < d; k++)
            cf[l*d + k] = cf[j*d + k];
          df[l] = df[j];
          l++;
        }
      }

      /* Update the number of faces */
      nFaces = nFaces - num_visible_ind;
      faces = (int*)realloc(faces, nFaces*d * sizeof(int));
      cf = (double*)realloc(cf, nFaces*d * sizeof(double));
      df = (double*)realloc(df, nFaces * sizeof(double));

      /* start is the first row of the new faces */
      start = nFaces;

      /* Add faces connecting horizon to the new point */
      n_newfaces = horizon_size1;
      for (j = 0; j < n_newfaces; j++) {
        nFaces++;
        faces = (int*)realloc(faces, nFaces*d * sizeof(int));
        cf = (double*)realloc(cf, nFaces*d * sizeof(double));
        df = (double*)realloc(df, nFaces * sizeof(double));
        for (k = 0; k < d - 1; k++)
          faces[(nFaces - 1)*d + k] = horizon[j*(d - 1) + k];
        faces[(nFaces - 1)*d + (d - 1)] = i;

        /* Calculate and store appropriately the plane coefficients of the faces */
        for (k = 0; k < d; k++)
          for (l = 0; l < d; l++)
            p_s[k*d + l] = points[(faces[(nFaces - 1)*d + k])*(d + 1) + l];
        _convhull_plane_3d(p_s, cfi, &dfi);
        for (k = 0; k < d; k++)
          cf[(nFaces - 1)*d + k] = cfi[k];
        df[(nFaces - 1)] = dfi;
        if (nFaces > SGL_PHYSICS_CONVHULL_MAX_NUM_FACES) {
#ifdef SGL_PHYSICS_CONVHULL_VERBOSE
          printf("ERROR: model is too complex, number of faces in convex hull "
            "exceeds the maximum number of faces allowed. Please simplify the mesh "
            "and try again!\n");
#endif
          FUCKED = 1; /* hahaha */
          nFaces = 0;
          break;
        }
      }

      /* Orient each new face properly */
      hVec = (int*)malloc(nFaces * sizeof(int));
      hVec_mem_face = (int*)malloc(nFaces * sizeof(int));
      for (j = 0; j < nFaces; j++)
        hVec[j] = j;
      for (k = start; k < nFaces; k++) {
        for (j = 0; j < d; j++)
          face_s[j] = faces[k*d + j];
        _convhull_sort_int(face_s, NULL, NULL, d, 0);
        _convhull_ismember(hVec, face_s, hVec_mem_face, nFaces, d);
        num_p = 0;
        for (j = 0; j < nFaces; j++)
          if (!hVec_mem_face[j])
            num_p++;
        pp = (int*)malloc(num_p * sizeof(int));
        for (j = 0, l = 0; j < nFaces; j++) {
          if (!hVec_mem_face[j]) {
            pp[l] = hVec[j];
            l++;
          }
        }
        index = 0;
        detA = 0.0;

        /* While new point is coplanar, choose another point */
        while (detA == 0.0) {
          for (j = 0; j < d; j++)
            for (l = 0; l < d + 1; l++)
              A[j*(d + 1) + l] = points[(faces[k*d + j])*(d + 1) + l];
          for (; j < d + 1; j++)
            for (l = 0; l < d + 1; l++)
              A[j*(d + 1) + l] = points[pp[index] * (d + 1) + l];
          index++;
          detA = _convhull_det_4x4(A);
        }

        /* Orient faces so that each point on the original simplex can't see the opposite face */
        if (detA < 0.0) {
          /* If orientation is improper, reverse the order to change the volume sign */
          for (j = 0; j < 2; j++)
            face_tmp[j] = faces[k*d + d - j - 1];
          for (j = 0; j < 2; j++)
            faces[k*d + d - j - 1] = face_tmp[1 - j];

          /* Modify the plane coefficients of the properly oriented faces */
          for (j = 0; j < d; j++)
            cf[k*d + j] = -cf[k*d + j];
          df[k] = -df[k];
          for (l = 0; l < d; l++)
            for (j = 0; j < d + 1; j++)
              A[l*(d + 1) + j] = points[(faces[k*d + l])*(d + 1) + j];
          for (; l < d + 1; l++)
            for (j = 0; j < d + 1; j++)
              A[l*(d + 1) + j] = points[pp[index] * (d + 1) + j];
#ifdef SGL_PHYSICS_CONVHULL_VERBOSE
          /* Check */
          detA = _convhull_det_4x4(A);
          /* If you hit this assertion error, then the face cannot be properly orientated */
          if (detA <= 0.0)
            printf(
              "WARNING: the face cannot be properly orientated.\n"
              "This warning is usually caused by overlapping points in your model. "
              "For example, in your model file, a single point in 3D space may appear "
              "as multiple vertices due to differences in texture coordinates "
              "(e.g., across a UV seam), resulting in duplicate vertices being "
              "loaded at the same 3D location."
            );
#endif
        }
        free(pp);
      }
      if (horizon_size1 > 0)
        free(horizon);
      free(f0);
      free(nonvisible_faces);
      free(visible);
      free(hVec);
      free(hVec_mem_face);
    }
    if (FUCKED) {
      break;
    }
  }

  /* output */
  if (FUCKED) {
    (*out_faces) = NULL;
    (*nOut_faces) = 0;
  }
  else {
    (*out_faces) = (int*)malloc(nFaces*d * sizeof(int));
    memcpy((*out_faces), faces, nFaces*d * sizeof(int));
    (*nOut_faces) = nFaces;
  }

  /* clean-up */
  free(visible_ind);
  free(points_cf);
  free(points_s);
  free(face_s);
  free(gVec);
  free(meanp);
  free(absdist);
  free(reldist);
  free(desReldist);
  free(ind);
  free(span);
  free(points);
  free(faces);
  free(aVec);
  free(cf);
  free(cfi);
  free(df);
  free(p_s);
  free(fVec);
  free(asfVec);
  free(bVec);
  free(A);
}

/*
exports the vertices, face indices, and face normals,
as an 'obj' file, ready for GPU (for 3d convexhulls only)
*/
void _convhull_3d_export_obj(
  /* input arguments */
  const Vec3* const vertices,         /* vector of input vertices; nVert x 1 */
  const int nVert,                    /* number of vertices */
  int* const faces,                   /* face indices; flat: nFaces x 3 */
  const int nFaces,                   /* number of faces in hull */
  const int keepOnlyUsedVerticesFLAG, /* 0: exports in_vertices, 1: exports only used vertices  */
  const char* obj_file                /* obj filename, WITH extension ".obj" */
) {
  int i, j;
  FILE* fp;

  errno = 0;
  fp = fopen(obj_file, "w");

  if (fp == NULL) {
    printf("Error %d \n", errno);
    printf("It's null");
  }
  fprintf(fp, "o\n");
  double scale;
  Vec3 v1, v2, normal;

  /* export vertices */
  if (keepOnlyUsedVerticesFLAG) {
    for (i = 0; i < nFaces; i++)
      for (j = 0; j < 3; j++)
        fprintf(fp, "v %f %f %f\n", vertices[faces[i * 3 + j]].x,
          vertices[faces[i * 3 + j]].y, vertices[faces[i * 3 + j]].z);
  }
  else {
    for (i = 0; i < nVert; i++)
      fprintf(fp, "v %f %f %f\n", vertices[i].x,
        vertices[i].y, vertices[i].z);
  }

  /* export the face normals */
  for (i = 0; i < nFaces; i++) {
    /* calculate cross product between v1-v0 and v2-v0 */
    v1 = vertices[faces[i * 3 + 1]];
    v2 = vertices[faces[i * 3 + 2]];
    v1.x -= vertices[faces[i * 3]].x;
    v1.y -= vertices[faces[i * 3]].y;
    v1.z -= vertices[faces[i * 3]].z;
    v2.x -= vertices[faces[i * 3]].x;
    v2.y -= vertices[faces[i * 3]].y;
    v2.z -= vertices[faces[i * 3]].z;
    normal = _convhull_cross(&v1, &v2);

    /* normalise to unit length */
    scale = 1.0 / (sqrt(pow(normal.x, 2.0) + pow(normal.y, 2.0) + pow(normal.z, 2.0)) + (double)2.23e-9);
    normal.x *= scale;
    normal.y *= scale;
    normal.z *= scale;
    fprintf(fp, "vn %f %f %f\n", normal.x, normal.y, normal.z);
  }

  /* export the face indices */
  if (keepOnlyUsedVerticesFLAG) {
    for (i = 0; i < nFaces; i++) {
      /* vertices are in same order as the faces, and normals are in order */
      fprintf(fp, "f %u//%u %u//%u %u//%u\n",
        i * 3 + 1, i + 1,
        i * 3 + 1 + 1, i + 1,
        i * 3 + 2 + 1, i + 1);
    }
  }
  else {
    /* just normals are in order  */
    for (i = 0; i < nFaces; i++) {
      fprintf(fp, "f %u//%u %u//%u %u//%u\n",
        faces[i * 3] + 1, i + 1,
        faces[i * 3 + 1] + 1, i + 1,
        faces[i * 3 + 2] + 1, i + 1);
    }
  }
  fclose(fp);
}

/*
exports the vertices, face indices, and face normals, as an 'm' file,
for MatLab verification (for 3d convexhulls only)
*/
void _convhull_3d_export_m(
  /* input arguments */
  const Vec3* const vertices,         /* vector of input vertices; nVert x 1 */
  const int nVert,                    /* number of vertices */
  int* const faces,                   /* face indices; flat: nFaces x 3 */
  const int nFaces,                   /* number of faces in hull */
  char* const m_file                  /* m filename, WITH extension ".m" */
) {
  int i;
  FILE* fp;
  fp = fopen(m_file, "w");

  /* save face indices and vertices for verification in matlab */
  fprintf(fp, "vertices = [\n");
  for (i = 0; i < nVert; i++)
    fprintf(fp, "%f, %f, %f;\n", vertices[i].x, vertices[i].y, vertices[i].z);
  fprintf(fp, "];\n\n\n");
  fprintf(fp, "faces = [\n");
  for (i = 0; i < nFaces; i++) {
    fprintf(fp, " %u, %u, %u;\n",
      faces[3 * i + 0] + 1,
      faces[3 * i + 1] + 1,
      faces[3 * i + 2] + 1);
  }
  fprintf(fp, "];\n\n\n");
  fclose(fp);
}

/*
reads an 'obj' file and extracts only the vertices
(for 3d convexhulls only)
*/
void _convhull_extract_vertices_from_obj(
  /* input arguments */
  const char* obj_file,               /* obj filename, WITH extension ".obj" */
  /* output arguments */
  Vec3** out_vertices,                /* & of empty Vec3*, output vertices; out_nVert x 1 */
  int* out_nVert                      /* & of int, number of vertices */
) {
  FILE* fp;
  fp = fopen(obj_file, "r");

  /* determine number of vertices */
  unsigned int nVert = 0;
  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    char* vexists = strstr(line, "v ");
    if (vexists != NULL)
      nVert++;
  }
  (*out_nVert) = nVert;
  (*out_vertices) = (Vec3*)malloc(nVert * sizeof(Vec3));

  /* extract the vertices */
  rewind(fp);
  int i = 0;
  int vertID, prev_char_isDigit, current_char_isDigit;
  char vert_char[256] = { 0 };
  while (fgets(line, sizeof(line), fp)) {
    char* vexists = strstr(line, "v ");
    if (vexists != NULL) {
      prev_char_isDigit = 0;
      vertID = -1;
      for (size_t j = 0; j < strlen(line) - 1; j++) {
        if (isdigit(line[j]) || line[j] == '.' || line[j] == '-' || line[j] == '+' || line[j] == 'E' || line[j] == 'e') {
          vert_char[strlen(vert_char)] = line[j];
          current_char_isDigit = 1;
        }
        else
          current_char_isDigit = 0;
        if ((prev_char_isDigit && !current_char_isDigit) || j == strlen(line) - 2) {
          vertID++;
          if (vertID > 4) {
            /* not a valid file */
            free((*out_vertices));
            (*out_vertices) = NULL;
            (*out_nVert) = 0;
            return;
          }
          (*out_vertices)[i].i[vertID] = (double)atof(vert_char);
          memset(vert_char, 0, 256 * sizeof(char));
        }
        prev_char_isDigit = current_char_isDigit;
      }
      i++;
    }
  }
}


/* A C version of the ND quickhull matlab implementation from here:
  * https://www.mathworks.com/matlabcentral/fileexchange/48509-computational-geometry-toolbox?focused=3851550&tab=example
  * (*out_faces) is returned as NULL, if triangulation fails *
  * Original Copyright (c) 2014, George Papazafeiropoulos
  * Distributed under the BSD (2-clause) license
  * Reference: "The Quickhull Algorithm for Convex Hull, C. Bradford Barber, David P. Dobkin
  *             and Hannu Huhdanpaa, Geometry Center Technical Report GCG53, July 30, 1993"
  */

  /*
  builds the N-Dimensional convexhull of a grid of points
  */
void _convhull_nd_build(
  /* input arguments */
  double* const in_points,              /* Matrix of points in 'd' dimensions; FLAT: nPoints x d */
  const int nPoints,                  /* number of points */
  const int d,                        /* Number of dimensions */
  /* output arguments */
  int** out_faces,                    /* (&) output face indices; FLAT: nOut_faces x d */
  double** out_cf,                      /* (&) contains the coefficients of the planes (set to NULL if not wanted);
                                          FLAT: nOut_faces x d */
  double** out_df,                      /* (&) contains the constant terms of the planes (set to NULL if not wanted);
                                          nOut_faces x 1 */
  int* nOut_faces                     /* (&) number of output face indices */
) {
  int i, j, k, l, h;
  int nFaces, p;
  int* aVec, *faces;
  double dfi, v, max_p, min_p;
  double* points, *cf, *cfi, *df, *p_s, *span;

  /* Solution not possible... */
  if (nPoints <= d || in_points == NULL || d > SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS) {
#ifdef SGL_PHYSICS_CONVHULL_VERBOSE
    printf("ERROR: cannot build convex hull because a valid solution is not possible...\n"
      "Possible reasons are: 1) number of vertices is lower than dimension, 2) "
      "in_vertices is NULL, 3) dimension is too high (must lower than CONVHULL_ND_MAX_DIMENSIONS).\n");
#endif
    (*out_faces) = NULL;
    (*nOut_faces) = 0;
    if (out_cf != NULL) (*out_cf) = NULL;
    if (out_df != NULL) (*out_df) = NULL;
    return;
  }

  /* Add noise to the points */
  points = (double*)malloc(nPoints*(d + 1) * sizeof(double));
  for (i = 0; i < nPoints; i++) {
    for (j = 0; j < d; j++)
      points[i*(d + 1) + j] = in_points[i*d + j] + SGL_PHYSICS_CONVHULL_NOISE_VALUE * _convhull_rnd(i, j);
    points[i*(d + 1) + d] = 1.0; /* add a last column of ones. Used only for determinant calculation */
  }

  /* Find the span */
  span = (double*)malloc(d * sizeof(double));
  for (j = 0; j < d; j++) {
    max_p = double(-2.23e+13); min_p = double(2.23e+13);
    for (i = 0; i < nPoints; i++) {
      max_p = max(max_p, points[i*(d + 1) + j]);
      min_p = min(min_p, points[i*(d + 1) + j]);
    }
    span[j] = max_p - min_p;
    /* If you hit this assertion error, then the input vertices do not span all 'd' dimensions. Therefore the convex hull cannot be built.
      * In these cases, reduce the dimensionality of the points and call convhull_nd_build() instead with d<3 */
    if (span[j] <= SGL_PHYSICS_CONVHULL_NOISE_VALUE) {
#ifdef SGL_PHYSICS_CONVHULL_VERBOSE
      printf("ERROR: input vertices do not span all 'd' dimensions. "
        "Therefore the convex hull cannot be built. "
        "In these cases, reduce the dimensionality of the "
        "points and call convhull_nd_build() instead with d<3.\n");
#endif
      free(span);
      free(points);
      (*out_faces) = NULL;
      (*nOut_faces) = 0;
      if (out_cf != NULL) (*out_cf) = NULL;
      if (out_df != NULL) (*out_df) = NULL;
      return;
    }
  }

  /* The initial convex hull is a simplex with (d+1) facets, where d is the number of dimensions */
  nFaces = (d + 1);
  faces = (int*)calloc(nFaces*d, sizeof(int));
  aVec = (int*)malloc(nFaces * sizeof(int));
  for (i = 0; i < nFaces; i++)
    aVec[i] = i;

  /* Each column of cf contains the coefficients of a plane */
  cf = (double*)malloc(nFaces*d * sizeof(double));
  cfi = (double*)malloc(d * sizeof(double));
  df = (double*)malloc(nFaces * sizeof(double));
  p_s = (double*)malloc(d*d * sizeof(double));
  for (i = 0; i < nFaces; i++) {
    /* Set the indices of the points defining the face  */
    for (j = 0, k = 0; j < (d + 1); j++) {
      if (aVec[j] != i) {
        faces[i*d + k] = aVec[j];
        k++;
      }
    }

    /* Calculate and store the plane coefficients of the face */
    for (j = 0; j < d; j++)
      for (k = 0; k < d; k++)
        p_s[j*d + k] = points[(faces[i*d + j])*(d + 1) + k];

    /* Calculate and store the plane coefficients of the face */
    _convhull_plane_nd(d, p_s, cfi, &dfi);
    for (j = 0; j < d; j++)
      cf[i*d + j] = cfi[j];
    df[i] = dfi;
  }
  double *A;
  int *bVec, *fVec;
  int face_tmp[2];

  /* Check to make sure that faces are correctly oriented */
  bVec = (int*)malloc((d + 1) * sizeof(int));
  for (i = 0; i < d + 1; i++)
    bVec[i] = i;

  /* A contains the coordinates of the points forming a simplex */
  A = (double*)calloc((d + 1)*(d + 1), sizeof(double));
  fVec = (int*)malloc((d + 1) * sizeof(int));
  for (k = 0; k < (d + 1); k++) {
    /* Get the point that is not on the current face (point p) */
    for (i = 0; i < d; i++)
      fVec[i] = faces[k*d + i];
    _convhull_sort_int(fVec, NULL, NULL, d, 0); /* sort accending */
    p = k;
    for (i = 0; i < d; i++)
      for (j = 0; j < (d + 1); j++)
        A[i*(d + 1) + j] = points[(faces[k*d + i])*(d + 1) + j];
    for (; i < (d + 1); i++)
      for (j = 0; j < (d + 1); j++)
        A[i*(d + 1) + j] = points[p*(d + 1) + j];

    /* det(A) determines the orientation of the face */
    if (d == 3)
      v = _convhull_det_4x4(A);
    else
      v = _convhull_det_NxN(A, d + 1);

    /* Orient so that each point on the original simplex can't see the opposite face */
    if (v < 0) {
      /* Reverse the order of the last two vertices to change the volume */
      for (j = 0; j < 2; j++)
        face_tmp[j] = faces[k*d + d - j - 1];
      for (j = 0; j < 2; j++)
        faces[k*d + d - j - 1] = face_tmp[1 - j];

      /* Modify the plane coefficients of the properly oriented faces */
      for (j = 0; j < d; j++)
        cf[k*d + j] = -cf[k*d + j];
      df[k] = -df[k];
      for (i = 0; i < d; i++)
        for (j = 0; j < (d + 1); j++)
          A[i*(d + 1) + j] = points[(faces[k*d + i])*(d + 1) + j];
      for (; i < (d + 1); i++)
        for (j = 0; j < (d + 1); j++)
          A[i*(d + 1) + j] = points[p*(d + 1) + j];
    }
  }

  /* Coordinates of the center of the point set */
  double* meanp, *reldist, *desReldist, *absdist;
  meanp = (double*)calloc(d, sizeof(double));
  for (i = d + 1; i < nPoints; i++)
    for (j = 0; j < d; j++)
      meanp[j] += points[i*(d + 1) + j];
  for (j = 0; j < d; j++)
    meanp[j] = meanp[j] / (double)(nPoints - d - 1);

  /* Absolute distance of points from the center */
  absdist = (double*)malloc((nPoints - d - 1)*d * sizeof(double));
  for (i = d + 1, k = 0; i < nPoints; i++, k++)
    for (j = 0; j < d; j++)
      absdist[k*d + j] = (points[i*(d + 1) + j] - meanp[j]) / span[j];

  /* Relative distance of points from the center */
  reldist = (double*)calloc((nPoints - d - 1), sizeof(double));
  desReldist = (double*)malloc((nPoints - d - 1) * sizeof(double));
  for (i = 0; i < (nPoints - d - 1); i++)
    for (j = 0; j < d; j++)
      reldist[i] += pow(absdist[i*d + j], 2.0);

  /* Sort from maximum to minimum relative distance */
  int num_pleft, cnt;
  int* ind, *pleft;
  ind = (int*)malloc((nPoints - d - 1) * sizeof(int));
  pleft = (int*)malloc((nPoints - d - 1) * sizeof(int));
  _convhull_sort_float(reldist, desReldist, ind, (nPoints - d - 1), 1);

  /* Initialize the vector of points left. The points with the larger relative
    distance from the center are scanned first. */
  num_pleft = (nPoints - d - 1);
  for (i = 0; i < num_pleft; i++)
    pleft[i] = ind[i] + d + 1;

  /* Loop over all remaining points that are not deleted. Deletion of points
    occurs every #iter2del# iterations of this while loop */
  memset(A, 0, (d + 1)*(d + 1) * sizeof(double));

  /* cnt is equal to the points having been selected without deletion of
    nonvisible points (i.e. points inside the current convex hull) */
  cnt = 0;

  /* The main loop for the quickhull algorithm */
  double detA;
  double* points_cf, *points_s;
  int* visible_ind, *visible, *nonvisible_faces, *f0, *face_s, *u, *gVec, *horizon, *hVec, *pp, *hVec_mem_face;
  int num_visible_ind, num_nonvisible_faces, n_newfaces, count, vis;
  int f0_sum, u_len, start, num_p, index, horizon_size1;
  int FUCKED;
  FUCKED = 0;
  u = horizon = NULL;
  nFaces = d + 1;
  visible_ind = (int*)malloc(nFaces * sizeof(int));
  points_cf = (double*)malloc(nFaces * sizeof(double));
  points_s = (double*)malloc(d * sizeof(double));
  face_s = (int*)malloc(d * sizeof(int));
  gVec = (int*)malloc(d * sizeof(int));
  while ((num_pleft > 0)) {
    /* i is the first point of the points left */
    i = pleft[0];

    /* Delete the point selected */
    for (j = 0; j < num_pleft - 1; j++)
      pleft[j] = pleft[j + 1];
    num_pleft--;
    if (num_pleft == 0)
      free(pleft);
    else
      pleft = (int*)realloc(pleft, num_pleft * sizeof(int));

    /* Update point selection counter */
    cnt++;

    /* find visible faces */
    for (j = 0; j < d; j++)
      points_s[j] = points[i*(d + 1) + j];
    points_cf = (double*)realloc(points_cf, nFaces * sizeof(double));
    visible_ind = (int*)realloc(visible_ind, nFaces * sizeof(int));
    for (j = 0; j < nFaces; j++) {
      points_cf[j] = 0;
      for (k = 0; k < d; k++)
        points_cf[j] += points_s[k] * cf[j*d + k];
    }
    num_visible_ind = 0;
    for (j = 0; j < nFaces; j++) {
      if (points_cf[j] + df[j] > 0.0) {
        num_visible_ind++; /* will sum to 0 if none are visible */
        visible_ind[j] = 1;
      }
      else
        visible_ind[j] = 0;
    }
    num_nonvisible_faces = nFaces - num_visible_ind;

    /* proceed if there are any visible faces */
    if (num_visible_ind != 0) {
      /* Find visible face indices */
      visible = (int*)malloc(num_visible_ind * sizeof(int));
      for (j = 0, k = 0; j < nFaces; j++) {
        if (visible_ind[j] == 1) {
          visible[k] = j;
          k++;
        }
      }

      /* Find nonvisible faces */
      nonvisible_faces = (int*)malloc(num_nonvisible_faces*d * sizeof(int));
      f0 = (int*)malloc(num_nonvisible_faces*d * sizeof(int));
      for (j = 0, k = 0; j < nFaces; j++) {
        if (visible_ind[j] == 0) {
          for (l = 0; l < d; l++)
            nonvisible_faces[k*d + l] = faces[j*d + l];
          k++;
        }
      }

      /* Create horizon (count is the number of the edges of the horizon) */
      count = 0;
      for (j = 0; j < num_visible_ind; j++) {
        /* visible face */
        vis = visible[j];
        for (k = 0; k < d; k++)
          face_s[k] = faces[vis*d + k];
        _convhull_sort_int(face_s, NULL, NULL, d, 0);
        _convhull_ismember(nonvisible_faces, face_s, f0, num_nonvisible_faces*d, d);
        u_len = 0;

        /* u are the nonvisible faces connected to the face v, if any */
        for (k = 0; k < num_nonvisible_faces; k++) {
          f0_sum = 0;
          for (l = 0; l < d; l++)
            f0_sum += f0[k*d + l];
          if (f0_sum == d - 1) {
            u_len++;
            if (u_len == 1)
              u = (int*)malloc(u_len * sizeof(int));
            else
              u = (int*)realloc(u, u_len * sizeof(int));
            u[u_len - 1] = k;
          }
        }
        for (k = 0; k < u_len; k++) {
          /* The boundary between the visible face v and the k(th) nonvisible face connected to the face v forms part of the horizon */
          count++;
          if (count == 1)
            horizon = (int*)malloc(count*(d - 1) * sizeof(int));
          else
            horizon = (int*)realloc(horizon, count*(d - 1) * sizeof(int));
          for (l = 0; l < d; l++)
            gVec[l] = nonvisible_faces[u[k] * d + l];
          for (l = 0, h = 0; l < d; l++) {
            if (f0[u[k] * d + l]) {
              horizon[(count - 1)*(d - 1) + h] = gVec[l];
              h++;
            }
          }
        }
        if (u_len != 0)
          free(u);
      }
      horizon_size1 = count;
      for (j = 0, l = 0; j < nFaces; j++) {
        if (!visible_ind[j]) {
          /* Delete visible faces */
          for (k = 0; k < d; k++)
            faces[l*d + k] = faces[j*d + k];

          /* Delete the corresponding plane coefficients of the faces */
          for (k = 0; k < d; k++)
            cf[l*d + k] = cf[j*d + k];
          df[l] = df[j];
          l++;
        }
      }

      /* Update the number of faces */
      nFaces = nFaces - num_visible_ind;
      faces = (int*)realloc(faces, nFaces*d * sizeof(int));
      cf = (double*)realloc(cf, nFaces*d * sizeof(double));
      df = (double*)realloc(df, nFaces * sizeof(double));

      /* start is the first row of the new faces */
      start = nFaces;

      /* Add faces connecting horizon to the new point */
      n_newfaces = horizon_size1;
      for (j = 0; j < n_newfaces; j++) {
        nFaces++;
        faces = (int*)realloc(faces, nFaces*d * sizeof(int));
        cf = (double*)realloc(cf, nFaces*d * sizeof(double));
        df = (double*)realloc(df, nFaces * sizeof(double));
        for (k = 0; k < d - 1; k++)
          faces[(nFaces - 1)*d + k] = horizon[j*(d - 1) + k];
        faces[(nFaces - 1)*d + (d - 1)] = i;

        /* Calculate and store appropriately the plane coefficients of the faces */
        for (k = 0; k < d; k++)
          for (l = 0; l < d; l++)
            p_s[k*d + l] = points[(faces[(nFaces - 1)*d + k])*(d + 1) + l];
        _convhull_plane_nd(d, p_s, cfi, &dfi);
        for (k = 0; k < d; k++)
          cf[(nFaces - 1)*d + k] = cfi[k];
        df[(nFaces - 1)] = dfi;
        if (nFaces > SGL_PHYSICS_CONVHULL_MAX_NUM_FACES) {
          FUCKED = 1;
          nFaces = 0;
          break;
        }
      }

      /* Orient each new face properly */
      hVec = (int*)malloc(nFaces * sizeof(int));
      hVec_mem_face = (int*)malloc(nFaces * sizeof(int));
      for (j = 0; j < nFaces; j++)
        hVec[j] = j;
      for (k = start; k < nFaces; k++) {
        for (j = 0; j < d; j++)
          face_s[j] = faces[k*d + j];
        _convhull_sort_int(face_s, NULL, NULL, d, 0);
        _convhull_ismember(hVec, face_s, hVec_mem_face, nFaces, d);
        num_p = 0;
        for (j = 0; j < nFaces; j++)
          if (!hVec_mem_face[j])
            num_p++;
        pp = (int*)malloc(num_p * sizeof(int));
        for (j = 0, l = 0; j < nFaces; j++) {
          if (!hVec_mem_face[j]) {
            pp[l] = hVec[j];
            l++;
          }
        }
        index = 0;
        detA = 0.0;

        /* While new point is coplanar, choose another point */
        while (detA == 0.0) {
          for (j = 0; j < d; j++)
            for (l = 0; l < d + 1; l++)
              A[j*(d + 1) + l] = points[(faces[k*d + j])*(d + 1) + l];
          for (; j < d + 1; j++)
            for (l = 0; l < d + 1; l++)
              A[j*(d + 1) + l] = points[pp[index] * (d + 1) + l];
          index++;
          if (d == 3)
            detA = _convhull_det_4x4(A);
          else
            detA = _convhull_det_NxN((double*)A, d + 1);
        }

        /* Orient faces so that each point on the original simplex can't see the opposite face */
        if (detA < 0.0) {
          /* If orientation is improper, reverse the order to change the volume sign */
          for (j = 0; j < 2; j++)
            face_tmp[j] = faces[k*d + d - j - 1];
          for (j = 0; j < 2; j++)
            faces[k*d + d - j - 1] = face_tmp[1 - j];

          /* Modify the plane coefficients of the properly oriented faces */
          for (j = 0; j < d; j++)
            cf[k*d + j] = -cf[k*d + j];
          df[k] = -df[k];
          for (l = 0; l < d; l++)
            for (j = 0; j < d + 1; j++)
              A[l*(d + 1) + j] = points[(faces[k*d + l])*(d + 1) + j];
          for (; l < d + 1; l++)
            for (j = 0; j < d + 1; j++)
              A[l*(d + 1) + j] = points[pp[index] * (d + 1) + j];
#ifdef SGL_PHYSICS_CONVHULL_VERBOSE
          /* Check */
          if (d == 3)
            detA = _convhull_det_4x4(A);
          else
            detA = _convhull_det_NxN((double*)A, d + 1);
          /* If you hit this assertion error, then the face cannot be properly orientated and building the convex hull is likely impossible */
          if (detA <= 0.0)
            printf("WARNING: the face cannot be properly orientated and building the convex hull is likely impossible.\n");
#endif
        }
        free(pp);
      }
      if (horizon_size1 > 0)
        free(horizon);
      free(f0);
      free(nonvisible_faces);
      free(visible);
      free(hVec);
      free(hVec_mem_face);
    }
    if (FUCKED) {
      break;
    }
  }

  /* output */
  if (FUCKED) {
    (*out_faces) = NULL;
    if (out_cf != NULL) (*out_cf) = NULL;
    if (out_df != NULL) (*out_df) = NULL;
    (*nOut_faces) = 0;
  }
  else {
    (*out_faces) = (int*)malloc(nFaces*d * sizeof(int));
    memcpy((*out_faces), faces, nFaces*d * sizeof(int));
    (*nOut_faces) = nFaces;
    if (out_cf != NULL) {
      (*out_cf) = (double*)malloc(nFaces*d * sizeof(double));
      memcpy((*out_cf), cf, nFaces*d * sizeof(double));
    }
    if (out_df != NULL) {
      (*out_df) = (double*)malloc(nFaces * sizeof(double));
      memcpy((*out_df), df, nFaces * sizeof(double));
    }
  }

  /* clean-up */
  free(visible_ind);
  free(points_cf);
  free(points_s);
  free(face_s);
  free(gVec);
  free(meanp);
  free(absdist);
  free(reldist);
  free(desReldist);
  free(ind);
  free(span);
  free(points);
  free(faces);
  free(aVec);
  free(cf);
  free(cfi);
  free(df);
  free(p_s);
  free(fVec);
  free(bVec);
  free(A);
}


/*
Computes the Delaunay triangulation (mesh) of an arrangement
of points in N-dimensional space
*/
void _convhull_delaunay_nd_mesh(
  /* input Arguments */
  const double* points,                 /* The input points; FLAT: nPoints x nd */
  const int nPoints,                  /* Number of points */
  const int nd,                       /* The number of dimensions */
  /* output Arguments */
  int** Mesh,                         /* (&) the indices defining the Delaunay triangulation of the points;
                                          FLAT: nMesh x (nd+1) */
  int* nMesh                          /* (&) Number of triangulations */
) {
  int i, j, k, nHullFaces, maxW_idx, nVisible;
  int* hullfaces;
  double w0, w_optimal, w_optimal2;
  double* projpoints, *cf, *df, *p0, *p, *visible;

  /* Project the N-dimensional points onto a N+1-dimensional paraboloid */
  projpoints = (double*)malloc(nPoints*(nd + 1) * sizeof(double));
  for (i = 0; i < nPoints; i++) {
    projpoints[i*(nd + 1) + nd] = 0.0;
    for (j = 0; j < nd; j++) {
      projpoints[i*(nd + 1) + j] = (double)points[i*nd + j] + SGL_PHYSICS_CONVHULL_NOISE_VALUE * _convhull_rnd(i, j);
      projpoints[i*(nd + 1) + nd] += (projpoints[i*(nd + 1) + j] * projpoints[i*(nd + 1) + j]); /* w vector */
    }
  }

  /* The N-dimensional delaunay triangulation requires first computing the convex hull of this N+1-dimensional paraboloid */
  hullfaces = NULL;
  cf = df = NULL;
  _convhull_nd_build(projpoints, nPoints, nd + 1, &hullfaces, &cf, &df, &nHullFaces);

  /* Find the coordinates of the point with the maximum (N+1 dimension) coordinate (i.e. the w vector) */
  double maxVal;
  maxVal = double(-2.23e13);
  maxW_idx = -1;
  for (i = 0; i < nPoints; i++) {
    if (projpoints[i*(nd + 1) + nd] > maxVal) {
      maxVal = projpoints[i*(nd + 1) + nd];
      maxW_idx = i;
    }
  }
  if (maxW_idx == -1) {
#ifdef SGL_PHYSICS_CONVHULL_VERBOSE
    printf("ERROR: maxW_idx==-1.\n");
#endif
    free(projpoints);
    *Mesh = NULL;
    *nMesh = 0;
    return;
  }
  w0 = projpoints[maxW_idx*(nd + 1) + nd];
  p0 = (double*)malloc(nd * sizeof(double));
  for (j = 0; j < nd; j++)
    p0[j] = projpoints[maxW_idx*(nd + 1) + j];

  /* Find the point where the plane tangent to the point (p0,w0) on the paraboloid crosses the w axis.
    * This is the point that can see the entire lower hull. */
  w_optimal = 0.0;
  for (j = 0; j < nd; j++)
    w_optimal += (2.0*pow(p0[j], 2.0));
  w_optimal = w0 - w_optimal;

  /* Subtract 1000 times the absolute value of w_optimal to ensure that the point where the tangent plane
    * crosses the w axis will see all points on the lower hull. This avoids numerical roundoff errors. */
  w_optimal2 = w_optimal - double(1000.0) * fabs(w_optimal);

  /* Set the point where the tangent plane crosses the w axis */
  p = (double*)calloc((nd + 1), sizeof(double));
  p[nd] = w_optimal2;

  /* Find all faces that are visible from this point */
  visible = (double*)malloc(nHullFaces * sizeof(double));
  for (i = 0; i < nHullFaces; i++) {
    visible[i] = 0.0;
    for (j = 0; j < nd + 1; j++)
      visible[i] += cf[i*(nd + 1) + j] * p[j];
  }
  nVisible = 0;
  for (j = 0; j < nHullFaces; j++) {
    visible[j] += df[j];
    if (visible[j] > 0.0)
      nVisible++;
  }

  /* Output */
  (*nMesh) = nVisible;
  if (nVisible > 0) {
    (*Mesh) = (int*)malloc(nVisible*(nd + 1) * sizeof(int));
    for (i = 0, j = 0; i < nHullFaces; i++) {
      if (visible[i] > 0.0) {
        for (k = 0; k < nd + 1; k++)
          (*Mesh)[j*(nd + 1) + k] = hullfaces[i*(nd + 1) + k];
        j++;
      }
    }
#ifdef SGL_PHYSICS_CONVHULL_VERBOSE
    if (j != nVisible) {
      printf("WAARNING: j!=nVisible .\n");
    }
#endif
  }

  /* clean up */
  free(projpoints);
  free(hullfaces);
  free(cf);
  free(df);
  free(p0);
  free(p);
  free(visible);
}

bool Convex::build_from_points(const std::vector<Vec3>& points, int precision, bool verbose)
{
  this->destroy();

  /*
  remove vertices that are too close from each other using `precision`
  */
  std::vector<Vec3> points_;
  std::set<std::string> pset;
  auto hash = [](const Vec3& p, int precision) -> std::string {
    std::stringstream x, y, z;
    x << std::fixed << std::setprecision(precision) << p.x;
    y << std::fixed << std::setprecision(precision) << p.y;
    z << std::fixed << std::setprecision(precision) << p.z;
    return x.str() + "," + y.str() + "," + z.str();
  };
  int points_removed = 0;
  for (int i = 0; i < len(points); i++) {
    std::string hstring = hash(points[i], precision);
    if (pset.find(hstring) == pset.end()) {
      pset.insert(hstring);
      points_.push_back(points[i]);
    }
    else {
      points_removed++;
    }
  }
  if (points_removed > 0 && verbose) {
    printf("%d points removed from point cloud when building the convex hull.\n", points_removed);
  }

  /*
  build convex hull shape.
  */
  int* faceIdxs = NULL;
  int nFaces = 0;
  _convhull_3d_build(points_.data(), (int)points_.size(), &faceIdxs, &nFaces);
  if (faceIdxs == NULL) {
    /* cannot build convex hull, return empty shape */
    if (verbose) {
      printf("Error: Cannot build convex hull from points.\n");
    }
    return false;
  }

  /* pack all vertices */
  std::vector<int> vMap; /* packed vertex index -> unpacked index */
  for (int iFace = 0; iFace < nFaces; iFace++) {
    for (int iTriangle = 0; iTriangle < 3; iTriangle++) {
      int unpackedIndex, inArray = 0;
      unpackedIndex = faceIdxs[iFace * 3 + iTriangle];
      for (int j = 0; j < vMap.size(); j++) {
        if (vMap[j] == unpackedIndex) { /* this vertex is already in array */
          inArray = 1; break;
        }
      }
      if (!inArray) vMap.push_back(unpackedIndex);
    }
  }
  /* fill in pHull */
  for (int i = 0; i < vMap.size(); i++) 
    this->points.push_back(points_[vMap[i]]);
  /* pack face indices */
  int* faceIdxsPacked = (int*)malloc(nFaces * 3 * sizeof(int));
  for (int i = 0; i < vMap.size(); i++) {
    for (int j = 0; j < nFaces * 3; j++) {
      if (faceIdxs[j] == vMap[i]) faceIdxsPacked[j] = i;
    }
  }
  /* now vMap is useless, clear it */
  vMap.clear();
  /* fill in face indices */
  for (int iF = 0; iF < nFaces; iF++) {
    IVec3 f;
    f.x = faceIdxsPacked[iF * 3 + 0];
    f.y = faceIdxsPacked[iF * 3 + 1];
    f.z = faceIdxsPacked[iF * 3 + 2];
    this->faces.push_back(f);
  }

  free(faceIdxs);
  free(faceIdxsPacked);

  /* compute normals */

  std::map<int, std::vector<Vec3>> vn_map; /* vertex index -> {triangle normals} */
  for (size_t i_f = 0; i_f < this->faces.size(); i_f++) {
    const int& iv0 = this->faces[i_f].i[0];
    const int& iv1 = this->faces[i_f].i[1];
    const int& iv2 = this->faces[i_f].i[2];
    Vec3 p0 = this->points[iv0];
    Vec3 p1 = this->points[iv1];
    Vec3 p2 = this->points[iv2];
    Vec3 normal = normalize(cross(p1 - p0, p2 - p0));
    if (vn_map.find(iv0) == vn_map.end()) 
      vn_map.emplace(iv0, std::vector<Vec3>());
    if (vn_map.find(iv1) == vn_map.end()) 
      vn_map.emplace(iv1, std::vector<Vec3>());
    if (vn_map.find(iv2) == vn_map.end()) 
      vn_map.emplace(iv2, std::vector<Vec3>());
    vn_map[iv0].push_back(normal);
    vn_map[iv1].push_back(normal);
    vn_map[iv2].push_back(normal);
  }

  for (int i_v = 0; i_v < (int)this->points.size(); i_v++) {
    Vec3 normal = Vec3(0, 0, 0);
    for (int i_n = 0; i_n < (int)vn_map[i_v].size(); i_n++) {
      Vec3 n = vn_map[i_v][i_n];
      if (!n.is_finite())
        continue;
      else
        normal += n;
    }
    normal = normalize(normal);
    this->normals.push_back(normal);
  }

  return true;
}

Convex build_convex_3D(const std::vector<Vec3>& points)
{
  Convex convobj;
  convobj.build_from_points(points);
  return convobj;
}

Convex build_convex_3D(const Model & model)
{
  const std::vector<sgl::Mesh>& meshes = model.get_meshes();
  std::vector<sgl::Vec3> vertices;
  for (size_t i = 0; i < meshes.size(); i++) {
    for (size_t i_v = 0; i_v < meshes[i].vertices.size(); i_v++) {
      vertices.push_back(meshes[i].vertices[i_v].position);
    }
  }
  return build_convex_3D(vertices);
}

Convex build_convex_3D(const Mesh & mesh)
{
  std::vector<sgl::Vec3> vertices;
  for (size_t i_v = 0; i_v < mesh.vertices.size(); i_v++) {
    vertices.push_back(mesh.vertices[i_v].position);
  }
  return build_convex_3D(vertices);
}

Convex build_convex_3D(const char * zip_file, const char* model_fname)
{
  Model model;
  if (!model.load_zip(zip_file, model_fname))
    return Convex();
  else
    return build_convex_3D(model);
}

triangle Convex::get_triangle(const int & ind) const
{
  triangle t;
  t.p[0] = this->points[faces[ind].x];
  t.p[1] = this->points[faces[ind].y];
  t.p[2] = this->points[faces[ind].z];
  return t;
}

Vec3 Convex::center_of_mass() const
{
  /* http://melax.github.io/volint.html */
  Vec3 CoM(0, 0, 0); /* center of mass */
  double volume = 0.0; /* actually accumulates the volume*6 */
  int nFaces = (int)faces.size();
  for (int i = 0; i < nFaces; i++) {
    triangle T = get_triangle(i);
    Vec3& w1 = T.p[0], &w2 = T.p[1], &w3 = T.p[2];
    Mat3x3 A = Mat3x3(
      w1.x, w1.y, w1.z,
      w2.x, w2.y, w2.z,
      w3.x, w3.y, w3.z);
    double vol = det(A); /* dont bother to divide by 6 since it does not affect CoM */
    CoM += vol * (w1 + w2 + w3); /* divide by 4 at end */
    volume += vol;
  }
  CoM = CoM / (volume * 4.0);
  return CoM;
}

Mat3x3 Convex::inertia_tensor(double mass, const Vec3 & CoM) const
{
  /*
  The moments are calculated based on the center of rotation com which you
  should calculate first.

  assume mass==1.0  you can multiply by mass later.

  for improved accuracy the next 3 variables, the determinant d, and its calculation
  should be changed to double.
  */
  double volume = 0.0; /* technically this variable accumulates the volume times 6  */
  Vec3 diag;          /* accumulate matrix main diagonal integrals [x*x, y*y, z*z] */
  Vec3 offd;          /* accumulate matrix off-diagonal integrals [y*z, x*z, x*y]  */
  int nFaces = (int)faces.size();
  for (int i = 0; i < nFaces; i++) {
    triangle T = get_triangle(i);
    Vec3 w1 = T.p[0] - CoM, w2 = T.p[1] - CoM, w3 = T.p[2] - CoM;
    Mat3x3 A = Mat3x3(
      w1.x, w1.y, w1.z,
      w2.x, w2.y, w2.z,
      w3.x, w3.y, w3.z);
    double d = det(A); /* vol of tiny parallelapiped = d * dr * ds * dt
                       (the 3 partials of my tetral triple integral equasion) */
    volume += d;     /* add vol of current tetra (note it could be negative -
                     that's ok we need that sometimes) */
    for (int j = 0; j < 3; j++) {
      int j1 = (j + 1) % 3;
      int j2 = (j + 2) % 3;
      diag.i[j] += (A.at(0, j) * A.at(1, j) + A.at(1, j) * A.at(2, j) + A.at(2, j) * A.at(0, j) +
        A.at(0, j) * A.at(0, j) + A.at(1, j) * A.at(1, j) + A.at(2, j) * A.at(2, j)) *d; /* divide by 60.0f later */
      offd.i[j] += (A.at(0, j1) * A.at(1, j2) + A.at(1, j1) * A.at(2, j2) + A.at(2, j1) * A.at(0, j2) +
        A.at(0, j1) * A.at(2, j2) + A.at(1, j1) * A.at(0, j2) + A.at(2, j1) * A.at(1, j2) +
        A.at(0, j1) * A.at(0, j2) * 2 + A.at(1, j1) * A.at(1, j2) * 2 + A.at(2, j1) * A.at(2, j2) * 2) *d; /* divide by 120.0f later */
    }
  }
  /* divide by total volume (vol/6) since density=1/volume */
  diag = diag / (volume * (60.0 / 6.0));
  offd = offd / (volume * (120.0 / 6.0));
  return mass * Mat3x3(
    diag.y + diag.z, -offd.z, -offd.y,
    -offd.z, diag.x + diag.z, -offd.x,
    -offd.y, -offd.x, diag.x + diag.y);
}

double Convex::bounding_sphere(const Vec3 & center) const
{
  if (this->points.size() == 0)
    return 0.0;
  double radius = length(this->points[0] - center);
  for (int i = 1; i < len(this->points); i++) {
    radius = max(radius, length(points[i] - center));
  }
  return radius;
}

bool Convex::export_obj(const char * file) const
{
  
  /* now vn holds all the averaged normals for all vertices */
  FILE* fp = fopen(file, "w");
  if (fp == NULL) return false;
  auto write = [&](const char* s) -> void {
    fwrite(s, 1, strlen(s), fp);
  };
  
  write("# convex shape\n");
  
  for (int i_v = 0; i_v < this->points.size(); i_v++) {
    char buf[1024];
    sprintf(buf, "v %.6f %.6f %.6f\n", this->points[i_v].x, this->points[i_v].y, this->points[i_v].z);
    write(buf);
  }
  
  write("\n");

  for (int i_n = 0; i_n < this->normals.size(); i_n++) {
    char buf[1024];
    sprintf(buf, "vn %.6f %.6f %.6f\n", this->normals[i_n].x, this->normals[i_n].y, this->normals[i_n].z);
    write(buf);
  }

  for (int i_f = 0; i_f < this->faces.size(); i_f++) {
    char buf[1024];
    sprintf(buf, "f %d//%d %d//%d %d//%d\n",
      this->faces[i_f].x + 1, this->faces[i_f].x + 1,
      this->faces[i_f].y + 1, this->faces[i_f].y + 1,
      this->faces[i_f].z + 1, this->faces[i_f].z + 1);
    write(buf);
  }

  fclose(fp);
  return true;
}

bool Convex::from_obj(const char* file)
{
  destroy();
  sgl::SimpleOBJLoader loader;
  std::vector<sgl::Vec3> vertices = loader.load_v(file);
  if (vertices.size() == 0)
    return false;

  this->build_from_points(vertices);

  if (this->points.size() == 0)
    return false;

  return true;
}

void Convex::destroy() {
  this->points.clear();
  this->points.shrink_to_fit();
  this->normals.clear();
  this->normals.shrink_to_fit();
  this->faces.clear();
  this->faces.shrink_to_fit();
}

Vec3 Convex::get_point(int index) const {
  return this->points[index];
}

const std::vector<Vec3>& Convex::get_points() const
{
  return this->points;
}

Vec3 Convex::get_normal(int index) const
{
  return this->normals[index];
}

IVec3 Convex::get_triangle_indices(int index) const
{
  return this->faces[index];
}

int Convex::num_points() const 
{ 
  return (int)this->points.size();
}

int Convex::num_faces() const
{
  return (int)this->faces.size();
}

};
};

