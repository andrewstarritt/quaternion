/* quaternian_utilities.c
 *
 * This file is part of the Python quaternion module. It provides a number of
 * mathematic Quaternian functions. Where such functins are defined, they aim
 * to mimic the equivalent functions out of the math/cmath module.
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

#include "quaternion_utilities.h"


/* ----------------------------------------------------------------------------
 */
bool
PyQuaternionUtil_NumberAsDouble(PyObject *obj, double* value)
{
   bool result = true;

   PyObject *float_obj = NULL;
   /* Return as a PyFloatObject or NULL
    */
   float_obj = PyNumber_Float (obj);

   if (float_obj != NULL) {
      /* Extract C double from Python float.
      */
      *value = PyFloat_AsDouble (float_obj);
      result = true;
   } else {
      result = false;
   }
   return result;
}



/* ----------------------------------------------------------------------------
 */
bool
PyQuaternionUtil_ParseIter(PyObject *obj,
                           double *target,
                           const int ndims,
                           const int *dimSizes,
                           const char* fname, const char* aname)
{
   bool result = true;
   PyObject *iter = NULL;
   PyObject *item = NULL;
   int count = 0;

   if (obj == NULL) {
      PyErr_Format(PyExc_TypeError,
                   "%.200s (%.200s) missing/null parameter",
                   fname, aname);
      return false;
   }

   if (ndims <= 0) {
      PyErr_Format(PyExc_RuntimeError,
                   "%.200s (%.200s): negative dimensions (%d) not allowed",
                   fname, aname, ndims);
   }

   iter = PyObject_GetIter(obj);
   if (!iter) {
      PyErr_Format(PyExc_TypeError,
                   "%.200s (%.200s): argument must be iterator (list, tuple, etc.) not '%.200s'",
                   fname, aname, Py_TYPE(obj)->tp_name);
      return false;
   }

   count = 0;
   for (item = PyIter_Next(iter); item != NULL; item = PyIter_Next(iter)) {
      count++;
      if (count > dimSizes[0]) {
         result = false;
         break;
      }

      if (ndims == 1) {
         bool status;
         double value;
         status = PyQuaternionUtil_NumberAsDouble (item, &value);
         if (!status) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s(%.200s) argument must be float/floatable, not '%.200s'",
                         fname, aname, Py_TYPE(item)->tp_name);
            return false;
         }
         target [count - 1] = value;

      } else {
         /* Must be 2 dimensional or higher.
          * Just go recursive to handle this case.
          *
          * First work out the item size as in number of doubles, so that we
          * can calculate the target index.
          */
         int j;
         int itemSize = 1;
         for (j = 1; j < ndims ; j++) {
            itemSize *= dimSizes[j];
         }

         int index = (count - 1)*itemSize;

         bool status;
         status = PyQuaternionUtil_ParseIter (item, &target[index], ndims - 1,
                                              &dimSizes [1], fname, aname);
         if (!status) return false;  /* error already reported */
      }
   }

   if (count != dimSizes[0]) {
      result = false;
   }

   if (!result) {
      PyErr_Format(PyExc_TypeError,
                   "%.200s (%.200s): incorrect number of elements: %d, expect: %d",
                   fname, aname, count, dimSizes[0]);
   }

   return result;
}

/* ----------------------------------------------------------------------------
 *
 */
bool
PyQuaternionUtil_ParseTriple(PyObject *obj, Py_quat_triple* triple,
                             const char* fname, const char* aname)
{
   double xyz [3];
   int dimSizes [1] = { 3 };
   bool status;

   status = PyQuaternionUtil_ParseIter (obj, xyz, 1, dimSizes,
                                        fname, aname);
   if (status) {
      triple->x = xyz[0];
      triple->y = xyz[1];
      triple->z = xyz[2];
   }

   return status;
}


/* end */
