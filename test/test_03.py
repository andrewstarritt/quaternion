#!/usr/bin/env python
#

import math
import quaternion as qn

Qn = qn.Quaternion

# Let's be < 3.6 friendly
#
tau = qn.tau


def test_construct():
    a = 1.0
    b = (2.0, 3.0, 4.0)
    # Construct a rotation Qn
    r = Qn(angle=a, axis=b)
    assert math.isclose(abs(r), 1.0)


def test_expected_errors():

    a = 1.0
    b = (2.0, 3.0, 4.0)

    try:
        a = Qn(angle=a)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Qn(axis=b)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Qn(angle=None, axis=None)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Qn(angle=None, axis=b)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Qn(angle=a, axis=None)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Qn(angle=1.0, axis=(1.0, 2.0))
        assert False, "Expecting TypeError"
    except TypeError as te:
        print("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Qn(angle=1.0, axis=(1.0, 2.0, 3.0, 4.0))
        assert False, "Expecting TypeError"
    except TypeError as te:
        print("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    b = (0.0, 0.0, 0.0)
    try:
        r = Qn(angle=a, axis=b)
        print(r)
        assert False, "Expecting ValueError"
    except ValueError as te:
        print("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise


def test_rotation1():
    a = tau / 8.0         # 45 deg
    b = (0.0, 0.0, 1.0)   # +z axis

    r = Qn(angle=a, axis=b)

    line = (1.0, 0.0, 0.0)
    t = line
    print("  ( {x:6.3f}, {y:6.3f}, {z:6.3f} )".format(x=t[0], y=t[1], z=t[2]))
    for k in range(8):
        t = r.rotate(t)
        print("  ( {x:6.3f}, {y:6.3f}, {z:6.3f} )".format(x=t[0], y=t[1], z=t[2]))

    print()

    s = (t[0] - line[0])**2 + (t[1] - line[1])**2 + (t[2] - line[2])**2
    s = math.sqrt(s)

    assert s < 1.0e-9


def test_rotation2():
    a = tau / 6.0         # 60 deg
    b = (1.0, 1.0, 1.0)   # diagonal

    r = Qn(angle=a, axis=b)

    line = (1.0, 0.0, 0.0)
    t = line
    print("  ( {x:6.3f}, {y:6.3f}, {z:6.3f} )".format(x=t[0], y=t[1], z=t[2]))

    # Do 6 rotations of a sixth of a turn
    #
    for k in range(6):
        t = r.rotate(t)
        print("  ( {x:6.3f}, {y:6.3f}, {z:6.3f} )".format(x=t[0], y=t[1], z=t[2]))

    print()

    # Check we are back where we stated from
    #
    s = (t[0] - line[0])**2 + (t[1] - line[1])**2 + (t[2] - line[2])**2
    s = math.sqrt(s)

    assert s < 1.0e-9


def test_rotation3():
    # Choose quazi random angle and axis
    #
    a = 0.54321
    b = (0.12, -0.34, 0.56)

    r = Qn(angle=a, axis=b)

    # Choose quazi random point
    #
    c = (2.7, -7.2, 3.4)

    # Rotate using:  r*c*r.conjugate
    #
    t1 = r.rotate(c)

    # Extract rotation matrix
    #
    m = r.rotation_matrix()

    # And perform a matrix multiply
    #
    tx = m[0][0] * c[0] + m[0][1] * c[1] + m[0][2] * c[2]
    ty = m[1][0] * c[0] + m[1][1] * c[1] + m[1][2] * c[2]
    tz = m[2][0] * c[0] + m[2][1] * c[1] + m[2][2] * c[2]
    t2 = (tx, ty, tz)

    # ... and compare the result.
    #
    assert abs(t1[0] - t2[0]) < 1.0e-9
    assert abs(t1[1] - t2[1]) < 1.0e-9
    assert abs(t1[2] - t2[2]) < 1.0e-9


def test_rotation4():
    # Choose quazi random angle and axis
    #
    a1 = 0.54321

    x, y, z = 0.12, -0.34, -0.56
    s = (x*x + y*y + z*z)**0.5
    u1 = (x/s, y/s, z/s)

    r = Qn(angle=a1, axis=u1)

    a2 = r.rotation_angle()
    u2 = qn.axis(r)

    print(u1, u1[0]**2 + u1[1]**2 + u1[2]**2)
    print(u2, u2[0]**2 + u2[1]**2 + u2[2]**2)

    # ... and compare the result.
    #
    assert abs(a1    - a2)    < 1.0e-9
    assert abs(u1[0] - u2[0]) < 1.0e-9
    assert abs(u1[1] - u2[1]) < 1.0e-9
    assert abs(u1[2] - u2[2]) < 1.0e-9


if __name__ == "__main__":
    test_construct()
    test_expected_errors()
    test_rotation1()
    test_rotation2()
    test_rotation3()
    test_rotation4()

# end
