/* quaternion_object.h
 *
 * This file is part of the Python quaternion module. It privides the
 * Quaternion type.
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

#ifndef QUATERNION_OBJECT_H
#define QUATERNION_OBJECT_H 1

#include <Python.h>
#include <stdbool.h>
#include "quaternion_basic.h"

// -----------------------------------------------------------------------------
// PyQuaternionObject : the Quaternion PyObject definition
//
typedef struct {
   PyObject_HEAD
   /* Type-specific fields go here. */
   Py_quaternion qval;
} PyQuaternionObject;


/* Use PyAPI_FUNC ?? */

PyTypeObject* PyQuaternionType ();

/* Quaternion type check functions
 */
bool PyQuaternion_Check (PyObject *op);

bool PyQuaternion_CheckExact (PyObject *op);

/* Extract C quaternion value from Python Quaternion.
 */
Py_quaternion PyQuaternion_AsCQuaternion(PyObject *);

/* Return Python Quaternion from C quaternion (new object)
 */
PyObject * PyQuaternion_FromCQuaternion(const Py_quaternion);

/* Returns the obj converted to a Quaternion object on success or NULL on failure.
 * Allowed input object types are PyLong, PyFLoat, PyComplex and PyQuaternion.
 *
 * If the input obj is already a PyQuaternion, then the same object is returned
 * with INCREFing of the input.
 *
 * This is the equivalent of the Python expression: Quaternion (obj), however to
 * be consistant with other number types, the only place where a string is
 * allowed to be used as a value in during object contruction/initialisation.
 */
PyObject * PyObject_AsQuaternion(PyObject *obj);

/* This effectively combines the functionality of PyObject_AsQuaternion and
 * PyQuaternion_AsCQuaternion without creating an intermediate object.
 * Returns true on success, and false on failure.
 * On failure, qval is untouched.
 */
bool PyObject_AsCQuaternion(PyObject *obj,  Py_quaternion* qval);

#endif /* QUATERNION_OBJECT_H */
