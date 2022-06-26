#!/usr/bin/env python
#

import quaternion as qn

Qn = qn.Quaternion
Qa = qn.QuaternionArray

def test_array_assign():
    a = Qa ([1,2,3,4])
    assert len(a) == 4
    
    b = Qa ([1,2,3,4])
    c = Qa ([1,2,3,4,5])
   
    assert a == a, "Arrays not equal"
    assert a == b, "Arrays not equal"
    assert a != c, "Arrays are equal"
    
def test_array_attributes():
    r = 1027
    a = Qa (reserve=r)
    assert a.itemsize == 32, f"itemsize is {a.itemsize} and not 32"
    assert a.allocated == r, f"allocated is {a.allocated } and not {r}"


def test_array_reverse():
    a = Qa ([1,2,3,4,7])
    b = Qa ([7,4,3,2,1])

    a.reverse()
    assert a == b, "Revserse failed"


if __name__ == "__main__":
    test_array_assign()
    test_array_attributes()
    test_array_reverse()
 
# end
