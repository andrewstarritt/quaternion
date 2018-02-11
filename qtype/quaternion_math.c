/* quaternian_math.c
 *
 * This file is part of the Python quaternion module. It provides a number of
 * mathematic Quaternian functions. Where such functins are defined, they aim
 * to mimic the equivilent functions out of the math/cmath module.
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

#include <Python.h>
#include "quaternion_basic.h"
#include "quaternion_object.h"
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define DEBUG_TRACE  printf ("%s:%d\n", __FUNCTION__, __LINE__)


/* -----------------------------------------------------------------------------
 */
#define BASIC_ONE_ARGUMENT_FUNCTION(name)                                      \
                                                                               \
static PyObject *                                                              \
qmath_##name(PyObject *module, PyObject *arg)                                  \
{                                                                              \
   PyObject * result = NULL;                                                   \
   Py_quaternion q;                                                            \
   Py_quaternion r;                                                            \
   bool s;                                                                     \
                                                                               \
   s = PyObject_AsCQuaternion (arg, &q);                                       \
   if (s) {                                                                    \
      r = _Py_quat_##name (q);                                                 \
      result = PyQuaternion_FromCQuaternion (r);                               \
   } else {                                                                    \
      PyErr_Format(PyExc_TypeError,                                            \
                   "%s() argument must be a number, not '%.200s'",             \
                   #name, Py_TYPE(arg)->tp_name);                              \
   }                                                                           \
                                                                               \
   return result;                                                              \
}


/* -----------------------------------------------------------------------------
 */
#define BOOLEAN_ONE_ARGUMENT_FUNCTION(name)                                    \
                                                                               \
static PyObject *                                                              \
qmath_##name(PyObject *module, PyObject *arg)                                  \
{                                                                              \
   PyObject * result = NULL;                                                   \
   Py_quaternion q;                                                            \
   bool r;                                                                     \
   bool s;                                                                     \
                                                                               \
   s = PyObject_AsCQuaternion (arg, &q);                                       \
   if (s) {                                                                    \
      r = _Py_quat_##name (q);                                                 \
      result = r ? Py_True : Py_False;                                         \
      Py_INCREF(result);                                                       \
   } else {                                                                    \
      PyErr_Format(PyExc_TypeError,                                            \
                   "%s() argument must be a number, not '%.200s'",             \
                   #name, Py_TYPE(arg)->tp_name);                              \
   }                                                                           \
                                                                               \
   return result;                                                              \
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_isfinite__doc__,
             "isfinite(q)\n"
             "\n"
             "Return True if all the parts of q are finite, else False.");

BOOLEAN_ONE_ARGUMENT_FUNCTION (isfinite)


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_isinf__doc__,
             "isinfin(q)\n"
             "\n"
             "Return True if any part of q is fininite, else False.");

BOOLEAN_ONE_ARGUMENT_FUNCTION (isinf)


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_isnan__doc__,
             "isnan(q)\n"
             "\n"
             "Return True if any part of q is not a number (NaN), else False.");

BOOLEAN_ONE_ARGUMENT_FUNCTION (isnan)


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_sqrt__doc__,
             "sqrt(q)\n"
             "\n"
             "Return a sqare root of q.");

BASIC_ONE_ARGUMENT_FUNCTION (sqrt)


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_exp__doc__,
             "exp(q)\n"
             "\n"
             "Return the exponent of q.");

BASIC_ONE_ARGUMENT_FUNCTION (exp)


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_log__doc__,
             "log(q [, base])\n"
             "\n"
             "Return the logarithm of q to the given base.\n"
             "If the base not specified, returns the natural logarithm (base e) of q.");

static PyObject *
qmath_log(PyObject *module, PyObject *args)
{
   PyObject * result = NULL;

   PyObject *arg = NULL;
   PyObject *base = NULL;
   int status;
   bool s;
   Py_quaternion q;
   Py_quaternion r;

   status = PyArg_ParseTuple (args, "O|O:quaternion.log", &arg, &base);
   if (!status) {
      return NULL;
   }

   /* Belts 'n' braces
    */
   if (arg == NULL) {
      PyErr_Format(PyExc_TypeError,
                   "quaternion.log() expected at least 1 arguments, got 0");
      return NULL;
   }

   /* extarct the input value if we can
    */
   s = PyObject_AsCQuaternion (arg, &q);
   if (s) {
      /* Do the basic log base e function.
       */
      r = _Py_quat_log (q);

      if (base != NULL) {
         if (PyFloat_Check (base) || PyLong_Check (base)) {
            /* We need PyNumber_AsDouble to complement PyNumber_Check
             */
            double b = PyFloat_Check (base) ? PyFloat_AsDouble (base) : PyLong_AsDouble (base);
            double logBase_e = 1.0 / log (b);

            r.s *= logBase_e;
            r.x *= logBase_e;
            r.y *= logBase_e;
            r.z *= logBase_e;

         } else {
            PyErr_Format(PyExc_TypeError,
                         "quaternion.log() base must be 'float' or 'int'', not '%.200s'",
                         Py_TYPE(base)->tp_name);
         }
      }

      result = PyQuaternion_FromCQuaternion (r);

   } else {
      PyErr_Format(PyExc_TypeError,
                   "quaternion.log() must be a number, not '%.200s'",
                   Py_TYPE(arg)->tp_name);
   }

   return result;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_log10__doc__,
             "log10(q)\n"
             "\n"
             "Return the logarithm of q to base 10.");

static PyObject *
qmath_log10(PyObject *module, PyObject *arg)
{
   PyObject * result = NULL;
   bool s;
   Py_quaternion q;
   Py_quaternion r;

   s = PyObject_AsCQuaternion (arg, &q);
   if (s) {
      r = _Py_quat_log (q);

      /* Primitive log is log base e
       * To get log base 10, multiply by log10(e) i.e.  0.4342944819032518
       * which is the inverse of log.e(10)
       */
      static const double log10_e = 0.4342944819032518;
      r.s *= log10_e;
      r.x *= log10_e;
      r.y *= log10_e;
      r.z *= log10_e;
      result = PyQuaternion_FromCQuaternion (r);
   } else {
      PyErr_Format(PyExc_TypeError,
                   "quaternion.log10() must be a number, not '%.200s'",
                   Py_TYPE(arg)->tp_name);
   }

   return result;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_cos__doc__,
             "cos(q)\n"
             "\n"
             "Return the cosine of q.");

BASIC_ONE_ARGUMENT_FUNCTION (cos)

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_sin__doc__,
             "sin(q)\n"
             "\n"
             "Return the sine of q.");

BASIC_ONE_ARGUMENT_FUNCTION (sin)

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_tan__doc__,
             "tan(q)\n"
             "\n"
             "Return the tangent of q.");

BASIC_ONE_ARGUMENT_FUNCTION (tan)


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_isclose__doc__,
             "isclose(a, b, *, rel_tol=1.0e-09, abs_tol=0.0) -> bool\n"
             "\n"
             "Determine whether two Quaternion numbers are close in value.\n"
             "\n"
             "   rel_tol\n"
             "       maximum difference for being considered \"close\", relative to the\n"
             "       magnitude of the input values\n"
             "\n"
             "   abs_tol\n"
             "       maximum difference for being considered \"close\", regardless of the\n"
             "       magnitude of the input values\n"
             "\n"
             "Return True if a is close in value to b, and False otherwise.\n"
             "\n"
             "For the values to be considered close, the difference between them\n"
             "must be smaller than at least one of the tolerances.\n"
             "\n"
             "-inf, inf and NaN behave similarly to the IEEE 754 Standard.  That\n"
             "is, NaN is not close to anything, even itself.  inf and -inf are\n"
             "only close to themselves.\n");

static PyObject *
qmath_isclose(PyObject *module, PyObject *args, PyObject *kwds)
{
   static char *kwlist[] = {"a", "b", "rel_tol", "abs_tol", 0};

   PyObject *result = NULL;
   PyObject *a = NULL;
   PyObject *b = NULL;
   double rel_tol = 1.0e-09;
   double abs_tol = 0.0;
   int status;

   bool s;
   Py_quaternion ca;
   Py_quaternion cb;

   status = PyArg_ParseTupleAndKeywords
         (args, kwds, "OO|dd:quaternion.isclose", kwlist,
          &a, &b, &rel_tol, &abs_tol);
   if(!status) {
      return NULL;
   }

   /* Belts 'n' braces
    */
   if (a == NULL || b == NULL) {
      PyErr_Format(PyExc_TypeError,
                   "quaternion.isclose() expected at least 2 arguments, got 0 or 1");
      return NULL;
   }

   s = PyObject_AsCQuaternion (a, &ca);
   if (!s) {
      PyErr_Format(PyExc_TypeError,
                   "quaternion.isclose() 'a' must be a number, not '%.200s'",
                   Py_TYPE(a)->tp_name);
      return NULL;
   }

   s = PyObject_AsCQuaternion (b, &cb);
   if (!s) {
      PyErr_Format(PyExc_TypeError,
                   "quaternion.isclose() 'b' must be a number, not '%.200s'",
                   Py_TYPE(b)->tp_name);
      return NULL;
   }

   if (rel_tol < 0.0 || abs_tol < 0.0) {
      /* Bizarre - python formating does not support %f/%e/%g
       */
      char wtf1 [20];
      char wtf2 [20];
      snprintf (wtf1, 20, "%.3e", rel_tol);
      snprintf (wtf2, 20, "%.3e", abs_tol);
      PyErr_Format(PyExc_ValueError,
                   "quaternion.isclose() tolerances must be non-negative,"
                   " not rel_tol=%s and abs_tol=%s", wtf1, wtf2);
      return NULL;
   }

   if (_Py_quat_eq (ca, cb)) {
      /* Short circuit exact equality - needed to catch two infinities of
       * the same sign. And perhaps speeds things up a bit sometimes.
       */
      result = Py_True;
   } else {
      if (_Py_quat_isinf (ca) || _Py_quat_isinf (cb)) {
         /* This catches the case of two infinities of opposite sign, or
          * one infinity and one finite number. Two infinities of opposite
          * sign would otherwise have an infinite relative tolerance.
          * Two infinities of the same sign are caught by the equality check
          * above.
          */
         result = Py_False;
      } else {
         /* Okay - now we get down to the nitty-gritty
          */
         Py_quaternion cd = _Py_quat_diff (ca, cb);
         double diff = _Py_quat_abs (cd);
         double absa = _Py_quat_abs (ca);
         double absb = _Py_quat_abs (cb);

         if (((diff <= rel_tol * absa) || (diff <= rel_tol * absb)) || (diff <= abs_tol)) {
            result = Py_True;
         } else {
            result = Py_False;
         }
      }
   }

   Py_INCREF (result);
   return result;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_polar__doc__,
             "polar(q)\n"
             "\n"
             "Convert a Quaternion from rectangular coordinates to polar coordinates.\n"
             "The polar coordinates of Quaternion are a tuple (length, phase, axis)\n"
             "such that q = length *(math.cos(phase) + axis*math.sin(phase))\n"
             "Note: axis is itself tuple = (x, y, z) and |axis| is 1\n"
             "polar(q) is equivalent to (abs(q), phase(q), axis(q)).");

static PyObject *
qmath_polar(PyObject *module, PyObject *arg)
{
   PyObject * result = NULL;
   Py_quaternion q;
   bool s;

   s = PyObject_AsCQuaternion (arg, &q);
   if (s) {
      double radius;
      Py_quat_triple axis;
      double phase;
      _Py_quat_into_polar (q, &radius, &axis, &phase);

      result = Py_BuildValue("dd(ddd)", radius, phase, axis.x, axis.y, axis.z);
   } else {
      PyErr_Format(PyExc_TypeError,
                   "polar() argument must be a number, not '%.200s'",
                   Py_TYPE(arg)->tp_name);
   }

   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_axis__doc__,
             "axis(q)\n"
             "\n"
             "Returns the axis part of the polar coordinates.\n"
             "The polar coordinates of Quaternion are length, axis, and phase\n"
             "such that q = length *(math.cos(phase) + axis*math.sin(phase))\n"
             "Note: |axis| is 1");

static PyObject *
qmath_axis(PyObject *module, PyObject *arg)
{
   PyObject * result = NULL;
   Py_quaternion q;
   bool s;

   s = PyObject_AsCQuaternion (arg, &q);
   if (s) {
      double radius;
      Py_quat_triple axis;
      double phase;
      _Py_quat_into_polar (q, &radius, &axis, &phase);

      result = Py_BuildValue("(ddd)", axis.x, axis.y, axis.z);
   } else {
      PyErr_Format(PyExc_TypeError,
                   "polar() argument must be a number, not '%.200s'",
                   Py_TYPE(arg)->tp_name);
   }

   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_phase__doc__,
             "phase(q)\n"
             "\n"
             "Return the phase or angle part of the polar coordinates of q.\n"
             "The polar coordinates of Quaternion are length, axis, and phase\n"
             "such that q = length *(math.cos(phase) + axis*math.sin(phase))");

static PyObject *
qmath_phase(PyObject *module, PyObject *arg)
{
   PyObject * result = NULL;
   Py_quaternion q;
   bool s;

   s = PyObject_AsCQuaternion (arg, &q);
   if (s) {
      double radius;
      Py_quat_triple axis;
      double phase;
      _Py_quat_into_polar (q, &radius, &axis, &phase);

      result = Py_BuildValue("d", phase);
   } else {
      PyErr_Format(PyExc_TypeError,
                   "phase() argument must be a number, not '%.200s'",
                   Py_TYPE(arg)->tp_name);
   }

   return result;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(qmath_rect__doc__,
             "rect(length, phase, axis)\n"
             "\n"
             "Convert from polar coordinates to rectangular coordinates.\n"
             "Equivalent to: length *(math.cos(phase) + axis*sin(math.phase))\n"
             "Note: axis is normalised such that |axis| = 1 if required.");

static PyObject *
qmath_rect(PyObject *module, PyObject *args)
{
   PyObject * result = NULL;

   double radius;
   Py_quat_triple axis;
   double phase;
   int status;
   Py_quaternion r;

   status = PyArg_ParseTuple (args, "dd(ddd):quaternion.rect",
                              &radius, &phase, &axis.x, &axis.y, &axis.z);
   if (status) {
      /* Note: _Py_quat_from_polar normalised axis if need be */
      r = _Py_quat_from_polar(radius, axis, phase);
      result = PyQuaternion_FromCQuaternion (r);
   } else {
      result = NULL;
   }

   return result;
}

/* -----------------------------------------------------------------------------
 * METH_O - one argument,  (in addition to the module argument)
 */
static PyMethodDef qmath_methods[] = {
   {"isfinite", (PyCFunction)qmath_isfinite, METH_O,        qmath_isfinite__doc__},
   {"isinf",    (PyCFunction)qmath_isinf,    METH_O,        qmath_isinf__doc__},
   {"isnan",    (PyCFunction)qmath_isnan,    METH_O,        qmath_isnan__doc__},
   {"sqrt",     (PyCFunction)qmath_sqrt,     METH_O,        qmath_sqrt__doc__},
   {"exp",      (PyCFunction)qmath_exp,      METH_O,        qmath_exp__doc__},
   {"log",      (PyCFunction)qmath_log,      METH_VARARGS,  qmath_log__doc__},
   {"log10",    (PyCFunction)qmath_log10,    METH_O,        qmath_log10__doc__},
   {"cos",      (PyCFunction)qmath_cos,      METH_O,        qmath_cos__doc__},
   {"sin",      (PyCFunction)qmath_sin,      METH_O,        qmath_sin__doc__},
   {"tan",      (PyCFunction)qmath_tan,      METH_O,        qmath_tan__doc__},
   {"isclose",  (PyCFunction)qmath_isclose,  METH_KEYWORDS |
                                             METH_VARARGS,  qmath_isclose__doc__},
   {"polar",   (PyCFunction)qmath_polar,     METH_O,        qmath_polar__doc__},
   {"axis",    (PyCFunction)qmath_axis,      METH_O,        qmath_axis__doc__},
   {"phase",   (PyCFunction)qmath_phase,     METH_O,        qmath_phase__doc__},
   {"rect",    (PyCFunction)qmath_rect,      METH_VARARGS,  qmath_rect__doc__},
   {NULL, NULL}  /* sentinel */
};


/* Allow module defn code to access the quaternion PyMethodDef.
 */
PyMethodDef* _PyQmathMethods ()
{
   return qmath_methods;
}

/* end */
