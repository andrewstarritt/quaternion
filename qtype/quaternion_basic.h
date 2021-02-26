/* quaternion_basic.h
 *
 * This file is part of the Python quaternion module. It privides basic
 * quaternion maths operation with minimalist reference to Python.
 *
 * Copyright (c) 2018-2021  Andrew C. Starritt
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
 * andrew.starritt@gmail.com
 * PO Box 3118, Prahran East, Victoria 3181, Australia.
 *
 * source formatting:
 *    indent -kr -pcs -i3 -cli3 -nbbo -nut -l96
 */

#ifndef QUATERNION_BASIC_H
#define QUATERNION_BASIC_H 1

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* For the complex type, the equivalent of these operations are within the
 * object definition file. I use a separate module.
 */

/* -----------------------------------------------------------------------------
 * Simple C type quaternion - it is the one and only type-specific field in
 * the PyQuaternionObject definition.
 */
typedef struct {
   double w;                    /* real or scalar component */
   double x;                    /* i component */
   double y;                    /* j component */
   double z;                    /* k component */
} Py_quaternion;

/* Used for points and axies
 */
typedef struct {
   double x;
   double y;
   double z;
} Py_quat_triple;


/* Used for 3x3 matricies
 */
typedef struct {
   double r11, r12, r13;
   double r21, r22, r23;
   double r31, r32, r33;
} Py_quat_matrix;


/* Infinities and NaNs
 */

/* true if all parts finite */
bool _Py_quat_isfinite (const Py_quaternion a);

/* true if any part positive or negative infinite */
bool _Py_quat_isinf    (const Py_quaternion a);

/* true if any part is NaN */
bool _Py_quat_isnan    (const Py_quaternion a);


/* Basic Py_quaternion number functions
 */
int _Py_quat_eq (const Py_quaternion a, const Py_quaternion b);
int _Py_quat_ne (const Py_quaternion a, const Py_quaternion b);

double _Py_quat_abs (const Py_quaternion a);
Py_quaternion _Py_quat_neg (const Py_quaternion a);
Py_quaternion _Py_quat_conjugate (const Py_quaternion a);
Py_quaternion _Py_quat_inverse   (const Py_quaternion a);
Py_quaternion _Py_quat_normalise (const Py_quaternion a);
Py_quaternion _Py_quat_round     (const Py_quaternion a, const int n);

Py_quaternion _Py_quat_sum  (const Py_quaternion a, const Py_quaternion b);
Py_quaternion _Py_quat_diff (const Py_quaternion a, const Py_quaternion b);

/* note: multiplication does not commute: a*b != b*a */
Py_quaternion _Py_quat_prod (const Py_quaternion a, const Py_quaternion b);

/* note: division is a * inverse(b) */
Py_quaternion _Py_quat_quot (const Py_quaternion a, const Py_quaternion b);

/* calc a ** b - two special forms */
Py_quaternion _Py_quat_pow1  (const Py_quaternion a, const double b);
Py_quaternion _Py_quat_pow2  (const double a, const Py_quaternion b);

/* Other functions
 */
double _Py_quat_quadrance (const Py_quaternion a);
double _Py_quat_dot_prod (const Py_quaternion a, const Py_quaternion b);

/* Rotation related functions
 */
Py_quaternion _Py_quat_calc_rotation (const double angle,
                                      const Py_quat_triple axis);

/* Determine equivilent 3D rotation matrix of a rotation quaternion
 */
void _Py_quat_rotation_matrix (const Py_quaternion a,
                               Py_quat_matrix* matrix);

Py_quat_triple _Py_quat_rotate (const Py_quaternion a,
                                const Py_quat_triple point,
                                const Py_quat_triple origin);

/* To/from polar
 * unit is an imaginary unit vector.
 */
void _Py_quat_into_polar (const Py_quaternion a,
                          double* m,
                          Py_quat_triple* unit,
                          double* phase);

Py_quaternion _Py_quat_from_polar (const double m,
                                   const Py_quat_triple unit,
                                   const double phase);

/* Math functions
 */
Py_quaternion _Py_quat_sqrt (const Py_quaternion a);
Py_quaternion _Py_quat_exp (const Py_quaternion a);
Py_quaternion _Py_quat_log (const Py_quaternion a);

Py_quaternion _Py_quat_sin (const Py_quaternion a);
Py_quaternion _Py_quat_cos (const Py_quaternion a);
Py_quaternion _Py_quat_tan (const Py_quaternion a);

Py_quaternion _Py_quat_asin (const Py_quaternion a);
Py_quaternion _Py_quat_acos (const Py_quaternion a);
Py_quaternion _Py_quat_atan (const Py_quaternion a);

Py_quaternion _Py_quat_sinh (const Py_quaternion a);
Py_quaternion _Py_quat_cosh (const Py_quaternion a);
Py_quaternion _Py_quat_tanh (const Py_quaternion a);

Py_quaternion _Py_quat_asinh (const Py_quaternion a);
Py_quaternion _Py_quat_acosh (const Py_quaternion a);
Py_quaternion _Py_quat_atanh (const Py_quaternion a);

#ifdef __cplusplus
}
#endif

#endif                          /* QUATERNION_BASIC_H */
