#!/usr/bin/env python
#

import array
import pickle
import quaternion as qn

Qn = qn.Quaternion
Qa = qn.QuaternionArray

Qn.for_repr_use_str()

q0 = Qn(0, 1, 2, 3)
q1 = Qn(4, 5, 6, 7)
q2 = Qn(8, 9, 10, 11)
q3 = Qn(12, 13, 14, 15)

ql = (q0, q1, q2, q3)
qr = (q3, q2, q1, q0)

qx = Qn(2, -1, 8, -4)


def test_array_assign():
    print("test_array_assign")
    a = Qa([1, 2, 3, 4])
    assert len(a) == 4, "Wrong length"

    b = Qa([1, 2, 3, 4])
    c = Qa([1, 2, 3, 4, 5])
    assert len(c) == 5, "Wrong length"

    assert a == a, "Arrays not equal"
    assert a == b, "Arrays not equal"
    assert a != c, "Arrays are equal"


def test_array_attributes():
    print("test_array_attributes")
    r = 1027
    a = Qa(reserve=r)
    assert a.itemsize == 32, f"itemsize is {a.itemsize} and not 32"
    assert a.reserved == r, f"reserved is {a.reserved } and not {r}"
    assert a.allocated >= r, f"allocated {a.allocated} is < reserved {r}"


def test_array_append():
    print("test_array_append")
    a = Qa(ql)
    n = len(a)

    a.append(qx)
    m = len(a)

    assert m == n + 1, "Append count to dnot increment by 1"
    q = a[m - 1]
    assert q == qx, "Appended item not added to the end"


def test_array_buffer_info():
    print("test_array_buffer_info")
    a = Qa()
    bi = a.buffer_info()
    assert bi == (0, 0), "Buffer info tuple is not (0, 0)"

    a = Qa(ql)
    bi = a.buffer_info()
    assert bi[1] == len(ql), "Buffer info length element is not 4"


def test_array_byteswap():
    print("test_array_byteswap")
    a = Qa(ql)
    b = array.array('d')
    c = array.array('d')

    # This test leverages off the fact that a quaternion
    # is structurally not dissimilar to a double array.array
    # of length 4.
    #
    for q in a:
        b.extend(q.data)
    b.byteswap()

    a.byteswap()
    for q in a:
        c.extend(q.data)

    assert b == c, "Byte swap fail"


def test_array_clear():
    print("test_array_clear")
    a = Qa(qr)
    assert len(a) == len(qr), "Assign failure"
    a.clear()
    assert len(a) == 0, "Clear failure"


def test_array_count():
    print("test_array_count")
    a = Qa(qr)
    assert a.count(q2) == 1, "Count fail, expecting 1"
    assert a.count(qx) == 0, "Count fail, expecting 0"
    a.append(q2)
    a.append(qx)
    a.append(q2)
    assert a.count(q2) == 3, "Count fail, expecting 3"
    assert a.count(qx) == 1, "Count fail, expecting 1"


def test_array_extend():
    print("test_array_extend")
    a = Qa(ql)
    a.extend(qr)
    n = len(ql) + len(qr)
    assert len(a) == n, f"Assign failure, expecting a length of {n}"

    qc = (*ql, *qr)

    for j in range(n):
        assert a[j] == qc[j], f"Miss-match on the {j}th element"

    # repeat with iteration
    #
    j = 0
    for x, y in zip(a, qc):
        assert x == y, f"Miss-match on the {j}th element"
        j += 1


def test_array_index():
    print("test_array_index")
    a = Qa(ql)
    i = a.index(q0)
    assert i == 0, f"Expecting index to return 0, got {i}"
    i = a.index(q1)
    assert i == 1, f"Expecting index to return 1, got {i}"
    i = a.index(q2)
    assert i == 2, f"Expecting index to return 2, got {i}"
    i = a.index(q3)
    assert i == 3, f"Expecting index to return 3, got {i}"

    # Check we get the expected error
    #
    try:
        i = a.index(qx)
        assert False, "Expecting a ValueError"
    except ValueError:
        pass
    except BaseException:
        assert False, "Expecting a ValueError"


def test_array_insert():
    print("test_array_insert")
    a = Qa()
    a.insert(0, qx)
    n = len(a)
    assert n == 1, f"Insert - got wrong length expecyting 1, got {n}"
    i = a.index(qx)
    assert i == 0, f"Expecting index to return 0, got {i}"

    a = Qa(ql)
    a.insert(1, qx)
    n = len(a)
    assert n == 5, f"Insert - got wrong length expecyting 1, got {n}"
    i = a.index(qx)
    assert i == 1, f"Expecting index to return 1, got {i}"

    a = Qa(ql)
    a.insert(-1, qx)
    n = len(a)
    assert n == 5, f"Insert - got wrong length expecyting 1, got {n}"
    i = a.index(qx)
    assert i == n - 2, f"Expecting index to return {n-2}, got {i}"


def test_array_pop():
    print("test_array_pop")
    a = Qa(ql)
    n = len(a)

    q = a.pop()
    m = len(a)
    assert m == n - 1, f"Pop failure, wrong array length, expecting {n-1}, got {m}"
    assert q == q3, f"Wrong element popped: got  {q}, expected  {q3}"

    a = Qa(ql)
    q = a.pop(0)
    assert q == q0, f"Wrong element popped: got  {q}, expected  {q0}"

    a = Qa(ql)
    q = a.pop(2)
    assert q == q2, f"Wrong element popped: got  {q}, expected  {q0}"


def test_array_remove():
    print("test_array_remove")
    a = Qa(ql)

    for q in ql:
        a.remove(q)
        n = a.count(q)
        assert n == 0, f"Expecting no occurances of {q}, got {n}"

    n = len(a)
    assert n == 0, f"Expecting a length of 0, got {n}"

    # Check we get the expected error
    #
    try:
        a = Qa(ql)
        a.remove(qx)
        assert False, "Expecting a ValueError"
    except ValueError:
        pass
    except BaseException:
        assert False, "Expecting a ValueError"


def test_array_reverse():
    print("test_array_reverse")
    a = Qa(ql)
    b = Qa(qr)
    a.reverse()
    assert a == b, "Reverse failed"


def test_array_reserve():
    print("test_array_reserve")
    a = Qa(ql, reserve=123)
    n = a.reserved
    assert n == 123, f"Reserve - expecting 123, got {n}"

    for m in (12, 42, 4200, 1339, 131, 5000):
        a.reserve(m)
        n = a.reserved
        assert n == m, f"Reserve - expecting {m}, got {n}"


def test_array_to_from_bytes():
    print("test_array_to_from_bytes")
    a = Qa(ql)
    b = a.tobytes()

    n = len(a) * 4 * 8  # 4 doubles per quaternion
    m = len(b)
    assert m == n, f"Expecting {n} bytes, got {m} bytes"

    c = Qa()
    c.frombytes(b)
    assert a == c, "tobytes/frombytes failure"

    # Check we get the expected error
    #
    b += b'\x01'
    c = Qa()
    try:
        c.frombytes(b)
        assert False, "Expecting a ValueError"
    except ValueError:
        pass
    except BaseException:
        assert False, "Expecting a ValueError"


def test_array_buffer_api():
    print("test_array_buffer_api")
    a =            Qa([Qn(1.1e2, 2.3, 3.2, 4), Qn(55, 666, -7.7, 1e-8)])
    b = array.array('d', [1.1e2, 2.3, 3.2, 4,     55, 666, -7.7, 1e-8])

    assert bytes(a) == bytes(b), "bytes(a) failed"
    assert bytes(a) == a.tobytes(), "bytes(a) != a.tobytes()"


def test_array_to_from_file():
    print("test_array_to_from_file")
    fname = '/tmp/test_array_to_from_file.dat'
    a = Qa(ql)
    n = len(a)

    with open(fname, 'wb') as f:
        a.tofile(f)

    c = Qa()
    with open(fname, 'rb') as f:
        c.fromfile(f, n)

    assert a == c, "tofile/fromfile failure"

    # Check we get the expected error
    #
    c = Qa()
    try:
        with open(fname, 'rb') as f:
            c.fromfile(f, n + 2)
        assert False, "Expecting an EOFError"
    except EOFError:
        pass
    except BaseException:
        assert False, "Expecting an EOFError"


def test_array_pickle():
    print("test_array_pickle")
    a = Qa(ql, reserve=131)
    s = pickle.dumps(a)
    c = pickle.loads(s)
    assert a == c, "pickle/unpickle failure"

    assert a.reserved == c.reserved, "pickle/unpickle reserved failure"


def test_array_concat():
    print("test_array_concat")
    a = Qa(ql)
    n = len(a)

    b = Qa(qr)
    m = len(b)

    c = a + b
    p = len(c)
    assert p == n + m, f"Concat length failure {p} != {n} + {m}"

    for j in range(p):
        x = c[j]
        if j < n:
            y = a[j]
        else:
            y = b[j - n]
        assert x == y, f"Concat error: {j}th element {x} is not  {y}"

    # Now do concat in place
    #
    a = Qa(ql)
    n = len(a)

    b = Qa(qr)
    m = len(b)

    c = Qa(ql)
    c += b
    p = len(c)
    assert p == n + m, f"Concat length failure {p} != {n} + {m}"

    for j in range(p):
        x = c[j]
        if j < n:
            y = a[j]
        else:
            y = b[j - n]
        assert x == y, f"Concat error: {j}th element {x} is not  {y}"


def test_array_repeat():
    print("test_array_repeat")
    a = Qa(ql)
    n = len(a)

    b = 3 * a
    m = len(b)
    assert m == 3 * n, f"Repeat length failure {m} != 3*{n}"

    for j in range(m):
        x = b[j]
        y = a[j % n]
        assert x == y, f"Repeat error: {j}th element {x} is not  {y}"

    # Now do repeat in place
    #
    a = Qa(ql)
    n = len(a)

    b = Qa(ql)
    b *= 3
    m = len(b)
    assert m == 3 * n, f"Repeat length failure {m} != 3*{n}"
    for j in range(m):
        x = b[j]
        y = a[j % n]
        assert x == y, f"Repeat error: {j}th element {x} is not {y}"


def test_array_iteration():
    print("test_array_iteration")
    a = Qa(ql)

    j = 0
    for q in a:
        x = ql[j]
        assert q == x, f"Iteration error: {j}th item {q} != {x}"
        j += 1

    # Test a nested interation on the same array object
    i = 0
    for p in a:
        j = 0
        for q in a:
            x = ql[i]
            y = ql[j]
            assert p == x, f"Iteration error: {i}th outer item, {j}th inner cycle"
            assert q == y, f"Iteration error: {i}th outer cycle, {j}th inner item"
            j += 1
        i += 1


def equivilent(a, b):
    if isinstance(a, (tuple, list, array.array, qn.QuaternionArray)) and  \
       isinstance(b, (tuple, list, array.array, qn.QuaternionArray)):
        result = len(a) == len(b)
        if result:
            for x, y in zip(a, b):
                if x != y:
                    result = False
                    break
    else:
        result = (a == b)

    return result


def test_array_slice():
    print("test_array_slice")
    a = array.array('d', range(23))
    b = Qa(a)

    # We leverage of doing the equivilent slice action on both an array.array
    # of doubles and a quaternion array, and comapring the result.
    #
    assert equivilent(a, b), "Arrays not equivilent to start with"

    assert equivilent(a[0], b[0]), "Array index error"
    assert equivilent(a[5], b[5]), "Array index error"
    assert equivilent(a[-5], b[-5]), "Array index error"
    assert equivilent(a[-1], b[-1]), "Array index error"

    assert equivilent(a[::], b[::]), "Array slice error"
    assert equivilent(a[::2], b[::2]), "Array slice error"
    assert equivilent(a[::3], b[::3]), "Array slice error"
    assert equivilent(a[::4], b[::4]), "Array slice error"
    assert equivilent(a[0::4], b[0::4]), "Array slice error"
    assert equivilent(a[1::4], b[1::4]), "Array slice error"
    assert equivilent(a[3::4], b[3::4]), "Array slice error"
    assert equivilent(a[3:-1:4], b[3:-1:4]), "Array slice error"
    assert equivilent(a[3:-2:4], b[3:-3:4]), "Array slice error"
    assert equivilent(a[3:-8:4], b[3:-8:4]), "Array slice error"

    assert equivilent(a[::-1], b[::-1]), "Array slice error"
    assert equivilent(a[::-2], b[::-2]), "Array slice error"
    assert equivilent(a[::-3], b[::-3]), "Array slice error"
    assert equivilent(a[::-4], b[::-4]), "Array slice error"
    assert equivilent(a[0::-4], b[0::-4]), "Array slice error"
    assert equivilent(a[-1::-4], b[-1::-4]), "Array slice error"
    assert equivilent(a[-3::-4], b[-3::-4]), "Array slice error"

    a[3], a[14] = a[14], a[3]
    b[3], b[14] = b[14], b[3]
    assert equivilent(a, b), "Array assign error"

    a[5:8], a[15:18] = a[15:18], a[5:8]
    b[5:8], b[15:18] = b[15:18], b[5:8]
    assert equivilent(a, b), "Array assign error"

    a[5:18:3] = a[4:17:3]
    b[5:18:3] = b[4:17:3]
    assert equivilent(a, b), "Array slice assign error"

    a[-1:3:-5] = a[-2:2:-5]
    b[-1:3:-5] = b[-2:2:-5]
    assert equivilent(a, b), "Array slice assign error"

    new_items = [101, 102, 103, 104, 105, 106, 107, 108, 109, 111.11]
    a[3:6] = array.array('d', new_items)
    b[3:6] = Qa(new_items)
    assert equivilent(a, b), "Array assign error"

    del (a[5])
    del (b[5])
    assert equivilent(a, b), "Array del error"

    del (a[::3])
    del (b[::3])
    assert equivilent(a, b), "Array del error"

    del (a[2::7])
    del (b[2::7])
    assert equivilent(a, b), "Array del error"


if __name__ == "__main__":
    test_array_assign()
    test_array_attributes()
    test_array_append()
    test_array_buffer_info()
    test_array_byteswap()
    test_array_clear()
    test_array_count()
    test_array_extend()
    test_array_index()
    test_array_insert()
    test_array_pop()
    test_array_remove()
    test_array_reverse()
    test_array_reserve()
    test_array_to_from_bytes()
    test_array_buffer_api()
    test_array_to_from_file()
    test_array_pickle()
    test_array_concat()
    test_array_repeat()
    test_array_iteration()
    test_array_slice()

# end
