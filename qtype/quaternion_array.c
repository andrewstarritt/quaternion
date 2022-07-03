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
#include "quaternion_array_iter.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static const long pickleFormatVersion = 1;

/* -----------------------------------------------------------------------------
 * Macro to check that the allocated memory is sensible.
 * Note: we always have some memory allocated.
 */
#define SANITY_CHECK(pObj, errReturn) {                                       \
   if (!pObj->aval.qvalArray ||                                               \
       (pObj->aval.count > pObj->aval.allocated) ||                           \
       (pObj->aval.allocated < pObj->aval.reserved)) {                        \
       PyErr_Format(PyExc_MemoryError,                                        \
                    "quaternion array corrupted (%d)", __LINE__);             \
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
 * Returns true iff successful, otherwise reports error and returns false.
 */
static bool
qa_reallocate (Py_quaternion_array *aval, const Py_ssize_t new_size, const bool exact)
{
   Py_ssize_t new_allocation;

   /* Re allocate data
    */
   if (exact) {
      new_allocation = new_size;
   } else {
      new_allocation = qa_next_allocated_size(new_size);
   }

   /* Must always have at least reserved number of items.
    */
   if (new_allocation < aval->reserved) {
       new_allocation = aval->reserved;
   }

   if (aval->qvalArray) {
      /* Re allocation- but only if really needed.
       */
      if (aval->allocated != new_allocation) {
         aval->allocated = new_allocation;
         aval->qvalArray = PyMem_REALLOC(aval->qvalArray,
                                         aval->allocated * sizeof(Py_quaternion));
      }
   } else {
      /* Initial allocation
       */
      aval->allocated = new_allocation;
      aval->qvalArray = PyMem_MALLOC(aval->allocated * sizeof(Py_quaternion));

   }

   if (!aval->qvalArray) {
      PyErr_Format(PyExc_MemoryError, "allocation for %ld quaternion items failed",
                   new_allocation);
      return false;
   }

   return true;
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
static bool
qa_extract_and_add(PyObject *initializer,
                   Py_quaternion_array *aval)
{
   PyObject *iter = NULL;
   PyQuaternionObject *pQuat = NULL;
   PyObject *item = NULL;

   if (!initializer)     return false;  /* sanity check */
   if (!aval)            return false;  /* sanity check */
   if (!aval->qvalArray) return false;  /* sanity check */

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
      return true;
   }

   iter = PyObject_GetIter(initializer);
   if (!iter) return false;  /* belts 'n' braces sanity check */

   for (item = PyIter_Next(iter); item != NULL; item = PyIter_Next(iter)) {
      /* Caste to a PyQuaternionObject otherwise retuns NULL
       */
      pQuat = (PyQuaternionObject*) PyObject_AsQuaternion(item);
      if (!pQuat) return false;  /* belts 'n' braces sanity check */

      aval->qvalArray [aval->count++] = pQuat->qval;
      Py_DECREF (pQuat);
   }

   return true;
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
 * Methods
 * -----------------------------------------------------------------------------
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
   bool status;

   if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO:QuaternionArray", kwlist,
                                    &initializer, &reserve)) {
      return NULL;
   }

   aval.reserved = 0;      /* none unless we told otherwise */
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
      initialNumber = qa_next_allocated_size (0);  /* about 10 */
   }

   if (reserve) {
      if (!PyLong_Check(reserve)) {
         PyErr_Format(PyExc_TypeError,
                      "array sizes must be integers (got type %s)",
                      Py_TYPE(reserve)->tp_name);
         return NULL;
      }

      aval.reserved = PyLong_AsSsize_t(reserve);
      if (aval.reserved < 0) {
         PyErr_Format(PyExc_ValueError,
                      "array reserved sizes can't be negative (got %ld)",
                      aval.reserved);
         return NULL;
      }
   }

   /* Allocate the storage
    */
   aval.qvalArray = NULL;
   status = qa_reallocate(&aval, initialNumber, true);
   if (!status)
      return NULL;

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
      PyMem_FREE(pObj->aval.qvalArray);
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
 * __reduce__
 */
PyDoc_STRVAR(quaternion_array_reduce_doc,
             "__reduce__(self,/)\n"
             "Helper for pickle.");

static PyObject *
quaternion_array_reduce (PyObject* self)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   PyObject* empty;
   PyObject* data;
   PyObject* state;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   empty = Py_BuildValue("()");  /* empty arg list for construction */

   data = PyBytes_FromStringAndSize((char *) pObj->aval.qvalArray,
                                     pObj->aval.count * sizeof(Py_quaternion));

   /* Form the state data tuple object.
    * Note: we don't preserve the actual number allocated.
    */
   state = Py_BuildValue("(llO)", pickleFormatVersion, pObj->aval.reserved, data);

   result = Py_BuildValue("OOO", Py_TYPE(self), empty, state); /* , None, None, None ); */

   return result;
}

/* -----------------------------------------------------------------------------
 * __setstate__
 */
PyDoc_STRVAR(quaternion_array_setstate_doc,
             "__setstate__(self, state_data,/)\n"
             "Helper for unpickle.");

static PyObject *
quaternion_array_setstate(PyObject* self, PyObject *args)
{
   PyQuaternionArrayObject* pObj;
   long version;
   PyObject* dataObj;
   Py_ssize_t data_size = 0;
   char* data = NULL;
   Py_ssize_t reserved = 0;
   bool status;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the version, the number reserved and the data object.
    */
   if (!PyArg_ParseTuple(args, "(llO):__setstate__",
                         &version, &reserved, &dataObj))
      return NULL;

   if (!PyBytes_Check(dataObj)) {
      PyErr_Format(PyExc_TypeError,
                   "Expecting pickled quaternion array data to be type bytes (got type %s)",
                   Py_TYPE(dataObj)->tp_name);
      return NULL;
   }

   if (version != pickleFormatVersion) {
      PyErr_Format(PyExc_ValueError,
                   "Expecting pickled quaternion array data format version %ld (got %ld)",
                   pickleFormatVersion, version);
      return NULL;
   }

   data_size = PyBytes_Size (dataObj);
   data = PyBytes_AsString (dataObj);

   if ((data_size < 0) || ((data_size % sizeof(Py_quaternion)) != 0)) {
      PyErr_Format(PyExc_ValueError,
                   "bytes length %ld not a multiple of quaternion size %ld",
                   data_size, sizeof(Py_quaternion));
      return NULL;
   }

   /* So how many Quaternion objects are there?
    */
   pObj->aval.count = data_size / sizeof(Py_quaternion);
   pObj->aval.reserved = reserved;
   status = qa_reallocate(&pObj->aval, pObj->aval.count, false);
   if (!status)
      return NULL;

   /* Copy the data.
    */
   memcpy(&pObj->aval.qvalArray[0], data, data_size);

   Py_RETURN_NONE;
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
   bool status;

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

   aval.reserved = 0;
   aval.allocated = 0;
   aval.count = pObj->aval.count + pArg->aval.count;
   aval.qvalArray = NULL;
   status = qa_reallocate(&aval, aval.count, false);
   if (!status)
      return NULL;

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
 * sq_inplace_concat
 */
static PyObject *
quaternion_array_inplace_concat(PyObject *self, PyObject *arg)
{
   PyQuaternionArrayObject* pObj;
   PyQuaternionArrayObject* pArg;
   Py_ssize_t objCount;
   Py_ssize_t argCount;
   bool status;

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

   /* Save these, esp if doing a += a
    */
   objCount = pObj->aval.count;
   argCount = pArg->aval.count;
   pObj->aval.count = objCount + argCount;

   /* Do we have enough room?
    */
   if (pObj->aval.count > pObj->aval.allocated) {
      status = qa_reallocate(&pObj->aval, pObj->aval.count, false);
      if (!status)
         return NULL;
   }

   if (argCount > 0) {
      /* Now copy data and append
       * void *memcpy(void *dest, const void *src, size_t n);
       */
      memcpy(&pObj->aval.qvalArray[objCount],
             &pArg->aval.qvalArray[0],
             argCount*sizeof(Py_quaternion));
   }

   Py_INCREF(self);   /** most important **/
   return self;
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
   bool status;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   if (repeat < 0) repeat = 0;
   aval.reserved = 0;
   aval.allocated = 0;
   aval.count = pObj->aval.count * repeat;
   aval.qvalArray = NULL;
   status = qa_reallocate(&aval, aval.count, false);
   if (!status)
      return NULL;

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
 * sq_inplace_repeat
 */
static PyObject *
quaternion_array_inplace_repeat (PyObject *self, Py_ssize_t repeat)
{
   PyQuaternionArrayObject* pObj;
   Py_ssize_t objCount;
   Py_ssize_t k;
   bool status;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   objCount = pObj->aval.count;  /* save for later */

   if (repeat < 0) repeat = 0;
   pObj->aval.count = objCount * repeat;

   /* Do we have enough room?
    */
   if (pObj->aval.count > pObj->aval.allocated) {
      status = qa_reallocate(&pObj->aval, pObj->aval.count, false);
      if (!status)
         return NULL;
   }

   /* Now copy data repeat times
    * void *memcpy(void *dest, const void *src, size_t n);
    */
   for (k = 1; k < repeat; k++) {
      memcpy(&pObj->aval.qvalArray[k*objCount],
             &pObj->aval.qvalArray[0],
             objCount*sizeof(Py_quaternion));
   }

   Py_INCREF(self);   /** most important **/
   return self;
}

/* -----------------------------------------------------------------------------
 * sq_item
 */
PyObject *
PyQuaternionArrayGetItem(PyObject *self, Py_ssize_t index)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   Py_quaternion qval;

   /* This is callable externally - hence the extra check.
    */
   if (!PyQuaternionArray_Check(self)) {
      PyErr_Format(PyExc_TypeError,
                   "a Quaternion argument is required (got type %s)",
                   Py_TYPE(self)->tp_name);
      return NULL;
   }

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
   PyQuaternionArrayObject* pObj;
   PyObject* value;
   PyQuaternionObject* pQuat;
   bool status;

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
      status = qa_reallocate(&pObj->aval, pObj->aval.count + 1, false);
      if (!status)
         return NULL;
   }

   pObj->aval.qvalArray[pObj->aval.count++] = pQuat->qval;

   Py_RETURN_NONE;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_insert_doc,
             "insert(self, i, q, /)\n"
             "Insert quaternion q into the array before position i.");

static PyObject *
quaternion_array_insert(PyObject *self, PyObject *args)
{
   PyQuaternionArrayObject* pObj;
   PyObject* indexObj;
   Py_ssize_t index;
   PyObject* valueObj;
   PyQuaternionObject* pQuatObj;
   bool status;

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
      status = qa_reallocate(&pObj->aval, pObj->aval.count + 1, false);
      if (!status)
         return NULL;
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

   Py_RETURN_NONE;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_extend_doc,
             "extend(self, iter, /)\n"
             "Append quaternions to the end of the array.");

static PyObject *
quaternion_array_extend(PyObject *self, PyObject *args)
{
   PyQuaternionArrayObject* pObj;
   PyObject *initializer = NULL;
   Py_ssize_t additional;
   bool status;

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
      status = qa_reallocate(&pObj->aval, pObj->aval.count + additional, false);
      if (!status)
         return NULL;
   }

   qa_extract_and_add(initializer, &pObj->aval);

   Py_RETURN_NONE;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_frombytes_doc,
             "frombytes(self, buffer, /)\n"
             "Appends quaternions from the string, interpreting it as an array of machine values.");

static PyObject *
quaternion_array_frombytes(PyObject* self, PyObject *args)
{
   PyQuaternionArrayObject* pObj;
   PyObject *dataObj = NULL;
   int bytesStatus;
   int byteArrayStatus;
   Py_ssize_t data_size = 0;
   char* data = NULL;
   Py_ssize_t additional;
   bool status;

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

   if ((data_size < 0) || ((data_size % sizeof(Py_quaternion)) != 0)) {
      PyErr_Format(PyExc_ValueError,
                   "bytes length %ld not a multiple of quaternion size %ld",
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
      status = qa_reallocate(&pObj->aval, pObj->aval.count + additional, false);
      if (!status)
         return NULL;
   }

   /* Copy the data
    */
   memcpy(&pObj->aval.qvalArray[pObj->aval.count], data, data_size);
   pObj->aval.count += additional;

   Py_RETURN_NONE;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_fromfile_doc,
             "fromfile(self, f, n, /)\n"
             "Read n quaternion objects from the file object f and append them to the end of the array.");

static PyObject *
quaternion_array_fromfile(PyObject* self, PyObject *args)
{
   _Py_IDENTIFIER(read);

   PyQuaternionArrayObject* pObj;
   PyObject *fileObj = NULL;
   PyObject *numberObj = NULL;
   PyObject *bytesObj = NULL;
   Py_ssize_t number;
   Py_ssize_t nbytes;
   Py_ssize_t data_size;
   Py_ssize_t additional;
   char* data = NULL;
   int notEnoughBytes;
   bool status;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the file data object number items from the tuple.
    */
   if (!PyArg_ParseTuple(args, "OO:fromfile", &fileObj, &numberObj))
      return NULL;

   // Py_io.BufferedReader_Check function??
   // _PyObject_CallMethodId handles the error reporting

   if (!PyLong_Check(numberObj)) {
      PyErr_Format(PyExc_TypeError,
                   "number of quaternions to read must be integer (got type %s)",
                   Py_TYPE(numberObj)->tp_name);
      return NULL;
   }

   number = PyLong_AsSsize_t (numberObj);
   if (number < 0) {
       PyErr_SetString(PyExc_ValueError, "number of quaternions is negative");
       return NULL;
   }

   if (number > (PY_SSIZE_T_MAX / sizeof (Py_quaternion))) {
       PyErr_NoMemory();
       return NULL;
   }

   /* Calc number of bytes to read from the file and attempt to read.
    */
   nbytes = number * sizeof (Py_quaternion);
   bytesObj = _PyObject_CallMethodId(fileObj, &PyId_read, "n", nbytes);
   if (!bytesObj) {
      return NULL;
   }

   if (!PyBytes_Check(bytesObj)) {
       PyErr_SetString(PyExc_TypeError,
                       "read() didn't return bytes");
       Py_DECREF(bytesObj);
       return NULL;
   }

   /* Extract info re what we read.
    */
   data_size = PyBytes_GET_SIZE(bytesObj);
   data = PyBytes_AsString (bytesObj);

   notEnoughBytes = (data_size != nbytes);

   /* We can read less that what we want, if that is all that is available,
    * but what is availablemust still be a whole number of quaternions.
    * We do not round down and just go with that.
    */
   if ((data_size < 0) || ((data_size % sizeof(Py_quaternion)) != 0) ) {
      Py_DECREF(bytesObj);
      PyErr_Format(PyExc_ValueError,
                   "bytes length %d not a multiple of quaternion size %d",
                   data_size, sizeof(Py_quaternion));
      return NULL;
   }

   /* How many quaternions are being read?
    */
   additional = data_size / sizeof(Py_quaternion);

   /* Do we have enough room in the array?
    */
   if (pObj->aval.count + additional > pObj->aval.allocated) {
      /* Re allocate data
       */
      status = qa_reallocate(&pObj->aval, pObj->aval.count + additional, false);
      if (!status)
         return NULL;
   }

   /* Move the data
    */
   memcpy(&pObj->aval.qvalArray[pObj->aval.count], data, data_size);
   pObj->aval.count += additional;
   Py_DECREF(bytesObj);

   if (notEnoughBytes) {
       PyErr_Format(PyExc_EOFError,
                    "read() didn't return enough bytes (read %ld, wanted %ld)",
                    data_size, nbytes);
       return NULL;
   }

   Py_RETURN_NONE;
}


/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_info_doc,
             "buffer_info(self, /)\n"
             "Return a tuple (address, length) giving the current memory address and\n"
             "the length in quaternions of the buffer used to hold array's contents.\n"
             "\n"
             "The length should be multiplied by the itemsize attribute to calculate\n"
             "the buffer length in bytes.");

static PyObject *
quaternion_array_info(PyObject* self)
{
   PyObject *result = NULL;
   PyQuaternionArrayObject* pObj;
   Py_ssize_t address;

   pObj = (PyQuaternionArrayObject *)self;
   // No SANITY_CHECK here

   address = pObj->aval.count > 0 ? (Py_ssize_t) pObj->aval.qvalArray : 0;
   result = Py_BuildValue("(ll)", address, pObj->aval.count);

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
   PyQuaternionArrayObject* pObj;
   bool status;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   pObj->aval.count = 0;
   status = qa_reallocate(&pObj->aval, 0, false);
   if (!status)
      return NULL;

   Py_RETURN_NONE;
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

         Py_RETURN_NONE;
      }
   }

   PyErr_SetString(PyExc_ValueError, "array.remove(q): q not in array.");
   return NULL;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_reserve_doc,
             "reserve(self, n, /)\n"
             "reserve (pre-allocate) space for at least n items in the array.");

static PyObject *
quaternion_array_reserve(PyObject *self, PyObject *args)
{
   PyQuaternionArrayObject* pObj;
   PyObject* sizeObj;
   Py_ssize_t new_reserve;
   bool status;

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

   new_reserve = PyLong_AsSsize_t(sizeObj);
   if (new_reserve < 0) {
      PyErr_Format(PyExc_ValueError,
                   "array reserved sizes can't be negative (got %ld)", new_reserve);
      return NULL;
   }

   pObj->aval.reserved = new_reserve;

   /* Do we have enough room? Recall allocated >= count.
    *** What about deallocation if/when reserve is smaller?
    */
   if (pObj->aval.allocated < pObj->aval.reserved) {
      /* Re allocate data
       */
      status = qa_reallocate(&pObj->aval, pObj->aval.reserved, true);
      if (!status)
         return NULL;
   }

   Py_RETURN_NONE;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_reverse_doc,
             "Reverse the order of the quaternions in the array.");
static PyObject *
quaternion_array_reverse(PyObject* self)
{
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

   Py_RETURN_NONE;
}

/* -----------------------------------------------------------------------------
 */
PyDoc_STRVAR(quaternion_array_byteswap_doc,
             "Byteswap all items of the array.");
static PyObject *
quaternion_array_byteswap(PyObject* self)
{
   /* We "know" each quaternion has 4 doubles.
    * And each double has 8 bytes.
    */
   typedef char bytequad [4][8];

   PyQuaternionArrayObject* pObj;
   bytequad *byteData;
   Py_ssize_t i;
   int j;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   byteData = (bytequad *)pObj->aval.qvalArray;
   for (i = 0; i < pObj->aval.count; i++) {
      for (j = 0; j < 4; j++) {
         char *d = byteData[i][j];
         char t;
         t = d[0]; d[0] = d[7]; d[7] = t;
         t = d[1]; d[1] = d[6]; d[6] = t;
         t = d[2]; d[2] = d[5]; d[5] = t;
         t = d[3]; d[3] = d[4]; d[4] = t;
      }
   }

   Py_RETURN_NONE;
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
 */
PyDoc_STRVAR(quaternion_array_tofile_doc,
             "tofile(self, f /)\n"
             "Write all quaternions (as machine values) to the file object f.\n");

/** Totally cribbed from array_array_tofile **/
static PyObject *
quaternion_array_tofile(PyObject* self, PyObject *args)
{
   _Py_IDENTIFIER(write);

   PyQuaternionArrayObject* pObj;
   PyObject *fileObj = NULL;
   Py_ssize_t nbytes;
   int blockSize;
   Py_ssize_t nblocks;
   Py_ssize_t i;

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, NULL);

   /* Extract the file object from the one element tuple.
    */
   if (!PyArg_ParseTuple(args, "O:tofile", &fileObj))
      return NULL;

   nbytes = pObj->aval.count * sizeof (Py_quaternion);

   /* Write 64K blocks at a time.
    */
   blockSize = 64*1024;

   nblocks = (nbytes + blockSize - 1) / blockSize;  /* round up */
   for (i = 0; i < nblocks; i++) {
       char* ptr = (char *)pObj->aval.qvalArray + i*blockSize;
       Py_ssize_t size = blockSize;
       PyObject *bytes;
       PyObject *res;

       /* Truncate last block it needs be.
        */
       if (i*blockSize + size > nbytes)
           size = nbytes - i*blockSize;

       bytes = PyBytes_FromStringAndSize(ptr, size);
       if (bytes == NULL)
           return NULL;
       res = _PyObject_CallMethodId(fileObj, &PyId_write, "O", bytes);
       Py_DECREF(bytes);

       if (res == NULL)
           return NULL;
       Py_DECREF(res);  /* drop write result */
   }

   Py_RETURN_NONE;
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
      result = PyQuaternionArrayGetItem(self, index);

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

      aval.reserved = 0;
      aval.count = 0;
      aval.qvalArray = NULL;
      status = qa_reallocate(&aval, count, false);
      if (!status)
         return NULL;

      for (j = 0; j < count; j++) {
          Py_ssize_t index = start + j*step;
          if ((index >= 0) && (index < pObj->aval.count)) { /* sanity check */
             aval.qvalArray[aval.count++] = pObj->aval.qvalArray [index];
          } else {
             DEBUG_TRACE ("out of range j: %ld  index: %ld\n", j, index);
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
         status = qa_reallocate(aval, new_count, false);
         if (!status)
            return false;
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

      /* Create a C quaternion_array from the value so that we can use
       * the qa_extract_and_add function.
       */
      Py_quaternion_array assigned;
      assigned.reserved = 0;
      assigned.count = 0;
      assigned.allocated = 0;
      assigned.qvalArray = NULL;
      status = qa_reallocate(&assigned, number_assigned, true);
      if (!status)
         return false;
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
      assigned.reserved = 0;
      assigned.count = 0;
      assigned.allocated = 0;
      assigned.qvalArray = NULL;
      status = qa_reallocate(&assigned, number_assigned, true);
      if (!status)
         return false;
      qa_extract_and_add(value, &assigned);

      Py_ssize_t j;
      for (j = 0; j < number_assigned; j++) {
          Py_ssize_t index = start + j*step;
          if ((index >= 0) && (index < aval->count)) { /* sanity check */
             aval->qvalArray [index] = assigned.qvalArray[j];
         } else {
             DEBUG_TRACE ("out of range j: %ld  index: %ld\n", j, index);
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

   /* Release memory if appropriate.
    */
   Py_ssize_t threshold = (aval->allocated * 3) / 5;  /* 60% */
   if (aval->count < (threshold - 10)) {
      status = qa_reallocate(aval, threshold, true);
      if (!status)
         return false;
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

   result = Py_None;   /* hypothesize failure */
   Py_INCREF(result);

   pObj = (PyQuaternionArrayObject *)self;
   SANITY_CHECK(pObj, result);

   if (PyLong_Check(key)) {
      /* Caller has supplied an index integer
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
      /* Caller has supplied a slice
       */
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
   // No SANITY_CHECK here

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
         else if (strcmp(name, "reserved") == 0) {
            result = PyLong_FromLong(pObj->aval.reserved);
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
   {"__reduce__",   (PyCFunction)quaternion_array_reduce,    METH_NOARGS,  quaternion_array_reduce_doc    },
   {"__setstate__", (PyCFunction)quaternion_array_setstate,  METH_VARARGS, quaternion_array_setstate_doc  },
   {"append",       (PyCFunction)quaternion_array_append,    METH_VARARGS, quaternion_array_append_doc    },
   {"buffer_info",  (PyCFunction)quaternion_array_info,      METH_NOARGS,  quaternion_array_info_doc      },
   {"byteswap",     (PyCFunction)quaternion_array_byteswap,  METH_NOARGS,  quaternion_array_byteswap_doc  },
   {"clear",        (PyCFunction)quaternion_array_clear,     METH_NOARGS,  quaternion_array_clear_doc     },
   {"count",        (PyCFunction)quaternion_array_count,     METH_VARARGS, quaternion_array_count_doc     },
   {"extend",       (PyCFunction)quaternion_array_extend,    METH_VARARGS, quaternion_array_extend_doc    },
   {"frombytes",    (PyCFunction)quaternion_array_frombytes, METH_VARARGS, quaternion_array_frombytes_doc },
   {"fromfile",     (PyCFunction)quaternion_array_fromfile,  METH_VARARGS, quaternion_array_fromfile_doc  },
   {"index",        (PyCFunction)quaternion_array_index,     METH_VARARGS, quaternion_array_index_doc     },
   {"insert",       (PyCFunction)quaternion_array_insert,    METH_VARARGS, quaternion_array_insert_doc    },
   {"pop",          (PyCFunction)quaternion_array_pop,       METH_VARARGS, quaternion_array_pop_doc       },
   {"remove",       (PyCFunction)quaternion_array_remove,    METH_VARARGS, quaternion_array_remove_doc    },
   {"reserve",      (PyCFunction)quaternion_array_reserve,   METH_VARARGS, quaternion_array_reserve_doc   },
   {"reverse",      (PyCFunction)quaternion_array_reverse,   METH_NOARGS,  quaternion_array_reverse_doc   },
   {"tobytes",      (PyCFunction)quaternion_array_tobytes,   METH_NOARGS,  quaternion_array_tobytes_doc   },
   {"tofile",       (PyCFunction)quaternion_array_tofile,    METH_VARARGS, quaternion_array_tofile_doc    },
   { NULL, NULL, 0, NULL}  /* sentinel */
};

/* -----------------------------------------------------------------------------
 * Sequence methods
 */
static PySequenceMethods quaternion_array_sequence = {
   (lenfunc)quaternion_array_length,                /* sq_length */
   (binaryfunc)quaternion_array_concat,             /* sq_concat */
   (ssizeargfunc)quaternion_array_repeat,           /* sq_repeat */
   (ssizeargfunc)PyQuaternionArrayGetItem,          /* sq_item */
   0,                                               /* was sq_slice */
   (ssizeobjargproc)0,                              /* sq_ass_item */
   0,                                               /* was sq_ass_slice */
   (objobjproc)0,                                   /* sq_contains */
   (binaryfunc)quaternion_array_inplace_concat,     /* sq_inplace_concat*/
   (ssizeargfunc)quaternion_array_inplace_repeat    /* sq_inplace_repeat*/
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
             "In addition to the array.array API, the QuaternionArray API provides:\n"
             "   QuaternionArray() - no type code, however it does allow the reserve size \n"
             "                       (number of items) of the internal buffer to be specified.\n"
             "   clear()   - removes all items from the array.\n"
             "   reserve() - (re)specifies (and re-extends if necessary) the internal buffer.\n"
             "\n"
             "and two additional attributes: allocated and reserved - see below.\n"
             "\n"
             "Still to be implemented:\n"
             "   fromlist()\n"
             "   tolist()\n"
             "\n"
             "Iteration\n"
             "The QuaternionArray object fully supports iteraltion.\n"
             "\n"
             "Attributes\n"
             "allocated - the length in quaternions of the buffer allocated. This is\n"
             "            always greater than or equal to the actual number of quaternions\n"
             "            values held in the array.\n"
             "itemsize  - the length in bytes of one quaternion array element.\n"
             "reserved  - the minimum buffer size allocation."
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
   (getiterfunc)PyQuaternionArrayIter,        /* tp_iter */
   0,                                         /* tp_iternext */
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
