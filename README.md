# <span style='color:#00c000'>quaternion</span>

A Python extension to provide a Quaternion type and some associated math functions.

[general](#general)<br>
[mathematical operations](#mathops)<br>
[mixed mode arithmetic](#mixedmode)<br>
[construction](#construction)<br>
[attributes](#attributes)<br>
[instance functions](#instfuncs)<br>
[magic functions](#magicfuncs)<br>
[math functions](#mathfuncs)<br>
[module variables](#variables)<br>
[hash function](#hash)<br>
[background](#background)<br>
[references](#references)<br>
[credits](#credits)<br>

## <a name = "general"/><span style='color:#00c000'>general</span>

Within _this_ module, a Quaternion q is defined to be:

    q = w + x.i + y.j + z.k

(_through out this document = means 'is equal to', as opposed to assignment_)<br>
where the coefficients w, x, y and z are real; and i, j and k three imaginary
numbers such that:

    i.i = j.j = k.k = i.j.k = -1
    i.j = +k,  j.k = +i,  k.i = +j
    j.i = -k,  k.j = -i,  i.k = -j

The Quaternion type has four member attributes to access these coefficients.
These instance attributes are w, x, y and z respectively.

A Quaternion may also be considered to be a real scalar part plus a vector (with
3 real components).
The real part accessible via the real attribute.
Both q.w and q.real return the real or scalar part of q.

The vector part is accessible via both the vector and imag attributes which provide
a tuple of floats. The following Python expressions are equivalent:

    q.vector
    q.imag
    (q.x, q.y, q.z)

q.real and q.imag provide a "complex" like view of a Quaternion at the expense
of providing an un-Pythonic duplication of q.w and q.vector respectively.

## <a name = "mathops"/><span style='color:#00c000'>mathematical operations</span>

The expected mathematical operations are provided.

unary: +, -, abs

binary: +, -, \*, /

power: \*\*

There is no mod (%) or integer division (//) operation available.
Therefore the pow() function can only take two arguments - see below.

The Quaternion type is associative under both addition and multiplication, i.e.:

    (p + q) + r  =  p + (q + r)
    (p * q) * r  =  p * (q * r)

The Quaternion type is non-commutative with respect to multiplication and division,
i.e.  p \* q  and  q \* p in general provide different values. To divide one
Quaternion by another, there are two options:

    p * q.inverse() ; or
    q.inverse() * p.

The quotient function returns the former, therefore:

    (p / q) * q = p

This non-commutative nature also explains why p ** q is undefined, as this could implemented as:

    exp (q * log (p))  ; or
    exp (log (p) * q)

However, mixed-mode ** is possible - see below.

___Experimental/tentative___

matrix mult: @

p @ q is the same a dot (p, q) and returns the dot product of two Quaternians
treated as a normal 4-tuple.
Is this a sensible use of @ ??

## <a name = "mixedmode"/><span style='color:#00c000'>mixed mode arithmetic</span>

Quaternions numbers and scalar numbers, i.e. int or float, are inter-operable.
int and float numbers are treated as Quaternions with zero imaginary components.
Note: float numbers (a) and Quaternion numbers (q) do commute under
multiplication:

    q * a = a * q

Mixed mode with complex numbers is also allowed. A complex number, z, is treated
as a Quaternions, q, such that q.w = z.real, q.y = z.imag, and q.x and q.z are
zero.

The choice of aligning the imaginary part of a complex number to the j imaginary
component as opposed to i or k is mathematically arbitrary.
However for Python, j is the natural choice and then the following, bar any
rounding errors, will hold true for any complex value z:

    Quaternion(z) = Quaternion(str(z))

The complex part of a Quaternion may be obtained using the complex attribute,
such that:

    q.complex = complex(q.w, q.y).

There is _no_ complementary attribute to obtain q.x and q.z as a single item.

Mixed mode is also available for the ** operator.
If a is a float or integer number, then with some restrictions the following
are both provided:

     q ** a
     a ** q           -- a must be > 0.0


## <a name = "construction"/><span style='color:#00c000'>construction</span>

A Quaternion number may be constructed using one of the following forms:

* Quaternion ()                                     -> quaternion zero
* Quaternion (w[, x[, y[, z]]])                     -> quaternion number
* Quaternion (real=float,imag=(float,float,float))  -> quaternion number
* Quaternion (angle=float,axis=(float,float,float)) -> quaternion rotation number
* Quaternion (number)                               -> quaternion number
* Quaternion ("string representation")              -> quaternion number

A Quaternion number may be created from:

a) the real part and an optional imaginary parts. w, x, y and z must be float
   or number types which can be converted to float;

b) from a real number and a 3-tuple vector;

c) from an angle (radians) and a 3-tuple axis of rotation (which is automatically
   normalised), which generates a rotator Quaternion, which can then be used in
   conjunction with the rotate method;

d) from a single number parameter: int, float, complex or another Quaternion.
   When the number is complex, the imaginary part of the complex number is
   assigned to the j imaginary part; or

e) from the string representation of a Quaternion (modeled on the complex type).
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


## <a name = "attributes"/><span style='color:#00c000'>attributes</span>

* w       - float - real/scalar part
* x       - float - i imaginary part
* y       - float - j imaginary part
* z       - float - k imaginary part
* vector  - tuple - the tuple (x, y, z)
* complex - complex - the complex number w + y.j
* real    - float - real/scalar part
* imag    - tuple - the imaginary part, the same as vector.


## <a name = "instfuncs"/><span style='color:#00c000'>instance functions</span>

### <span style='color:#00c000'>conjugate</span>

q.conjugate() returns the Quaternion conjugate of q.

### <span style='color:#00c000'>inverse</span>

q.inverse ()  returns s such that: s \* q = q \* s = 1

### <span style='color:#00c000'>normalise</span>

q.normalise () returns s such that: s = q / abs (q)

### <span style='color:#00c000'>quadrance</span>

q.quadrance () returns s such that s = q.w\*q.w + q.x\*q.x + q.y\*q.y + q.z\*q.z

### <span style='color:#00c000'>rotate</span>

q.rotate (point, origin=None) -> point, where q is a rotation number,
i.e. q = Quaternion (angle=a,axis=(x,y,z)).
The returned value is rotated by an angle a radians about the axis (x,y,z).


## <a name = "magicfuncs"/><span style='color:#00c000'>magic functions</span>

### <span style='color:#00c000'>\_\_format\_\_</span>

q.\_\_format\_\_ (fmtstr) -> str

Format to a string according to format_spec. This allows, for example:

    q = Quaternion(...)
    s1 = "... {p:20.2f} ...".format(p=q)
    s2 = f"... {q:20.2f} ..."

### <span style='color:#00c000'>\_\_getnewargs\_\_</span>

q.\_\_getnewargs\_\_ () returns a 4-tuple s, such that s = (q.w, q.x, q.y, q.z)

This allows Quaternion numbers to be pickled and unpickled, and hence used
with jsonpickle.

### <span style='color:#00c000'>\_\_round\_\_</span>

q.\_\_round\_\_ (ndigits=0) returns a Quaternion with each component rounded,
e.g. round (q.w, ndigits).
While the method can be called directly, one would normally invoke

    round(q)
    round(q, n)

This is the equivalent of round (float, [ndigits]), and ndigits may be either
positive of negative. This is perhaps most useful as an alternative to
using format when printing Quaternion, e.g.

    print("result : %s" % round(q,2))


## <a name = "mathfuncs"/><span style='color:#00c000'>math functions</span>

A number of math functions that operate on Quaternions are also provided.
Most of these functions provide the equivalent quaternion function as the
functions of the same name out of the math and/or cmath module.

The functions provided are:

    isfinite
    isinf
    isnan
    isclose
    dot
    sqrt
    exp
    log
    log10
    cos
    sin
    tan
    acos
    asin
    atan
    cosh
    sinh
    tanh
    acosh
    asinh
    atanh
    polar
    phase
    axis
    rect
    angle
    rotation_matrix

Note: there is no separate qmath module.

## <a name = "variables"/><span style='color:#00c000'>module variables</span>

* one = Quaternion (1.0, 0.0, 0.0, 0.0)
* i   = Quaternion (0.0, 1.0, 0.0, 0.0)
* j   = Quaternion (0.0, 0.0, 1.0, 0.0)
* k   = Quaternion (0.0, 0.0, 0.0, 1.0)
* e   = 2.718281828459045 - float
* pi  = 3.141592653589793 - float
* tau = 6.283185307179586 - float
* \_\_version\_\_ = the version number as str.


## <a name = "hash"/><span style='color:#00c000'>hash function</span>

The hash of a quaternion follows the ideas used in the complex hash function such
that if q = Quaternion (q.complex) then hash(q) = hash (q.complex), and
if q = Quaternion (q.real) then hash(q) = hash (q.real)

## <a name = "background"/><span style='color:#00c000'>background</span>

This was initially developed more of an experiment to create a Python
extension written in C that was a bit more challenging than just a
"hello world" extension.

Although there are already a number of Quaternion Python implementations out
there, this has the advantage of speed over the pure Python implementations
and the advantage of no dependencies on any other modules such as numpy.

## <a name = "references"/><span style='color:#00c000'>references</span>

* http://onlinelibrary.wiley.com/doi/10.1002/9780470682906.app4/pdf
* https://www.geometrictools.com/Documentation/Quaternions.pdf
* https://en.wikipedia.org/wiki/Quaternion

## <a name = "credits"/><span style='color:#00c000'>credits</span>

Guidance from https://docs.python.org/3.5/extending/newtypes.html
together with cribbing many code-snippets and ideas from the complex type,
and last _but not least_ Sir William R. Hamilton.


<font size="-1">Last updated: Mon Feb 22 15:24:34 AEDT 2021</font>
<br>
