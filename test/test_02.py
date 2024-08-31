#!/usr/bin/env python
#

import quaternion
from quaternion import Quaternion
import math
import cmath


qlist = (Quaternion(+0.16, +0.32, +1.48, +0.80),
         Quaternion(+1.16, +1.32, +1.48, -0.80),
         Quaternion(+2.16, +0.00, -0.01, +0.00),
         Quaternion(+3.16, +0.32, -1.48, -2.80),
         Quaternion(+4.16, -2.32, +1.48, +0.80),
         Quaternion(+0.16, -0.32, +1.48, -0.80),
         Quaternion(+6.16, -3.32, -1.48, +3.80),
         Quaternion(+7.16, -0.32, -3.48, -0.80),
         Quaternion(+0.16, +0.32, +1.48, +4.80))


def qfoo(q):
    """ Rotate axis by +tau/3 about axis (1,1,1) """
    return Quaternion(q.w, q.y, q.z, q.x)


def qbar(q):
    """ Rotate axis by -tau/3 about axis (1,1,1) """
    return Quaternion(q.w, q.z, q.x, q.y)


def check(rfn, cfn, qfn, q):
    """ compares the Quaternion function (qfn) with the equivilent
        real (rfn) and complex (cfn) functions
    """
    a = Quaternion(rfn(q.real))
    b = qfn(Quaternion(q.real))
    assert quaternion.isclose(a, b)

    a = Quaternion(cfn(q.complex))
    b = qfn(Quaternion(q.complex))
    assert quaternion.isclose(a, b)

    # For single value functions, we can create a z with same r and phi as the q
    # Call the complex function, then create a Quaternion with f(r, phi) of the
    # complex result merged in with the original axis.
    #
    q_polar = quaternion.polar(q)
    z = cmath.rect(q_polar[0], q_polar[1])
    fz = cfn(z)
    fz_polar = cmath.polar(fz)
    fi = quaternion.rect(fz_polar[0], fz_polar[1], q_polar[2])
    fd = qfn(q)
    assert quaternion.isclose(fi, fd)

    # Test cut'n'paste errors referenceing imaginary components
    # by rotating the axis,
    #
    a = qfn(qfoo(q))
    b = qfoo(qfn(q))
    assert quaternion.isclose(a, b)

    a = qfn(qbar(q))
    b = qbar(qfn(q))
    assert quaternion.isclose(a, b)


def test_isclose():
    print("test_isclose")
    a = Quaternion(1.23e10)
    b = a + 0.001

    assert a != b
    assert quaternion.isclose(a, b)
    assert quaternion.isclose(a, a)


def test_exp():
    print("test_exp")
    for q in qlist:
        # Do special for the zeroth term
        #
        t = quaternion.one
        terms = [t]
        for j in range(1, 1000):
            t = t * q / j
            terms.append(t)
            if abs(t) < 1.0E-200:
                break

        # Add in reverse - smallest terms first
        #
        s = Quaternion(0.0)
        m = len(terms) - 1
        for j in range(m, -1, -1):
            s += terms[j]

        r = quaternion.exp(q)
        assert quaternion.isclose(r, s)


def test_log():
    print("test_log")
    for q in qlist:
        check(math.log, cmath.log, quaternion.log, q)


def test_log10():
    print("test_log10")
    for q in qlist:
        check(math.log10, cmath.log10, quaternion.log10, q)


def test_log_exp():
    print("test_log_exp")
    for a in qlist:
        b = quaternion.log(a)
        c = quaternion.exp(b)
        assert abs(a - c) < 1.0e-9


def test_sqrt():
    print("test_sqrt")
    a = Quaternion(4.0, 0.0, 0.0, 0.0)
    b = Quaternion(2.0, 0.0, 0.0, 0.0)

    c = quaternion.sqrt(a)
    assert abs(b - c) < 1.0e-9

    a = Quaternion(1.23, 4.56, -7.89, 2.456)
    b = quaternion.sqrt(a)
    c = b * b
    assert abs(a - c) < 1.0e-9

    for q in qlist:
        a = q * q
        b = quaternion.sqrt(a)
        assert quaternion.isclose(q, b)

        a = quaternion.sqrt(q)
        b = a * a
        assert quaternion.isclose(q, b)


def test_sin():
    print("test_sin")
    for q in qlist:
        check(math.sin, cmath.sin, quaternion.sin, q)

        # Do special for the zeroth term
        #
        t = q
        terms = [t]
        for j in range(3, 1000, 2):
            t = - t * q * q / (float(j) * (j - 1))
            terms.append(t)
            if abs(t) < 1.0E-200:
                break

        # Add in reverse - smallest terms first
        #
        s = Quaternion(0.0)
        m = len(terms) - 1
        for j in range(m, -1, -1):
            s += terms[j]

        r = quaternion.sin(q)
        assert quaternion.isclose(r, s)


def test_cos():
    print("test_cos")
    for q in qlist:
        check(math.cos, cmath.cos, quaternion.cos, q)

        # Do special for the zeroth term
        #
        t = quaternion.one
        terms = [t]
        for j in range(2, 1000, 2):
            t = - t * q * q / (float(j) * (j - 1))
            terms.append(t)
            if abs(t) < 1.0E-200:
                break

        # Add in reverse - smallest terms first
        #
        s = Quaternion(0.0)
        m = len(terms) - 1
        for j in range(m, -1, -1):
            s += terms[j]

        r = quaternion.cos(q)
        assert quaternion.isclose(r, s)


def test_tan():
    print("test_tan")
    for q in qlist:
        s = quaternion.sin(q)
        c = quaternion.cos(q)
        t = s / c

        r = quaternion.tan(q)
        assert quaternion.isclose(r, t)


def test_asin():
    print("test_asin")
    for q in qlist:
        q = q / 10.0
        check(math.asin, cmath.asin, quaternion.asin, q)


def test_acos():
    print("test_acos")
    for q in qlist:
        q = q / 10.0
        check(math.acos, cmath.acos, quaternion.acos, q)


def test_atan():
    print("test_atan")
    for q in qlist:
        q = q / 10.0
        check(math.atan, cmath.atan, quaternion.atan, q)


def test_sinh():
    print("test_sinh")
    for q in qlist:
        check(math.sinh, cmath.sinh, quaternion.sinh, q)


def test_cosh():
    print("test_cosh")
    for q in qlist:
        check(math.cosh, cmath.cosh, quaternion.cosh, q)


def test_tanh():
    print("test_tanh")
    for q in qlist:
        check(math.tanh, cmath.tanh, quaternion.tanh, q)


def test_asinh():
    print("test_asinh")
    for q in qlist:
        q = q / 10.0
        check(math.asinh, cmath.asinh, quaternion.asinh, q)


def test_acosh():
    print("test_acosh")
    for q in qlist:
        q = q + 1.0
        check(math.acosh, cmath.acosh, quaternion.acosh, q)


def test_atanh():
    print("test_atanh")
    for q in qlist:
        q = q / 10.0
        check(math.atanh, cmath.atanh, quaternion.atanh, q)


def test_dot1():
    print("test_dot1")
    b = Quaternion(+17.16, -1.32, -1.48, -2.80)
    for a in qlist:
        u = quaternion.dot(a, a)
        v = a.quadrance()
        assert abs(u - v) < 1.0e-15

        u = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z
        v = quaternion.dot(a, b)
        assert abs(u - v) < 1.0e-15

        u = quaternion.dot(b, a)
        v = quaternion.dot(a, b)
        assert abs(u - v) < 1.0e-15


def test_dot2():
    print("test_dot2")
    b = Quaternion(+17.16, -1.32, -1.48, -2.80)
    for a in qlist:
        u = a @ a
        v = a.quadrance()
        assert abs(u - v) < 1.0e-15

        u = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z
        v = a @ b
        assert abs(u - v) < 1.0e-15

        u = b @ a
        v = a @ b
        assert abs(u - v) < 1.0e-15


def test_pythagoras():
    print("test_pythagoras")
    for a in qlist:
        u = quaternion.sin(a)**2 + quaternion.cos(a)**2
        v = abs(u)
        assert v - 1.0 < 1.0e-10


def test_polar_rect():
    print("test_polar_rect")
    for a in qlist:
        t = quaternion.polar(a)
        c = quaternion.rect(*t)
        assert abs(a - c) < 1.0e-10


def test_lerp():
    print("test_lerp")
    a = Quaternion(+3.16, -1.32, -1.48, -2.80)
    b = Quaternion(-1.32, -3.48, -2.80, 3.142)

    c = quaternion.lerp(a, b, 0.5)
    d = (a + b) / 2
    e = abs(c - d)
    assert e < 1.0e-15

    c = quaternion.lerp(a, b, 0.75)
    d = (a + 3 * b) / 4
    e = abs(c - d)
    assert e < 1.0e-15

    c = quaternion.lerp(a, -b, 0.9)
    d = (a - 9 * b) / 10
    e = abs(c - d)
    assert e < 1.0e-15


if __name__ == "__main__":
    test_isclose()
    test_exp()
    test_log()
    test_log10()
    test_log_exp()
    test_sqrt()
    test_sin()
    test_cos()
    test_tan()
    test_asin()
    test_acos()
    test_atan()
    test_sinh()
    test_cosh()
    test_tanh()
    test_asinh()
    test_acosh()
    test_atanh()
    test_dot1()
    test_dot2()
    test_pythagoras()
    test_polar_rect()
    test_lerp()

# end
