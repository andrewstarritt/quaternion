#!/usr/bin/env python
#

import math
import quaternion
from quaternion import Quaternion

# Let's be < 3.6 friendly
#
tau = math.pi * 2.0

def test_construct():
    a = 1.0
    b = (2.0,3.0,4.0)
    # Construct a rotation Quaternion
    r = Quaternion (angle=a, axis=b)
    assert math.isclose (abs (r), 1.0)


def test_expected_errors():

    a = 1.0
    b = (2.0,3.0,4.0)

    try:
        a = Quaternion (angle=a)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print ("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Quaternion (axis=b)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print ("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Quaternion (angle=None, axis=None)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print ("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Quaternion (angle=None, axis=b)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print ("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Quaternion (angle=a, axis=None)
        assert False, "Expecting TypeError"
    except TypeError as te:
        print ("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Quaternion (angle=1.0, axis=(1.0,2.0))
        assert False, "Expecting TypeError"
    except TypeError as te:
        print ("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise

    try:
        r = Quaternion (angle=1.0, axis=(1.0,2.0,3.0,4.0))
        assert False, "Expecting TypeError"
    except TypeError as te:
        print ("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise


    b = (0.0, 0.0, 0.0)
    try:
        r = Quaternion (angle=a, axis=b)
        print (r)
        assert False, "Expecting ValueError"
    except ValueError as te:
        print ("Expected Error: %s: %s" % (type(te).__name__, te))
    except:
        raise


def test_rotation1 ():
    a = tau / 8.0         # 45 deg
    b = (0.0, 0.0, 1.0)   # +z axis

    r = Quaternion (angle=a, axis=b)

    line = (1.0, 0.0, 0.0)
    t = line
    print ("  ( {x:6.3f}, {y:6.3f}, {z:6.3f} )".format (x=t[0],y=t[1],z=t[2]))
    for k in range (8):
        t = r.rotate (t)
        print ("  ( {x:6.3f}, {y:6.3f}, {z:6.3f} )".format (x=t[0],y=t[1],z=t[2]))

    print ()

    s = (t [0] - line [0])**2 +  (t [1] - line [1])**2 +  (t [2] - line [2])**2
    s = math.sqrt (s)

    assert s < 1.0e-9


def test_rotation2 ():
    a = tau / 6.0         # 60 deg
    b = (1.0, 1.0, 1.0)   # diagonal

    r = Quaternion (angle=a, axis=b)

    line = (1.0, 0.0, 0.0)
    t = line
    print ("  ( {x:6.3f}, {y:6.3f}, {z:6.3f} )".format (x=t[0],y=t[1],z=t[2]))
    for k in range (6):
        t = r.rotate (t)
        print ("  ( {x:6.3f}, {y:6.3f}, {z:6.3f} )".format (x=t[0],y=t[1],z=t[2]))

    print ()

    s = (t [0] - line [0])**2 +  (t [1] - line [1])**2 +  (t [2] - line [2])**2
    s = math.sqrt (s)

    assert s < 1.0e-9


if __name__ == "__main__":
    test_construct()
    test_expected_errors()
    test_rotation1 ()
    test_rotation2 ()

# end
