# quaternion

A Python extension to provide a Quaternion type and some associated math functions.

## general

The Quaternion type has a real component and three imaginary components. These are
accessable using the Quaternion instance attributes r, i, j, and k respectively.
Note: the instance attributes i, j, k should not be confused with the quaternion
module variables i, j and k. The former return floats where as the latter are unit
Quaternion instances such that i.i = j.j = k.k = 1, and i.r = i.j = i.k etc = 0.

A Quaternion may also be considered to be a real scalar plus a vector (with real
components). The vector part is accessable via the axis attribute which provides a
tuple. The following Python expressions are equvilent: q.axis and (q.i, q.j, q.k)

The Quaternion type is non-commutative, i.e.  q1*q2  and  q2*q1 in general
 providedifferent results. To divide one Quaternion by another, there are two
options, i.e.:  q1*inverse(q2) or inverse(q2)* q1. The quotient function returns
the former. So (q1/q2)*q2 == q1

Mixed mode: Quaternions and scalar numbers, int or float, are interoperable.
int and float are treated as Quaternions with zero imaginary components.

Mixed mode with complex numbers is also allowed. A complex number, z, is treated
as a Quaternions, q, such that q.r = z.real, q.j = z.imag, and q.i and q.k are
zero. The complex part of a Quaternion may be obtained using the complex
attribute, such that:  q.complex == complex(q.r, q.j).

The choice of allocating the imaginary part of a complex number to j as opposed
to i or k is mathematically arbitary, but for Python j is the natural choice.

## construction

A Quaternion type amy be constructions using one of the following forms:

* Quaternion ()                                     -> quaternion zero
* Quaternion (r[, i[, j[, k]]])                     -> quaternion number
* Quaternion (angle=float,axis=(float,float,float)) -> quaternion rotation number
* Quaternion (number)                               -> quaternion number
* Quaternion ("str representation")                 -> quaternion number

A Quaternion number may be created from:

a) the real part and an optional imaginary parts. r, i, j and k must be float
   or type which can be converted to float;

b) from an angle (radians) and a 3-tuple axis of rotation (automatically 
   normalised),  which generates a rotator quaternion that can be used with
   the rotate mothod;

d) from a single number parameter: int, float, complex or another Quaternion.
   When the number is complex, the imaginary part of the complex is assigned
   to the j imaginary part. So that Quaternion(z) == Quaternion(str(z)); or

c) from the string representation of a quaternion (cf float and complex).


## attributes

* r       - float - real/scalar part
* i       - float - i imaginary part
* j       - float - j imaginary part
* k       - float - k imaginary part
* vector  - tuple - the tuple (i,j,k) 
* complex - complex - the complex number (r, j)


## math functions

These functions aim to mimic the equivilent functions out of the math/cmath module.
The functions prvided are:

    isfinite
    isinf
    isnan
    sqrt
    exp
    log
    log10
    cos
    sine
    tan
    isclose
    polar
    rect

## module variables

* one = Quaternion (1.0, 0.0, 0.0, 0.0)
* i   = Quaternion (0.0, 1.0, 0.0, 0.0)
* i   = Quaternion (0.0, 0.0, 1.0, 0.0)
* i   = Quaternion (0.0, 0.0, 0.0, 1.0)

## backround

This was initally more an experiment to create a Python extension written in C
that was a bit more challenging than just a "hello world" extension.

Altough there are already a number of Quaternion Python packages out there, this
has the advantage of speed over the pure Python implementations and no dependencies
on other modules such as numpy

## references

* http://onlinelibrary.wiley.com/doi/10.1002/9780470682906.app4/pdf
* https://www.geometrictools.com/Documentation/Quaternions.pdf
* https://en.wikipedia.org/wiki/Quaternion

