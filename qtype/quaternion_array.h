/* quaternion_array.h
 *
 * This file is part of the Python quaternion module. It provides the
 * Quaternion Array type.
 *
 * Copyright (c) 2022  Andrew C. Starritt
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

#ifndef QUATERNION_ARRAY_H
#define QUATERNION_ARRAY_H 1

#include <Python.h>
#include "quaternion_object.h"

/* basic c type
 */
typedef struct {
   Py_ssize_t allocated;       /* number of buffer entries/space available/allocated */
   Py_ssize_t count;           /* count of number actually in use <= number allocated */
   Py_ssize_t iter_index;      /* used by __iter__ and __next__ */
   Py_quaternion* qvalArray;   /* pointer to a dynamically allocated array of c quaternions */
} Py_quaternion_array;


/* -----------------------------------------------------------------------------
 * PyQuaternionArrayObject : the Quaternion Array PyObject definition
 * PyQuaternionArrayObject represents an array of quaternion numbers.
 */
typedef struct {
   PyObject_HEAD
   /* Type-specific fields go here. */
   Py_quaternion_array aval;
} PyQuaternionArrayObject;


/* Used by module setup
 */
PyAPI_FUNC (PyTypeObject*) PyQuaternionArrayType ();

/* QuaternionArray type check functions
 * We use functions as opposed to macros like the complex type
 * as we need to access to the PyQuaternionType anyway.
 */
PyAPI_FUNC (bool) PyQuaternionArray_Check (PyObject *op);
PyAPI_FUNC (bool) PyQuaternionArray_CheckExact (PyObject *op);

#endif  /* QUATERNION_ARRAY_H */
