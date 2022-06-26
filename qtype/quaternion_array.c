/* quaternion_array.c
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

#include "quaternion_array.h"
#include "quaternion_basic.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


/* -----------------------------------------------------------------------------
 * Macro to check that the allocated memory is sensible.
 * Mote: we always allocate some memory.
 */
#define SANITY_CHECK(pObj, errReturn) {                                       \
   if (!pObj->aval.qvalArray ||                                               \
       (pObj->aval.count > pObj->aval.allocated)) {                           \
       PyErr_SetString(PyExc_MemoryError,                                     \
                       "quaternion array corrupted");                         \
       return errReturn;                                                      \
   }                                                                          \
}


/* -----------------------------------------------------------------------------
 * Utility fuctions
 * -----------------------------------------------------------------------------
 */

/* -----------------------------------------------------------------------------
 * Returns a reference to a PyObject (PyQuaternionArray or subtype) set to aval
 */
static PyObject *
quaternion_array_subtype_from_c_quaternion_array(PyTypeObject *type,
                                                 const Py_quaternion_array aval)
{
   PyObject *op;

   op = type->tp_alloc(type, 0);  /* PyType_GenericAlloc, at least for PyQuaternionArray */
   if (op != NULL)
       ((PyQuaternionArrayObject *)op)->aval = aval;
   return op;
}

/* -----------------------------------------------------------------------------
 * Returns a reference to a PyObject (PyQuaternionArray) set to aval
 */
static PyObject *
quaternion_array_type_from_c_quaternion_array(const Py_quaternion_array aval)
{
   PyTypeObject *type;
   PyObject *op;

   type = PyQuaternionArrayType();
   op = quaternion_array_subtype_from_c_quaternion_array(type, aval);
   return op;
}

/* -----------------------------------------------------------------------------
 * When we do an initial allocation or need to do a reallocation, the
 * calculated size adds a bit of wiggle room to the minimum size.
 * Note: result is >= minimum_size
 */
static Py_ssize_t
qa_next_allocated_size(const Py_ssize_t minimum_size)
{
   Py_ssize_t result;

   result = (minimum_size * 11) / 10;  /* +10% */
   if (result < minimum_size + 10) {
       result = minimum_size + 10;     /* +10  */
   }

   return result;
}

/* -----------------------------------------------------------------------------
 * Allocate/Reallocate value array buffer.
 * If exact false then then a bit of wiggle room is added to the re allocation (~10%).
 * If exact true then then no wiggle room is added to the re allocation.
 */
static void
qa_reallocate (Py_quaternion_array *aval, const Py_ssize_t new_size, const bool exact)
{
   /* Re allocate data
    */
   if (exact) {
      aval->allocated = new_size;
   } else {
      aval->allocated = qa_next_allocated_size(new_size);
   }

   if (!aval->qvalArray) {
      aval->qvalArray = PyMem_Realloc(aval->qvalArray,
                                      aval->allocated * sizeof(Py_quaternion));
   } else {
      aval->qvalArray = PyMem_Malloc(aval->allocated * sizeof(Py_quaternion));
   }
}

/* -----------------------------------------------------------------------------
 * Validate and return the count of the number of items in the initializer.
 * Return -1 if any problem, such as non PyQuaternionObject-like objects, or
 * the initializer is not iterable.
 */
static int
qa_iterator_size(PyObject *initializer)
{
   PyObject *iter = NULL;
   PyQuaternionObject *pQuat = NULL;
   PyObject *item = NULL;
   int count;

   if (!initializer) {  /* sanity check */
      PyErr_SetString(PyExc_RuntimeError, "quaternion array program error.");
      return -1;
   }

   if (PyQuaternionArray_Check(initializer)) {
      /* The initializer itself is a QuaternionArray object.
       * We can short circuit a lot of vanilla logic.
       */
      PyQuaternionArrayObject* pObj;
      pObj = (PyQuaternionArrayObject *)initializer;
      SANITY_CHECK(pObj, -1);

      return pObj->aval.count;
   }

   iter = PyObject_GetIter(initializer);
   if (!iter) {
      PyErr_Format(PyExc_TypeError, "initializer is not iterable (type: '%s')",
                   Py_TYPE(initializer)->tp_name);
      return -1;
   }


   /* Iterate once to get a count of the number of items, and  also ensure all
    * the items are Quaternion or are castable to Quaternion i.e. int, float and
    * complex types.
    */
   count = 0;
   for (item = PyIter_Next(iter); item != NULL; item = PyIter_Next(iter)) {
      /* Caste to a PyQuaternionObject otherwise retuns NULL
       */
      pQuat = (PyQuaternionObject*) PyObject_AsQuaternion(item);
      if (!pQuat) {
         PyErr_Format(PyExc_TypeError,
                      "Quaternion argument expected (got type %s)",
                      Py_TYPE(item)->tp_name);
         return -1;
      }
      Py_DECREF (pQuat);
      count++;

   }
   return count;
}

/* -----------------------------------------------------------------------------
 * NOTE: this should only be called if qa_iterator_size() returns >= 0.
 * This function, while it does some basic checking, does no reporting.
 */
static void
qa_extract_and_add(PyObject *initializer,
                   Py_quaternion_array *aval)
{
   PyObject *iter = NULL;
   PyQuaternionObject *pQuat = NULL;
   PyObject *item = NULL;

   if (!initializer) return;  /* sanity check */
   if (!aval)        return;  /* sanity check */


   if (PyQuaternionArray_Check(initializer)) {
      /* The initializer itself is a QuaternionArray object.
       * We can short circuit a lot of vanilla logic.
       */
      PyQuaternionArrayObject* pObj;
      Py_ssize_t number;

      pObj = (PyQuaternionArrayObject *)initializer;
      number = pObj->aval.count;

      /* This works even when extending self.
       */
      memcpy(&aval->qvalArray [aval->count],
             &pObj->aval.qvalArray [0],
             number * sizeof (Py_quaternion));

      aval->count += number;
      return;
   }

   iter = PyObject_GetIter(initializer);
   if (!iter) return;  /* belts 'n' braces */


   for (item = PyIter_Next(iter); item != NULL; item = PyIter_Next(iter)) {
      /* Caste to a PyQuaternionObject otherwise retuns NULL
       */
      pQuat = (PyQuaternionObject*) PyObject_AsQuaternion(item);
      if (!pQuat) return;  /* belts 'n' braces */

      aval->qvalArray [aval->count++] = pQuat->qval;
      Py_DECREF (pQuat);
   }
}

/* -----------------------------------------------------------------------------
 * This is a wrapper around PySlice_AdjustIndices which does all the hard work.
 * The slice parameter must refer to a slice object.
 */
static bool
qa_decode_slice (const Py_quaternion_array aval, PyObject *slice,
                 Py_ssize_t *start, Py_ssize_t *stop,
                 Py_ssize_t *step, Py_ssize_t *count)
{
   if (!slice) return false;  /* sanity check */

   /* First unpack the slice.
    */
   PySlice_Unpack(slice, start, stop, step);

   if (*step == 0) {
      PyErr_SetString (PyExc_ValueError, "slice step cannot be zero");
      return false;
   }

   /* Next adjust taking into account the actual array length.
    */
   *count = PySlice_AdjustIndices (aval.count, start, stop, *step);
   return true;
}

/* -----------------------------------------------------------------------------
 */
static PyObject *
quaternion_array_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
   static char *kwlist[] = {"initializer", "reserve", 0};

   PyObject *result = NULL;
   PyObject *initializer = NULL;
   PyObject *reserve = NULL;
   Py_quaternion_array aval;
   Py_ssize_t initialNumber;

   if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO:QuaternionArray", kwlist,
                                    &initializer, &reserve)) {
      return NULL;
   }

   aval.iter_index = -1;   /* not iterating */
   aval.count = 0;         /* empty for now */

   if (initializer) {
      Py_ssize_t count;

      count = qa_iterator_size (initializer);
      if (count < 0) {
         /* qa_iterator_size has already called PyErr_SetString/PyErr_Format.
          */
         return NULL;
      }

      initialNumber = qa_next_allocated_size (count);
   } else {
      /* No initializer
       */
      initialNumber = qa_next_allocated_size (0);
   }

   if (reserve) {
      if (!PyLong_Check(reserve)) {
         PyErr_Format(PyExc_TypeError,
                      "array sizes must be integers (got type %s)",
                      Py_TYPE(reserve)->tp_name);
         return NULL;
      }

      Py_ssize_t minimumSize = PyLong_AsSsize_t(reserve);

      if (initialNumber < minimumSize) {
          initialNumber = minimumSize;
      }
   }

   /* Allocate the storage
    */
   aval.qvalArray = NULL;
   qa_reallocate (&aval, initialNumber, true);

   /* Now re-iterate to extact the required values.
    */
   if (initializer) {
      qa_extract_and_add (initializer, &aval);
   }

   result = quaternion_array_subtype_from_c_quaternion_array(type, aval);
   return result;
}

/* -----------------------------------------------------------------------------
 */
static void
quaternion_array_decalloc(PyObject* self)
{
   PyQuaternionArrayObject* pObj;
   pObj = (PyQuaternionArrayObject *)self;

   if (pObj->aval.qvalArray) {
      PyMem_Free(pObj->aval.qvalArray);
      pObj->aval.qvalArray = NULL;
   }
}

/* -----------------------------------------------------------------------------
 * tp_str
 */
static PyObject *
quaternion_array_str(PyObject* self)
{
   /** Static should be okay until we get the GILectomy ;-)
    **/
   static char image [80000];
   static const int maxImageSize = sizeof(image);

   PyQuaternionArrayObject* pObj;
   Py_ssize_t index;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   image[0] = '\0';
   strcat(image, "[");
   for (index = 0; index < pObj->aval.count; index++) {
      Py_quaternion qval = pObj->aval.qvalArray [index];
      char *qstr;
      qstr = _Py_quat_to_string(qval, 'r', 0);

      if(!qstr) {
         return PyErr_NoMemory();
      }

      strcat(image, qstr);
      PyMem_Free(qstr); /* Must free this */

      if (index < pObj->aval.count - 1) {
         /* Not last item, add a separator */
         strcat(image, ", ");

         if (strnlen(image, maxImageSize) > maxImageSize - 200) {
            /* Image buffer nearly full - stop here.
             */
            strcat(image, " ... ");
            break;
         }
      }
   }

   strcat(image, "]");

   return PyUnicode_FromString(image);
}

/* -----------------------------------------------------------------------------
 * tp_repr - use tp_str function for now.
 */
static PyObject *
quaternion_array_repr(PyObject* self)
{
   return quaternion_array_str(self);
}

/* -----------------------------------------------------------------------------
 * tp_hash
 * We have to do this explicitly - we can't just leave tp_hash null.
 */
static Py_hash_t
quaternion_array_hash(PyObject* self)
{
   PyErr_Format(PyExc_TypeError, "unhashable type: '%s'", Py_TYPE(self)->tp_name);
   return -1;
}

/* -----------------------------------------------------------------------------
 * tp_richcompare
 */
static PyObject *
quaternion_array_richcompare (PyObject *left, PyObject *right, int op)
{
   PyObject *result;
   PyQuaternionArrayObject* pObjL;
   PyQuaternionArrayObject* pObjR;
   int equal;

   if (!PyQuaternionArray_Check(left)) {
      PyErr_Format(PyExc_TypeError,
                   "a QuaternionArray argument is required (got type %s)",
                   Py_TYPE(left)->tp_name);
      return NULL;
   }

   pObjL = (PyQuaternionArrayObject *)left;
   SANITY_CHECK(pObjL, NULL);

   if (!PyQuaternionArray_Check(right)) {
      PyErr_Format(PyExc_TypeError,
                   "a QuaternionArray argument is required (got type %s)",
                   Py_TYPE(right)->tp_name);
      return NULL;
   }

   pObjR = (PyQuaternionArrayObject *)right;
   SANITY_CHECK(pObjR, NULL);

   /* It is an array of quaternions. Quaternions only support == and /=, as
    * it make so sense to test q1 < q2. The same applies to arrays.
    */
   if (op != Py_EQ && op != Py_NE) {
      Py_RETURN_NOTIMPLEMENTED;
   }

   equal = pObjL->aval.count == pObjR->aval.count;

   /* Only if lengths are equal to we proceed to compare the values.
    */
   if (equal) {
      /* Must compare term by term.
       * memcmp just won't do - it can't cope with +0.0 == -0.0
       */
      Py_ssize_t k;
      for (k = 0; k < pObjL->aval.count; k++) {
         equal = _Py_quat_eq(pObjL->aval.qvalArray[k], pObjR->aval.qvalArray[k]);
         if (!equal) break;
      }
   }

   if (equal == (op == Py_EQ))
      result = Py_True;
   else
      result = Py_False;

   Py_INCREF(result);

   return result;
}

/* -----------------------------------------------------------------------------
 * sq_concat - invoked by "+" operator - in lieu of nb_add.
 */
static PyObject *
quaternion_array_concat(PyObject *self, PyObject *arg)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyQuaternionArrayObject* pArg;
   Py_quaternion_array aval;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   if (!PyQuaternionArray_Check(arg)) {
      PyErr_Format(PyExc_TypeError,
                   "a QuaternionArray argument is required (got type %s)",
                   Py_TYPE(arg)->tp_name);
      return NULL;
   }

   pArg = (PyQuaternionArrayObject *)arg;
   SANITY_CHECK(pArg, NULL);

   aval.iter_index = -1;   /* not iterating */
   aval.count = pObj->aval.count + pArg->aval.count;
   aval.qvalArray = NULL;
   qa_reallocate(&aval, aval.count, false);

   /* Now copy data
    * void *memcpy(void *dest, const void *src, size_t n);
    */
   memcpy(&aval.qvalArray[0],
          &pObj->aval.qvalArray[0],
          pObj->aval.count*sizeof(Py_quaternion));

   memcpy(&aval.qvalArray[pObj->aval.count],
          &pArg->aval.qvalArray[0],
          pArg->aval.count*sizeof(Py_quaternion));

   result = quaternion_array_type_from_c_quaternion_array(aval);
   return result;
}

/* -----------------------------------------------------------------------------
 * sq_repeat
 */
static PyObject *
quaternion_array_repeat(PyObject *self, Py_ssize_t repeat)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   Py_quaternion_array aval;
   Py_ssize_t k;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   aval.iter_index = -1;   /* not iterating */
   aval.count = pObj->aval.count * repeat;
   aval.qvalArray = NULL;
   qa_reallocate(&aval, aval.count, false);

   /* Now copy data repeat times
    * void *memcpy(void *dest, const void *src, size_t n);
    */
   for (k = 0; k < repeat; k++) {
      memcpy(&aval.qvalArray[k*pObj->aval.count],
             &pObj->aval.qvalArray[0],
             pObj->aval.count*sizeof(Py_quaternion));
   }

   result = quaternion_array_type_from_c_quaternion_array(aval);
   return result;
}

/* -----------------------------------------------------------------------------
 * sq_item
 */
static PyObject *
quaternion_array_item(PyObject *self, Py_ssize_t index)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   Py_quaternion qval;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* negative means relative to end counting backward.
    */
   if (index < 0) index = pObj->aval.count + index;

   if ((index < 0) || (index >= pObj->aval.count)) {
      PyErr_SetString(PyExc_IndexError,
                      "array index out of range");
      return NULL;
   }

   qval = pObj->aval.qvalArray [index];
   result = PyQuaternion_FromCQuaternion(qval);

   return result;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_append_doc,
             "append(self, q, /)\n"
             "Append quaternion q to the end of the array.");

static PyObject *
quaternion_array_append(PyObject *self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject* value;
   PyQuaternionObject* pQuat;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the value
    * Alas no Q option, so go via a vanilla object.
    */
   if (!PyArg_ParseTuple(args, "O:append", &value))
      return NULL;

   pQuat = (PyQuaternionObject*) PyObject_AsQuaternion(value);
   if (!pQuat) {
      PyErr_Format(PyExc_TypeError,
                   "a Quaternion argument is required (got type %s)",
                   Py_TYPE(value)->tp_name);
      return NULL;
   }

   /* Do we have enough room?
    */
   if (pObj->aval.count + 1 > pObj->aval.allocated) {
      /* Re allocate data
       */
      qa_reallocate(&pObj->aval, pObj->aval.count + 1, false);
   }

   pObj->aval.qvalArray[pObj->aval.count++] = pQuat->qval;

   result = Py_None;
   Py_INCREF(result);
   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_insert_doc,
             "insert(self, i, q, /)\n"
             "Insert quaternion q into the array before position i.");

static PyObject *
quaternion_array_insert(PyObject *self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject* indexObj;
   Py_ssize_t index;
   PyObject* valueObj;
   PyQuaternionObject* pQuatObj;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the index and value
    * Alas no Q option, so go via a vanilla object.
    */
   if (!PyArg_ParseTuple(args, "OO:insert", &indexObj, &valueObj))
      return NULL;

   if (!PyLong_Check(indexObj)) {
      PyErr_Format(PyExc_TypeError,
                   "array indices must be integers (got type %s)",
                   Py_TYPE(indexObj)->tp_name);
      return NULL;
   }

   /* PyObject_AsQuaternion converts ints, floats, complex and quaternions
    * into a quaternion or returns NULL.
    */
   pQuatObj = (PyQuaternionObject*) PyObject_AsQuaternion(valueObj);
   if (!pQuatObj) {
      PyErr_Format(PyExc_TypeError,
                   "a Quaternion argument is required (got type %s)",
                   Py_TYPE(valueObj)->tp_name);
      return NULL;
   }

   index = PyLong_AsSsize_t(indexObj);

   /* relative to end counting backward.
    */
   if (index < 0) index = pObj->aval.count + index;

   /* for insert we can be a more flexible re meaning of the index.
    */
   if (index < 0) index = 0;
   if (index > pObj->aval.count) index = pObj->aval.count;

   /* Do we have enough room?
    */
   if (pObj->aval.count + 1 > pObj->aval.allocated) {
      /* Re allocate data
       */
      qa_reallocate(&pObj->aval, pObj->aval.count + 1, false);
   }

   if (index >= pObj->aval.count) {
      /* This is essentially just an append.
        */
      pObj->aval.qvalArray[pObj->aval.count++] = pQuatObj->qval;
   } else {
      /* This is an actual insert - shuffle up the data.
       */
      int numberToMove = pObj->aval.count - index;

      /* memmove (*dest, *src, size)  - not can't use memcpy.
       */
      if (numberToMove > 0) {
         memmove(&pObj->aval.qvalArray [index+1],
                 &pObj->aval.qvalArray [index],
                 numberToMove * sizeof(Py_quaternion));
      }
      pObj->aval.qvalArray[index] = pQuatObj->qval;
      pObj->aval.count++;
   }

   result = Py_None;
   Py_INCREF(result);
   return result;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_extend_doc,
             "extend(self, iter, /)\n"
             "Append quaternions to the end of the array.");

static PyObject *
quaternion_array_extend(PyObject *self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject *initializer = NULL;
   Py_ssize_t additional;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the initializer from the one element tuple
    */
   if (!PyArg_ParseTuple(args, "O:extend", &initializer))
      return NULL;

   if (!initializer) return NULL;  /* sanity check */

   additional = qa_iterator_size(initializer);
   if (additional < 0) {
      /* qa_iterator_size has already called PyErr_SetString/PyErr_Format.
       */
      return NULL;
   }

   /* Do we have enough room?
    */
   if (pObj->aval.count + additional > pObj->aval.allocated) {
      /* Re allocate data
       */
      qa_reallocate(&pObj->aval, pObj->aval.count + additional, false);
   }

   qa_extract_and_add(initializer, &pObj->aval);

   result = Py_None;
   Py_INCREF(result);
   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_frombytes_doc,
             "frombytes(self, buffer, /)\n"
             "Appends quaternions from the string, interpreting it as an array of machine values.");

static PyObject *
quaternion_array_frombytes(PyObject* self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject *dataObj = NULL;
   int bytesStatus;
   int byteArrayStatus;
   Py_ssize_t data_size = 0;
   char* data = NULL;
   Py_ssize_t additional;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the bytes data object from the one element tuple.
    */
   if (!PyArg_ParseTuple(args, "O:frombytes", &dataObj))
      return NULL;

   /* We allow either bytes or bytearray parameter.
    */
   bytesStatus = PyBytes_Check(dataObj);
   byteArrayStatus = PyByteArray_Check(dataObj);

   if (!bytesStatus && !byteArrayStatus) {
      PyErr_Format(PyExc_TypeError,
                   "a bytes-like object is required (got type %s)",
                   Py_TYPE(dataObj)->tp_name);
      return NULL;
   }

   /* Extract size and data ref.
    */
   if (bytesStatus) {
      data_size = PyBytes_Size (dataObj);
      data = PyBytes_AsString (dataObj);

   } else if (byteArrayStatus) {
      data_size = PyByteArray_Size (dataObj);
      data = PyByteArray_AsString (dataObj);

   } else {
      PyErr_Format(PyExc_TypeError,
                   "Unexpected type (got type %s)",
                   Py_TYPE(dataObj)->tp_name);
      return NULL;
   }

   if ( (data_size < 0) || ( (data_size % sizeof(Py_quaternion)) != 0) ) {
      PyErr_Format(PyExc_ValueError,
                   "bytes length %d not a multiple of quaternion size %d",
                   data_size, sizeof(Py_quaternion));
      return NULL;
   }

   /* How many quaternios are being added?
    */
   additional = data_size / sizeof(Py_quaternion);

   /* Do we have enough room?
    */
   if (pObj->aval.count + additional > pObj->aval.allocated) {
      /* Re allocate data
       */
      qa_reallocate(&pObj->aval, pObj->aval.count + additional, false);
   }

   /* Move the data
    */
   memcpy(&pObj->aval.qvalArray[pObj->aval.count], data, data_size);
   pObj->aval.count += additional;

   result = Py_None;
   Py_INCREF(result);
   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_info_doc,
             "buffer_info(self, /)\n"
             "Return a tuple (address, length) giving the current memory address and the length\n"
             "in quaternions of the buffer used to hold array's contents.\n"
             "\n"
             "The length should be multiplied by the itemsize attribute to calculate the\n"
             "buffer length in bytes.");

static PyObject *
quaternion_array_info(PyObject* self)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   result = Py_BuildValue("(ll)", (Py_ssize_t) pObj->aval.qvalArray,
                          pObj->aval.count);

   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_clear_doc,
             "clear(self, /)\n"
             "Remove all items from the array.");

static PyObject *
quaternion_array_clear(PyObject* self)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   pObj->aval.count = 0;
   if (pObj->aval.allocated > 10) {
      qa_reallocate(&pObj->aval, 10, true);
   }

   result = Py_None;
   Py_INCREF(result);
   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_count_doc,
             "count(self, q, /)\n"
             "Return number of occurrences of q in the array.");

static PyObject *
quaternion_array_count(PyObject* self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject *valueObj;
   PyQuaternionObject* pQuatObj;
   Py_ssize_t count;
   Py_ssize_t k;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the value
    */
   if (!PyArg_ParseTuple(args, "O:count", &valueObj))
      return NULL;

   /* PyObject_AsQuaternion converts ints, floats, complex and quaternions
    * into a quaternion or returns NULL.
    */
   pQuatObj = (PyQuaternionObject*) PyObject_AsQuaternion(valueObj);
   if (!pQuatObj) {
      PyErr_Format(PyExc_TypeError,
                   "a Quaternion argument is required (got type %s)",
                   Py_TYPE(valueObj)->tp_name);
      return NULL;
   }

   count = 0;
   for (k = 0; k < pObj->aval.count; k++) {
      if (_Py_quat_eq (pQuatObj->qval, pObj->aval.qvalArray[k])) {
         count++;
      }
   }

   result = PyLong_FromLong(count);
   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_index_doc,
             "index(self, q, /)\n"
             "Return index of first occurrence of q in the array.");

static PyObject *
quaternion_array_index(PyObject* self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject *valueObj;
   PyQuaternionObject* pQuatObj;
   Py_ssize_t index;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the value
    */
   if (!PyArg_ParseTuple(args, "O:index", &valueObj))
      return NULL;

   /* PyObject_AsQuaternion converts ints, floats, complex and quaternions
    * into a quaternion or returns NULL.
    */
   pQuatObj = (PyQuaternionObject*) PyObject_AsQuaternion(valueObj);
   if (!pQuatObj) {
      PyErr_Format(PyExc_TypeError,
                   "a Quaternion argument is required (got type %s)",
                   Py_TYPE(valueObj)->tp_name);
      return NULL;
   }

   for (index = 0; index < pObj->aval.count; index++) {
      if (_Py_quat_eq (pQuatObj->qval, pObj->aval.qvalArray[index])) {
         // Found it.
         //
         result = PyLong_FromLong(index);
         break;
      }
   }

   if (!result) {
      PyErr_SetString(PyExc_ValueError, "array.index(q): q not in array.");

   }

   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_pop_doc,
             "pop(self, i=-1, /)\n"
             "Return the i-th element and delete it from the array.\n"
             "i defaults to -1.");

static PyObject *
quaternion_array_pop(PyObject* self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject* indexObj = NULL;
   Py_ssize_t index;
   Py_quaternion qval;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the optional index
    */
   if (!PyArg_ParseTuple(args, "|O:pop", &indexObj))
      return NULL;

   if (indexObj) {
      /* an index argument has been provided.
       */
      if (!PyLong_Check(indexObj)) {
         PyErr_Format(PyExc_TypeError,
                      "array indices must be integers (got type %s)",
                      Py_TYPE(indexObj)->tp_name);
         return NULL;
      }
      index = PyLong_AsSsize_t(indexObj);
   } else {
      index = -1;  /* default */
   }

   /* negative means relative to end counting backward.
    */
   if (index < 0) index = pObj->aval.count + index;

   if ((index < 0) || (index >= pObj->aval.count)) {
      PyErr_SetString(PyExc_IndexError,
                      "pop index out of range");
      return NULL;
   }

   /* Create popped quaternion as an object.
    */
   qval  = pObj->aval.qvalArray [index];
   result = PyQuaternion_FromCQuaternion(qval);

   /* Shuffle data down toward zero-th index.
    * memmove (void *__dest, const void *__src, size_t __n)
    */
   int numberToMove = pObj->aval.count - index - 1;
   if (numberToMove > 0) {
      memmove(&pObj->aval.qvalArray [index],
              &pObj->aval.qvalArray [index+1],
              numberToMove * sizeof(Py_quaternion));
   }
   pObj->aval.count--;

   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_remove_doc,
             "remove(self, q, /)\n"
             "Remove the first occurrence of v in the array.");

static PyObject *
quaternion_array_remove(PyObject* self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject *valueObj;
   PyQuaternionObject* pQuatObj;
   Py_ssize_t index;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the value
    */
   if (!PyArg_ParseTuple(args, "O:remove", &valueObj))
      return NULL;

   /* PyObject_AsQuaternion converts ints, floats, complex and quaternions
    * into a quaternion or returns NULL.
    */
   pQuatObj = (PyQuaternionObject*) PyObject_AsQuaternion(valueObj);
   if (!pQuatObj) {
      PyErr_Format(PyExc_TypeError,
                   "a Quaternion argument is required (got type %s)",
                   Py_TYPE(valueObj)->tp_name);
      return NULL;
   }

   for (index = 0; index < pObj->aval.count; index++) {
      if (_Py_quat_eq (pQuatObj->qval, pObj->aval.qvalArray[index])) {
         /* Found it
          *
          * Shuffle data down toward zero-th index.
          * memmove (void *__dest, const void *__src, size_t __n)
          */
         int numberToMove = pObj->aval.count - index - 1;
         if (numberToMove > 0) {
            memmove(&pObj->aval.qvalArray [index],
                    &pObj->aval.qvalArray [index+1],
                    numberToMove * sizeof(Py_quaternion));
         }
         pObj->aval.count--;

         result = Py_None;
         Py_INCREF(result);
         break;
      }
   }

   if (!result) {
      PyErr_SetString(PyExc_ValueError, "array.remove(q): q not in array.");

   }

   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_reserve_doc,
             "reserve(self, n, /)\n"
             "reserve (pre-allocate) space for at least n items in the array.");

static PyObject *
quaternion_array_reserve(PyObject *self, PyObject *args)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject* sizeObj;
   Py_ssize_t bufferSize;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the minimum required size.
    */
   if (!PyArg_ParseTuple(args, "O:reserve", &sizeObj))
      return NULL;

   if (!PyLong_Check(sizeObj)) {
      PyErr_Format(PyExc_TypeError,
                   "array sizes must be integers (got type %s)",
                   Py_TYPE(sizeObj)->tp_name);
      return NULL;
   }

   bufferSize = PyLong_AsSsize_t(sizeObj);

   /* Do we have enough room?
    */
   if (pObj->aval.allocated < bufferSize) {
      /* Re allocate data
       */
      qa_reallocate(&pObj->aval, bufferSize, true);
   }

   result = Py_None;
   Py_INCREF(result);
   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_reverse_doc,
             "Reverse the order of the quaternions in the array.");
static PyObject *
quaternion_array_reverse(PyObject* self)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   Py_ssize_t half;
   Py_ssize_t left;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   half = pObj->aval.count / 2;  /* round-down when is good */

   for (left = 0; left < half; left++) {
      Py_ssize_t right = pObj->aval.count - left - 1;
      Py_quaternion tempLeft;

      tempLeft = pObj->aval.qvalArray [left];
      pObj->aval.qvalArray [left] = pObj->aval.qvalArray [right];
      pObj->aval.qvalArray [right] = tempLeft;
   }

   result = Py_None;
   Py_INCREF(result);
   return result;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_tobytes_doc,
             "Convert the array to an array of machine values "
             "and return the bytes representation.");
static PyObject *
quaternion_array_tobytes(PyObject* self)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   result = PyBytes_FromStringAndSize((char *) pObj->aval.qvalArray,
                                      pObj->aval.count * sizeof(Py_quaternion));

   return result;
}


/* -----------------------------------------------------------------------------
 * tp_iter
 */
static PyObject *
quaternion_array_iter(PyObject* self)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   pObj->aval.iter_index = 0;

   /* We use self at the iterator object rather than creating a new one.
    * That may change in future.
    */
   result = self;
   Py_INCREF(self);

   return result;
}

/* -----------------------------------------------------------------------------
 * tp_iternext
 */
static PyObject *
quaternion_array_next(PyObject* self)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   Py_quaternion qval;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   if (pObj->aval.iter_index < 0) {
      PyErr_SetString(PyExc_AssertionError, "iteration not initialised.");
      result = NULL;

   } else if (pObj->aval.iter_index < pObj->aval.count) {
      qval  = pObj->aval.qvalArray [pObj->aval.iter_index];
      result = PyQuaternion_FromCQuaternion(qval);
      pObj->aval.iter_index++;

   } else {
      PyErr_SetString(PyExc_StopIteration, "No more items");
      pObj->aval.iter_index = -1;  /* clear the iteration */
      result = NULL;
   }

   return result;
}

/* -----------------------------------------------------------------------------
 * sq_length and mp_length
 */
static Py_ssize_t
quaternion_array_length(PyObject* self)
{
   PyQuaternionArrayObject* pObj;

   pObj = (PyQuaternionArrayObject *)self;
   return pObj->aval.count;
}

/* -----------------------------------------------------------------------------
 * __getitem__
 */
static PyObject *
quaternion_array_get_subscript(PyObject* self, PyObject* key)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   if (PyLong_Check(key)) {
      /* Caller has supplied a integer
       */
      Py_ssize_t index = PyLong_AsSsize_t(key);
      result = quaternion_array_item(self, index);

   } else if (PySlice_Check(key)) {
      /* Caller has supplied a slice
       */
      Py_quaternion_array aval;
      Py_ssize_t start;
      Py_ssize_t stop;
      Py_ssize_t step;
      bool status;
      Py_ssize_t count;   /* extracted */
      Py_ssize_t j;

      /* Deal with negative start and stop values and ensure still in range.
       */
      status = qa_decode_slice (pObj->aval, key, &start, &stop, &step, &count);
      if (!status) {
         /* qa_decode_slice has allready called PyErr_SetString
          */
         return NULL;
      }

      aval.iter_index = -1;
      aval.count = 0;
      aval.qvalArray = NULL;
      qa_reallocate(&aval, count, false);

      for (j = 0; j < count; j++) {
          Py_ssize_t index = start + j*step;
          if ((index >= 0) && (index < pObj->aval.count)) { /* sanity check */
             aval.qvalArray[aval.count++] = pObj->aval.qvalArray [index];
          } else {
             printf ("*** out of range j: %ld  index: %ld\n", j, index);
          }
      }

      result = quaternion_array_type_from_c_quaternion_array(aval);

   } else  {
      /* Caller has got it wrong.
       */
      PyErr_Format(PyExc_TypeError,
                   "array indices must be integers or slices (got type %s)",
                   Py_TYPE(key)->tp_name);
      result =  NULL;
   }


   return result;
}


/* -----------------------------------------------------------------------------
 * called by quaternion_array_set_subscript (__setitem__)
 */
static bool
qa_assign_slice(Py_quaternion_array *aval, PyObject* slice, PyObject* value)
{
   Py_ssize_t start;
   Py_ssize_t stop;
   Py_ssize_t step;
   Py_ssize_t number_replaced;
   bool status;
   Py_ssize_t number_assigned;

   /* Deal with negative start and stop values and ensure still in range.
    */
   status = qa_decode_slice (*aval, slice, &start, &stop, &step, &number_replaced);
   if (!status) {
      /* qa_decode_slice has already called PyErr_SetString/PyErr_Format.
       */
      return false;
   }

   number_assigned = qa_iterator_size(value);
   if (number_assigned < 0) {
      /* qa_iterator_size has already called PyErr_SetString/PyErr_Format.
       */
      return false;
   }

   if (step == 1) {
      /* basic slice assignment - sizes need not match
       * count is number replaced.
       */
      Py_ssize_t new_count;
      new_count = aval->count - number_replaced + number_assigned;

      if (new_count > aval->allocated) {
         qa_reallocate(aval, new_count, false);
      }

      /* First shuffle up/down the tail end - if needs be.
       * memmove (void *__dest, const void *__src, size_t __n)
       */
      if (number_assigned > number_replaced) {
         Py_ssize_t numberToMove = aval->count - stop;
         Py_ssize_t offset = number_assigned - number_replaced;
         if (numberToMove > 0) {
            memmove(&aval->qvalArray [stop + offset],
                    &aval->qvalArray [stop],
                    numberToMove * sizeof(Py_quaternion));
         }

      } else if (number_assigned < number_replaced) {
         Py_ssize_t numberToMove = aval->count - stop;
         Py_ssize_t offset = number_replaced - number_assigned;
         if (numberToMove > 0) {  /* number replaced */
            memmove(&aval->qvalArray [stop - offset],
                    &aval->qvalArray [stop],
                    numberToMove * sizeof(Py_quaternion));
         }

      } /* else  exact fit */

      /* Create a C quaternion_array from the value.
       */
      Py_quaternion_array assigned;
      assigned.count = 0;
      assigned.allocated = 0;
      assigned.qvalArray = NULL;
      qa_reallocate(&assigned, number_assigned, true);
      qa_extract_and_add(value, &assigned);

      memcpy(&aval->qvalArray [start],
             &assigned.qvalArray [0],
             number_assigned * sizeof(Py_quaternion));
      aval->count = new_count;

      PyMem_Free(assigned.qvalArray);  // done with this.


   } else {
      /* extended slice assignment - sizes must match
       */
      if (number_replaced != number_assigned) {
         PyErr_Format(PyExc_TypeError,
                      "array attempt to assign sequence of size %ld to extended slice of size %ld",
                      number_assigned, number_replaced);
         return false;
      }

      /* Create a C quaternion_array from the value.
       */
      Py_quaternion_array assigned;
      assigned.count = 0;
      assigned.allocated = 0;
      assigned.qvalArray = NULL;
      qa_reallocate(&assigned, number_assigned, true);
      qa_extract_and_add(value, &assigned);

      Py_ssize_t j;
      for (j = 0; j < number_assigned; j++) {
          Py_ssize_t index = start + j*step;
          if ((index >= 0) && (index < aval->count)) { /* sanity check */
             aval->qvalArray [index] = assigned.qvalArray[j];
         } else {
             printf (">>> out of range j: %ld  index: %ld\n", j, index);
         }
      }

      PyMem_Free(assigned.qvalArray);  // done with this.
   }

   return true;
}

/* -----------------------------------------------------------------------------
 * called by quaternion_array_set_subscript (__delitem__)
 */
static bool
qa_remove_slice(Py_quaternion_array *aval, PyObject* slice)
{
   Py_ssize_t start;
   Py_ssize_t stop;
   Py_ssize_t step;
   Py_ssize_t number_deleted;
   bool status;
   Py_ssize_t j;

   /* Deal with negative start and stop values and ensure still in range.
    */
   status = qa_decode_slice (*aval, slice, &start, &stop, &step, &number_deleted);
   if (!status) {
      /* qa_decode_slice has already called PyErr_SetString/PyErr_Format.
       */
      return false;
   }

   /* Because we are deleting, we don't care about the order we delete,
    * so convert negative stepping into postive stepping.
    */
   if (step < 0) {
      stop =  start + 1;
      start = start + (number_deleted - 1)*step;
      step = -step;
   }

   const Py_ssize_t new_count = aval->count - number_deleted;

   if (step == 1) {
      /* basic slice removal.
       * Shuffle down the tail end - if needs be.
       * memmove (void *__dest, const void *__src, size_t __n)
       */
      Py_ssize_t numberToMove = aval->count - stop;
      Py_ssize_t offset = number_deleted;
      if (numberToMove > 0) {  /* number delete */
         memmove(&aval->qvalArray [stop - offset],
                 &aval->qvalArray [stop],
                 numberToMove * sizeof(Py_quaternion));
      }

   } else {

      /* We want to do a smat shuffle
       */
      Py_ssize_t numberPerMovedStep = step - 1;
      for (j = 0; j < number_deleted; j++) {
         Py_ssize_t index = start + j*step;
         Py_ssize_t src = index + 1;
         Py_ssize_t dest = start + j*numberPerMovedStep;

         /* First shuffle down the content
          * memmove (void *__dest, const void *__src, size_t __n)
          */
         memmove(&aval->qvalArray[dest], &aval->qvalArray[src],
                 numberPerMovedStep * sizeof(Py_quaternion));
      }

      /* Lastly shuffe the ramaining items if any.
       */
      Py_ssize_t lastSrc = start + number_deleted*step;
      Py_ssize_t numberToMove = aval->count - lastSrc;
      if (numberToMove > 0) {
         Py_ssize_t dest = start + number_deleted*numberPerMovedStep;
         memmove(&aval->qvalArray[dest], &aval->qvalArray[lastSrc],
                  numberToMove* sizeof(Py_quaternion));
      }
   }

   aval->count = new_count;

   Py_ssize_t threshold = (aval->allocated * 3) / 5;  /* 60% */
   if (aval->count < (threshold - 10)) {
      qa_reallocate(aval, threshold, true);
   }

   return true;
}

/* -----------------------------------------------------------------------------
 * __setitem__ (value not null) and __delitem__ (value is null)
 *
 ** NOTE: We set error and return Py_None when there is a problem otherwise  **
 **       return NULL when all OK. This is OPPOSITE to every other function. **
 **       Maybe because this is a statement, e.g.  a[2:4] = (....)
 */
static PyObject *
quaternion_array_set_subscript(PyObject* self, PyObject* key, PyObject* value)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyQuaternionObject* pQuat;

   result = Py_None;   /* hythosize failure */
   Py_INCREF(result);

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, result);

   if (PyLong_Check(key)) {
      /* Caller has supplied a integer
       */
      Py_ssize_t index = PyLong_AsSsize_t(key);

      /* negative means relative to end counting backward.
       */
      if (index < 0) index = pObj->aval.count + index;

      if ((index < 0) || (index >= pObj->aval.count)) {
         PyErr_SetString(PyExc_IndexError,
                         "array index out of range");
         return result; /** Py_None **/
      }

      if (value) {
         /* __setitem__
          */
         pQuat = (PyQuaternionObject*) PyObject_AsQuaternion(value);
         if (!pQuat) {
            PyErr_Format(PyExc_TypeError,
                         "a Quaternion argument is required (got type %s)",
                         Py_TYPE(value)->tp_name);
            return result; /** Py_None **/
         }

         pObj->aval.qvalArray[index] = pQuat->qval;

      } else {
         /* __delitem__
          *
          * Shuffle data down toward zero-th index.
          * memmove (void *__dest, const void *__src, size_t __n)
          */
         int numberToMove = pObj->aval.count - index - 1;
         if (numberToMove > 0) {
            memmove(&pObj->aval.qvalArray [index],
                    &pObj->aval.qvalArray [index+1],
                    numberToMove * sizeof(Py_quaternion));
         }
         pObj->aval.count--;
      }

   } else if (PySlice_Check(key)) {
      bool status;

      if (value) {
         status = qa_assign_slice(&pObj->aval, key, value);
      } else {
         status = qa_remove_slice(&pObj->aval, key);
      }

      if (!status) {
         /* qa_assign_slice/qa_remove_slice has already called
          * PyErr_SetString/PyErr_Format.
          */
         return result; /** Py_None **/
      }

   } else  {
      /* Caller has got it wrong.
       */
      PyErr_Format(PyExc_TypeError,
                   "array indices must be integers or slices (got type %s)",
                   Py_TYPE(key)->tp_name);
      return result; /** Py_None **/
   }

   Py_DECREF (result); /** Py_None **/
   result = NULL;
   return result;
}

/* -----------------------------------------------------------------------------
 * Add attributes for number or memory usage
 */
static PyObject *
quaternion_array_getattro(PyObject *self, PyObject *attr)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   const char* name = NULL;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   if (PyUnicode_Check(attr)) {
      /* The returned buffer always has an extra null byte appended
       * In the case of an error, NULL is returned with an exception set.
       * The caller is not responsible for deallocating the buffer.
       */
      name = PyUnicode_AsUTF8(attr);
      if (name) {
         /* Check for our own attributes
          */
         if (strcmp(name, "itemsize") == 0) {
            result = PyLong_FromLong(sizeof(Py_quaternion));
         }
         else if (strcmp(name, "allocated") == 0) {
            result = PyLong_FromLong(pObj->aval.allocated);
         }
      }
   }

   if (result == NULL) {
      /* Use default generic get attributes function for members and functions.
       * This could maybe be placed inside the unicode check or the name check.
       */
      result = PyObject_GenericGetAttr(self, attr);
   }

   if (result == NULL) {
      PyErr_Format(PyExc_AttributeError,
                   "'quaternion.QuaternionArray' object has no attribute '%.200s'", name);
   }

   return result;
}


   /* =============================================================================
 * Tables
 * =============================================================================
 *
 *  prune ()
 */
static PyMethodDef quaternion_array_methods [] = {
   {"append",     (PyCFunction)quaternion_array_append,    METH_VARARGS, quaternion_array_append_doc},
   {"buffer_info",(PyCFunction)quaternion_array_info,      METH_NOARGS,  quaternion_array_info_doc},
   {"clear",      (PyCFunction)quaternion_array_clear,     METH_NOARGS,  quaternion_array_clear_doc},
   {"count",      (PyCFunction)quaternion_array_count,     METH_VARARGS, quaternion_array_count_doc},
   {"extend",     (PyCFunction)quaternion_array_extend,    METH_VARARGS, quaternion_array_extend_doc},
   {"frombytes",  (PyCFunction)quaternion_array_frombytes, METH_VARARGS, quaternion_array_frombytes_doc},
   {"index",      (PyCFunction)quaternion_array_index,     METH_VARARGS, quaternion_array_index_doc},
   {"insert",     (PyCFunction)quaternion_array_insert,    METH_VARARGS, quaternion_array_insert_doc},
   {"pop",        (PyCFunction)quaternion_array_pop,       METH_VARARGS, quaternion_array_pop_doc},
   {"remove",     (PyCFunction)quaternion_array_remove,    METH_VARARGS, quaternion_array_remove_doc},
   {"reserve",    (PyCFunction)quaternion_array_reserve,   METH_VARARGS, quaternion_array_reserve_doc},
   {"reverse",    (PyCFunction)quaternion_array_reverse,   METH_NOARGS,  quaternion_array_reverse_doc},
   {"tobytes",    (PyCFunction)quaternion_array_tobytes,   METH_NOARGS,  quaternion_array_tobytes_doc},
   { NULL, NULL, 0, NULL}  /* sentinel */
};

/* -----------------------------------------------------------------------------
 * Sequence methods
 */
static PySequenceMethods quaternion_array_sequence = {
   (lenfunc)quaternion_array_length,           /* sq_length */
   (binaryfunc)quaternion_array_concat,        /* sq_concat */
   (ssizeargfunc)quaternion_array_repeat,      /* sq_repeat */
   (ssizeargfunc)quaternion_array_item,        /* sq_item */
   0,                                          /* was sq_slice */
   (ssizeobjargproc)0,                         /* sq_ass_item */
   0,                                          /* was sq_ass_slice */
   (objobjproc)0,                              /* sq_contains */
   (binaryfunc)0,                              /* sq_inplace_concat*/
   (ssizeargfunc)0                             /* sq_inplace_repeat*/
};

/* -----------------------------------------------------------------------------
 * Mapping methods define  __len__, __getitem__, __setitem__ and __delitem__
 * We don't need to define the dunder methods explicitly and we
 * also get the doco. for free.
 */
static PyMappingMethods quaternion_array_mapping = {
   (lenfunc) quaternion_array_length,               /* mp_length */
   (binaryfunc) quaternion_array_get_subscript,     /* mp_subscript */
   (objobjargproc) quaternion_array_set_subscript   /* mp_ass_subscript */
};

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_doc,
             "QuaternionArray([initializer],[reserve]) -> quaternion array\n"
             "\n"
             "The QuaternionArray docuentation is still work in progress.\n"
             "\n"
             "API\n"
             "The API has been choosen to mimic the array.array API as far as resonably possible.\n"
             "In additon to the array.array API, the QuaternionArray API provides:\n"
             "   QuaternionArray() - no type code, however it does allow the initial size \n"
             "                       (number of items) of the internal buffer to be specified.\n"
             "   clear()   - removes all items from the array.\n"
             "   reserve() - extends the internal buffer to accomodate the number specified.\n"
             "\n"
             "Still to be implemented:\n"
             "   byteswap()\n"
             "   fromfile()\n"
             "   fromlist()\n"
             "   tofile()\n"
             "   tolist()\n"
             "\n"
             "Iteration\n"
             "When iterating, the QuaternionArray object itself is currently returned as the\n"
             "interator object, as such multiple and/or nested iterations of not allowed.\n"
             "\n"
             "Pickling\n"
             "While Quaternion objects may be pickled, this functionionality is still\n"
             "is still to be implmented for QuaternionArray objects.\n"
             "\n"
             "Attributes\n"
             "allocated - the length in quaternions of the buffer allocated. This is\n"
             "            always greater than or equal to the actual number of quaternions\n"
             "            values held in the array.\n"
             "itemsize  - the length in bytes of one quaternion array element.\n"
             );


static PyTypeObject QuaternionArrayType = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "quaternion.QuaternionArray",              /* tp_name */
   sizeof(PyQuaternionArrayObject),           /* tp_basicsize */
   0,                                         /* tp_itemsize */
   (destructor)quaternion_array_decalloc,     /* tp_dealloc */
   0,                                         /* tp_print */
   0,                                         /* tp_getattr */
   0,                                         /* tp_setattr */
   0,                                         /* tp_reserved */
   (reprfunc)quaternion_array_repr,           /* tp_repr */
   0,                                         /* tp_as_number */
   &quaternion_array_sequence,                /* tp_as_sequence */
   &quaternion_array_mapping,                 /* tp_as_mapping */
   (hashfunc)quaternion_array_hash,           /* tp_hash */
   0,                                         /* tp_call */
   (reprfunc)quaternion_array_str,            /* tp_str */
   (getattrofunc)quaternion_array_getattro,   /* tp_getattro */
   0,                                         /* tp_setattro */
   0,                                         /* tp_as_buffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
   quaternion_array_doc,                      /* tp_doc */
   0,                                         /* tp_traverse */
   0,                                         /* tp_clear */
   quaternion_array_richcompare,              /* tp_richcompare */
   0,                                         /* tp_weaklistoffset */
   quaternion_array_iter,                     /* tp_iter */
   quaternion_array_next,                     /* tp_iternext */
   quaternion_array_methods,                  /* tp_methods */
   0,                                         /* tp_members */
   0,                                         /* tp_getset */
   0,                                         /* tp_base */
   0,                                         /* tp_dict */
   0,                                         /* tp_descr_get */
   0,                                         /* tp_descr_set */
   0,                                         /* tp_dictoffset */
   0,                                         /* tp_init */
   (allocfunc)PyType_GenericAlloc,            /* tp_alloc */
   (newfunc)quaternion_array_new,             /* tp_new */
   PyObject_Del                               /* tp_free */
};


/* -----------------------------------------------------------------------------
 * Allow module definiton code to access the Quaternion Array PyTypeObject.
 */
PyTypeObject* PyQuaternionArrayType()
{
   return &QuaternionArrayType;
}

/* -----------------------------------------------------------------------------
 */
bool PyQuaternionArray_Check(PyObject *op)
{
   return PyObject_TypeCheck(op, &QuaternionArrayType);
}

/* -----------------------------------------------------------------------------
 */
bool PyQuaternionArray_CheckExact(PyObject *op)
{
   return (Py_TYPE(op) == &QuaternionArrayType);
}

/* end */
