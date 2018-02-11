/* quaternion_basic.c
 *
 * This file is part of the Python quaternion module. It provides basic
 * quaternion maths operation with minimalist reference to Python.
 *
 * Copyright (c) 2018  Andrew C. Starritt
 *
 * The quaternion module is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The quaternion module is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the quaternion module.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact details:
 * starritt@netspace.net.au
 * PO Box 3118, Prahran East, Victoria 3181, Australia.
 *
 * source formatting:
 *    indent -kr -pcs -i3 -cli3 -nbbo -nut -l96
 */

#include <Python.h>
#include <pymath.h>
#include "quaternion_basic.h"

static const Py_quaternion q_one = { 1.0, 0.0, 0.0, 0.0 };
static const double pi = 3.14159265358979323846;


/* -----------------------------------------------------------------------------
 * PRIVATE FUNCTIONS
 * -----------------------------------------------------------------------------
 * normalise x in to range -pi .. +pi
 */
static double angle_mod (double x) {

   int long n = (long) (x / pi);  /* truncates towards 0 */
   if (n >= 0) {
       n = ((n+1)/2)*2;   /* ensure even n */
   } else {
       n = ((n-1)/2)*2;   /* ensure even n */
   }
   x = x - n*pi;          /* n*pi is m*tau */
   return x;
}

/* -----------------------------------------------------------------------------
 * Find the largest absolute coefficient of a
 */
static double quat_max_abs_elem (const Py_quaternion a)
{
   double r, t;
   r = fabs (a.s);
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

   /* Scale coordinates - avoid overflow
    */
   double x = t.x / m;
   double y = t.y / m;
   double z = t.z / m;

   return m * sqrt (x*x + y*y + z*z);
}

/* -----------------------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -----------------------------------------------------------------------------
 *  true if all parts finite
 */
bool _Py_quat_isfinite (const Py_quaternion a)
{
   return Py_IS_FINITE (a.s) && Py_IS_FINITE (a.x) &&
          Py_IS_FINITE (a.y) && Py_IS_FINITE (a.z);
}

/* -----------------------------------------------------------------------------
 *  true if any part positive or negative infinite
 */
bool _Py_quat_isinf    (const Py_quaternion a)
{
   return Py_IS_INFINITY (a.s) || Py_IS_INFINITY (a.x) ||
          Py_IS_INFINITY (a.y) || Py_IS_INFINITY (a.z);
}

/* -----------------------------------------------------------------------------
 *  true if any part is NaN
 */
bool _Py_quat_isnan    (const Py_quaternion a)
{
   return Py_IS_NAN (a.s) || Py_IS_NAN (a.x) ||
          Py_IS_NAN (a.y) || Py_IS_NAN (a.z);
}


/* -----------------------------------------------------------------------------
 * Returns: a == b
 */
int _Py_quat_eq (const Py_quaternion a, const Py_quaternion b)
{
   return (a.s == b.s) && (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
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
   r.s = a.s + b.s;
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
   r.s = a.s - b.s;
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
   r.s = (a.s * b.s) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z);
   r.x = (a.s * b.x) + (a.x * b.s) + (a.y * b.z) - (a.z * b.y);
   r.y = (a.s * b.y) + (a.y * b.s) + (a.z * b.x) - (a.x * b.z);
   r.z = (a.s * b.z) + (a.z * b.s) + (a.x * b.y) - (a.y * b.x);
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: a / b
 */
Py_quaternion _Py_quat_quot (const Py_quaternion a, const Py_quaternion b)
{
   Py_quaternion r;             /* the result */

   double m;
   m = quat_max_abs_elem (b);
   if (m == 0.0) {
      errno = EDOM;     /**  Needs Python.h  **/
      r.s = r.x = r.y = r.z = 0.0;
   } else {
      Py_quaternion sa, sb, sbc, nom;
      double denom;

      /* Scale a and b by 1/m:
       * Note: a/b == (a/m) / (b/m)
       */
      sa.s = a.s / m;
      sa.x = a.x / m;
      sa.y = a.y / m;
      sa.z = a.z / m;

      sb.s = b.s / m;
      sb.x = b.x / m;
      sb.y = b.y / m;
      sb.z = b.z / m;

      /* Form b^, i.e. complex conjugate of b
       */
      sbc.s = + sb.s;
      sbc.x = - sb.x;
      sbc.y = - sb.y;
      sbc.z = - sb.z;

      /* form nominator = a * b^
       */
      nom = _Py_quat_prod (sa, sbc);

      /* denominator is b * b^ which is real
       */
      denom = (sb.s * sb.s) + (sb.x * sb.x) + (sb.y * sb.y) + (sb.z * sb.z);

      r.s = nom.s / denom;
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
   r.s = -a.s;
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
   r.s = +a.s;
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
   denom = (a.s * a.s) + (a.x * a.x) + (a.y * a.y) + (a.z * a.z);

   if (denom == 0.0) {
      errno = EDOM;     /** Needs Python.h **/
      r.s = r.x = r.y = r.z = 0.0;
   } else {
      /* conjugate / denom */
      r.s = +a.s / denom;
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
      r.s = r.x = r.y = r.z = 0.0;
   } else {
      r.s = a.s / m;
      r.x = a.x / m;
      r.y = a.y / m;
      r.z = a.z / m;
   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: a ** b
 */
Py_quaternion _Py_quat_pow (const Py_quaternion a, const double x)
{
   Py_quaternion r;

   /* special cases */
   if (x == 0.0) {
      /* a ** 0 == 1 (even when a == 0) */
      r.s = 1.0;
      r.x = 0.0;
      r.y = 0.0;
      r.z = 0.0;

   } else if (a.s == 0.0 && a.x == 0.0 && a.y == 0.0 && a.z == 0.0) {
      /* 0 ** a == 0 */
      r.s = r.x = r.y = r.z = 0.0;

      /* unless negative power */
      if (x < 0.0)
         errno = EDOM;

   } else if (x == 1) {
      /* a ** 1 == a */
      r = a;

   } else {
      /* convert to polar coordinates
       */
      double length;
      Py_quat_triple unit;
      double angle;

      _Py_quat_into_polar (a, &length, &unit, &angle);
      r = _Py_quat_from_polar (pow (length, x), unit, angle*x);
   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: abs (z) or |z|
 */
double _Py_quat_abs (const Py_quaternion a)
{
   /* sets errno = ERANGE on overflow;  otherwise errno = 0 */
   double result;

   /* Is any element infinite or NaN - needs Python.h
    */
   if (!Py_IS_FINITE (a.s) || !Py_IS_FINITE (a.x) || !Py_IS_FINITE (a.y) || !Py_IS_FINITE (a.z)) {
      /* C99 rules: if either the real or the imaginary parts is an
         infinity, return infinity, even if the other part is a NaN.
       */
      if (Py_IS_INFINITY (a.s)) {
         result = fabs (a.s);
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
      double ss = a.s / m;
      double sx = a.x / m;
      double sy = a.y / m;
      double sz = a.z / m;
      result = m * sqrt (ss * ss + sx * sx + sy * sy + sz * sz);
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
      r.s = r.x = r.y = r.z = 0.0;
   } else {
      /* Scale to avoid over flows
       */
      double sx = axis.x / m;
      double sy = axis.y / m;
      double sz = axis.z / m;

      double norm = sqrt (sx * sx + sy * sy + sz * sz);

      double caot = cos (angle / 2.0);
      double saot = sin (angle / 2.0);

      r.s = caot;
      r.x = sx * saot / norm;
      r.y = sy * saot / norm;
      r.z = sz * saot / norm;
   }

   return r;
}

/* -----------------------------------------------------------------------------
 * Returns:
 */
Py_quat_triple _Py_quat_rotate (const Py_quaternion a,
                                const Py_quat_triple point,
                                const Py_quat_triple origin)
{
   Py_quat_triple r;

   Py_quaternion p;
   Py_quaternion b;
   Py_quaternion ap;
   Py_quaternion t;

   p.s = 0.0;
   p.x = point.x - origin.x;
   p.y = point.y - origin.y;
   p.z = point.z - origin.z;

   b = _Py_quat_conjugate (a);

   ap = _Py_quat_prod (a, p);
   t = _Py_quat_prod (ap, b);

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
                          double* angle)
{
   Py_quat_triple v;
   double c, s;
   *m = _Py_quat_abs (a);

   /* Is this essentially a null quaternion?
    */
   if (*m < 1.0e-160) {
      *angle  = 0.0;
      unit->x = 0.0;
      unit->y = 1.0;
      unit->z = 0.0;
      return;
   }

   /* Normalise a
   */
   c   = a.s / (*m);    /* cos (angle) */
   v.x = a.x / (*m);
   v.y = a.y / (*m);
   v.z = a.z / (*m);

   /* v already normalised, so no overlow issues here.
    */
   s = sqrt (v.x*v.x + v.y*v.y + v.z*v.z);   /* sin (angle) */

   *angle = atan2 (s, c);
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

  /* Cosmetic fluff: ensure that on balance, unit imaginary vector is +ve.
   * This is mainly so that when there is one imaginary component, is is
   * always positive, just like with complex numbers:
   *   j is always a unit vector in +ve direction
   */
   if (unit->x + unit->y + unit->z < 0) {
      *angle  = -*angle;
      unit->x = -unit->x;
      unit->y = -unit->y;
      unit->z = -unit->z;
   }
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
                                   const double angle)
{
   Py_quaternion r;
   double u;
   double c, s;
   double t;

   u = length_triple (unit);
   if (u == 0.0) {
      errno = EDOM;     /** Needs Python.h **/
      r.s = r.x = r.y = r.z = 0.0;
   } else {

      c = cos (angle);
      s = sin (angle);

      r.s = m * c;

      t = m*s/u;    /* combine length, sine and normalisation factor. */
      r.x = t * unit.x;
      r.y = t * unit.y;
      r.z = t * unit.z;
   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns sqrt (q)
 * Based on: http://onlinelibrary.wiley.com/doi/10.1002/9780470682906.app4/pdf
 */
Py_quaternion _Py_quat_sqrt (const Py_quaternion a)
{
   Py_quaternion r;
   double m;
   Py_quat_triple unit;
   double angle;

   _Py_quat_into_polar (a, &m, &unit, &angle);
   r = _Py_quat_from_polar (sqrt (m), unit, angle/2.0);
   return r;
}

/* Used by both exp and log.
 */
static const double angle_limit = 1.0e-15;  // was 0.00001

/* -----------------------------------------------------------------------------
 * Returns exp (q)
 * Based on: https://www.geometrictools.com/Documentation/Quaternions.pdf
 */
Py_quaternion _Py_quat_exp (const Py_quaternion a)
{
   Py_quaternion r;
   Py_quat_triple u;
   double angle;
   double eas;

   /*  a = s + (xi + yj +zk) = s + v
    *  exp (a) = exp (s + v)
    *          = exp (s)*exp(v)
    *          = exp (s)*exp(u.|v|)
    *          = exp (s)*exp(u.angle)
    *          = exp (s)*(cos(angle) + u.sin(angle))
    */
   eas = exp (a.s);
   angle = sqrt (a.x*a.x + a.y*a.y + a.z*a.z);   /* size if the imaginary part */

   if (fabs (angle) < angle_limit) {
      /* Essentially real
       */
      r.s = eas;
      r.x = r.y = r.z = 0.0;
   } else {
      double ca = cos (angle);
      double sa = sin (angle);

      /* Form unit vector
       */
      u.x = a.x / angle;
      u.y = a.y / angle;
      u.z = a.z / angle;

      r.s = eas * ca;
      r.x = eas * u.x * sa;
      r.y = eas * u.y * sa;
      r.z = eas * u.z * sa;
   }

   return r;
}

/* -----------------------------------------------------------------------------
 * Returns log (a) to base e
 * Based on: https://www.geometrictools.com/Documentation/Quaternions.pdf
 */
Py_quaternion _Py_quat_log (const Py_quaternion a)
{
   Py_quaternion r;
   double m;
   Py_quat_triple u;
   double angle;

   _Py_quat_into_polar (a, &m, &u, &angle);

   if (fabs (angle) < angle_limit) {
      /* Essentially real
       */
      if (a.s >= 0) {
         r.s = log (a.s);
         r.x = r.y = r.z = 0.0;
      } else {
         /* For -ve real, we need pi worth of imaginary, as exp (i.pi) == -1
          * We allocate this to the j component, so that a quaternion behaves
          * like complex numbers.
          */
         r.s = log (-a.s);
         r.y = pi;
         r.x = r.z = 0.0;
      }

   } else {
      /* log (a) = log (m.v) = log(m) + log(v)
       *                     = log(m) + log (cos(angle) + u.sin(angle))
       *                     = log(m) + log (cos(angle) + u.sin(angle))
       *                     = log(m) + log (exp (u.angle))
       *                     = log(m) + u.angle
       */
      r.s = log (m);
      r.x = u.x * angle;
      r.y = u.y * angle;
      r.z = u.z * angle;
   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: sine of a
 *
 * We use the standard seried expansion to finds the sine of a, i.e.:
 *
 *  a - a**3/3! + a**5/5! - a**7/7! + a**9/9! ...
 */
Py_quaternion _Py_quat_sin (const Py_quaternion a)
{
   Py_quaternion an;
   Py_quaternion r;
   int sign;
   double factorial;
   double m;
   double t2, r2;
   Py_quaternion a_sqd;
   Py_quaternion ap;
   Py_quaternion t;
   int j;

   /* normalise the real part of a to range -pi .. +pi
    */
   an = a;
   an.s = angle_mod (a.s);

   a_sqd = _Py_quat_prod(an, an);

   sign = +1;
   factorial = 1.0;
   ap = an;       /* first term */
   r = ap;        /* initialise with first term */

   for (j = 3; j < 51; j += 2) {
      ap = _Py_quat_prod (ap, a_sqd);

      sign = -sign;
      factorial = factorial * j * (j - 1);
      m = sign/factorial;

      /* Don't use _Py_quat_prod as muliplying with a scalar, and inline add.
       */
      t.s = ap.s * m;  r.s += t.s;
      t.x = ap.x * m;  r.x += t.x;
      t.y = ap.y * m;  r.y += t.y;
      t.z = ap.z * m;  r.z += t.z;

      /* If term is now insignificant, exit loop
       *
       *    abs (term) <= 1.0e-20 * abs (r)
       *
       * To avoid the sqrt we examine squares of the abs values.
       */
      t2 = (t.s*t.s) + (t.x*t.x) + (t.y*t.y) + (t.z*t.z);
      r2 = (r.s*r.s) + (r.x*r.x) + (r.y*r.y) + (r.z*r.z);

      if (t2 <= 1.0e-40 * r2)
         break;
   }

   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: cosine of a
 *
 * We use the standard seried expansion to find the cosine of a, i.e.:
 *
 *  1 - a**2/2! + a**4/4! - a**6/6! + a**8/8! ...
 */
Py_quaternion _Py_quat_cos (const Py_quaternion a)
{
   /* see the sin function for detailed comments - they work the same way */
   Py_quaternion an;
   Py_quaternion r;
   int sign;
   double factorial;
   double m;
   double t2, r2;
   Py_quaternion a_sqd;
   Py_quaternion ap;
   Py_quaternion t;
   int j;

   an = a;
   an.s = angle_mod (a.s);

   a_sqd = _Py_quat_prod(an, an);

   sign = +1;
   factorial = 1.0;
   ap = q_one;    /* first term */
   r = ap;        /* initialise with first term*/

   for (j = 2; j < 50; j += 2) {
      ap = _Py_quat_prod (ap, a_sqd);

      sign = -sign;
      factorial = factorial * j * (j - 1);
      m = sign/factorial;

      t.s = ap.s * m;  r.s += t.s;
      t.x = ap.x * m;  r.x += t.x;
      t.y = ap.y * m;  r.y += t.y;
      t.z = ap.z * m;  r.z += t.z;

      t2 = (t.s*t.s) + (t.x*t.x) + (t.y*t.y) + (t.z*t.z);
      r2 = (r.s*r.s) + (r.x*r.x) + (r.y*r.y) + (r.z*r.z);

      if (t2 <= 1.0e-40 * r2)
         break;
   }

   return r;
}

/* -----------------------------------------------------------------------------
 * Returns: tangent of a
 */
Py_quaternion _Py_quat_tan (const Py_quaternion a)
{
   Py_quaternion s, c, r;

   s = _Py_quat_sin (a);
   c = _Py_quat_cos (a);

   /* The axis of sin (a) and cos(a) are parallel, so the divide makes sense.
    * That is s and c commute in this case, and (1/c)*s == s*(1/c)
    */
   r = _Py_quat_quot (s, c);

   return r;
}

/* end */
