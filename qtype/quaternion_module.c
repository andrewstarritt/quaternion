/* quaternion_module.c
 *
 * This file is part of the Python quaternion module. It privides module
 * framework.
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
 */

/* The version definition is read by setup.py
 */
#define __version__ "1.1.7"


/* Development environment:
 * Python 3.6.3
 * CentOS Linux release 7.6.1810 (Core) x86_64
 * gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-36)
 */

#include <Python.h>
#include <pymath.h>

#include "quaternion_basic.h"
#include "quaternion_object.h"
#include "quaternion_math.h"

static Py_quaternion q0 = {0.0, 0.0, 0.0, 0.0};
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
   "Within this module, a Quaternion q is defined to be:\n"
   "\n"
   "    q = w + x.i + y.j + z.k\n"
   "\n"
   "where the coefficients w, x, y and z are real; and i, j and k three imaginary\n"
   "numbers such that:\n"
   "\n"
   "    i.i = j.j = k.k = i.j.k = -1\n"
   "    i.j = +k,  j.k = +i,  k.i = +j\n"
   "    j.i = -k,  k.j = -i,  i.k = -j\n"
   "\n"
   "The Quaternion type has four member attributes to access these coefficients.\n"
   "These instance attributes are w, x, y and z respectively.\n"
   "\n"
   "A Quaternion may also be considered to be a real scalar part plus a vector (with\n"
   "three real components). The real part accessable via the real attribute. Thus\n"
   "both q.w and q.real return the real or scalar part of q.\n"
   "\n"
   "The vector part is accessable via both the vector and imag attributes which provide\n"
   "a tuple of floats. The following Python expressions are equivalent:\n"
   "\n"
   "    q.vector\n"
   "    q.imag\n"
   "    (q.x, q.y, q.z)\n"
   "\n"
   "q.real and q.imag provide a \"complex\" like view of a Quaternion at the expense\n"
   "of providing an un-Pythonic duplication of q.w and q.vector respectively.\n"
   "\n"
   "The Quaternion type is associative under both addition and multiplication, i.e.:\n"
   "\n"
   "    (p + q) + r  =  p + (q + r)\n"
   "    (p * q) * r  =  p * (q * r)\n"
   "\n"
   "The Quaternion type is non-commutative with respect to multiplication and division,\n"
   "i.e.  p * q  and  q * p in general provide different values. To divide one\n"
   "Quaternion by another, there are two options:\n"
   "\n"
   "    p * q.inverse() ; or\n"
   "    q.inverse() * p.\n"
   "\n"
   "The quotient function returns the former, therefore:\n"
   "\n"
   "    (p / q) * q = p\n"
   "\n"
   "\n"
   "Quaternions numbers and scalar numbers, i.e. int or float, are interoperable.\n"
   "int and float numbers are treated as Quaternions with zero imaginary components.\n"
   "\n"
   "Mixed mode with complex numbers is also allowed. A complex number, z, is treated\n"
   "as a Quaternions, q, such that q.w = z.real, q.y = z.imag, and q.x and q.z are\n"
   "zero.\n"
   "\n"
   "The choice of alligning the imaginary part of a complex number to the j imaginary\n"
   "component as opposed to i or k is mathematically arbitary. However for Python, j\n"
   "is the natural choice, such that the following, bar any rounding errors, will\n"
   "hold true:\n"
   "\n"
   "    Quaternion(z) = Quaternion(str(z))\n"
   "\n"
   "The complex part of a Quaternion may be obtained using the complex attribute,\n"
   "such that:\n"
   "\n"
   "    q.complex = complex(q.w, q.y).\n"
   "\n"
   "There is no complementary attribute to obtain q.x and q.z as a single item.\n"
   "\n"
   "\n"
   "Math functions\n"
   "A number of math functions that operate on Quaternions are also provided. Where\n"
   "provided, these provide the equivalent quaternion function as the functions of\n"
   "the same name out of the math and/or cmath module.\n"
   "\n"
   "Note: there is no qmath module equivalent of math and cmath, all math functions\n"
   "are in the quaternion module.\n"
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
   PyModule_AddObject(module, "zero", PyQuaternion_FromCQuaternion(q0));
   PyModule_AddObject(module, "one", PyQuaternion_FromCQuaternion(q1));
   PyModule_AddObject(module, "i", PyQuaternion_FromCQuaternion(qi));
   PyModule_AddObject(module, "j", PyQuaternion_FromCQuaternion(qj));
   PyModule_AddObject(module, "k", PyQuaternion_FromCQuaternion(qk));
   PyModule_AddObject(module, "__version__", PyUnicode_FromString (__version__));

   /* Replicate math/cmath constants
    */
   PyModule_AddObject(module, "e",   PyFloat_FromDouble (Py_MATH_El));  // 2.718281828459045));
   PyModule_AddObject(module, "pi",  PyFloat_FromDouble (Py_MATH_PIl)); // 3.141592653589793));
   PyModule_AddObject(module, "tau", PyFloat_FromDouble (Py_MATH_TAU)); // 6.283185307179586));

   return module;
}

/* end */
