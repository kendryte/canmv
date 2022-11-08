# Test for uzlib.DecompIO.init()
try:
    import uzlib
    import uio as io
except ImportError:
    print("SKIP")
    raise SystemExit

# Need to pass valid header
inp = uzlib.DecompIO(io.BytesIO(b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03"), 25)

inp.init(
    io.BytesIO(
        b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03"
        b"\xcbH\xcd\xc9\xc9\xcf\x00\x11\x00h\x97\x8c\xf5\n\x00\x00\x00"
    )
)
print(inp.read())

inp.init(
    io.BytesIO(
        b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03"
        b"+\xcf/\xcaI)\x07\x11\x00\x8c\xa3\x96\xb9\n\x00\x00\x00"
    )
)
print(inp.read())
