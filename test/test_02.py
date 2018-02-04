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
    """ Rotate axis by +tau/3 """
    return Quaternion(q.r, q.j, q.k, q.i)


def qbar(q):
    """ Rotate axis by -tau/3 """
    return Quaternion(q.r, q.k, q.i, q.j)


def check(rfn, cfn, qfn, q):
    """ compares the Quaternion function (qfn) with the equivilent
        real (rfn) and complex (cfn) functions
    """
    a = Quaternion(rfn(q.r))
    b = qfn(Quaternion(q.r))
    assert quaternion.isclose(a, b)

    a = Quaternion(cfn(q.complex))
    b = qfn(Quaternion(q.complex))
    assert quaternion.isclose(a, b)

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


def test_pythagoras():
    for a in qlist:
        u = quaternion.sin(a)**2 + quaternion.cos(a)**2
        v = abs(u)
        assert v - 1.0 < 1.0e-3


def test_tan():
    for q in qlist:
        check(math.tan, cmath.tan, quaternion.tan, q)


if __name__ == "__main__":
    test_isclose()
    test_log_exp()
    test_sin()
    test_cos()
    test_tan()
    test_pythagoras()

# end
