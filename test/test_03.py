#!/usr/bin/env python
#

import math
import quaternion as qn

Qn = qn.Quaternion

tau = qn.tau


# -----------------------------------------------------------------------------
#
def det(m):
    """ Returns the determinent of a 3x3 matrix m.
        m is expected to be a tuple/list of tuples and/or lists.
    """
    t1 = m[0][0] * (m[1][1]*m[2][2] - m[1][2]*m[2][1])
    t2 = m[0][1] * (m[1][2]*m[2][0] - m[1][0]*m[2][2])
    t3 = m[0][2] * (m[1][0]*m[2][1] - m[1][1]*m[2][0])

    return t1 + t2 + t3


# -----------------------------------------------------------------------------
#
def transpose(m):
    """ Returns the transpose the matrix m.
        m is expected to be a tuple/list of tuples and/or lists.
        Returns a tuple of tuples.
    """
    result = []

    # Find number of rows and coles of the transpose.
    #
    nr = len(m[0])
    nc = len(m)

    for i in range(nr):
        row = []
        for j in range(nc):
            row.append(m[j][i])
        result.append(tuple(row))

    result = tuple(result)
    return result


# -----------------------------------------------------------------------------
#
def mmult(left, right):
    """ Simple no frills and no checks matrix * matrix and matrix * vector
        multiplication function.
        left and right are are expected to be tuples/lists (of tuples/lists).
        Returns a tuple or a tuple of tuples.
    """
    if isinstance(right[0], (tuple, list)):
        # matrix
        #
        nr = len(left)
        nk1 = len(left[0])
        nk2 = len(right)
        nc = len(right[0])
        if nk1 != nk2:
            raise ValueError(f"Matrix size mis-match mmult (left={nr}x{nk1}, right={nk2}x{nc})")

        result = []
        for i in range(nr):
            row = []
            for j in range(nc):
                s = 0.0
                for k in range(nk1):
                    s += left[i][k]*right[k][j]
                row.append(s)

            result.append(tuple(row))
        result = tuple(result)

    else:
        # vector
        #
        nr = len(left)
        nk1 = len(left[0])
        nk2 = len(right)
        if nk1 != nk2:
            raise ValueError(f"Matrix/vector size mis-match mmult (left={nr}x{nk1}, right=[{nk2}])")

        result = []
        for i in range(nr):
            s = 0.0
            for j in range(nk1):
                s += left[i][j]*right[j]
            result.append(s)
        result = tuple(result)

    return result


# -----------------------------------------------------------------------------
#
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

    print()


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
    m = r.matrix()

    # And perform a matrix multiply
    #
    t2 = mmult(m, c)

    # ... and compare the result.
    #
    assert abs(t1[0] - t2[0]) < 1.0e-9
    assert abs(t1[1] - t2[1]) < 1.0e-9
    assert abs(t1[2] - t2[2]) < 1.0e-9
    print()


def test_rotation4():
    # Choose quazi random angle and axis
    #
    a1 = 0.54321

    x, y, z = 0.12, -0.34, -0.56
    s = (x*x + y*y + z*z)**0.5
    u1 = (x/s, y/s, z/s)

    r = Qn(angle=a1, axis=u1)

    a2 = r.angle()
    u2 = qn.axis(r)

    print(u1, u1[0]**2 + u1[1]**2 + u1[2]**2)
    print(u2, u2[0]**2 + u2[1]**2 + u2[2]**2)

    # ... and compare the result.
    #
    assert abs(a1 - a2) < 1.0e-9
    assert abs(u1[0] - u2[0]) < 1.0e-9
    assert abs(u1[1] - u2[1]) < 1.0e-9
    assert abs(u1[2] - u2[2]) < 1.0e-9
    print()


def test_rotation5():
    # Choose quazi random rotation matrix
    #
    m = ((-0.6644335128480408, -0.7456022251949338, +0.0510434010306547),
         (+0.4065266142924398, -0.4178917055696902, -0.8124670050457317),
         (+0.6271078207743372, -0.5190798052327008, +0.5807684021391342))

    r = Qn(matrix=m)

    # Choose quazi random point
    #
    c = (2.71, +6.23, -3.49)

    # Rotate using:  r*c*r.conjugate()
    #
    t1 = r.rotate(c)

    # And perform a matrix multiply
    #
    t2 = mmult(m, c)

    # ... and compare the result.
    #
    assert abs(t1[0] - t2[0]) < 1.0e-9
    assert abs(t1[1] - t2[1]) < 1.0e-9
    assert abs(t1[2] - t2[2]) < 1.0e-9


def test_rotation6():
    p = Qn(angle=3.0, axis=(1, -1, 2))
    q = Qn(angle=1.5, axis=(-2, 3, -3))
    print(f"{p:.6f}")
    print(f"{q:.6f}")
    print()

    pm = p.matrix()
    qm = q.matrix()

    rm = mmult(pm, qm)
    r = Qn(matrix=rm)
    s = p * q
    print(f"{r:.6f}")
    print(f"{s:.6f}")
    t = abs(r - s)
    print(f"{t:.2e}")
    print()
    assert t <= 1.0e-15, "Quaternion/matrix multiplication comparison failure (1)"

    # And reverse order of multiplication
    #
    rm = mmult(qm, pm)
    r = Qn(matrix=rm)
    s = q * p
    print(f"{r:.6f}")
    print(f"{s:.6f}")
    t = abs(r - s)
    print(f"{t:.2e}")
    print()
    assert t <= 1.0e-15, "Quaternion/matrix multiplication comparison failure (2)"


if __name__ == "__main__":
    test_construct()
    test_expected_errors()
    test_rotation1()
    test_rotation2()
    test_rotation3()
    test_rotation4()
    test_rotation5()
    test_rotation6()

# end
