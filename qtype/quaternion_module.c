/* quaternion_module.c
 *
 * This file is part of the Python quaternion module. It privides module
 * framework.
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
 */

/* Development environment:
 * Python 3.6.3
 * CentOS Linux release 7.4.1708 (Core) x86_64
 * gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-16)
 */

#include <Python.h>

#include "quaternion_basic.h"
#include "quaternion_object.h"
#include "quaternion_math.h"

static Py_quaternion q1 = {1.0, 0.0, 0.0, 0.0};
static Py_quaternion qi = {0.0, 1.0, 0.0, 0.0};
static Py_quaternion qj = {0.0, 0.0, 1.0, 0.0};
static Py_quaternion qk = {0.0, 0.0, 0.0, 1.0};


static PyModuleDef QuaternionModule = {
   PyModuleDef_HEAD_INIT,       /* m_base */
   "quaternion",                /* m_name */
   "\n"                         /* m_doc */
   "The quaternion module provides the Quaternion type and associated math functions.\n"
   "\n"
   "The Quaternion type has a real component and three imaginary components. These are\n"
   "accessable using the Quaternion instance attributes r, i, j, and k respectively.\n"
   "Note: the instance attributes i, j, k should not be confused with the quaternion\n"
   "module variables i, j and k. The former return floats where as the latter are unit\n"
   "Quaternion instances such that i.i = j.j = k.k = 1, and i.r = i.j = i.k etc = 0.\n"
   "\n"
   "A Quaternion may also be considered to be a real scalar plus a vector (with real\n"
   "components). The vector part is accessable via the axis attribute which provides a\n"
   "tuple. The following Python expressions are equvilent: q.axis and (q.i, q.j, q.k)\n"
   "\n"
   "The Quaternion type is non-commutative, i.e.  q1*q2  and  q2*q1 in general\n"
   " providedifferent results. To divide one Quaternion by another, there are two\n"
   "options, i.e.:  q1*inverse(q2) or inverse(q2)* q1. The quotient function returns\n"
   "the former. So (q1/q2)*q2 == q1\n"
   "\n"
   "Mixed mode: Quaternions and scalar numbers, int or float, are interoperable.\n"
   "int and float are treated as Quaternions with zero imaginary components.\n"
   "\n"
   "Mixed mode with complex numbers is also allowed. A complex number, z, is treated\n"
   "as a Quaternions, q, such that q.r = z.real, q.j = z.imag, and q.i and q.k are\n"
   "zero. The complex part of a Quaternion may be obtained using the complex\n"
   "attribute, such that:  q.complex == complex(q.r, q.j).\n"
   "\n"
   "The choice of allocating the imaginary part of a complex number to j as opposed\n"
   "to i or k is mathematically arbitary, but for Python j is the natural choice.\n"
   "\n"
   "References\n"
   "http://onlinelibrary.wiley.com/doi/10.1002/9780470682906.app4/pdf\n"
   "https://www.geometrictools.com/Documentation/Quaternions.pdf\n"
   "https://en.wikipedia.org/wiki/Quaternion\n"
   "\n",
   -1,                          /* m_size */
   NULL,                        /* m_methods */
   NULL,                        /* m_slots */
   NULL,                        /* m_traverse */
   NULL,                        /* m_clear */
   NULL                         /* m_free */
};


PyMODINIT_FUNC
PyInit_quaternion(void)
{
   PyObject* module;

   PyTypeObject* quaternionType = PyQuaternionType();

   if (PyType_Ready(quaternionType) < 0)
      return NULL;

   QuaternionModule.m_methods = _PyQmathMethods ();

   module = PyModule_Create(&QuaternionModule);
   if (module == NULL)
      return NULL;

   Py_INCREF(quaternionType);
   PyModule_AddObject(module, "Quaternion", (PyObject *)quaternionType);
   PyModule_AddObject(module, "one", PyQuaternion_FromCQuaternion(q1));
   PyModule_AddObject(module, "i", PyQuaternion_FromCQuaternion(qi));
   PyModule_AddObject(module, "j", PyQuaternion_FromCQuaternion(qj));
   PyModule_AddObject(module, "k", PyQuaternion_FromCQuaternion(qk));

   return module;
}

/* end */
