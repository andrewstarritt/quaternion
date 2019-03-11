/* quaternion_math.h
 *
 * This file is part of the Python quaternion module. It privides the
 * Quaternion math functions.
 *
 * Copyright (c) 2018-2019  Andrew C. Starritt
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

#ifndef QUATERNION_MATH_H
#define QUATERNION_MATH_H 1

#include <Python.h>

/* Provides a reference to the methods provided by quaternion_math.c
 */
PyAPI_FUNC (PyMethodDef*) _PyQmathMethods ();

#endif    /* */
