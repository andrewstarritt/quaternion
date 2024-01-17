# <span style='color:#00c000'>quaternion</span>

A Python extension to provide a Quaternion type and some associated math functions
together with a QuaternionArray object.

[general](#general)<br>
[mathematical operations](#mathops)<br>
[mixed mode arithmetic](#mixedmode)<br>
[construction](#construction)<br>
[attributes](#attributes)<br>
[instance functions](#instfuncs)<br>
[static functions](#static_funcs)<br>
[magic functions](#magicfuncs)<br>
[maths functions](#mathsfuncs)<br>
[module variables](#variables)<br>
[hash function](#hash)<br>
[rotation matrices](#rot_mat)<br>
[quarternion arrays](#qnarray)<br>
[miscellaneous](#miscellaneous)<br>
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

The following mathematical operations are provided.

unary: +, -, abs

binary: +, -, \*, /

power: \*\*

There is no mod (%) or integer division (//) operation available.
Therefore only the two argument version of the pow() function is available -
 see below.

The Quaternion type is associative under both addition and multiplication, i.e.:

    (p + q) + r  =  p + (q + r)
    (p * q) * r  =  p * (q * r)

The Quaternion type is also distributive:

    p * (q + r)  =  p*q + p*r

The Quaternion type is non-commutative with respect to multiplication and division,
i.e. in general p \* q  and q \* p result in different values.
To divide one Quaternion by another, there are two possible options:

    p * q.inverse() ; or
    q.inverse() * p.

The quotient function uses p * q.inverse(), therefore:

    (p / q) * q = p

This non-commutative nature also explains why p ** q is undefined,
as this could implemented as:

    exp (q * log (p))  ; or
    exp (log (p) * q)

However, mixed-mode ** is possible - please see below.

## <a name = "mixedmode"/><span style='color:#00c000'>mixed mode arithmetic</span>

Quaternions numbers and scalar numbers, i.e. int or float, are inter-operable.
int and float numbers are treated as Quaternions with zero imaginary components.
Note: float numbers (a) and Quaternion numbers (q) do commute under
multiplication and division:

    q * a = a * q
    q / a = (1/a) * q
    a / q  = q.inverse() * a

Mixed mode with complex numbers is also allowed. A complex number, z, is treated
as a Quaternions, q, such that q.w = z.real, q.y = z.imag, and q.x and q.z are
zero.

The choice of aligning the imaginary part of a complex number to the j imaginary
part as opposed to i or k imaginary parts is mathematically arbitrary.
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
    a ** q        -- a must be > 0.0


## <a name = "construction"/><span style='color:#00c000'>construction</span>

A Quaternion number may be constructed using one of the following forms:

* Quaternion ()                                      -> quaternion zero
* Quaternion (w[, x[, y[, z]]])                      -> quaternion number
* Quaternion (real=float,imag=(float,float,float))   -> quaternion number
* Quaternion (angle=float,axis=(float,float,float))  -> quaternion rotation number
* Quaternion (number)                                -> quaternion number
* Quaternion ("string representation")               -> quaternion number
* Quaternion (matrix=3x3 nested iterator of numerics) -> quaternion number

A Quaternion number may be created from:

a) the real part and an optional imaginary parts. w, x, y and z must be float
   or number types which can be converted to float;

b) from a real number and a 3-tuple vector;

c) from an angle (radians) and a 3-tuple axis of rotation (which is automatically
   normalised), which generates a rotator Quaternion, which can then be used in
   conjunction with the rotate method;

d) from a single number parameter: int, float, complex or another Quaternion.
   When the number is complex, the imaginary part of the complex number is
   assigned to the j imaginary part;

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

f) from a 3x3 matrix of floats (and/or float-able objects). The matrix must be
   a triplet iterator with each item itself being a triplet iterator of float
   or floatable objects.
   The matrix should ideally be a rotation matrix, i.e. the determinent should
   be 1, however no attempt is made to check this nor is any attempt made to
   normalise the matrix.
   The resulting quaternion may be subsequently normalised or reconstructed from the rotation angle and axis.


## <a name = "attributes"/><span style='color:#00c000'>attributes</span>

* w       - float - real/scalar part
* x       - float - i imaginary part
* y       - float - j imaginary part
* z       - float - k imaginary part
* vector  - tuple - the tuple (x, y, z)
* complex - complex - the complex number w + y.j
* real    - float - real/scalar part
* imag    - tuple - the imaginary part, the same as vector.
* data    - tuple - the raw data as a tuple (w, x, y, z).


## <a name = "instfuncs"/><span style='color:#00c000'>instance functions</span>

### <span style='color:#00c000'>conjugate</span>

q.conjugate() returns the Quaternion conjugate of q.

### <span style='color:#00c000'>inverse</span>

q.inverse ()  returns s such that: s \* q = q \* s = 1

### <span style='color:#00c000'>normalise</span>

q.normalise () returns s such that: s = q / abs (q)

### <span style='color:#00c000'>quadrance</span>

q.quadrance () returns s such that s = q.w\*q.w + q.x\*q.x + q.y\*q.y + q.z\*q.z

### <span style='color:#00c000'>matrix</span>

q.matrix () returns a 3-tuple of 3-tuple of floats representing
the 3x3 rotation matrix equivalent of q.
q should be a rotation quaternion.

### <span style='color:#00c000'>angle</span>

q.angle () returns the angle (float, in radians) of q.
q should be a rotation quaternion.
This method may raise a ValueError if q is not a rotation quaternion.

This should not be confused with the polor co-ordinate form phase
(aka argument) angle.

### <span style='color:#00c000'>angle</span>

q.axis () returns the normalised axis of q, where q should be a rotation
quaternion.
This is essentially identical to the maths function Quaternion.axis (q)

### <span style='color:#00c000'>rotate</span>

q.rotate (point, origin=None) -> point, where q is a rotation number,
i.e. q = Quaternion (angle=a,axis=(x,y,z)).
The returned value is rotated by an angle a radians about the axis (x,y,z).

## <a name = "static_funcs"/><span style='color:#00c000'>static functions</span>

These are the equivilent of "@staticmethod" functions

### <span style='color:#00c000'>brief</span>

Quaternion.brief() modifies the behaviour of the \_\_repr\_\_ function.
Within interactive python/ipython we have:

    In [2]: q = quaternion.Quaternion (1,0,1,0)
    In [3]: q
    Out[3]: quaternion.Quaternion(1, +0, +1, +0)

and after invoking brief we have:

    In [4]: quaternion.Quaternion.brief()
    In [5]: q
    Out[5]: (1+0i+1j+0k)

This also impacts how lists ,tuples, dictionarys etc. that contain quaternions
are converted to str and printed.

### <span style='color:#00c000'>reset</span>

Quaternion.reset() un-does the  \_\_repr\_\_ function behaviour modification
instigated by the call to brief().
I might change this name.

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


## <a name = "mathsfuncs"/><span style='color:#00c000'>maths functions</span>

A number of maths functions that operate on Quaternions are also provided.
Where these function have the same name as a function out of the math and/or
cmath modules, the function provides the equivalent operation on a quaternion
or pair of quaternions.

The functions provided are:

    acos
    acosh
    asin
    asinh
    atan
    atanh
    axis
    cos
    cosh
    dot
    exp
    isclose
    isfinite
    isinf
    isnan
    log
    log10
    phase
    polar
    rect
    sin
    sinh
    slerp   - since 1.3.4
    sqrt
    tan
    tanh

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

## <a name = "rot_mat"/><span style='color:#00c000'>rotation matrices</span>

If quaternion number, q,  is a rotation quaternion, then q.matrix() function
can be used to obtain the equivilent 3x3 rotation matrix.
A rotation quaternion is one who's length is 1, and is typically created
using:

    q = Quaternion(angle=..., axis=(..., ..., ...))

If abs(q) is not 1, there is no obvious interpretation of the generated
matrix.
Likewise a 3x3 rotation matrix, A, may be used to create the equivilent quaternion:

    q = Quaternion(matix=((A11,A12,A13), (A21,A22,A23), (A31,A32,A33)))

However, if A is not a rotation matrix, i.e. det(A) != 1, then the
quaternion will have no obvious interpretation  with respect to the
matrix.

Matrices have 9 degrees of freedom while quaternions have only 4, and rotation
quaternions have effectivly only 3.
This is why only rotation matrices can be sensibly converted to a
meaningfull quaternion number.

__Note:__ neither the Quaternion(matrix=...) constructor nor the matrix()
method attempt to validate or normalise the input values.
They just run the algorithms <span style='color:#4060A0'>__"AS IS"__</span>.


## <a name = "qnarray"/><span style='color:#00c000'>quaternon array</span>

Release 1.3 and later sees the introduction of a QuaternionArray type.
The API and behaviour of this class has been implemented to mimic as
far as resonabley possbile the API and behaviour of the inbuilt
 __array.array__ type, save that it only holds quaternions.

### <span style='color:#00c000'>additional methods/attributes</span>

The QuaternionArray class provides a number of additonal methods:
- clear() - this is equivilent to the list class clear() method;
- reserve(int) - this method allows the minimum space allocated
  for the array to be specified (expressed in number of quaterions,
  __not__ number of bytes).

The QuaternionArray class also provides two additional attributes:
- allocated - provides the allocated memory size
  (expressed in quaterions);
- reserved - provides the minimum allocated memory size
  (expressed in quaterions)

### <span style='color:#00c000'>missing methods/attributes</span>

There is (currently) no equivilent of the fromlist and tolist methods.
The extend method provides essentially the same functionality as the
fromlist method (with no argument restriction).
The fuctionality of &nbsp; _array.tolist()_ &nbsp; can be acheived by
calling &nbsp; _list(array)_.

## <a name = "miscellaneous"/><span style='color:#00c000'>miscellaneous</span>

### <span style='color:#00c000'>pickle</span>

Both the Quaternion and QuaternionArray types support pickle.
Example:

    q = Quaternion(1,2,3,4)
    s = pickle.dumps(q)
    print(s)
    b'\x80\x04\x95F\x00\x00\x00\x00\x00\x00\x00\x8c\nquaternion\x94\x8c\n   \
      Quaternion\x94\x93\x94(G?\xf0\x00\x00\x00\x00\x00\x00G@\x00\x00\x00   \
      \x00\x00\x00\x00G@\x08\x00\x00\x00\x00\x00\x00G@\x10\x00\x00\x00\x00  \
      \x00\x00t\x94\x81\x94.'
    p = pickle.loads(s)
    print(p == q)
    True

### <span style='color:#00c000'>jsonpickle</span>

While jsonpickle seems to work with Quaternions, there are issues with
QuaternionArrays.

### <span style='color:#00c000'>buffer API</span>

Both the Quaternion and QuaternionArray types support the (read only)
buffer API, i.e may be supplied as a parameter to bytes and bytearray.
Examples:


    q = Quaternion(1.111, 2.222, 3.333, 4.444)
    print(bytes(q))
    b'-\xb2\x9d\xef\xa7\xc6\xf1?-\xb2\x9d\xef\xa7\xc6\x01@D\x8bl    \
    \xe7\xfb\xa9\n@-\xb2\x9d\xef\xa7\xc6\x11@'

    a = QuaternionArray( ... )
    b = bytearray(a)
    print(b == a.tobytes())
    True

Note: __a.tobytes()__ is effectively idential to __bytes(a)__.


## <a name = "background"/><span style='color:#00c000'>background</span>

This was initially developed more or less as an experiment to create a Python
extension written in C that was a bit more challenging than just a simple
"hello world" extension.

Although there are already a number of Quaternion Python implementations out
there, this has the advantages of speed over the pure Python implementations
together with no dependencies on any other modules such as numpy.

## <a name = "references"/><span style='color:#00c000'>references</span>

* http://onlinelibrary.wiley.com/doi/10.1002/9780470682906.app4/pdf
* https://www.geometrictools.com/Documentation/Quaternions.pdf
* https://en.wikipedia.org/wiki/Quaternion

## <a name = "credits"/><span style='color:#00c000'>credits</span>

Guidance from https://docs.python.org/3.5/extending/newtypes.html
together with cribbing many code-snippets and ideas from the complex type
and the array.array type; and last _but not least_ Sir William R. Hamilton.


<font size="-1">Last updated: Sun Jan 14 14:18:07 AEDT 2024</font>
<br>
