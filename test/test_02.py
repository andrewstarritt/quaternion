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
    a = Quaternion(1.23e10)
    b = a + 0.001

    assert a != b
    assert quaternion.isclose(a, b)
    assert quaternion.isclose(a, a)


def test_exp():
    for q in qlist:
        check(math.exp, cmath.exp, quaternion.exp, q)


def test_log():
    for q in qlist:
        check(math.log, cmath.log, quaternion.log, q)


def test_log10():
    for q in qlist:
        check(math.log10, cmath.log10, quaternion.log10, q)


def test_log_exp():
    for a in qlist:
        b = quaternion.log(a)
        c = quaternion.exp(b)
        assert abs(a - c) < 1.0e-9


def test_sqrt():
    a = Quaternion(4.0, 0.0, 0.0, 0.0)
    b = Quaternion(2.0, 0.0, 0.0, 0.0)

    c = quaternion.sqrt(a)
    assert abs(b - c) < 1.0e-9

    a = Quaternion(1.23, 4.56, -7.89, 2.456)
    b = quaternion.sqrt(a)
    c = b * b
    assert abs(a - c) < 1.0e-9

    for q in qlist:
        check(math.sqrt, cmath.sqrt, quaternion.sqrt, q)


def test_sin():
    for q in qlist:
        check(math.sin, cmath.sin, quaternion.sin, q)


def test_cos():
    for q in qlist:
        check(math.cos, cmath.cos, quaternion.cos, q)


def test_tan():
    for q in qlist:
        check(math.tan, cmath.tan, quaternion.tan, q)


def test_asin():
    for q in qlist:
        q = q / 10.0
        check(math.asin, cmath.asin, quaternion.asin, q)


def test_acos():
    for q in qlist:
        q = q / 10.0
        check(math.acos, cmath.acos, quaternion.acos, q)


def test_atan():
    for q in qlist:
        q = q / 10.0
        check(math.atan, cmath.atan, quaternion.atan, q)


def test_sinh():
    for q in qlist:
        check(math.sinh, cmath.sinh, quaternion.sinh, q)


def test_cosh():
    for q in qlist:
        check(math.cosh, cmath.cosh, quaternion.cosh, q)


def test_tanh():
    for q in qlist:
        check(math.tanh, cmath.tanh, quaternion.tanh, q)


def test_asinh():
    for q in qlist:
        q = q / 10.0
        check(math.asinh, cmath.asinh, quaternion.asinh, q)


def test_acosh():
    for q in qlist:
        q = q + 1.0
        check(math.acosh, cmath.acosh, quaternion.acosh, q)


def test_atanh():
    for q in qlist:
        q = q / 10.0
        check(math.atanh, cmath.atanh, quaternion.atanh, q)


def test_pythagoras():
    for a in qlist:
        u = quaternion.sin(a)**2 + quaternion.cos(a)**2
        v = abs(u)
        assert v - 1.0 < 1.0e-3


def test_polar_rect():
    for a in qlist:
        t = quaternion.polar(a)
        c = quaternion.rect(*t)
        assert abs(a - c) < 1.0e-9


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
    test_pythagoras()
    test_polar_rect()

# end
