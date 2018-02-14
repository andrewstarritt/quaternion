/* quaternion_basic.h
 *
 * This file is part of the Python quaternion module. It privides basic
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

#ifndef QUATERNION_BASIC_H
#define QUATERNION_BASIC_H 1

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

Py_quaternion _Py_quat_sum  (const Py_quaternion a, const Py_quaternion b);
Py_quaternion _Py_quat_diff (const Py_quaternion a, const Py_quaternion b);

/* note: multiplication does not commute: a*b != b*a */
Py_quaternion _Py_quat_prod (const Py_quaternion a, const Py_quaternion b);
Py_quaternion _Py_quat_quot (const Py_quaternion a, const Py_quaternion b);

Py_quaternion _Py_quat_pow  (const Py_quaternion a, const double x);

/* Rotation related functions
 */
Py_quaternion _Py_quat_calc_rotation (const double angle,
                                      const Py_quat_triple axis);

Py_quat_triple _Py_quat_rotate (const Py_quaternion a,
                                const Py_quat_triple point,
                                const Py_quat_triple origin);

/* To from polar
 */
void _Py_quat_into_polar (const Py_quaternion a,
                          double* m,
                          Py_quat_triple* unit,
                          double* angle);

Py_quaternion _Py_quat_from_polar (const double m,
                                   const Py_quat_triple unit,
                                   const double angle);

/* Math functions
 */
Py_quaternion _Py_quat_sqrt (const Py_quaternion a);
Py_quaternion _Py_quat_exp (const Py_quaternion a);
Py_quaternion _Py_quat_log (const Py_quaternion a);

Py_quaternion _Py_quat_sin (const Py_quaternion a);
Py_quaternion _Py_quat_cos (const Py_quaternion a);
Py_quaternion _Py_quat_tan (const Py_quaternion a);

#ifdef __cplusplus
}
#endif

#endif                          /* QUATERNION_BASIC_H */
