/* quaternian_utilities.h
 *
 * This file is part of the Python quaternion module. It provides a number of
 * mathematic Quaternian utility functions.
 *
 * Copyright (c) 2022-2023  Andrew C. Starritt
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
 */

#ifndef QUATERNION_UTILITIES_H
#define QUATERNION_UTILITIES_H 1

#include <Python.h>
#include <stdbool.h>
#include "quaternion_basic.h"


/* Return true if the object can be converted to double else returns false.
 * Sort of equivalent to PyFloat_AsDouble(PyNumber_Float) in includes
 * any type where float(obj) yields a float.
 * Note: this function does NOT call PyErr_Format.
 */
bool
PyQuaternionUtil_NumberAsDouble(PyObject *obj,
                                double *value);


/* Parse nested iterator, typically a 3x2 matrix or 3 vector, however can in
 * principle hamd any arbitary number of dimensions (>=1) of any size.
 * Returns true if all okay, otherwise calls PyErr_Format(...) and returns false.
 * Allows the various quaternion methods and functions to be more flexible
 * than just handing tuples (or tuples of tuples).
 */
bool
PyQuaternionUtil_ParseIter(PyObject *obj,
                           double* target,
                           const int ndims,
                           const int* dimSizes,
                           const char* fname,
                           const char* aname);


/* Decode a tuple, list etc argument (of floats and/or longs) into a triple of
 * doubles and returns false on failure, true on success.
 * The 'fname' and 'aname' arguments are for PyErr_Format.
 *
 * Wrapper conveniance function around PyQuaternionUtil_ParseIter
 */
bool
PyQuaternionUtil_ParseTriple(PyObject *obj,
                             Py_quat_triple* triple,
                             const char* fname,
                             const char* aname);

#endif    /* QUATERNION_UTILITIES_H */
