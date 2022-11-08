# memoryview constructor with offset/size and array
try:
    from uarray import array
except ImportError:
    try:
        from array import array
    except ImportError:
        print("SKIP")
        raise SystemExit

try:
    memoryview(b"", 0, 0)
except:
    print("SKIP")
    raise SystemExit

buf = array("i", [1, 2, 3, 0x7f000000, -0x80000000])

m = memoryview(buf, 1, 3)
print(list(m))

m = memoryview(buf, -1, 100)
print(list(m))
