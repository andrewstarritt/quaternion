#!/usr/bin/env python
#

import math
import quaternion
from quaternion import Quaternion, zero, one, i, j, k


zero = Quaternion(0)

a  = Quaternion(1.2, -3.4, +5.6, -7.8)
ac = Quaternion(1.2, +3.4, -5.6, +7.8)
b = Quaternion(+7.8, +9.0, -1.2, -3.4)


def test_attributes():

    print(quaternion.__version__)

    assert zero.w == 0.0
    assert zero.x == 0.0
    assert zero.y == 0.0
    assert zero.z == 0.0

    assert one.w == 1.0
    assert one.x == 0.0
    assert one.y == 0.0
    assert one.z == 0.0

    assert i.w == 0.0
    assert i.x == 1.0
    assert i.y == 0.0
    assert i.z == 0.0

    assert j.w == 0.0
    assert j.x == 0.0
    assert j.y == 1.0
    assert j.z == 0.0

    assert k.w == 0.0
    assert k.x == 0.0
    assert k.y == 0.0
    assert k.z == 1.0

    assert a.w == +1.2
    assert a.x == -3.4
    assert a.y == +5.6
    assert a.z == -7.8

    assert b.w == +7.8
    assert b.x == +9.0
    assert b.y == -1.2
    assert b.z == -3.4

    assert a.imag == (-3.4, +5.6, -7.8)
    assert a.vector == (-3.4, +5.6, -7.8)
    assert a.complex == 1.2 + 5.6j

    assert b.vector == b.imag
    assert b.vector == (+9.0, -1.2, -3.4)
    assert b.complex == 7.8 - 1.2j


def test_init():
    z = Quaternion()
    assert z == zero

    a1 = Quaternion(a)
    a2 = Quaternion("1.2-3.4i+5.6j-7.8k")
    a3 = Quaternion("  (  1.2-3.4i+5.6j-7.8k  )  ")

    assert a == a1
    assert a == a2
    assert a == a3
    assert a1 == a2

    z = 11.2 - 25.6j
    zq = Quaternion(z)
    assert zq.w == z.real
    assert zq.y == z.imag

    q = Quaternion(a)
    assert q == a
    assert q is a
    assert id(q) == id(a)

    try:
        # No spaces within number
        a2 = Quaternion("1.2 -3.4i +5.6j -7.8k")
        assert False, "Expecting ValueError"
    except ValueError:
        pass
    except:
        raise

    try:
        a2 = Quaternion("1.2-3.4i+5.6j-7.8k", "")
        assert False, "Expecting TypeError"
    except TypeError:
        pass
    except:
        raise


def test_equal():
    """ Test equality and non-equality
    """
    assert zero == zero
    assert a == a
    assert b == b

    assert a != zero
    assert b != zero
    assert a != b
    assert b != a

    t = Quaternion(13.34)
    assert t == 13.34
    assert t != 23.34

    t = Quaternion(6.34, 0.0)
    assert t == 6.34
    assert t != 9.34

    t = Quaternion(-3.34, 0.0, 0.0, 0.0)
    assert t == -3.34
    assert t != +3.34

    t = Quaternion(3.34, 0.0, 8.0, 0.0)
    assert t == 3.34 + 8j
    assert t != 3.34 - 8j

    z = 11.7 - 8.9j
    t = Quaternion(z)
    assert t == z
    assert t.complex == z

    t = Quaternion(34, 0.0, 0.0, 0.0)
    assert t == 34
    assert t != 43


def test_conjugate():
    assert a.conjugate() == ac
    assert ac.conjugate() == a

    assert a.conjugate() != a
    assert a.conjugate().conjugate() == a

    d = a - a.conjugate()
    assert d.real == 0.0

    d = a + a.conjugate()
    assert d.x == 0.0
    assert d.y == 0.0
    assert d.z == 0.0

    p = (a * b).conjugate()
    q = b.conjugate() * a.conjugate()

    assert (quaternion.isclose(p, q))


def test_abs():
    assert abs(zero) == 0.0
    assert abs(one) == 1.0
    assert abs(i) == 1.0
    assert abs(j) == 1.0
    assert abs(k) == 1.0

    t = Quaternion(1.0, 1.0, 1.0, 1.0)
    assert abs(t) == 2.0

    p = abs(a) * abs(a)
    q = a * a.conjugate()
    assert math.isclose(p, q.real)
    assert abs(p - q) < 1.0e-9

    p = abs(b) * abs(b)
    q = b * b.conjugate()
    assert math.isclose(p, q.real)
    assert abs(p - q) < 1.0e-9


def test_quadrance():
    p = abs(a) * abs(a)
    q = a.quadrance()
    assert abs(p - q) < 1.0e-9

    c = a * b
    p = a.quadrance()
    q = b.quadrance()
    r = c.quadrance()
    assert abs(p * q - r) < 1.0e-9


def test_add():
    assert a == +a
    assert zero + a == a
    assert a + zero == a
    assert a + b == b + a

    # arg conversion
    #
    d = Quaternion(a.w + 7, a.x, a.y, a.z)
    assert d == a + 7
    assert d == a + 7.0
    assert d == a + (7.0 + 0j)
    assert d == a + Quaternion(7, 0, 0, 0)

    e = Quaternion(a.w + 7.3, a.x, a.y + 11.3, a.z)

    assert e == a + (7.3 + 11.3j)
    assert e == a + Quaternion(7.3, 0, 11.3, 0)


def test_sub():
    assert a == -(-a)
    assert zero - a == -a
    assert a - zero == a
    assert a - b == - (b - a)

    # arg conversion
    #
    d = Quaternion(a.w - 7, a.x, a.y, a.z)
    assert d == a - 7
    assert d == a - 7.0
    assert d == a - (7.0 + 0j)
    assert d == a - Quaternion(7, 0, 0, 0)

    e = Quaternion(a.w - 7.3, a.x, a.y - 11.3, a.z)

    assert e == a - (7.3 + 11.3j)
    assert e == a - Quaternion(7.3, 0, 11.3, 0)


def test_mul():
    assert a * one == a
    assert one * a == a
    # in general a*b != b*a

    m1 = -one

    assert i * i == m1
    assert j * j == m1
    assert k * k == m1

    assert i * j == -j * i
    assert j * k == -k * j
    assert k * i == -i * k

    assert i * j == k
    assert j * k == i
    assert k * i == j

    assert i * j * k == m1

    t = Quaternion(3.0, 0.0, 0.0, 0.0)
    assert t * a == a + a + a
    assert t * a == 3.0 * a
    assert t * a == 3 * a
    assert t * a == (3.0 + 0j) * a

    # arg conversion
    #
    d = Quaternion(a.w * 7, a.x * 7, a.y * 7, a.z * 7)
    assert d == a * 7
    assert d == a * 7.0
    assert d == a * (7.0 + 0j)
    assert d == a * Quaternion(7, 0, 0, 0)


def test_div():
    assert a / one == a
    assert (4.6 * a) / a == Quaternion(4.6)

    f = 1.3
    assert (a / f) == Quaternion(a.w / f, a.x / f, a.y / f, a.z / f)

    t = ((a / b) * b)
    assert abs(t - a) < 1.0e-9

    t = ((b / a) * a)
    assert abs(t - b) < 1.0e-9

    # arg conversion
    #
    d = Quaternion(a.w / 7, a.x / 7, a.y / 7, a.z / 7)
    assert d == a / 7
    assert d == a / 7.0
    assert d == a / (7.0 + 0j)
    assert d == a / Quaternion(7, 0, 0, 0)


def test_inverse():
    b = a.inverse()
    t = one - a * b
    assert abs(t) < 1.0e-9
    t = one - b * a
    assert abs(t) < 1.0e-9
    b = a.inverse().inverse()
    t = a - b
    assert abs(t) < 1.0e-9
    
    assert (quaternion.isclose(b*b.inverse(), one))
    assert (quaternion.isclose(b.inverse()*b, one))


def test_pow():
    assert a ** 0 == one
    assert a ** 1 == a
    assert abs(a ** 2 - (a * a)) <= 1.0e-12
    assert abs(a ** 3 - (a * a * a)) <= 1.0e-12

    t = a * a * a * a * a * a * a

    p = ((a ** 7) - t) / abs(t)
    assert abs(p) <= 1.0e-12

    t = a ** 0.5
    p = (t * t - a) / abs(a)
    assert abs(p) <= 1.0e-12

    t = a ** 0.25
    p = (t * t * t * t - a) / abs(a)
    assert abs(p) <= 1.0e-12

    r = 2.3456
    t = abs(b)**r
    s = abs(b**r)
    assert abs(t - s) <= 1.0e-12


    p = a ** r
    q = quaternion.exp (quaternion.log(a) * r)
    assert quaternion.isclose(p, q)
       
    
    p = r ** a
    q = quaternion.exp (math.log(r) * a)
    assert quaternion.isclose(p, q)
   

def test_pow2():
    assert a.w ** zero == one
    assert a.w ** one  == a.w

    t1 = a.w ** b
    t2 = quaternion.exp (math.log(a.w) * b)
    p = t1 - t2
    assert abs(p) <= 1.0e-12


def test_hash():
    n = 234
    r = float(n)
    c = complex(r)
    q = Quaternion(c)

    assert hash(q) == hash(n)
    assert hash(q) == hash(r)
    assert hash(q) == hash(c)

    r = 1.234
    c = complex(r)
    q = Quaternion(c)

    assert hash(q) == hash(r)
    assert hash(q) == hash(c)

    c = -1.234 + 0.567j
    q = Quaternion(c)
    assert hash(q) == hash(c)

    assert hash(one) != hash(i)
    assert hash(one) != hash(j)
    assert hash(one) != hash(k)

    assert hash(i) != hash(j)
    assert hash(j) != hash(k)
    assert hash(k) != hash(i)


def run_stuff():
    n = Quaternion()
    print("n", n)
    n = Quaternion(n)
    print("n", n)
    print("")

    print("i*j", i * j)
    print("j*i", j * i)

    print("s*t  ")
    print("j*k", j * k)
    print("k*j", k * j)

    print("k*i", k * i)
    print("i*k", i * k)

    print("i*j*k", i * j * k)

    print("")
    p = Quaternion()
    print("p", p)

    p = 1.2 + 3 * i + 4.6 * j - 7.89 * k
    print("p", p, p.w, p.x, p.y, p.z)

    q = Quaternion(p)
    print("q", q)

    print("Inverse")
    s = Quaternion(0.12, 0.23, 0.34, 0.45)
    print("s      ", s)

    t = Quaternion(1.0) / s
    print("t (1/s)", t)

    print("s*t    ", s * t)
    print("t*s    ", t * s)

    print("")

    s = Quaternion(1, 2, 3, 4)
    print("s", s)

    s = Quaternion(True, False, True, False)
    print("s", s)

    s = Quaternion(1.2, 2.3, 3.4)
    print("s", s)

    s = Quaternion(1.2, 2.3)
    print("s", s)

    s = Quaternion(1.2)
    print("s", s)

    s = Quaternion(x=2.3)
    print("s", s)

    s = Quaternion(y=3.4)
    print("s", s)

    s = Quaternion(z=4.5)
    print("s", s)

    print("")
    z = 113.0 + 355.0j
    print("z", z)

    s = Quaternion(z)
    print("s", s, s.complex)

    print("")
    r = Quaternion("2.4")
    s = Quaternion("  2.4  ")
    t = Quaternion(" (  2.4  ) ")
    print("rst", r, s, t)

    t = Quaternion("2.4+5i")
    print("t", t)
    t = Quaternion("2.4-6j")
    print("t", t)
    t = Quaternion("2.4+7k")
    print("t", t)

    print("t", t)
    t = Quaternion("+5i")
    print("t", t)
    t = Quaternion("-6j")
    print("t", t)
    t = Quaternion("+7.0k")
    print("t", t)

    t = Quaternion("1-2i+3j-4k")
    print("t", t)

    t1 = complex("1+3j")
    t2 = Quaternion("1+3j")
    print("t", t1, t2, t1 == t2)
    print("")


if __name__ == "__main__":
    test_attributes()
    test_init()
    test_equal()
    test_conjugate()
    test_quadrance()
    test_abs()
    test_add()
    test_sub()
    test_mul()
    test_div()
    test_pow()
    test_pow2()
    test_hash()
#   run_stuff()

# end
