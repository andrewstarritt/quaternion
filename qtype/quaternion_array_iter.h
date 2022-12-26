/* quaternion_array_iter.h
 *
 * This file is part of the Python quaternion module. It provides the
 * Quaternion Array type.
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
 *
 * source formatting:
 *    indent -kr -pcs -i3 -cli3 -nbbo -nut -l96
 */

#ifndef QUATERNION_ARRAY_ITER_H
#define QUATERNION_ARRAY_ITER_H 1

#include <Python.h>
#include "quaternion_array.h"

/* -----------------------------------------------------------------------------
 */
typedef struct {
   PyObject_HEAD
   /* Type-specific fields go here. */
   Py_ssize_t index;                  /* used by __iter__ and __next__ */
   PyQuaternionArrayObject* qaobj;    /* the associated QuaternionArray object */
} PyQuaternionArrayIterObject;


PyAPI_FUNC (PyObject *)
PyQuaternionArrayIter(PyObject *qaobj);

/* Used by module setup
 */
PyAPI_FUNC (PyTypeObject*) PyQuaternionArrayIterType ();

PyAPI_FUNC (bool) PyQuaternionArrayIter_Check (PyObject *op);
PyAPI_FUNC (bool) PyQuaternionArrayIter_CheckExact (PyObject *op);

#endif  /* QUATERNION_ARRAY_ITER_H */
