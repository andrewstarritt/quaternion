/* quaternion_basic.c
 *
 * This file is part of the Python quaternion module. It provides basic
 * quaternion maths operation with minimalist reference to Python.
 *
 * Copyright (c) 2018-2024  Andrew C. Starritt
 *
 * The quaternion module is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The quaternion module is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the quaternion module.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact details:
 * andrew.starritt@gmail.com
 * PO Box 3118, Prahran East, Victoria 3181, Australia.
 *
 * source formatting:
 *    indent -kr -pcs -i3 -cli3 -nbbo -nut -l96
 */

#include "quaternion_basic.h"
#include <stdio.h>
#include <stdarg.h>
#include <Python.h>
#include <complex.h>

static const char* red   = "\033[31;1m";
static const char* green = "\033[32;1m";
static const char* blue  = "\033[34;1m";
static const char* reset = "\033[00m";

static bool do_use_colour = false;

/* -----------------------------------------------------------------------------
 * PRIVATE FUNCTIONS
 * -----------------------------------------------------------------------------
 * Find the largest absolute coefficient of a
 */
static double quat_max_abs_elem (const Py_quaternion a)
{
   double r, t;
   r = fabs (a.w);
   t = fabs (a.x);
   if (t > r)
      r = t;
   t = fabs (a.y);
   if (t > r)
      r = t;
   t = fabs (a.z);
   if (t > r)
      r = t;
   return r;
}

/* -----------------------------------------------------------------------------
 * Find length of a triple.
 */
static double length_triple (const Py_quat_triple t)
{
   double ax = fabs (t.x);
   double ay = fabs (t.y);
   double az = fabs (t.z);

   /* Find max abs value of the tiplet
    */
   double m = ax;
   if (ay > m) m = ay;
   if (az > m) m = az;

   if (m == 0.0)
      return m;

   /* Scale coordinates - avoid potential intermediate overflow
    */
   double x = t.x / m;
   double y = t.y / m;
   double z = t.z / m;

   return m * sqrt (x*x + y*y + z*z);
}


/* -----------------------------------------------------------------------------
 * Decomposes q in to return real, imag and unit as follows:
 * Let v be the Quaternion (1, 0, 0, 0)
 * Let u be the Quaternion (0, q.x, q.y, q.z) normalised,
 * i.e. the imaginary part of q normalised,
 * then we can define q as:
 *     q = real*v + imag*u
 * where real and imag are the real and the imaginary coefficiants (both real).
 */
static void decompose (const Py_quaternion q,
                       double* real, double* imag,
                       Py_quat_triple* unit)
{
   Py_quat_triple temp;

   /* Extract the real part of q
    */
   *real = q.w;

   /* Extract and normalise the vector or imaginary part of q
    */
   temp.x = q.x;
   temp.y = q.y;
   temp.z = q.z;
   *imag = length_triple (temp);

   /* Form unit/normalised imaginary part
    */
   if (*imag != 0.0) {
      unit->x = q.x / (*imag);
      unit->y = q.y / (*imag);
      unit->z = q.z / (*imag);
   } else {
      /* Any unit vector will do - go with j.
       */
      unit->x = 0.0;
      unit->y = 1.0;
      unit->z = 0.0;
   }

   /* q = real + imag*unit */
}

/* -----------------------------------------------------------------------------
 */
static Py_quaternion compose (const double real, const double imag,
                              const Py_quat_triple unit)
{
   Py_quaternion result;

   result.w = real;
   result.x = imag * unit.x;
   result.y = imag * unit.y;
   result.z = imag * unit.z;

   return result;
}


/* -----------------------------------------------------------------------------
 * Returns: f(q) by leveraging of the equivalent complex function, e.g csin.
 * We do not use a series expansion, but reply on the fact that because the
 * the function can be defined as a series expansion, then the axis of f(q)
 * is the same as the axis of q.
 * Let v be the Quaternion (1, 0, 0, 0)
 * Let u be the normalised Quaternion (0, q.x, q.y, q.z), the imaginary part of q,
 * then we can define q as:
 *     q = A*v + B*u
 * where A and B are the real and the imaginary coefficiants (both real).
 * Define complex z to be A + B*I
 * Invoke f(z) to yield fz = C + D*I
 * Then f(q) = C*v + D*u
 * Note: this is about 80% the time of the series expansion.
 *
 * Question: is it koshar to use csin, ctan etc.? py complex does not.
 * We may have create local wrapper function to do csin, ctan if needs be.
 */
typedef complex (*cfunc) (complex z);

static Py_quaternion use_complex_func (const Py_quaternion q, const cfunc f)
{
   Py_quaternion result;
   double a, b;
   Py_quat_triple unit;
   complex z;
   complex fz;
   double c, d;

   /* First decompse q
    */
   decompose (q, &a, &b, &unit);

   /* q = a + b*unit
    * form the complex equivalent of q
    */
   z = a + I*b;

   fz = f(z);   /* let f do the hard work */

   c = creal(fz);
   d = cimag(fz);

   /* form the quaternion equivalent of f(q)
    * f(q) = c + d*unit
    */
   result = compose (c, d, unit);
   return result;
}


/* -----------------------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -----------------------------------------------------------------------------
 *
 */
void _Py_quat_set_use_colour(bool use_colour)
{
   do_use_colour = use_colour;
}

/* -----------------------------------------------------------------------------
 */
bool _Py_quat_using_colour()
{
   return do_use_colour;
}

/* -----------------------------------------------------------------------------
 */
char * _Py_quat_to_string (const Py_quaternion a,
                           char format_code,
                           const int precision)
{
   char *result = NULL;

   /* If these are non-NULL, they'll need to be freed.
    */
   char *ps = NULL;
   char *px = NULL;
   char *py = NULL;
   char *pz = NULL;

   ps = PyOS_double_to_string(a.w, format_code, precision, 0, NULL);
   if (!ps) {
      goto done;
   }

   px = PyOS_double_to_string(a.x, format_code, precision, Py_DTSF_SIGN, NULL);
   if (!px) {
      goto done;
   }

   py = PyOS_double_to_string(a.y, format_code, precision, Py_DTSF_SIGN, NULL);
   if (!px) {
      goto done;
   }

   pz = PyOS_double_to_string(a.z, format_code, precision, Py_DTSF_SIGN, NULL);
   if (!px) {
      goto done;
   }

   result = _Py_quat_to_string2 (0, ps, px, py, pz);

done:
   if (ps) PyMem_Free(ps);
   if (px) PyMem_Free(px);
   if (py) PyMem_Free(py);
   if (pz) PyMem_Free(pz);

   return result;
}

/* -----------------------------------------------------------------------------
 */
char * _Py_quat_to_string2 (const int size,
                            const char* psin,
                            const char* pxin,
                            const char* pyin,
                            const char* pzin)
{
   char *result = NULL;
   size_t total;
   int spaces;
   size_t alloc;
   int j;
   char *image;
   int imsize;

   /* Local copies - data constant - not the pointer
    */
   const char* ps = psin;
   const char* px = pxin;
   const char* py = pyin;
   const char* pz = pzin;

   /* Strip leading spaces.
    */
   while (*ps == ' ') ps++;
   while (*px == ' ') px++;
   while (*py == ' ') py++;
   while (*pz == ' ') pz++;

   /* Do the i, j and/or k components need a sign?
    */
   bool sx = (*px != '+') && (*px != '-');
   bool sy = (*py != '+') && (*py != '-');
   bool sz = (*pz != '+') && (*pz != '-');

   /* ps, px, py and pz may include +/- but not:  'i', 'j', 'k' , '(' and ')'
    */
   total = strlen(ps) + strlen(px) + strlen(py) + strlen(pz) + 5 + sx + sy + sz;

   spaces = size > total ? size - total : 0;
   alloc = (spaces + total) + (do_use_colour ? + 48 : 12);    /* 48/12 carries some spare as well */

   result = PyMem_Malloc(alloc);

   /* pre-fill start with required number of spaces - if any.
    */
   for (j = 0; j < spaces; j++)
      result[j] = ' ';

   image = result + spaces;  /* sprintf offset */
   imsize = alloc - spaces;  /* and remaining space */

   if (do_use_colour) {
      snprintf(image, imsize, "(%s%s%s%si%s%s%s%sj%s%s%s%sk%s)", ps,
               sx ? "+" : "", px, red, reset,
               sy ? "+" : "", py, green, reset,
               sz ? "+" : "", pz, blue, reset);
   } else {
      snprintf(image, imsize, "(%s%s%si%s%sj%s%sk)", ps,
               sx ? "+" : "", px,
               sy ? "+" : "", py,
               sz ? "+" : "", pz);
   }

   return result;
}


/* -----------------------------------------------------------------------------
 *  true if all parts finite
 */
bool _Py_quat_isfinite (const Py_quaternion a)
{
   return Py_IS_FINITE (a.w) && Py_IS_FINITE (a.x) &&
          Py_IS_FINITE (a.y) && Py_IS_FINITE (a.z);
}

/* -----------------------------------------------------------------------------
 *  true if any part positive or negative infinite
 */
bool _Py_quat_isinf    (const Py_quaternion a)
{
   return Py_IS_INFINITY (a.w) || Py_IS_INFINITY (a.x) ||
          Py_IS_INFINITY (a.y) || Py_IS_INFINITY (a.z);
}

/* -----------------------------------------------------------------------------
 *  true if any part is NaN
 */
bool _Py_quat_isnan    (const Py_quaternion a)
{
   return Py_IS_NAN (a.w) || Py_IS_NAN (a.x) ||
          Py_IS_NAN (a.y) || Py_IS_NAN (a.z);
}


/* -----------------------------------------------------------------------------
 * Returns: a == b
 */
int _Py_quat_eq (const Py_quaternion a, const Py_quaternion b)
{
   return (a.w == b.w) && (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

/* -----------------------------------------------------------------------------
 * Returns: a != b
 */
int _Py_quat_ne (const Py_quaternion a, const Py_quaternion b)
{
   return !_Py_quat_eq (a, b);
}

/* -----------------------------------------------------------------------------
 * Returns: a + b
 */
Py_quaternion _Py_quat_sum (const Py_quaternion a, const Py_quaternion b)
{
   Py_quaternion r;
   r.w = a.w + b.w;
   r.x = a.x + b.x;
   r.y = a.y + b.y;
   r.z = a.z + b.z;
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: a - b
 */
Py_quaternion _Py_quat_diff (const Py_quaternion a, const Py_quaternion b)
{
   Py_quaternion r;
   r.w = a.w - b.w;
   r.x = a.x - b.x;
   r.y = a.y - b.y;
   r.z = a.z - b.z;
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: a * b
 * Note: in general a * b != b * a
 */
Py_quaternion _Py_quat_prod (const Py_quaternion a, const Py_quaternion b)
{
   Py_quaternion r;
   r.w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z);
   r.x = (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y);
   r.y = (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z);
   r.z = (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x);
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: a / b, technically a * inverse (b), as opposed to inverse (b) * a
 */
Py_quaternion _Py_quat_quot (const Py_quaternion a, const Py_quaternion b)
{
   Py_quaternion r;             /* the result */

   double m;
   m = quat_max_abs_elem (b);
   if (m == 0.0) {
      errno = EDOM;     /**  Needs Python.h  **/
      r.w = r.x = r.y = r.z = 0.0;
   } else {
      Py_quaternion sa, sb, sbc, nom;
      double denom;

      /* Scale a and b by 1/m:
       * Note: a/b == (a/m) / (b/m)
       */
      sa.w = a.w / m;
      sa.x = a.x / m;
      sa.y = a.y / m;
      sa.z = a.z / m;

      sb.w = b.w / m;
      sb.x = b.x / m;
      sb.y = b.y / m;
      sb.z = b.z / m;

      /* Form b^, i.e. complex conjugate of b
       */
      sbc.w = + sb.w;
      sbc.x = - sb.x;
      sbc.y = - sb.y;
      sbc.z = - sb.z;

      /* form numerator = a * b^
       * Note we use a*b^, not b^*a
       */
      nom = _Py_quat_prod (sa, sbc);

      /* denominator is b * b^ which is real
       */
      denom = (sb.w * sb.w) + (sb.x * sb.x) + (sb.y * sb.y) + (sb.z * sb.z);

      r.w = nom.w / denom;
      r.x = nom.x / denom;
      r.y = nom.y / denom;
      r.z = nom.z / denom;
   }

   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: -a
 */
Py_quaternion _Py_quat_neg (const Py_quaternion a)
{
   Py_quaternion r;
   r.w = -a.w;
   r.x = -a.x;
   r.y = -a.y;
   r.z = -a.z;
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: conjugate of a
 */
Py_quaternion _Py_quat_conjugate (const Py_quaternion a)
{
   Py_quaternion r;
   r.w = +a.w;
   r.x = -a.x;
   r.y = -a.y;
   r.z = -a.z;
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: inverse of a
 */
Py_quaternion _Py_quat_inverse (const Py_quaternion a)
{
   Py_quaternion r;
   double denom;

   /* denominator is a * a^ which is real
    */
   denom = (a.w * a.w) + (a.x * a.x) + (a.y * a.y) + (a.z * a.z);

   if (denom == 0.0) {
      errno = EDOM;     /** Needs Python.h **/
      r.w = r.x = r.y = r.z = 0.0;
   } else {
      /* conjugate / denom */
      r.w = +a.w / denom;
      r.x = -a.x / denom;
      r.y = -a.y / denom;
      r.z = -a.z / denom;
   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: a / |a|
 */
Py_quaternion _Py_quat_normalise (const Py_quaternion a)
{
   Py_quaternion r;
   double m;
   m = _Py_quat_abs (a);

   if (m == 0.0) {
      errno = EDOM;     /** Needs Python.h **/
      r.w = r.x = r.y = r.z = 0.0;
   } else {
      r.w = a.w / m;
      r.x = a.x / m;
      r.y = a.y / m;
      r.z = a.z / m;
   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: round of each component.
 */
Py_quaternion _Py_quat_round (const Py_quaternion a, const int n)
{
   Py_quaternion r;
   double t;
   int j;

   t = 1.0;
   if (n > 0) {
      for (j = 0; j < n; j++)
         t *= 10.0;
   } else if (n < 0) {
      for (j = 0; j > n; j--)
         t *= 10.0;
      t = 1.0/t;
   }

   r.w = round (a.w * t) / t;
   r.x = round (a.x * t) / t;
   r.y = round (a.y * t) / t;
   r.z = round (a.z * t) / t;

   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: a ** b
 */
Py_quaternion _Py_quat_pow1 (const Py_quaternion a, const double b)
{
   Py_quaternion r;
   double ar, ai;
   Py_quat_triple unit;
   complex za, zb;
   complex zr;
   double rr, ri;

   /* special cases */
   if (b == 0.0) {
      /* b == 0: a ** 0 == 1 (even when a == 0) */
      r.w = 1.0;
      r.x = 0.0;
      r.y = 0.0;
      r.z = 0.0;

   } else if (a.w == 0.0 && a.x == 0.0 && a.y == 0.0 && a.z == 0.0) {
      /* a == 0: 0 ** b == 0 */
      r.w = r.x = r.y = r.z = 0.0;

      /* unless negative power */
      if (b < 0.0)
         errno = EDOM;

   } else if (b == 1.0) {
      /* a ** 1 == a */
      r = a;

   } else {

      /* Form complex equivalent of a
       */
      decompose(a, &ar, &ai, &unit);
      za = ar + I*ai;

      /* Form complex equivalent of b
       */
      zb = b + I*0.0;

      zr = cpow (za, zb);   /* Let cpow do all the hard work */

      /* form the quaternion equivalent of zr
       * r = rr + ri*unit
       */
      rr = creal (zr);
      ri = cimag (zr);
      r = compose (rr, ri, unit);

   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: a ** b
 */
Py_quaternion _Py_quat_pow2 (const double a, const Py_quaternion b)
{
   Py_quaternion r;
   double br, bi;
   Py_quat_triple unit;
   complex za, zb;
   complex zr;
   double rr, ri;

   /* special cases */
   if (b.w == 0.0 && b.x == 0.0 && b.y == 0.0 && b.z == 0.0) {
      /* b == 0: a ** 0 == 1 (even when a == 0) */
      r.w = 1.0;
      r.x = 0.0;
      r.y = 0.0;
      r.z = 0.0;

   } else if (a == 0.0) {
      /* a == 0: 0 ** b == 0 */
      r.w = r.x = r.y = r.z = 0.0;

      /* unless negative or has imaginary part a la complex */
      if (b.w < 0.0 || b.x != 0.0 || b.y != 0.0 || b.z != 0.0)
         errno = EDOM;

   } else if (b.w == 1.0 && b.x == 0.0 && b.y == 0.0 && b.z == 0.0) {
      /* b == 1: a ** 1 == a */
      r.w = a;
      r.x = 0.0;
      r.y = 0.0;
      r.z = 0.0;

   } else {

      /* Form complex equivalent of a
       */
      za = a + I*0.0;

      /* Form complex equivalent of b
    */
      decompose(b, &br, &bi, &unit);
      zb = br + I*bi;

      zr = cpow (za, zb);   /* Let cpow do all the hard work */

      /* form the quaternion equivalent of zr
    * r = rr + ri*unit
    */
      rr = creal (zr);
      ri = cimag (zr);
      r = compose (rr, ri, unit);
   }
   return r;
}


/* -----------------------------------------------------------------------------
 * Returns: abs (a) or |a|
 */
double _Py_quat_abs (const Py_quaternion a)
{
   /* sets errno = ERANGE on overflow;  otherwise errno = 0 */
   double result;

   /* Is any element infinite or NaN - needs Python.h
    */
   if (!Py_IS_FINITE (a.w) || !Py_IS_FINITE (a.x) || !Py_IS_FINITE (a.y) || !Py_IS_FINITE (a.z)) {
      /* C99 rules: if either the real or the imaginary parts is an
         infinity, return infinity, even if the other part is a NaN.
       */
      if (Py_IS_INFINITY (a.w)) {
         result = fabs (a.w);
         errno = 0;
         return result;
      }
      if (Py_IS_INFINITY (a.x)) {
         result = fabs (a.x);
         errno = 0;
         return result;
      }
      if (Py_IS_INFINITY (a.y)) {
         result = fabs (a.y);
         errno = 0;
         return result;
      }
      if (Py_IS_INFINITY (a.z)) {
         result = fabs (a.z);
         errno = 0;
         return result;
      }
      /* either the real or imaginary part is a NaN,
         and neither is infinite. Result should be NaN. */
      return Py_NAN;
   }

   double m = quat_max_abs_elem (a);
   if (m > 0.0) {
      double ss = a.w / m;
      double sx = a.x / m;
      double sy = a.y / m;
      double sz = a.z / m;
      result = m * sqrt ((ss * ss) + (sx * sx) + (sy * sy) + (sz * sz));
   } else {
      result = 0.0;
   }

   if (!Py_IS_FINITE (result))
      errno = ERANGE;
   else
      errno = 0;
   return result;
}

/* -----------------------------------------------------------------------------
 * Returns: the quadrance of a, i.e. abs(a)**2, or a.a
 */
double _Py_quat_quadrance (const Py_quaternion a)
{
   double result;
   errno = 0;
   /* Needs checking as per abs
    * Use _Py_quat_dot_prod (a, a) ??
    */
   result = (a.w*a.w) + (a.x*a.x) + (a.y*a.y) + (a.z*a.z);
   return result;
}

/* -----------------------------------------------------------------------------
 * Returns: inner product
 */
double _Py_quat_dot_prod (const Py_quaternion a, const Py_quaternion b)
{
   double result;
   errno = 0;

   /* Needs checking as per abs
    */
   result = (a.w*b.w) + (a.x*b.x) + (a.y*b.y) + (a.z*b.z);
   return result;
}


/* -----------------------------------------------------------------------------
 * Returns: linear interpolation between two quaternions - simple
 */
Py_quaternion _Py_quat_lerp (const Py_quaternion a, const Py_quaternion b, const double t)
{
   Py_quaternion r;   // the result

   double sa = 1.0 - t;   /* scale factor for a */
   double sb = t;         /* scale factor for b */

   r.w = sa*a.w + sb*b.w;
   r.x = sa*a.x + sb*b.x;
   r.y = sa*a.y + sb*b.y;
   r.z = sa*a.z + sb*b.z;

   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: spherical linear interpolation between two quaternions
 * https://en.wikipedia.org/wiki/Slerp
 *
 * We tried   a * (a.inverse() * b)**t
 * This seems better when a and or b are not rotation quaternions.
 */
Py_quaternion _Py_quat_slerp (const Py_quaternion a_in, const Py_quaternion b, const double t)
{
   Py_quaternion r;   // the result
   Py_quaternion a = a_in;

   /* Go with linear interpolation, unless we satisfy criteria
    */
   double sa = 1.0 - t;   /* scale factor for a */
   double sb = t;         /* scale factor for b */

   /* Calculate the angle, theta, between two quaternions using dot product, dp,
    * and the asbsolue values of each quaternion.
    *
    * recall:
    *     dp = |a|.|b|.cos(theta)
    * and:
    *     dp = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z
    *
    * and therefore:
    *     cos(theta) = dp / (|a|.|b|)
    */
   double k = _Py_quat_abs (a_in) * _Py_quat_abs (b);

   /* We need to avoid the divide by zero.
    * Test against 1.0e-nn as opposed to zero ??
    */
   if (k > 0.0) {

      double dp = _Py_quat_dot_prod (a, b);

      /* If pointing in opposite directins , i.e. |theta| > 90deg
       * then negate one input, a, and the dot product.
       * We can do this as a and -a effectively represent the same rotation.
       * This avoids going the long-way-around.
       */
      if (dp < 0.0) {
         dp = -dp;
         a = _Py_quat_neg(a);
      }

      const double cos_theta = dp / k;

      /* Check if cos theta nearly one, implies theta nearly 0,
       * i.e. a and b are going in nearly same direction - approx within 0.5 degrees.
       * This check also avoid floating errors yielding a value just a squillionth
       * greater than 1.0
       */
    if (cos_theta < 0.99996) {
         double theta = acos (cos_theta);
         double sin_theta = sin (theta);

         sa = sin(sa * theta) / sin_theta;
         sb = sin(sb * theta) / sin_theta;
      }
      /* else theta approching zero, just  go with liniar.
       * In the limit x ---> 0,  sin (t * x) / x  ---> t
       */

   }
   /* else either or both a and b are zero - go with liniar. */

   r.w = sa*a.w + sb*b.w;
   r.x = sa*a.x + sb*b.x;
   r.y = sa*a.y + sb*b.y;
   r.z = sa*a.z + sb*b.z;

   return r;
}


/* -----------------------------------------------------------------------------
 * Returns: rotation quaternion value
 */
Py_quaternion _Py_quat_calc_rotation (const double angle, const Py_quat_triple axis)
{
   Py_quaternion r;
   double m;

   /* Find length of ther axis or rotation
    */
   m = length_triple (axis);
   if (m == 0.0) {
      errno = EDOM;     /** Needs Python.h **/
      r.w = r.x = r.y = r.z = 0.0;
   } else {
      /* Scale to avoid potential intermediate overflows
       */
      double sx = axis.x / m;
      double sy = axis.y / m;
      double sz = axis.z / m;

      double norm = sqrt (sx * sx + sy * sy + sz * sz);

      /* Note: half the angle here
       */
      double caot = cos (angle / 2.0);
      double saot = sin (angle / 2.0);

      r.w = caot;
      r.x = saot * sx / norm;
      r.y = saot * sy / norm;
      r.z = saot * sz / norm;
   }

   return r;
}


/* -----------------------------------------------------------------------------
 * Returns: the equivilent 3D rotation matrix of a.
 */
void _Py_quat_to_rotation_matrix (const Py_quaternion a,
                                  Py_quat_matrix* matrix)
{
   /* Based on:
    * https://automaticaddison.com/how-to-convert-a-quaternion-to-a-rotation-matrix/
    */

   /* First row of the rotation matrix
    */
   matrix->r11 = 2 * (a.w * a.w + a.x * a.x) - 1.0;
   matrix->r12 = 2 * (a.x * a.y - a.w * a.z);
   matrix->r13 = 2 * (a.x * a.z + a.w * a.y);

   /* Second row of the rotation matrix
    */
   matrix->r21 = 2 * (a.x * a.y + a.w * a.z);
   matrix->r22 = 2 * (a.w * a.w + a.y * a.y) - 1.0;
   matrix->r23 = 2 * (a.y * a.z - a.w * a.x);

   /* Third row of the rotation matrix
    */
   matrix->r31 = 2 * (a.x * a.z - a.w * a.y);
   matrix->r32 = 2 * (a.y * a.z + a.w * a.x);
   matrix->r33 = 2 * (a.w * a.w + a.z * a.z) - 1.0;
}

/* -----------------------------------------------------------------------------
 * Composes a quaternion from a 3D rotation matrix
 * Based on:
 * https://math.stackexchange.com/questions/893984/conversion-of-rotation-matrix-to-quaternion
 * https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
 *
 * with the matrix transposed.
 *
 */
Py_quaternion _Py_quat_from_rotation_matrix (const Py_quat_matrix* matrix)
{
   Py_quaternion r;
   double t;
   double st;

   if (matrix->r33 < 0) {
      if (matrix->r11 > matrix->r22) {
         t = 1.0 + matrix->r11 - matrix->r22 - matrix->r33;
         r.w = matrix->r32 - matrix->r23;
         r.x = t;
         r.y = matrix->r21 + matrix->r12;
         r.z = matrix->r13 + matrix->r31;
      } else {
         t = 1.0 - matrix->r11 + matrix->r22 - matrix->r33;
         r.w = matrix->r13 - matrix->r31;
         r.x = matrix->r21 + matrix->r12;
         r.y = t;
         r.z = matrix->r32 + matrix->r23;
      }
   } else {
      if (matrix->r11 < -matrix->r22) {
         t = 1.0 - matrix->r11 - matrix->r22 + matrix->r33;
         r.w = matrix->r21 - matrix->r12;
         r.x = matrix->r13 + matrix->r31;
         r.y = matrix->r32 + matrix->r23;
         r.z = t;
      } else {
         t = 1.0 + matrix->r11 + matrix->r22 + matrix->r33;
         r.w = t;
         r.x = matrix->r32 - matrix->r23;
         r.y = matrix->r13 - matrix->r31;
         r.z = matrix->r21 - matrix->r12;
      }
   }

   st = sqrt(t);

   r.w *= 0.5 / st;
   r.x *= 0.5 / st;
   r.y *= 0.5 / st;
   r.z *= 0.5 / st;

   return r;
}


/* -----------------------------------------------------------------------------
 * Returns: 3-tuple representing rotation of point about origin
 */
Py_quat_triple _Py_quat_rotate (const Py_quaternion a,
                                const Py_quat_triple point,
                                const Py_quat_triple origin)
{
   Py_quat_triple r;

   Py_quaternion p;
   Py_quaternion ac;
   Py_quaternion ap;
   Py_quaternion t;

   p.w = 0.0;
   p.x = point.x - origin.x;
   p.y = point.y - origin.y;
   p.z = point.z - origin.z;

   /* Calc: a * p * a^ 
    */
   ac = _Py_quat_conjugate (a);

   ap = _Py_quat_prod (a, p);
   t = _Py_quat_prod (ap, ac);

   r.x = t.x + origin.x;
   r.y = t.y + origin.y;
   r.z = t.z + origin.z;

   return r;
}

/* -----------------------------------------------------------------------------
 * Decomposes a quaternion into its polar format
 * a  ->  m * (cos(angle) + unit.sin(angle))
 */
void _Py_quat_into_polar (const Py_quaternion a,
                          double* m,
                          Py_quat_triple* unit,
                          double* phase)
{
   Py_quat_triple v;
   double c, s;
   *m = _Py_quat_abs (a);

   /* Is this essentially a null quaternion?
    */
   if (*m < 1.0e-160) {
      *phase  = 0.0;
      unit->x = 0.0;
      unit->y = 1.0;
      unit->z = 0.0;
      return;
   }

   /* Normalise a
   */
   c   = a.w / (*m);    /* cos (angle) */
   v.x = a.x / (*m);
   v.y = a.y / (*m);
   v.z = a.z / (*m);

   /* v already normalised, so no overlow issues here.
    */
   s = sqrt (v.x*v.x + v.y*v.y + v.z*v.z);   /* sin (angle) */

   *phase = atan2 (s, c);
   if (s < 1.0e-20) {
      /* Basically real - no imaginary parts
       */
      unit->x = 0.0;
      unit->y = 1.0;    /* arbitary - any unit vector would do. */
      unit->z = 0.0;
      return;
   }

   /* Form the unit vector.
    */
   unit->x = v.x/s;
   unit->y = v.y/s;
   unit->z = v.z/s;
}

/* -----------------------------------------------------------------------------
 * Composes a quaternion from its polar components.
 * a  <-  m * (cos(angle) + unit.sin(angle))
 *
 * Function ensures |unit| is one.
 *
 * Note: this is similar but subtly different from _Py_quat_calc_rotation
 * as calc_rotation returns a unit length quaternion and from_polar does not.
 * Also _Py_quat_calc_rotation uses the half the angle.
 */
Py_quaternion _Py_quat_from_polar (const double m,
                                   const Py_quat_triple unit,
                                   const double phase)
{
   Py_quaternion r;
   double u;
   double c, s;
   double t;

   u = length_triple (unit);
   if (u == 0.0) {
      errno = EDOM;     /** Needs Python.h **/
      r.w = r.x = r.y = r.z = 0.0;
   } else {

      c = cos (phase);
      s = sin (phase);

      r.w = m * c;

      t = m*s/u;    /* combine length, sine and normalisation factor. */
      r.x = t * unit.x;
      r.y = t * unit.y;
      r.z = t * unit.z;
   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns sqrt (q)
 */
Py_quaternion _Py_quat_sqrt (const Py_quaternion a)
{
   return use_complex_func (a, csqrt);
}

/* -----------------------------------------------------------------------------
 * Returns exp (q)
 */
Py_quaternion _Py_quat_exp (const Py_quaternion a)
{
   return use_complex_func (a, cexp);
}

/* -----------------------------------------------------------------------------
 * Returns log (a) to base e
 */
Py_quaternion _Py_quat_log (const Py_quaternion a)
{
   return use_complex_func (a, clog);
}

/* -----------------------------------------------------------------------------
 * Returns: sine of a
 * We do not use a series expansion, but leverage off the csin function.
 */
Py_quaternion _Py_quat_sin (const Py_quaternion a)
{
   return use_complex_func (a, csin);
}

/* -----------------------------------------------------------------------------
 * Returns: cosine of a
 * We do not use a series expansion, but leverage off the ccos function.
 */
Py_quaternion _Py_quat_cos (const Py_quaternion a)
{
   return use_complex_func (a, ccos);
}

/* -----------------------------------------------------------------------------
 * Returns: tangent of a
 */
Py_quaternion _Py_quat_tan (const Py_quaternion a)
{
   return use_complex_func (a, ctan);
}

/* -----------------------------------------------------------------------------
 * Returns: arc cosine of a
 */
Py_quaternion _Py_quat_acos (const Py_quaternion a)
{
   return use_complex_func (a, cacos);
}

/* -----------------------------------------------------------------------------
 * Returns: arc sine of a
 */
Py_quaternion _Py_quat_asin (const Py_quaternion a)
{
   return use_complex_func (a, casin);
}

/* -----------------------------------------------------------------------------
 * Returns: arc tangent of a
 */
Py_quaternion _Py_quat_atan (const Py_quaternion a)
{
   return use_complex_func (a, catan);
}

/* -----------------------------------------------------------------------------
 * Returns: hyperbolic sine of a
 * We do not use a series expansion, but leverage off the csinh function.
 */
Py_quaternion _Py_quat_sinh (const Py_quaternion a)
{
   return use_complex_func (a, csinh);
}

/* -----------------------------------------------------------------------------
 * Returns: hyperbolic cosine of a
 * We do not use a series expansion, but leverage off the ccosh function.
 */
Py_quaternion _Py_quat_cosh (const Py_quaternion a)
{
   return use_complex_func (a, ccosh);
}

/* -----------------------------------------------------------------------------
 * Returns: hyperbolic tangent of a
 */
Py_quaternion _Py_quat_tanh (const Py_quaternion a)
{
   return use_complex_func (a, ctanh);
}

/* -----------------------------------------------------------------------------
 * Returns: inverse hyperbolic sine of a
 * We do not use a series expansion, but leverage off the csinh function.
 */
Py_quaternion _Py_quat_asinh (const Py_quaternion a)
{
   return use_complex_func (a, casinh);
}

/* -----------------------------------------------------------------------------
 * Returns: inverse hyperbolic cosine of a
 * We do not use a series expansion, but leverage off the ccosh function.
 */
Py_quaternion _Py_quat_acosh (const Py_quaternion a)
{
   return use_complex_func (a, cacosh);
}

/* -----------------------------------------------------------------------------
 * Returns: inverse hyperbolic tangent of a
 */
Py_quaternion _Py_quat_atanh (const Py_quaternion a)
{
   return use_complex_func (a, catanh);
}

/* -----------------------------------------------------------------------------
 * Debugging helper
 */
void _Py_quat_debug_trace(const char* function,
                          const int line,
                          const char* format, ...)
{
   va_list args;
   char buffer [200];

   va_start (args, format);
   vsnprintf (buffer, sizeof (buffer), format, args);
   va_end (args);

   char notification [240];
   snprintf (notification, sizeof (notification), "%4d (%s): %s\n", line, function, buffer);
   /* Avoid (gcc 8.4.1) error: format not a string literal and no format arguments */
   printf ("%s", notification);
}

/* end */
