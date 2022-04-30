/* quaternion.c
 *
 * This file is part of the Python quaternion module.
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

/* From https://docs.python.org/3.5/extending/newtypes.html,
 * together with cribbing many code-snippets and ideas from the complex type.
 *
 * Development environment:
 * Python 3.9.2
 * CentOS Linux release 7.9.2009 (Core)
 * gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-44)
 */

#include "quaternion_object.h"
#include <structmember.h>
#include <string.h>

/* Forward declarations
 */
static PyObject *
quaternion_subtype_from_c_quaternion(PyTypeObject *type, Py_quaternion qval);

/* ----------------------------------------------------------------------------
 */
static
void debugTrace(const char* function,
                const int line,
                const char* format, ...)
{
   va_list args;
   char buffer [200];

   va_start (args, format);
   vsnprintf (buffer, sizeof (buffer), format, args);
   va_end (args);

   char notification [240];
   snprintf (notification, sizeof (notification), "%4d (%s): %s\n", line, function, buffer);
   /* Avoid (gcc 8.4.1) error: format not a string literal and no format arguments */
   printf ("%s", notification);
}

#define DEBUG_TRACE(...)  debugTrace(__FUNCTION__, __LINE__, __VA_ARGS__)



/* ----------------------------------------------------------------------------
 * Sort of equivalent to PyFloat_AsDouble(PyNumber_Float))
 * Pre-requisite - verify obj with PyNumber_Check ... but this includes complex.
 */
static double
_PyNumber_AsDouble(PyObject *obj, bool* is_okay)
{
   double result = 0.0;
   PyObject *float_obj = NULL;

   /* Return as a PyFloatObject or NULL
    */
   float_obj = PyNumber_Float (obj);

   if (float_obj != NULL) {
      /* Extract C double from Python float.
      */
      result = PyFloat_AsDouble (float_obj);
      if (is_okay) *is_okay = true;
   } else {
      if (is_okay) *is_okay = false;
   }
   return result;

   // qtype/quaternion_object.c:46:13: warning: 'debugTrace' defined but not used [-Wunused-function]
   //
   DEBUG_TRACE ("make compiler warning go away");
}

/// ----------------------------------------------------------------------------
// Optimizations ...
//   /* Do the usual cases first
//    */
//   if (PyFloat_Check (arg)) {
//      *value = PyFloat_AsDouble(arg);
//      return true;
//   }

//   if (PyLong_Check (arg)) {
//      *value = PyLong_AsDouble(arg);
//      return true;
//   }

//   /* Don't allow '4.0' to 4.0 conversion here.
//    */
//   result = !PyUnicode_Check(arg);

//   if (result) {
//      /* Does this type provide a float() function for the given arg type ?
//       */
//      nbr = arg->ob_type->tp_as_number;
//      result = (nbr == NULL || nbr->nb_float == NULL);
//      if (result) {
//         number = PyNumber_Float (arg);
//         result = (number != NULL);
//         if (result) {
//            result = PyFloat_Check (number);  /** over kill ?? **/
//            if (result) {
//               *value = PyFloat_AsDouble (number);
//            }
//            Py_DECREF (number);
//         }
//      }
//   }
/// ----------------------------------------------------------------------------


/* ----------------------------------------------------------------------------
 * Used internally by add, sub, mult etc.
 * On conversion error  - sets pobj to Py_NotImplemented
 * TODO: Leverage off PyObject_AsCQuaternion and or rationalize.
 */
static bool
to_c_quaternion(PyObject **pobj, Py_quaternion *pc)
{
   PyObject *obj = *pobj;

   pc->w = pc->x = pc->y = pc->z = 0.0;

   if (PyLong_Check(obj)) {
      pc->w = PyLong_AsDouble(obj);
      if (pc->w == -1.0 && PyErr_Occurred()) {
         *pobj = NULL;
         return false;
      }
      return true;
   }

   if (PyFloat_Check(obj)) {
      pc->w = PyFloat_AsDouble(obj);
      return true;
   }

   if (PyComplex_Check(obj)) {
      Py_complex z = PyComplex_AsCComplex (obj);
      pc->w = z.real;
      pc->y = z.imag;
      return true;
   }

   if (PyQuaternion_Check(obj)) {
      *pc = ((PyQuaternionObject *)(obj))->qval;
      return true;
   }

   Py_INCREF(Py_NotImplemented);
   *pobj = Py_NotImplemented;
   return false;
}

/* NOTE This wrapper macro may return and modify the obj to NULL! Nasty...
 * It also checks if obj is already Quaternion, thus saving function call
 */
#define TO_C_QUATERNION(obj, c) {                                              \
   if (PyQuaternion_Check(obj))                                                \
      c = ((PyQuaternionObject *)(obj))->qval;                                 \
   else if (!to_c_quaternion(&(obj), &(c)))                                    \
      return (obj);                                                            \
}


/* -----------------------------------------------------------------------------
 * Decode a tuple argument (of floats and/or longs) into a triple of doubles
 * and returns false on failure, true on success.
 * The 'function' and 'arg_name' arguments are for PyErr_Format.
 */
static bool
quaternion_parse_triple(PyObject *arg, Py_quat_triple* triple,
                        char* function, char* arg_name)
{
   bool result;

   if (arg == NULL) {
      PyErr_Format(PyExc_TypeError, "%.200s() missing 1 required positional argument: '%.200s'",
                   function, arg_name);
      return false;
   }

   if (!PyTuple_Check(arg)) {
      PyErr_Format(PyExc_TypeError,
                   "%.200s() argument '%.200s' must be tuple, not '%.200s'",
                   function, arg_name,Py_TYPE(arg)->tp_name);
      return false;
   }

   /* PyArg_ParseTuple does all the hard work, but we do lose the exact
    * nature of the error.
    */
   result = PyArg_ParseTuple (arg, "ddd", &triple->x, &triple->y, &triple->z);

   if (!result) {
      /* Reset the generic error to something more appropiate.
       */
      PyErr_Format(PyExc_TypeError,
                   "%.200s() argument '%.200s' requires exactly 3 numeric values",
                   function, arg_name);
   }

   return result;
}


/* =============================================================================
 * init support functions.
 */

/* -----------------------------------------------------------------------------
 * a valid Quaternion string usually takes one of these forms:
 *
 *    <float><signed-float>i<signed-float>j<signed-float>k
 *    <float><signed-float>i<signed-float>j
 *    <float><signed-float>i<signed-float>k
 *    <float><signed-float>j<signed-float>k
 *    <float><signed-float>i
 *    <float><signed-float>j
 *    <float><signed-float>k
 *    <float>
 *
 *    <float>i<signed-float>j<signed-float>k
 *    <float>i<signed-float>j
 *    <float>i<signed-float>k
 *    <float>j<signed-float>k
 *
 *    <float>i
 *    <float>j
 *    <float>k
 *
 * where <float> represents any numeric string that's accepted by the
 * float constructor (including 'nan', 'inf', 'infinity', etc.), and
 * <signed-float> is any string of the form <float> whose first
 * character is '+' or '-'.
 *
 * i, j and k may be 'i', 'j', 'k' and/or 'I', 'J', 'K'
 * any real, i, j, k components may be ommitted, but must always be in order.
 *
 * Leading/trailing spaces allowed
 * Leading/trailing "(" and ")" allowed.
 *
 * This mirrors the complex type behaviour.
 */
static PyObject *
quaternion_init_from_string_inner(const char *s, Py_ssize_t len, void *type)
{
   Py_quaternion qval = { 0.0, 0.0, 0.0, 0.0 };  /* initialised working copy - when needed */

   double r=0.0, x=0.0, y=0.0, z=0.0;
   double dval;
   int got_bracket=0;
   const char *start;
   char *end;
   int e;
   int allowed;
   int at_least_one;

   /* position on first non-blank */
   start = s;

   while (Py_ISSPACE(*s))
      s++;

   if (*s == '(') {
      /* Skip over possible bracket from repr(). */
      got_bracket = 1;
      s++;
      while (Py_ISSPACE(*s))
         s++;
   }

   allowed = 1;
   at_least_one = 0;
   for (e = 0; e < 4; e++) {

      /* first look for forms starting with <float> */
      dval = PyOS_string_to_double(s, &end, NULL);
      if (dval == -1.0 && PyErr_Occurred()) {
         if (PyErr_ExceptionMatches(PyExc_ValueError))
            PyErr_Clear();
         else
            return NULL;
      }

      if (end == s) {
         /* We did not read a float */
         goto parse_error;
      }

      s = end;   /* skip to end of the float */

      if ((allowed <= 1) &&
          (*s != 'i')  && (*s != 'I')  &&
          (*s != 'j')  && (*s != 'J')  &&
          (*s != 'k')  && (*s != 'K')) {
         r = dval;
         at_least_one = 1;
         allowed = 2;
      } else if ((allowed <= 2) && (*s == 'i' || *s == 'I')) {
         s++;
         x = dval;
         at_least_one = 1;
         allowed = 3;
      } else if ((allowed <= 3) && (*s == 'j' || *s == 'J')) {
         s++;
         y = dval;
         at_least_one = 1;
         allowed = 4;
      } else if ((allowed <= 4) && (*s == 'k' || *s == 'K')) {
         s++;
         z = dval;
         at_least_one = 1;
         allowed = 5;
      } else {
         goto parse_error;
      }

      if (s-start == len) {
         break;
      }

      if ((*s == ' ') || (*s == ')')) {
         break;
      }

      if ((allowed <= 5) && (*s == '+' || *s == '-')) {
         /* okay */
      } else {
         goto parse_error;
      }
   }

   /* dissallow empty string */
   if (!at_least_one)
      goto parse_error;

   /* trailing whitespace and closing bracket */
   while (Py_ISSPACE(*s))
      s++;

   if (got_bracket) {
      /* if there was an opening parenthesis, then the corresponding
          closing parenthesis should be right here */
      if (*s != ')')
         goto parse_error;
      s++;
      while (Py_ISSPACE(*s))
         s++;
   }

   /* we should now be at the end of the string */
   if (s-start != len) {
      goto parse_error;
   }

   /* all good = copy the values
    */
   qval.w = r;
   qval.x = x;
   qval.y = y;
   qval.z = z;
   return quaternion_subtype_from_c_quaternion (type, qval);

parse_error:
   PyErr_SetString(PyExc_ValueError,
                   "Quaternion() arg is a malformed string");
   return NULL;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_init_from_string(PyTypeObject *type, PyObject *v)
{
   const char *s;
   PyObject *s_buffer = NULL, *result = NULL;
   Py_ssize_t len;

   if (PyUnicode_Check(v)) {
       s_buffer = _PyUnicode_TransformDecimalAndSpaceToASCII(v);
       if (s_buffer == NULL) {
           return NULL;
       }
       s = PyUnicode_AsUTF8AndSize(s_buffer, &len);
       if (s == NULL) {
           goto exit;
       }
   }
   else {
       PyErr_Format(PyExc_TypeError,
           "complex() argument must be a string or a number, not '%.200s'",
           Py_TYPE(v)->tp_name);
       return NULL;
   }

   result = _Py_string_to_number_with_underscores (s, len, "Quaternion", v, type,
                                                   quaternion_init_from_string_inner);
 exit:
   Py_DECREF(s_buffer);
   return result;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
   static char *kwlist[] = {"w", "x", "y", "z", "real", "imag",
                            "angle", "axis", "matrix", 0};

//   PyQuaternionObject *self = NULL;
   PyObject *r = NULL, *i = NULL, *j = NULL, *k = NULL;
   PyObject *angle = NULL, *axis = NULL;
   PyObject *real = NULL, *imag = NULL;
   PyObject *matrix = NULL;
   unsigned int mask = 0;
   bool status;
   PyComplexObject* z = NULL;

   Py_quaternion qval = { 0.0, 0.0, 0.0, 0.0 };  /* initialised working copy - when needed */

   if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOOOOOO:Quaternion", kwlist,
                                    &r, &i, &j, &k, &real, &imag, &angle, &axis, &matrix)) {
      return NULL;
   }

   /* Create a mask of all supplied parameters.
    * This allows us to easily do various parameter combination checks.
    */
   mask = 0x00;
   if (r) mask |= 0x01;
   if (i) mask |= 0x02;
   if (j) mask |= 0x04;
   if (k) mask |= 0x08;
   if (angle) mask |= 0x10;
   if (axis)  mask |= 0x20;
   if (real)  mask |= 0x40;
   if (imag)  mask |= 0x80;
   if (matrix) mask |= 0x100;

   /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    * o - the object
    * d - the dest float(double)
    */
#define MMM(o,d)                                                               \
   if (o != NULL) {                                                            \
      if (PyNumber_Check(o)) {                                                 \
         d = _PyNumber_AsDouble (o, &status);                                  \
         if (!status) {                                                        \
            PyErr_SetString(PyExc_TypeError, "float(" #o ") didn't return a float");  \
            return NULL;                                                       \
         }                                                                     \
      } else {                                                                 \
         PyErr_Format(PyExc_TypeError,                                         \
                      "Quaternion() argument '"#o"' must be float, not '%.200s'", \
                      Py_TYPE(o)->tp_name);                                    \
         return NULL;                                                          \
      }                                                                        \
   }


   /* Do the special cases
    */
   if (mask == 0x00) {
      /* No arguments - null value - all parts are zero (set 0 by new function).
       */
      return quaternion_subtype_from_c_quaternion (type, qval);
   }

   if (mask == 0x01) {
      /* A single value as first parameter
       */

      /* Special-case for a single argument when type(arg) is quaternion.
       */
      if (PyQuaternion_CheckExact(r)) {
         /* Note that we can't know whether it's safe to return
            a quaternion *subclass* instance as-is, hence the restriction
            to exact quaternions here.  If either the input or the
            output is a quaternion subclass, it will be handled below
            as a non-orthogonal vector.
         */
         Py_INCREF(r);
         return r;
      }

      if (PyComplex_Check(r)) {
         /* init from complex */
         z = (PyComplexObject*) (r);
         qval.w = z->cval.real;
         qval.y = z->cval.imag;
         return quaternion_subtype_from_c_quaternion (type, qval);
      }

      if (PyNumber_Check(r)) {
         /* init from number */
         MMM (r, qval.w);
         return quaternion_subtype_from_c_quaternion (type, qval);
      }

      if (PyUnicode_Check(r)) {
         return quaternion_init_from_string (type, r);
      }

      PyErr_Format (PyExc_TypeError, "Quaternion() argument r, type '%.200s',"
                    "cannot be converted to a Quaternion", Py_TYPE(r)->tp_name);
      return NULL;
   }

   /* Rectalinear components - we want up to four floats ?
    */
   if (mask == (mask & 0x0F)) {

      /* mask == 0x01 already handled, but more trouble than worth it to
       * explicitly not handle that case
       */

      MMM (r, qval.w);
      MMM (i, qval.x);
      MMM (j, qval.y);
      MMM (k, qval.z);

      /* All good */
      return quaternion_subtype_from_c_quaternion (type, qval);
   }

   /* Angle and axis specification ?
    */
   if (mask == (mask & 0x30)) {
      /* We have been given one or both - we need need both angle and axis.
       */
      if (angle == NULL) {
          PyErr_SetString(PyExc_TypeError, "Quaternion() missing 1 required named argument: 'angle'");
         return NULL;
      }

      if (axis == NULL) {
          PyErr_SetString(PyExc_TypeError, "Quaternion() missing 1 required named argument: 'axis'");
         return NULL;
      }

      double a = 0.0;
      Py_quat_triple b;

      MMM(angle, a);

      status = quaternion_parse_triple (axis, &b, "Quaternion", "axis");
      if (!status) {
         return NULL;
      }

      errno = 0;
      qval = _Py_quat_calc_rotation (a, b);  /* will normalise the axis */
      if (errno == EDOM) {
         PyErr_SetString(PyExc_ValueError, "Quaternion() 'axis' argument has no direction - is zero");
         return NULL;
      }
      return quaternion_subtype_from_c_quaternion (type, qval);
   }

   /* real and imag specification ?
    */
   if (mask == (mask & 0xC0)) {
      /* We have been given one or both - we need need both angle and axis.
       */
      if (real == NULL) {
          PyErr_SetString(PyExc_TypeError, "Quaternion() missing 1 required named argument: 'real'");
         return NULL;
      }

      double a = 0.0;
      Py_quat_triple b;

      MMM(real, a);

      if (imag) {
         status = quaternion_parse_triple (imag, &b, "Quaternion", "imag");
         if (!status) {
            return NULL;
         }
      } else {
         b.x = b.y = b.z = 0.0;
      }

      errno = 0;
      qval.w = a;
      qval.x = b.x;
      qval.y = b.y;
      qval.z = b.z;
      return quaternion_subtype_from_c_quaternion (type, qval);
   }

   /* matrix specification ?
    */
   if (mask == (mask & 0x100)) {

      /* We have been given a matrix.
       */
      Py_quat_matrix mat;

      if (!PyTuple_Check(matrix)) {
         PyErr_Format(PyExc_TypeError,
                      "%.200s() argument '%.200s' must be tuple, not '%.200s'",
                      "Quaternion", "matrix", Py_TYPE(matrix)->tp_name);
         return false;
      }


      /* PyArg_ParseTuple does all the hard work, but we do lose the exact
       * nature of the error.
       */
      status = PyArg_ParseTuple(matrix, "(ddd)(ddd)(ddd):Quaternion()",
                                &mat.r11, &mat.r12, &mat.r13,
                                &mat.r21, &mat.r22, &mat.r23,
                                &mat.r31, &mat.r32, &mat.r33);

      if (!status) {
         /* Reset the generic error to something more appropriate.
          */
         PyErr_Format(PyExc_TypeError,
                      "%.200s() argument '%.200s' requires 3x3 nested tuple of numeric values",
                      "Quaternion", "matrix");
         return NULL;
      }

      qval = _Py_quat_from_rotation_matrix(&mat);
      return quaternion_subtype_from_c_quaternion(type, qval);
   }

   /* If we get here, it's an error.
    * Be more disserning re allow combinations. 
    */
   PyErr_SetString(PyExc_TypeError,
                   "Quaternion() can't mix components and/or 'angle'/'axis' and/or 'real'/'imag' and/or 'matrix' arguments");
   return NULL;

#undef MMM
}

/* =============================================================================
 * Methods
 */

/* -----------------------------------------------------------------------------
 * Attribute support function
 */
static PyObject *
quaternion_get_vector(PyObject *self)
{
   Py_quaternion q;
   PyObject *r = NULL;

   q = ((PyQuaternionObject *)self)->qval;
   r = Py_BuildValue("(ddd)", q.x, q.y, q.z);

   return r;
}

/* -----------------------------------------------------------------------------
 * Attribute support function
 */
static PyObject *
quaternion_get_complex(PyObject *self)
{
   Py_quaternion c;
   c = ((PyQuaternionObject *)self)->qval;
   return PyComplex_FromDoubles (c.w, c.y);  /* real and j component */
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_conjugate(PyObject *self)
{
   Py_quaternion c, r;
   c = ((PyQuaternionObject *)self)->qval;
   PyFPE_START_PROTECT("quaternion_conjugate", return 0);
   r = _Py_quat_conjugate(c);
   PyFPE_END_PROTECT(r);
   return PyQuaternion_FromCQuaternion(r);
}

PyDoc_STRVAR(quaternion_conjugate_doc,
             "quaternion.conjugate() -> quaternion\n"
             "\n"
             "Return the quaternion conjugate of its argument.\n"
             "Example: (3-4i+5j-6k).conjugate() == 3+4i-5j+6k.");

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_inverse(PyObject *self)
{
   Py_quaternion c, r;
   c = ((PyQuaternionObject *)self)->qval;
   PyFPE_START_PROTECT("quaternion_inverse", return 0);
   errno = 0;
   r = _Py_quat_inverse(c);
   PyFPE_END_PROTECT(r);
   if (errno == EDOM) {
      PyErr_SetString(PyExc_ZeroDivisionError, "quaternion inverse of zero");
      return NULL;
   }
   return PyQuaternion_FromCQuaternion(r);
}

PyDoc_STRVAR(quaternion_inverse_doc,
             "quaternion.inverse() -> quaternion\n"
             "\n"
             "Return the quaternion inverse of its argument,\n"
             "such that: q * q.inverse() == q.inverse() * q == 1+0i+0j+0k.");

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_normalise(PyObject *self)
{
   Py_quaternion c, r;
   c = ((PyQuaternionObject *)self)->qval;
   PyFPE_START_PROTECT("quaternion_normalise", return 0);
   r = _Py_quat_normalise(c);
   PyFPE_END_PROTECT(r);
   return PyQuaternion_FromCQuaternion(r);
}

PyDoc_STRVAR(quaternion_normalise_doc,
             "quaternion.normalise() -> quaternion\n"
             "\n"
             "Return the normalised quaternion of the argument such that\n"
             "abs (q.normalise()) == 1.0");


/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_rotation_matrix(PyObject *self)
{
   Py_quaternion q;
   Py_quat_matrix matrix;
   q = ((PyQuaternionObject *)self)->qval;
   PyFPE_START_PROTECT("quaternion_rotation_matrix", return 0);
   _Py_quat_to_rotation_matrix (q, &matrix);
   PyFPE_END_PROTECT(q);
   return Py_BuildValue("((ddd)(ddd)(ddd))",
                        matrix.r11, matrix.r12, matrix.r13,
                        matrix.r21, matrix.r22, matrix.r23,
                        matrix.r31, matrix.r32, matrix.r33);
}

PyDoc_STRVAR(quaternion_rotation_matrix_doc,
             "quaternion.rotation_matrix() -> 3x3 float matrix"
             "\n"
             "Returns the equivilent 3D rotation matrix of a rotation Quaternion.\n"
             "Returns a 3-tuple of 3-tuples of floats.\n"
             "The returned value may then be turned into numpy array.\n"
             "Example: \n"
             "    rot_mat = np.array(q.rotation_matrix())");

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_rotation_angle(PyObject *self)
{
   Py_quaternion c;
   double angle;
   c = ((PyQuaternionObject *)self)->qval;
   if (c.w >= -1 && c.w <= +1) {
      angle = acos (c.w) * 2.0;
   } else {
      PyErr_Format(PyExc_ValueError,
                   "rotation_angle() math domain error");
      return NULL;
   }
   return PyFloat_FromDouble (angle);
}

PyDoc_STRVAR(quaternion_rotation_angle_doc,
             "quaternion.rotation_angle() -> float"
             "\n"
             "Return the rotation angle of a rotation quaternion q, i.e. a\n"
             "quaternion constructed as:\n"
             "   q = Quaternion(angle=angle, axis=(...))\n"
             "\n"
             "Note: None-rotation quaternions may lead to a maths error.\n"
             "Note: this angle should not be confused with the polor\n"
             "co-ordinate's argument or phase angle");

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_getnewargs(PyComplexObject* self)
{
    Py_quaternion q;
    q = ((PyQuaternionObject *)self)->qval;

    return Py_BuildValue("(dddd)", q.w, q.x, q.y, q.z);
}

/* no doc string */

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion__format__(PyObject *self, PyObject *args)
{
   /* Wrapper around Py_DECREF that checks the ref exists.
    * And clears the ref pointer if needs be
    */
   #define QDECREF(obj) if (obj != NULL) {                                     \
      Py_DECREF(obj);                                                          \
      obj = NULL;                                                              \
   }


   PyObject *result = NULL;
   PyObject *format_spec = NULL;

   if (!PyArg_ParseTuple(args, "U:__format__", &format_spec))
      return NULL;

   if (!format_spec) {
      /* Belts amd braces
       */
      PyErr_SetString(PyExc_ValueError,
                      "Quaternion.__format__() null format spec");                                         \
      return NULL;
   }

   /* Leverage off the float formating function - this ensures consistancy
    * but at the cost of creating four float PyObjects objects and four
    * unicode PyObjects objects.
    */
   PyQuaternionObject *q = (PyQuaternionObject *)self;
   PyObject *r = PyFloat_FromDouble (q->qval.w);
   PyObject *i = PyFloat_FromDouble (q->qval.x);
   PyObject *j = PyFloat_FromDouble (q->qval.y);
   PyObject *k = PyFloat_FromDouble (q->qval.z);

   /* Do all components float objects exits?
    */
   if (r && i && j && k) {

      PyObject *fr = PyObject_Format (r, format_spec);
      PyObject *fi = PyObject_Format (i, format_spec);
      PyObject *fj = PyObject_Format (j, format_spec);
      PyObject *fk = PyObject_Format (k, format_spec);

      /* Do all float fromat Unicode objects exits?
       */
      if (fr && fi && fj && fk) {

         /* Convert to utf8 strings
          */
         const char *utf8_r = PyUnicode_AsUTF8 (fr);
         const char *utf8_i = PyUnicode_AsUTF8 (fi);
         const char *utf8_j = PyUnicode_AsUTF8 (fj);
         const char *utf8_k = PyUnicode_AsUTF8 (fk);

         /* Save required length - the formatting of r on its own will
          * capture ther overall required length.
          */
         const int rlen = strlen (utf8_r);

         /* Strip leading spaces.
          */
         while (*utf8_r == ' ') utf8_r++;
         while (*utf8_i == ' ') utf8_i++;
         while (*utf8_j == ' ') utf8_j++;
         while (*utf8_k == ' ') utf8_k++;

         /* Do the i, j and/or k start with a sign?
          */
         bool si = (*utf8_i == '+') || (*utf8_i == '-');
         bool sj = (*utf8_j == '+') || (*utf8_j == '-');
         bool sk = (*utf8_k == '+') || (*utf8_k == '-');

         /* If no sign add a '+', if component -ve format will always add a sign.
          */
         char image [240];
         snprintf (image, sizeof(image), "%s%s%si%s%sj%s%sk", utf8_r,
                   si ? "" : "+", utf8_i,
                   sj ? "" : "+", utf8_j,
                   sk ? "" : "+", utf8_k);


         int alen = strlen (image);
         int pad = rlen - alen;

         /* Constrain pad to available buffer size. */
         if (pad > (int) sizeof(image) - 1 - alen) {
             pad = (int) sizeof(image) - 1 - alen;
         }

         if (pad > 0) {
            memmove (&image[pad], &image [0], alen + 1);   /* Include the zero */
            int i;
            for (i = 0; i < pad; i++)
               image [i] = ' ';
         }

         image [sizeof(image) - 1] = '\0';     /* Belts n braces */

         result = PyUnicode_FromString (image);
      }

      /* Done with these objects - decrement the ref counts.
       */
      QDECREF (fr);
      QDECREF (fi);
      QDECREF (fj);
      QDECREF (fk);
   }

   /* And done with these objects - decrement the ref counts.
    */
   QDECREF (r);
   QDECREF (i);
   QDECREF (j);
   QDECREF (k);

   return  result;

#undef QDECREF
}

PyDoc_STRVAR(quaternion_format_doc,
             "quaternion.__format__() -> str\n"
             "\n"
             "Convert to a string according to format_spec.");


/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion__round__(PyObject *self, PyObject *args, PyObject *kwds)
{
   static char* kwlist[] = {"ndigits", NULL};

   Py_quaternion c, r;
   int s;
   PyObject* ndigits = NULL;
   int n;

   /* Parse into one optional argument
    */
   s = PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &ndigits);
   if (!s) {
      return NULL;
   }

   if (ndigits) {
      /* Caller supplied a parameter
       */
      if (PyLong_Check (ndigits)) {
         /* and is long
          */
         n = PyLong_AsLong (ndigits);
      } else {
         PyErr_Format(PyExc_TypeError,
                      "quaternion round: '%s' object cannot be interpreted as an integer",
                      Py_TYPE(ndigits)->tp_name);

         return NULL;
      }
   } else {
      n = 0;  /* default */
   }

   c = ((PyQuaternionObject *)self)->qval;
   PyFPE_START_PROTECT("quaternion__round__", return 0);
   r = _Py_quat_round(c, n);
   PyFPE_END_PROTECT(r);
   return PyQuaternion_FromCQuaternion(r);
}

PyDoc_STRVAR(quaternion_round_doc,
             "quaternion.__round__(ndigits=0) -> quaternion\n"
             "\n"
             "Returns the quaternion with rounded members, perhaps most useful\n"
             "when printing.\n"
             "\n"
             "ndigits - number of digits to round by, may be positive or negative.\n"
             "round(q,n) == Quaterion(round(q.w,n), round(q.x,n), round(q.y,n), round(q.z.n))");


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_rotate_doc,
             "quaternion.rotate (point, origin=None) -> point\n"
             "\n"
             "Rotates the point using self. Self should be constructed using the angle/axis.\n"
             "option. At the very least self should be normalised.\n"
             "\n"
             "point    - is the point to be rotated, expects a tuple with 3 float elements\n"
             "origin   - the point about which the rotation occurs; when not specified or\n"
             "           None the origin is deemed to be (0.0, 0.0, 0.0)\n");

static PyObject *
quaternion_rotate(PyObject *self, PyObject *args, PyObject *kwds)
{
   static char *kwlist[] = {"point", "origin", NULL};

   PyObject *result = NULL;
   PyObject *point = NULL;
   PyObject *origin = NULL;

   int s;
   Py_quat_triple c_point = { 0.0, 0.0, 0.0 };
   Py_quat_triple c_origin= { 0.0, 0.0, 0.0 };
   Py_quat_triple r_point;

   /* Parse into two arguments
    */
   s = PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &point, &origin);
   if (!s) {
      return NULL;
   }

   s = quaternion_parse_triple (point, &c_point, "rotate", "point");
   if (!s) {
      return NULL;
   }

   /* origin can be null or None */
   if ((origin != NULL) && (origin != Py_None)) {
      s = quaternion_parse_triple (origin, &c_origin, "rotate", "origin");
      if (!s) {
         return NULL;
      }
   }

   r_point = _Py_quat_rotate (((PyQuaternionObject *)self)->qval, c_point, c_origin);

   result = Py_BuildValue("(ddd)", r_point.x, r_point.y, r_point.z);
   return result;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_repr (PyQuaternionObject *v)
{
   int prec = 0;
   char format = 'r';

   PyObject *result = NULL;

   /* If these are non-NULL, they'll need to be freed.
    */
   char *ps = NULL;
   char *px = NULL;
   char *py = NULL;
   char *pz = NULL;

   ps = PyOS_double_to_string(v->qval.w, format, prec, 0, NULL);
   if (!ps) {
      PyErr_NoMemory();
      goto done;
   }

   px = PyOS_double_to_string(v->qval.x, format, prec, Py_DTSF_SIGN, NULL);
   if (!px) {
      PyErr_NoMemory();
      goto done;
   }

   py = PyOS_double_to_string(v->qval.y, format, prec, Py_DTSF_SIGN, NULL);
   if (!px) {
      PyErr_NoMemory();
      goto done;
   }

   pz = PyOS_double_to_string(v->qval.z, format, prec, Py_DTSF_SIGN, NULL);
   if (!px) {
      PyErr_NoMemory();
      goto done;
   }

   result = PyUnicode_FromFormat("quaternion.Quaternion(%s, %s, %s, %s)", ps, px, py, pz);

done:
   PyMem_Free(ps);
   PyMem_Free(px);
   PyMem_Free(py);
   PyMem_Free(pz);

   return result;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_str (PyQuaternionObject *v)
{
   int prec = 0;
   char format = 'r';

   PyObject *result = NULL;

   /* If these are non-NULL, they'll need to be freed.
    */
   char *ps = NULL;
   char *px = NULL;
   char *py = NULL;
   char *pz = NULL;

   ps = PyOS_double_to_string(v->qval.w, format, prec, 0, NULL);
   if (!ps) {
      PyErr_NoMemory();
      goto done;
   }

   px = PyOS_double_to_string(v->qval.x, format, prec, Py_DTSF_SIGN, NULL);
   if (!px) {
      PyErr_NoMemory();
      goto done;
   }

   py = PyOS_double_to_string(v->qval.y, format, prec, Py_DTSF_SIGN, NULL);
   if (!px) {
      PyErr_NoMemory();
      goto done;
   }

   pz = PyOS_double_to_string(v->qval.z, format, prec, Py_DTSF_SIGN, NULL);
   if (!px) {
      PyErr_NoMemory();
      goto done;
   }

   result = PyUnicode_FromFormat("(%s%si%sj%sk)", ps, px, py, pz);

done:
   PyMem_Free(ps);
   PyMem_Free(px);
   PyMem_Free(py);
   PyMem_Free(pz);

   return result;
}


/* -----------------------------------------------------------------------------
 */
static Py_hash_t
quaternion_hash (PyQuaternionObject *v)
{
   PyObject *z1, *z2;
   Py_uhash_t hashz1, hashz2, combined;

   /* z1 is the complex part of v, z2 is the other part of v as a complex number.
    * This ensures that numbers that compare equal return same hash value.
    */
   z1 = (PyObject *) PyComplex_FromDoubles (v->qval.w, v->qval.y);  /* w, j */
   z2 = (PyObject *) PyComplex_FromDoubles (v->qval.x, v->qval.z);  /* i, k */

   hashz1 = z1->ob_type->tp_hash (z1);
   hashz2 = z2->ob_type->tp_hash (z2);

   z1->ob_type->tp_free (z1);
   z2->ob_type->tp_free (z2);

   combined = hashz1 + (_PyHASH_MULTIPLIER + 0x5afe) * hashz2;

   /* Don't allow -1 as a hash value.
    */
   if (combined == (Py_uhash_t)(-1))
      combined = (Py_uhash_t)(-2);

   return (Py_hash_t)combined;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_richcompare (PyObject *v, PyObject *w, int op)
{
   PyObject *result;
   Py_quaternion a, b;
   int equal;

   if (op != Py_EQ && op != Py_NE) {
      goto Unimplemented;
   }

   TO_C_QUATERNION(v, a);
   TO_C_QUATERNION(w, b);

   equal = _Py_quat_eq (a, b);

   if (equal == (op == Py_EQ))
      result = Py_True;
   else
      result = Py_False;

   Py_INCREF(result);

   return result;

Unimplemented:
   Py_RETURN_NOTIMPLEMENTED;
}


/* -----------------------------------------------------------------------------
 * Functions referenced by Quaternion_as_number (via tp_as_number)
 */
/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_add(PyObject *v, PyObject *w)
{
   Py_quaternion result;
   Py_quaternion a, b;
   TO_C_QUATERNION(v, a);
   TO_C_QUATERNION(w, b);
   PyFPE_START_PROTECT("quaternion_add", return 0)
   result = _Py_quat_sum (a, b);
   PyFPE_END_PROTECT(result)
   return PyQuaternion_FromCQuaternion(result);
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_sub(PyObject *v, PyObject *w)
{
   Py_quaternion result;
   Py_quaternion a, b;
   TO_C_QUATERNION(v, a);
   TO_C_QUATERNION(w, b);
   PyFPE_START_PROTECT("quaternion_sub", return 0)
   result = _Py_quat_diff(a, b);
   PyFPE_END_PROTECT(result)
   return PyQuaternion_FromCQuaternion(result);
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_mul(PyObject *v, PyObject *w)
{
   Py_quaternion result;
   Py_quaternion a, b;
   TO_C_QUATERNION(v, a);
   TO_C_QUATERNION(w, b);
   PyFPE_START_PROTECT("quaternion_mul", return 0)
   result = _Py_quat_prod(a, b);
   PyFPE_END_PROTECT(result)
   return PyQuaternion_FromCQuaternion(result);
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_true_div(PyObject *v, PyObject *w)
{
   Py_quaternion result;
   Py_quaternion a, b;
   TO_C_QUATERNION(v, a);
   TO_C_QUATERNION(w, b);
   PyFPE_START_PROTECT("quaternion_div", return 0)
   errno = 0;
   result = _Py_quat_quot(a, b);
   PyFPE_END_PROTECT(result)
   if (errno == EDOM) {
      PyErr_SetString(PyExc_ZeroDivisionError, "quaternion division by zero");
      return NULL;
   }
   return PyQuaternion_FromCQuaternion(result);
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_remainder(PyObject *v, PyObject *w)
{
   PyErr_SetString(PyExc_TypeError,
                   "can't mod Quaternion numbers.");
   return NULL;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_divmod(PyObject *v, PyObject *w)
{
   PyErr_SetString(PyExc_TypeError,
                   "can't take floor or mod of Quaternion number.");
   return NULL;
}

/* -----------------------------------------------------------------------------
 * Unlike its complex counter part, a Quaternion cannot be raised to the
 * power of a Quaternion (except in special circumstances, e.g. no imarginary
 * parts). The usual exp (log(v)*w) does not work with Quaternions as this
 * yields a different result from exp (w*log(v)).
 * Special cases that are allowed are v or w are real (or int), i.e we can raise
 * a quaternion number to a real power or raise a real number to a quaternion
 * power. In both cases w*log(v) == log(v)*w, and therefore no ambiguity.
 */
static PyObject *
quaternion_pow(PyObject *v, PyObject *w, PyObject *z)
{   
   Py_quaternion result;
   Py_quaternion a;   /* v or w as appropriate */
   double real;
   bool real_is_okay;

   /* Cannot modulo a Quaternion, 3rd parameter not allowed.
    */
   if (z != Py_None) {
      Py_INCREF(Py_NotImplemented);
      return Py_NotImplemented;
   }

   /* In general case can only raise a Quaternion to a real power,
    * or  raise a real number to a Quaternion power
    */
   if (PyQuaternion_Check(v)) {

      /* v is a quaternion, check w is real (or int)
       */
      if (!PyFloat_Check (w) && !PyLong_Check (w)) {
         Py_INCREF(Py_NotImplemented);
         return Py_NotImplemented;
      }

      TO_C_QUATERNION(v, a);
      real = _PyNumber_AsDouble (w, &real_is_okay);
      if (!real_is_okay) {
         PyErr_SetString(PyExc_TypeError, "Quaternion pow() argument 2 didn't return a float");
         return NULL;
      }

      PyFPE_START_PROTECT("quaternion_pow", return 0);
      errno = 0;
      result = _Py_quat_pow1(a, real);
      PyFPE_END_PROTECT(result);

      if (errno == EDOM) {
         PyErr_SetString(PyExc_ZeroDivisionError,
                         "(0+0i+0j+0k) cannot be raised to a negative power");
         return NULL;
      }

   } else if (PyQuaternion_Check(w)) {

      /* w is a quaternion, check v is real (or int)
       */
      if (!PyFloat_Check (v) && !PyLong_Check (v)) {
         Py_INCREF(Py_NotImplemented);
         return Py_NotImplemented;
      }

      TO_C_QUATERNION(w, a);
      real = _PyNumber_AsDouble (v, &real_is_okay);
      if (!real_is_okay) {
         PyErr_SetString(PyExc_TypeError, "Quaternion pow() argument 1 didn't return a float");
         return NULL;
      }

      PyFPE_START_PROTECT("quaternion_pow", return 0);
      errno = 0;
      result = _Py_quat_pow2(real, a);
      PyFPE_END_PROTECT(result);

      if (errno == EDOM) {
         PyErr_SetString(PyExc_ZeroDivisionError,
                         "0.0 to a negative or quaternion power");
         return NULL;
      }


   } else {
      /* Neither v nor w is a quaternion - will this ever happen??
       */
      Py_INCREF(Py_NotImplemented);
      return Py_NotImplemented;
   }

   real_is_okay = true;

   Py_ADJUST_ERANGE2(result.w, result.x);
   if (errno == ERANGE) {
      real_is_okay = false;
   }

   Py_ADJUST_ERANGE2(result.y, result.z);
   if (errno == ERANGE) {
      real_is_okay = false;
   }

   if (!real_is_okay) {
      PyErr_SetString(PyExc_OverflowError,
                      "Quaternion numerical result out of range");
      return NULL;
   }

   return PyQuaternion_FromCQuaternion(result);
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_neg (PyQuaternionObject *v)
{
   Py_quaternion neg;
   neg = _Py_quat_neg (v->qval);
   return PyQuaternion_FromCQuaternion(neg);
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_pos (PyQuaternionObject *v)
{
   if (PyQuaternion_CheckExact((PyObject *)v)) {
      Py_INCREF(v);
      return (PyObject *)v;
   } else {
      return PyQuaternion_FromCQuaternion (v->qval);
   }
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_abs (PyQuaternionObject *v)
{
   double result;

   PyFPE_START_PROTECT("quaternion_abs", return 0)
   result = _Py_quat_abs(v->qval);
   PyFPE_END_PROTECT(result)

   if (errno == ERANGE) {
      PyErr_SetString(PyExc_OverflowError,
                      "Quaternion absolute value too large");
      return NULL;
   }
   return PyFloat_FromDouble (result);
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_quadrance (PyQuaternionObject *self)
{
   Py_quaternion c;
   double result;

   c = ((PyQuaternionObject *)self)->qval;
   PyFPE_START_PROTECT("quaternion_quadrance", return 0);
   errno = 0;
   result = _Py_quat_quadrance(c);
   PyFPE_END_PROTECT(result);
   if (errno == ERANGE) {
      PyErr_SetString(PyExc_OverflowError,
                      "Quaternion quadrance value too large");
      return NULL;
   }
   return PyFloat_FromDouble (result);
}

PyDoc_STRVAR(quaternion_quadrance_doc,
             "quaternion.quadrance() -> float"
             "\n"
             "Return the quadrance of the quaternion argument,\n"
             "such that: q.quadrance() == abs(q)**2.");


/* -----------------------------------------------------------------------------
 */
static int
quaternion_is_bool(PyQuaternionObject *v)
{
   return v->qval.w != 0.0 || v->qval.x != 0.0 || v->qval.y != 0.0 || v->qval.z != 0.0;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_int(PyObject *v)
{
   PyErr_SetString(PyExc_TypeError,
                   "can't convert Quaternion to int");
   return NULL;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_float(PyObject *v)
{
   PyErr_SetString(PyExc_TypeError,
                   "can't convert Quaternion to float");
   return NULL;
}

// -----------------------------------------------------------------------------
//
static PyObject *
quaternion_int_div(PyObject *v, PyObject *w)
{
   PyErr_SetString(PyExc_TypeError,
                   "can't take floor of Quaternion number.");
   return NULL;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_getattro(PyObject *self, PyObject *attr)
{
   PyObject *result = NULL;
   const char* name = NULL;

   if (PyUnicode_Check(attr)) {
      /* The returned buffer always has an extra null byte appended
       * In the case of an error, NULL is returned with an exception set.
       * The caller is not responsible for deallocating the buffer.
       */
      name = PyUnicode_AsUTF8 (attr);
      if (name) {
         /* Check for our own attributes
          */
         if (strcmp(name, "real") == 0) {
            Py_quaternion c;
            c = ((PyQuaternionObject *)self)->qval;
            result = PyFloat_FromDouble (c.w);

         } else if ((strcmp(name, "vector") == 0) || (strcmp(name, "imag") == 0)) {
            result = quaternion_get_vector (self);

         } else if (strcmp(name, "complex") == 0) {
            result = quaternion_get_complex (self);
         }
      }
   }

   if (result == NULL) {
      /* Use default generic get attributes function for members and functions.
       * This could maybe be placed inside the unicode check or the name check.
       */
      result = PyObject_GenericGetAttr (self, attr);
   }

   if (result == NULL) {
      PyErr_Format(PyExc_AttributeError,
                   "'quaternion.Quaternion' object has no attribute '%.200s'", name);
   }

   return result;
}


/* =============================================================================
 * Tables
 * =============================================================================
 */
static PyMethodDef QuaternionMethods [] = {
   {"__getnewargs__", (PyCFunction)quaternion_getnewargs, METH_NOARGS,   NULL},
   {"__format__",     (PyCFunction)quaternion__format__,  METH_VARARGS,  quaternion_format_doc},
   {"__round__",      (PyCFunction)quaternion__round__,   METH_VARARGS |
                                                          METH_KEYWORDS, quaternion_round_doc},
   {"conjugate",      (PyCFunction)quaternion_conjugate,  METH_NOARGS,   quaternion_conjugate_doc},
   {"inverse",        (PyCFunction)quaternion_inverse,    METH_NOARGS,   quaternion_inverse_doc},
   {"quadrance",      (PyCFunction)quaternion_quadrance,  METH_NOARGS,   quaternion_quadrance_doc},
   {"normalise",      (PyCFunction)quaternion_normalise,  METH_NOARGS,   quaternion_normalise_doc},
   {"rotation_matrix",
               (PyCFunction)quaternion_rotation_matrix,   METH_NOARGS,   quaternion_rotation_matrix_doc},
   {"rotation_angle",
               (PyCFunction)quaternion_rotation_angle,    METH_NOARGS,   quaternion_rotation_angle_doc},
   {"rotate",         (PyCFunction)quaternion_rotate,     METH_VARARGS |
                                                          METH_KEYWORDS, quaternion_rotate_doc},
   {NULL, NULL, 0, NULL},  /* sentinel */
};

/* -----------------------------------------------------------------------------
 */
static PyMemberDef QuaternionMembers[] = {
   {"w",  T_DOUBLE, offsetof(PyQuaternionObject, qval.w), READONLY, "the scalar part of a Quaternion number"},
   {"x",  T_DOUBLE, offsetof(PyQuaternionObject, qval.x), READONLY, "the i imaginary part of a Quaternion number"},
   {"y",  T_DOUBLE, offsetof(PyQuaternionObject, qval.y), READONLY, "the j imaginary part of a Quaternion number"},
   {"z",  T_DOUBLE, offsetof(PyQuaternionObject, qval.z), READONLY, "the k imaginary part of a Quaternion number"},
   {NULL},  /* sentinel */
};

/* -----------------------------------------------------------------------------
 */
static PyNumberMethods QuaternionAsNumber = {
   (binaryfunc)quaternion_add,                 /* nb_add */
   (binaryfunc)quaternion_sub,                 /* nb_subtract */
   (binaryfunc)quaternion_mul,                 /* nb_multiply */
   (binaryfunc)quaternion_remainder,           /* nb_remainder */
   (binaryfunc)quaternion_divmod,              /* nb_divmod */
   (ternaryfunc)quaternion_pow,                /* nb_power */
   (unaryfunc)quaternion_neg,                  /* nb_negative */
   (unaryfunc)quaternion_pos,                  /* nb_positive */
   (unaryfunc)quaternion_abs,                  /* nb_absolute */
   (inquiry)quaternion_is_bool,                /* nb_bool */
   0,                                          /* nb_invert */
   0,                                          /* nb_lshift */
   0,                                          /* nb_rshift */
   0,                                          /* nb_and */
   0,                                          /* nb_xor */
   0,                                          /* nb_or */
   quaternion_int,                             /* nb_int */
   0,                                          /* nb_reserved */
   quaternion_float,                           /* nb_float */
   0,                                          /* nb_inplace_add */
   0,                                          /* nb_inplace_subtract */
   0,                                          /* nb_inplace_multiply*/
   0,                                          /* nb_inplace_remainder */
   0,                                          /* nb_inplace_power */
   0,                                          /* nb_inplace_lshift */
   0,                                          /* nb_inplace_rshift */
   0,                                          /* nb_inplace_and */
   0,                                          /* nb_inplace_xor */
   0,                                          /* nb_inplace_or */
   (binaryfunc)quaternion_int_div,             /* nb_floor_divide */
   (binaryfunc)quaternion_true_div,            /* nb_true_divide */
   0,                                          /* nb_inplace_floor_divide */
   0,                                          /* nb_inplace_true_divide */
   0,                                          /* nb_index */
   0,                                          /* nb_matrix_multiply @ */
   0,                                          /* nb_inplace_matrix_multiply */
};


// -----------------------------------------------------------------------------
//
PyDoc_STRVAR(
      quaternion_doc,
      "Quaternion ()                                     -> quaternion zero\n"
      "Quaternion (w[, x[, y[, z]]])                     -> quaternion number\n"
      "Quaternion (real=float[,imag=(float,float,float)])-> quaternion number\n"
      "Quaternion (angle=float,axis=(float,float,float)) -> quaternion rotation number\n"
      "Quaternion (number)                               -> quaternion number\n"
      "Quaternion (\"str representation\")                 -> quaternion number\n"
      "Quaternion (matrix=3x3 nested tuple of numerics)  -> quaternion number\n"
      "\n"
      "A Quaternion number may be constructed using one of the following forms:\n"
      "\n"
      "a) the real part and an optional imaginary parts. w, x, y and z must be float\n"
      "   or a number type which can be converted to float;\n"
      "\n"
      "b) from an real part and optional a 3-tuple imaginary part;\n"
      "\n"
      "c) from an angle (radians) and a 3-tuple axis of rotation (which is automatically\n"
      "   normalised), which generates a rotator Quaternion that can be used in\n"
      "   conjuction with the rotate mothod;\n"
      "\n"
      "d) from a single number parameter: int, float, complex or another Quaternion.\n"
      "   When the number is complex, the imaginary part of the complex is assigned\n"
      "   to the j imaginary part. So that Quaternion(z) == Quaternion(str(z)); or\n"
      "\n"
      "e) from the string representation of a quaternion (modelled on the complex type).\n"
      "\n"
      "f) from a 3x3 matrix of floats (and/or float-able objects). The matrix must be\n"
      "   a 3-tuple, each element itself being a 3-tuple of floats. The matrix should\n"
      "   ideally be a rotation matrix, i.e. the determinent should be 1, however no \n"
      "   attempt is made to check this nor is any attempt made to normalise the matrix.\n"
      "   The resulting quaternion may be normalised or reconstructed from the rotation\n"
      "   angle and axis.\n"
      "\n"
      "Attributes\n"
      "w       - float - real/scalar part\n"
      "x       - float - i imaginary part\n"
      "y       - float - j imaginary part\n"
      "z       - float - k imaginary part\n"
      "vector  - tuple - the tuple (x, y, z) \n"
      "complex - complex - the complex number (w + y.j)\n"
      "real    - float - real/scalar part\n"
      "imag    - tuple - the imaginary part, the same as vector.\n"
      "\n"
      );

static PyTypeObject QuaternionType = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "quaternion.Quaternion",                   /* tp_name */
   sizeof(PyQuaternionObject),                /* tp_basicsize */
   0,                                         /* tp_itemsize */
   0,                                         /* tp_dealloc */
   0,                                         /* tp_print */
   0,                                         /* tp_getattr */
   0,                                         /* tp_setattr */
   0,                                         /* tp_reserved */
   (reprfunc)quaternion_repr,                 /* tp_repr */
   &QuaternionAsNumber,                       /* tp_as_number */
   0,                                         /* tp_as_sequence */
   0,                                         /* tp_as_mapping */
   (hashfunc)quaternion_hash,                 /* tp_hash */
   0,                                         /* tp_call */
   (reprfunc)quaternion_str,                  /* tp_str */
   (getattrofunc)quaternion_getattro,         /* tp_getattro */
   0,                                         /* tp_setattro */
   0,                                         /* tp_as_buffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
   quaternion_doc,                            /* tp_doc */
   0,                                         /* tp_traverse */
   0,                                         /* tp_clear */
   quaternion_richcompare,                    /* tp_richcompare */
   0,                                         /* tp_weaklistoffset */
   0,                                         /* tp_iter */
   0,                                         /* tp_iternext */
   QuaternionMethods,                         /* tp_methods */
   QuaternionMembers,                         /* tp_members */
   0,                                         /* tp_getset */
   0,                                         /* tp_base */
   0,                                         /* tp_dict */
   0,                                         /* tp_descr_get */
   0,                                         /* tp_descr_set */
   0,                                         /* tp_dictoffset */
   0,                                         /* tp_init */
   (allocfunc)PyType_GenericAlloc,            /* tp_alloc */
   (newfunc)quaternion_new,                   /* tp_new */
   PyObject_Del                               /* tp_free */
};

/* Allow module defn code to access the Quaternion PyTypeObject.
 */
PyTypeObject* PyQuaternionType ()
{
   return &QuaternionType;
}


/* =============================================================================
 * Public utility functions - need access to QuaternionType
 * -----------------------------------------------------------------------------
 * cf with PyComplex_Check (op), PyLong_Check (op), PyUnicode_Check(op) etc.
 */

/* -----------------------------------------------------------------------------
 */
bool PyQuaternion_Check (PyObject *op)
{
   return PyObject_TypeCheck(op, &QuaternionType);
}

/* -----------------------------------------------------------------------------
 */
bool PyQuaternion_CheckExact (PyObject *op)
{
   return (Py_TYPE(op) == &QuaternionType);
}

/* -----------------------------------------------------------------------------
 * Extract C quaternion from Python Quaternion.
 * Need better error handling - have a look at PyFloat_AsDouble PyComplex_AsCComplex
 */
Py_quaternion PyQuaternion_AsCQuaternion(PyObject *obj)
{
   Py_quaternion r = { 0.0, 0.0, 0.0, 0.0 };  // better zero than random garbage
   if (PyQuaternion_Check(obj)) {
      r = ((PyQuaternionObject *)(obj))->qval;
   }
   return r;
}

/* -----------------------------------------------------------------------------
 * Returns a reference to a PyObject set to qval
 */
static PyObject *
quaternion_subtype_from_c_quaternion(PyTypeObject *type, Py_quaternion qval)
{
    PyObject *op;

    op = type->tp_alloc(type, 0);
    if (op != NULL)
        ((PyQuaternionObject *)op)->qval = qval;
    return op;
}

/* -----------------------------------------------------------------------------
 * Returns a reference to a PyObject set to qval
 */
PyObject *
PyQuaternion_FromCQuaternion(const Py_quaternion qval)
{
   PyQuaternionObject *op;
   op = (PyQuaternionObject *) PyObject_MALLOC(sizeof(PyQuaternionObject));  /* Inline PyObject_New */
   if (op == NULL)
      return PyErr_NoMemory();

   (void)PyObject_INIT(op, &QuaternionType);   /* sets ref count to 1 plus stuff */

   op->qval = qval;
   return (PyObject *) op;
}

/* -----------------------------------------------------------------------------
 * Returns the object convert to a PyQuaternionObject
 */
PyObject * PyObject_AsQuaternion(PyObject *obj)
{
   Py_quaternion result = { 0.0, 0.0, 0.0, 0.0 };
   bool status;

   if (!obj) return NULL;

   /* Is it already a PyQuaternionObject - short circuit
    */
   if (PyQuaternion_Check(obj)) {
      Py_INCREF(obj);
      return obj;
   }

   status = PyObject_AsCQuaternion (obj, &result);
   if (status) {
      return PyQuaternion_FromCQuaternion (result);
   }

   return NULL;
}

/* -----------------------------------------------------------------------------
 */
bool PyObject_AsCQuaternion(PyObject *obj, Py_quaternion* qval)
{
   if (!obj) return false;

   /* Is it already a PyQuaternionObject - just extract the value
    */
   if (PyQuaternion_Check(obj)) {
      *qval = ((PyQuaternionObject *)(obj))->qval;
      return true;
   }

   /* Check other numeric types - excluding unicode for now
    */
   if (PyLong_Check(obj)) {
      double t = PyLong_AsDouble(obj);
      if (t == -1.0 && PyErr_Occurred()) {
         return false;
      }
      qval->w = t;
      qval->x = 0.0;
      qval->y = 0.0;
      qval->z = 0.0;
      return true;
   }

   if (PyFloat_Check(obj)) {
      qval->w = PyFloat_AsDouble(obj);
      qval->x = 0.0;
      qval->y = 0.0;
      qval->z = 0.0;
      return true;
   }

   if (PyComplex_Check(obj)) {
      Py_complex z = PyComplex_AsCComplex (obj);
      qval->w = z.real;
      qval->x = 0.0;
      qval->y = z.imag;
      qval->z = 0.0;
      return true;
   }

   return false;
}

// end
