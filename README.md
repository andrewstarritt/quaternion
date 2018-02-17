# quaternion

A Python extension to provide a Quaternion type and some associated math functions.

## general

Within _this_ module, a Quaternion q is defined to be:

    q = w + x.i + y.j + z.k

where the coefficients w, x, y and z are real; and i, j and k three imaginary
numbers such that:

    i.i = j.j = k.k = -1
    i.j = +k,  j.k = +i,  k.i = +j
    j.i = -k,  k.j = -i,  i.k = -j

The Quaternion type has four member attributes to access these coefficients.
These instance attributes are w, x, y and z respectively.

A Quaternion may also be considered to be a real scalar part plus a vector (with
real components). The real part accessable via the real attribute. Thus both
q.w and q.real return the real or scalar part of q.

The vector part is accessable via the vector and imag attributes which provides
a tuple of floats. The following Python expressions are equivilent:

    q.vector
    q.imag
    (q.x, q.y, q.z)

q.real and q.imag provide a "complex" like view of a Quaternion at the expense
of providing an un-Pythonic duplication of q.w and q.vector respectively.

## mathematical operations

The expected mathematical operations are provided.

unary: +, =, abs

binary: +, -, *, /

power: ** 

There is no mod (%)  or integer division (//) available.
Therefore the pow() function cal only take two arguments, and the exponent argument must be real.

The Quaternion type is associative under both addition and multiplication, i.e.:

    (p + q) + r  =  p + (q + r)
    (p * q) * r  =  p * (q * r)

The Quaternion type is non-commutative with respect to multiplication (and division),
i.e.  p \* q  and  q \* p in general provide different results. To divide one
Quaternion by another, there are two options:

    p * q.inverse() ; or
    q.inverse() * p.

The quotient function returns the former, therefore:

    (p / q) * q = p

## mixed mode arithmetic

Quaternions numbers and scalar numbers, i.e. int or float, are interoperable.
int and float are treated as Quaternions with zero imaginary components.

Mixed mode with complex numbers is also allowed. A complex number, z, is treated
as a Quaternions, q, such that q.w = z.real, q.y = z.imag, and q.x and q.z are
zero.

The choice of alligning the imaginary part of a complex number to the j imaginary
component as opposed to i or k is mathematically arbitary. However for Python, j
is the natural choice because the following, bar any rounding errors, will then
hold true:

    Quaternion(z) = Quaternion(str(z))

The complex part of a Quaternion may be obtained using the complex attribute,
such that:

    q.complex = complex(q.w, q.y).

There is _no_ complementary attribute to obtain q.x and q.z as a single item.


## construction

A Quaternion type may be constructions using one of the following forms:

* Quaternion ()                                     -> quaternion zero
* Quaternion (w[, x[, y[, z]]])                     -> quaternion number
* Quaternion (angle=float,axis=(float,float,float)) -> quaternion rotation number
* Quaternion (number)                               -> quaternion number
* Quaternion ("str representation")                 -> quaternion number

A Quaternion number may be created from:

a) the real part and an optional imaginary parts. w, x, y and z must be float
   or number types which can be converted to float;

b) from an angle (radians) and a 3-tuple axis of rotation (which is automatically
   normalised),  which generates a rotator Quaternion that can be used in
   conjuction with the rotate method;

c) from a single number parameter: int, float, complex or another Quaternion.
   When the number is complex, the imaginary part of the complex number is
   assigned to the j imaginary part; or

d) from the string representation of a quaternion (compare with complex).
   The following are valid:

    Quaternion("1.2")
    Quaternion("1.2+2i")
    Quaternion("1.22_33+4.11_22i")       -- 3.6 or later allows underscores
    Quaternion("1.2+3i-1j")
    Quaternion("(1.2+3i-1j)")
    Quaternion("1.2i+0.3k")
    Quaternion(" ( 1.2+0.3j ) ")
    Quaternion("1.2+3.4i+2.6j-2k")

The following are invalid:

    Quaternion("1.2 + 3.4i+ 2.6j- 2k")   -- spaces within actual number
    Quaternion("1.2+3.4i+2.6k-2j")       -- out of order
    Quaternion("(1.2+3.4i+2.6j-2k")      -- unmatched parenthesis


## attributes

* w       - float - real/scalar part
* x       - float - i imaginary part
* y       - float - j imaginary part
* z       - float - k imaginary part
* vector  - tuple - the tuple (x, y, z)
* complex - complex - the complex number w + y.j
* real    - float - real/scalar part
* imag    - tuple - the imaginary part, the same as vector.


## Quaternion instance functions

### conjugate

q.conjugate() returns the Quaternion conjugate of its argument.

### inverse

q.inverse ()  returns s such that s * q = q * s = 1

### normalise

q.normalise () returns s such that s = q / abs (q)

### rotate

q.rotate (point, origin=None) -> point, where q is a rotation number, 
i.e. q = Quaternion (angle=a,axis=(x,y,z)).
The returned value is rotated by an angle a radians about the axis (x,y,z).

## math functions

A number of math functions that operate on Quaternions are also provided. Where
provided, these provide the equivilent quaternion function as the functions of
the same name out of the math and/or cmath module.

The functions provided are:

    isfinite
    isinf
    isnan
    isclose
    sqrt
    exp
    log
    log10
    cos
    sin
    tan
    polar
    phase
    axis
    rect

## module variables

* one = Quaternion (1.0, 0.0, 0.0, 0.0)
* i   = Quaternion (0.0, 1.0, 0.0, 0.0)
* j   = Quaternion (0.0, 0.0, 1.0, 0.0)
* k   = Quaternion (0.0, 0.0, 0.0, 1.0)
* \_\_version\_\_ = the version number as str.

## backround

This was initally more of an experiment to create a Python extension written in C
that was a bit more challenging than just a "hello world" extension.

Altough there are already a number of Quaternion Python implementations out there,
this has the advantage of speed over the pure Python implementations and the advantage
of no dependencies on other modules such as numpy.

## references

* http://onlinelibrary.wiley.com/doi/10.1002/9780470682906.app4/pdf
* https://www.geometrictools.com/Documentation/Quaternions.pdf
* https://en.wikipedia.org/wiki/Quaternion

## credits

Guidence from https://docs.python.org/3.5/extending/newtypes.html
together with cribbing many code-snippets and ideas from the complex type,
and not forgetting Sir William R. Hamilton.

