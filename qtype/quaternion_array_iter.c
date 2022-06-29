/* quaternion_array_iter.c
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

#include "quaternion_array_iter.h"
#include "quaternion_basic.h"

/* -----------------------------------------------------------------------------
 * tp_iter of PyQuaternionArray
 */
PyObject *
PyQuaternionArrayIter(PyObject *qaobj)
{
    PyObject *obj = NULL;
    PyQuaternionArrayIterObject *iter = NULL;

   if (!PyQuaternionArray_Check(qaobj)) {
      PyErr_BadInternalCall();
      return NULL;
   }

   PyTypeObject *type;
   type = PyQuaternionArrayIterType ();
   obj = type->tp_alloc(type, 0);
   iter = (PyQuaternionArrayIterObject*) obj;
   if (iter == NULL)
      return NULL;

   iter->index = 0;
   iter->qaobj = (PyQuaternionArrayObject *)qaobj;
   Py_INCREF(qaobj);

   return (PyObject *)iter;
}

/* -----------------------------------------------------------------------------
 * tp_iternext
 */
static PyObject *
quaternion_array_iter_next(PyObject* self)
{
   PyQuaternionArrayIterObject *iter = NULL;
   PyQuaternionArrayObject *qaobj = NULL;

   /* Check self is what we expect.
    */
   if (!self || !PyQuaternionArrayIter_Check(self)) {
      PyErr_BadInternalCall();
      return NULL;
   }
   iter = (PyQuaternionArrayIterObject *)self;

   /* Check the array object is what we expect.
    */
   qaobj = iter->qaobj;
   if (!qaobj || !PyQuaternionArray_Check((PyObject *)qaobj)) {
      PyErr_BadInternalCall();
      return NULL;
   }

   if (iter->index < qaobj->aval.count) {
      return PyQuaternionArrayGetItem ((PyObject *)qaobj, iter->index++);
   }

   /* The iteration of over.
    */
   return NULL;
}

/* -----------------------------------------------------------------------------
 * tp_dealloc
 */
static void
quaternion_array_iter_dealloc(PyQuaternionArrayIterObject* self)
{
   Py_XDECREF(self->qaobj);
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_iter_doc,
             "__ArrayIter () -> iterator\n"
             "Used internally to interate over a QuaternionArray.\n"
             "\n");

static PyTypeObject QuaternionArrayIterType = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "quaternion.__ArrayIter",                  /* tp_name */
   sizeof(PyQuaternionArrayIterObject),       /* tp_basicsize */
   0,                                         /* tp_itemsize */
   (destructor)quaternion_array_iter_dealloc, /* tp_dealloc */
   0,                                         /* tp_print */
   0,                                         /* tp_getattr */
   0,                                         /* tp_setattr */
   0,                                         /* tp_reserved / tp_as_async */
   (reprfunc)0,                               /* tp_repr */
   0,                                         /* tp_as_number */
   0,                                         /* tp_as_sequence */
   0,                                         /* tp_as_mapping */
   (hashfunc)0,                               /* tp_hash */
   0,                                         /* tp_call */
   (reprfunc)0,                               /* tp_str */
   (getattrofunc)0,                           /* tp_getattro */
   0,                                         /* tp_setattro */
   0,                                         /* tp_as_buffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
   quaternion_array_iter_doc,                 /* tp_doc */
   0,                                         /* tp_traverse */
   0,                                         /* tp_clear */
   0,                                         /* tp_richcompare */
   0,                                         /* tp_weaklistoffset */
   PyObject_SelfIter,                         /* tp_iter */
   (iternextfunc)quaternion_array_iter_next,  /* tp_iternext */
   0,                                         /* tp_methods */
};


/* -----------------------------------------------------------------------------
 * Allow module definiton code to access the Quaternion Array PyTypeObject.
 */
PyTypeObject* PyQuaternionArrayIterType()
{
   return &QuaternionArrayIterType;
}

/* -----------------------------------------------------------------------------
 */
bool PyQuaternionArrayIter_Check(PyObject *op)
{
   return PyObject_TypeCheck(op, &QuaternionArrayIterType);
}

/* -----------------------------------------------------------------------------
 */
bool PyQuaternionArrayIter_CheckExact(PyObject *op)
{
   return (Py_TYPE(op) == &QuaternionArrayIterType);
}

/* end */
