print("noexc-finally")
try:
    print("try")
finally:
    print("finally")

print("noexc-finally-finally")
try:
    print("try1")
    try:
        print("try2")
    finally:
        print("finally2")
finally:
    print("finally1")
print()

print("noexc-finally-func-finally")
def func2():
    try:
        print("try2")
    finally:
        print("finally2")

try:
    print("try1")
    func2()
finally:
    print("finally1")
print()


print("exc-finally-except")
try:
    print("try1")
    try:
        print("try2")
        foo()
    except:
        print("except2")
finally:
    print("finally1")
print()

print("exc-finally-except-filter")
try:
    print("try1")
    try:
        print("try2")
        foo()
    except NameError:
        print("except2")
finally:
    print("finally1")
print()


print("exc-except-finally-finally")
try:  # top-level catch-all except to not fail script
    try:
        print("try1")
        try:
            print("try2")
            foo()
        finally:
            print("finally2")
    finally:
        print("finally1")
except:
    print("catch-all except")
print()

# case where a try-except within a finally cancels the exception
print("exc-finally-subexcept")
try:
    print("try1")
finally:
    try:
        print("try2")
        foo
    except:
        print("except2")
    print("finally1")
print()

# case where exception is raised after a finally has finished (tests that the finally doesn't run again)
def func():
    try:
        print("try")
    finally:
        print("finally")
    foo
try:
    func()
except:
    print("except")
