#!/usr/bin/env python
#

import quaternion
from quaternion import Quaternion
import math
import cmath


def test_isclose():
    return

    a = Quaternion(1.23e10)
    b = a + 0.001

    assert a != b
    assert quaternion.isclose(a, b)
    assert quaternion.isclose(a, a)


def test_log_exp():
    a = Quaternion(1.2, -3.4, +5.6, -7.8)
    b = quaternion.log (a)
    c = quaternion.exp (b)
    assert abs (a - c) < 1.0e-9;


def test_sqrt():
    a = Quaternion(4.0, 0.0, 0.0, 0.0)
    b = Quaternion(2.0, 0.0, 0.0, 0.0)

    c = quaternion.sqrt(a)
    assert abs (b - c) < 1.0e-9;

    a = Quaternion(1.23, 4.56, -7.89, 2.456)
    b = quaternion.sqrt(a)
    c = b*b
    assert abs (a - c) < 1.0e-9;

def test_sin():
    t = 1.23
    a = quaternion.sin(Quaternion(t))
    b = Quaternion(math.sin(t))
    assert abs (a - b) < 1.0e-9;

    t = 1.0 + 0.23j
    a = quaternion.sin(Quaternion(t))
    b = Quaternion(cmath.sin(t))
    assert abs (a - b) < 1.0e-9;


def test_cos():
    t = 1.23
    a = quaternion.cos(Quaternion(t))
    b = Quaternion(math.cos(t))
    assert abs (a - b) < 1.0e-9;

    t = 1.0 + 0.23j
    a = quaternion.cos(Quaternion(t))
    b = Quaternion(cmath.cos(t))
    assert abs (a - b) < 1.0e-9;


def test_pythagoras():
    a = Quaternion(-0.1, -0.1, +0.1, +0.1)
    u = quaternion.sin(a)**2 + quaternion.cos(a)**2
    v = abs (u)
    assert v - 1.0 < 1.0e-3;


def test_tan():
    t = 1.23
    a = quaternion.tan(Quaternion(t))
    b = Quaternion(math.tan(t))
    assert abs (a - b) < 1.0e-9;

    t = 1.0 + 0.23j
    a = quaternion.tan(Quaternion(t))
    b = Quaternion(cmath.tan(t))
    assert abs (a - b) < 1.0e-9;


if __name__ == "__main__":
    test_isclose()
    test_log_exp()
    test_sin()
    test_cos()
    test_tan()
    test_pythagoras()

# end
