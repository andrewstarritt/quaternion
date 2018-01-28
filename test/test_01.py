#!/usr/bin/env python
#

import math
from quaternion import Quaternion

zero = Quaternion(0)
one = Quaternion(1)
i = Quaternion(0,i=1)
j = Quaternion(0,j=1)
k = Quaternion(0,k=1)

a = Quaternion(1.2, -3.4, +5.6, -7.8)
ac = Quaternion(1.2, +3.4, -5.6, +7.8)
b = Quaternion(+7.8, +9.0, -1.2, -3.4)


def test_attributes():

    assert zero.r == 0.0
    assert zero.i == 0.0
    assert zero.j == 0.0
    assert zero.k == 0.0

    assert one.r == 1.0
    assert one.i == 0.0
    assert one.j == 0.0
    assert one.k == 0.0

    assert a.r == +1.2
    assert a.i == -3.4
    assert a.j == +5.6
    assert a.k == -7.8

    assert b.r == +7.8
    assert b.i == +9.0
    assert b.j == -1.2
    assert b.k == -3.4

    assert a.axis == (-3.4, +5.6, -7.8)
    assert a.complex == 1.2 + 5.6j

    assert b.axis == (+9.0, -1.2, -3.4)
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
    assert zq.r == z.real
    assert zq.j == z.imag

    try:
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
    """ Test qeuality and non-equality
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

    t = Quaternion(6.34, 0.0)
    assert t == 6.34

    t = Quaternion(-3.34, 0.0, 0.0, 0.0)
    assert t == -3.34

    t = Quaternion(3.34, 0.0, 8.0, 0.0)
    assert t == 3.34 + 8j

    z = 11.7 - 8.9j
    t = Quaternion(z)
    assert t == z
    assert t.complex == z

    t = Quaternion(34, 0.0, 0.0, 0.0)
    assert t == 34


def test_conjugate():
    assert a.conjugate() == ac
    assert ac.conjugate() == a

    assert a.conjugate() != a
    assert a.conjugate().conjugate() == a

    d = a - a.conjugate()
    assert d.r == 0.0

    d = a + a.conjugate()
    assert d.i == 0.0
    assert d.j == 0.0
    assert d.k == 0.0


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
    assert math.isclose (p, q.r)
    assert abs (p - q) < 1.0e-9
    
    p = abs(b) * abs(b)
    q = b * b.conjugate()
    assert math.isclose (p, q.r)
    assert abs (p - q) < 1.0e-9
    
   

def test_add():
    assert a == +a
    assert zero + a == a
    assert a + zero == a
    assert a + b == b + a
    
    # arg conversion
    #
    d = Quaternion (a.r + 7, a.i, a.j, a.k)
    assert d == a + 7
    assert d == a + 7.0
    assert d == a + (7.0+0j)
    assert d == a +  Quaternion (7, 0, 0, 0)
    
    e = Quaternion (a.r + 7.3, a.i, a.j + 11.3, a.k)

    assert e == a + (7.3+11.3j)
    assert e == a +  Quaternion (7.3, 0, 11.3, 0)


def test_sub():
    assert a == -(-a)
    assert zero - a == -a
    assert a - zero == a
    assert a - b == - (b - a)

    # arg conversion
    #
    d = Quaternion (a.r - 7, a.i, a.j, a.k)
    assert d == a - 7
    assert d == a - 7.0
    assert d == a - (7.0+0j)
    assert d == a -  Quaternion (7, 0, 0, 0)
    
    e = Quaternion (a.r - 7.3, a.i, a.j - 11.3, a.k)

    assert e == a - (7.3+11.3j)
    assert e == a -  Quaternion (7.3, 0, 11.3, 0)


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

    t = Quaternion(3.0, 0.0, 0.0, 0.0)
    assert t * a == a + a + a
    assert t * a == 3.0 * a
    assert t * a == 3 * a
    assert t * a == (3.0 + 0j) * a
    
    # arg conversion
    #
    d = Quaternion (a.r * 7, a.i * 7, a.j * 7, a.k * 7)
    assert d == a * 7
    assert d == a * 7.0
    assert d == a * (7.0+0j)
    assert d == a *  Quaternion (7, 0, 0, 0)
    
    

def test_div():
    assert a / one == a
    assert (4.6 * a) / a == Quaternion(4.6)

    f = 1.3
    assert (a / f) == Quaternion(a.r / f, a.i / f, a.j / f, a.k / f)

    t = ((a / b) * b)
    assert abs (t - a) < 1.0e-9

    t = ((b / a) * a)
    assert abs (t - b) < 1.0e-9

    # arg conversion
    #
    d = Quaternion (a.r / 7, a.i / 7, a.j / 7, a.k / 7)
    assert d == a / 7
    assert d == a / 7.0
    assert d == a / (7.0+0j)
    assert d == a /  Quaternion (7, 0, 0, 0)
    

def test_pow():
    assert a ** zero == one
    assert a ** one == a
    assert a ** 2 == a * a
    assert a ** 2 == a * a

    t = a * a * a * a * a * a * a

    p = ((a ** 7) - t) / abs(t)
    assert abs(p) <= 1.0e-12

    t = a ** 0.5
    p = (t * t - a) / abs(a)
    assert abs(p) <= 1.0e-12

    t = a ** 0.25
    p = (t * t * t * t - a) / abs(a)
    assert abs(p) <= 1.0e-12

    e = Quaternion(2.718281828459045)
    pi = Quaternion(3.141592653589793)

    t = e ** (-i * pi) + one
    assert abs(t) <= 1.0e-12

    t = e ** (-j * pi) + one
    assert abs(t) <= 1.0e-12

    t = e ** (-k * pi) + one
    assert abs(t) <= 1.0e-12
    
def test_hash():
    n = 234
    r = float (n)
    c = complex (r)
    q = Quaternion (c)
    
    assert hash(q) == hash (n)
    assert hash(q) == hash (r)
    assert hash(q) == hash (c)

    r = 1.234
    c = complex (r)
    q = Quaternion (c)
    
    assert hash(q) == hash (r)
    assert hash(q) == hash (c)
    
    c = -1.234 + 0.567j
    q = Quaternion (c)
    assert hash(q) == hash (c)
    
    assert hash (one) != hash (i)
    assert hash (one) != hash (j)
    assert hash (one) != hash (k)
    
    assert hash (i) != hash (j)
    assert hash (j) != hash (k)
    assert hash (k) != hash (i)
    


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
    print("p", p, p.r, p.i, p.j, p.k)

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

    s = Quaternion(i=2.3)
    print("s", s)

    s = Quaternion(j=3.4)
    print("s", s)

    s = Quaternion(k=4.5)
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
    test_abs()
    test_add()
    test_sub()
    test_mul()
    test_div()
    test_pow()
    test_hash()        
    run_stuff()

# end
