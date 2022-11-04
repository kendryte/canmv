# tests int constant folding in compiler

# positive
print(+1)
print(+100)

# negation
print(-1)
print(-(-1))

# 1's complement
print(~0)
print(~1)
print(~-1)

# addition
print(1 + 2)

# subtraction
print(1 - 2)
print(2 - 1)

# multiplication
print(1 * 2)
print(123 * 456)

# floor div and modulo
print(123 // 7, 123 % 7)
print(-123 // 7, -123 % 7)
print(123 // -7, 123 % -7)
print(-123 // -7, -123 % -7)

# power
print(2 ** 3)
print(3 ** 4)

# won't fold so an exception can be raised at runtime
try:
    1 << -1
except ValueError:
    print('ValueError')
