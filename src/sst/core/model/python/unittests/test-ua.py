from sst import UnitAlgebra


ua1 = UnitAlgebra("15ns")
ua2 = UnitAlgebra("10ns")

# Test all the math functions
print("Simple math operations:")

#addition
ua3 = ua1 + ua2
print("%s + %s = %s"%(ua1, ua2, ua3))

#subtraction
ua3 = ua1 - ua2
print("%s - %s = %s"%(ua1, ua2, ua3))

#multiplication
ua3 = ua1 * ua2
print("%s * %s = %s"%(ua1, ua2, ua3))

#division
ua3 = ua1 / ua2
print("%s / %s = %s"%(ua1, ua2, ua3))


# in-place functions
print("")
print("In-place math operations:")
print("executing the following for each operator:")
print("ua1 = UnitAlgebra(\"15ns\")")
print("ua2 = UnitAlgebra(\"10ns\")")
print("ua3 = ua1")
print("ua4 = ua3")
print("ua3 op= ua2")
print("print(ua3)  -- should have the result of operation")
print("print(ua4)  -- should be unchanged")
print("")

# During this, test that assignment and in-place operators work
# correctly as far as return new objects

# a = b should make a and b the same object.

# funny enough, a += b will return a new object for a (this is how
# it's supposed to work

print("Addition:")

ua3 = ua1
if id(ua3) != id(ua1):
    print("ERROR: assignement operator did not result in both objects being the same")
ua4 = ua3
ua3_id_before = id(ua3)
ua3 += ua2
if id(ua3) == ua3_id_before:
    print("ERROR: += operator returned the same object")
print(ua3)
print(ua4)

print("Subtraction:")

ua3 = ua1
ua4 = ua3
ua3_id_before = id(ua3)
ua3 -= ua2
if id(ua3) == ua3_id_before:
    print("ERROR: -= operator returned the same object")
print(ua3)
print(ua4)

print("Multiplication:")

ua3 = ua1
ua4 = ua3
ua3_id_before = id(ua3)
ua3 *= ua2
if id(ua3) == ua3_id_before:
    print("ERROR: *= operator returned the same object")
print(ua3)
print(ua4)

print("Division:")

ua3 = ua1
ua4 = ua3
ua3_id_before = id(ua3)
ua3 /= ua2
if id(ua3) == ua3_id_before:
    print("ERROR: \= operator returned the same object")
print(ua3)
print(ua4)

# Check to see if exceptions are triggered as they should be if the
# operands aren't correct

# Only need to try one math operation since they all call into the
# same underlying function where the type checks happen
correct_throw = False
try:
    ua1 = UnitAlgebra("10ns")
    ua2 = ua1 + 5
except TypeError:
    print("Exception correctly thrown for wrong operand type passed to math operation (+,-,*,/) as second argument")
    correct_throw = True

if not correct_throw:
    print("ERROR: Exception NOT correctly thrown for wrong operand type passed to math operation (+,-,*,/) as second argument")


correct_throw = False
try:
    ua1 = UnitAlgebra("10ns")
    ua2 = 5 + ua1
except TypeError:
    print("Exception correctly thrown for wrong operand type passed to math operation (+,-,*,/) as first argument")
    correct_throw = True

if not correct_throw:
    print("ERROR: Exception NOT correctly thrown for wrong operand type passed to math operation (+,-,*,/) as first argument")



print("")

print("Comparison functions:")
ua3 = UnitAlgebra("15ns")

print("%s > %s = %r"%(ua1,ua2,ua1 > ua2))
print("%s >= %s = %r"%(ua1,ua2,ua1 >= ua2))
print("%s < %s = %r"%(ua1,ua2,ua1 < ua2))
print("%s <= %s = %r"%(ua1,ua2,ua1 <= ua2))
print("%s == %s = %r"%(ua1,ua2,ua1 == ua2))
print("%s != %s = %r"%(ua1,ua2,ua1 != ua2))
print("")
print("%s > %s = %r"%(ua1,ua3,ua1 > ua3))
print("%s >= %s = %r"%(ua1,ua3,ua1 >= ua3))
print("%s < %s = %r"%(ua1,ua3,ua1 < ua3))
print("%s <= %s = %r"%(ua1,ua3,ua1 <= ua3))
print("%s == %s = %r"%(ua1,ua3,ua1 == ua3))
print("%s != %s = %r"%(ua1,ua3,ua1 != ua3))


# Check to make sure we get exceptions for invalid arguments.  Since
# it is the same function, really only need to check one operator.
# Need to check using the UnitAlgebra as both the first and second
# argument.  Also need to check equals and not equals separately,
# because they can fall back on just checking to see if they are the
# exact same object, in which case, then won't generate an error.
print("")
correct_throw = False
try:
    ua1 > 5
except TypeError:
    correct_throw = True
    print("Correctly got exception when comparing against invalid operand type as second argument")

if not correct_throw:
    print("ERROR: did not get exception when comparing against invalid argument type as second argument")

correct_throw = False
try:
    5 > ua1
except TypeError:
    correct_throw = True
    print("Correctly got exception when comparing against invalid operand type as first argument")

if not correct_throw:
    print("ERROR: did not get exception when comparing against invalid argument type as first argument")


correct_throw = False
try:
    ua1 == 5
except TypeError:
    correct_throw = True
    print("Correctly got exception when checking equals against invalid operand type as second argument")

if not correct_throw:
    print("ERROR: did not get exception when comparing against invalid operand type as second argument")


correct_throw = False
try:
    5 == ua1
except TypeError:
    correct_throw = True
    print("Correctly got exception when checking equals against invalid operand type as first argument")

if not correct_throw:
    print("ERROR: did not get exception when checking equals against invalid operand type as first argument")


correct_throw = False
try:
    ua1 != 5
except TypeError:
    correct_throw = True
    print("Correctly got exception when checking not equals against invalid operand type as second argument")

if not correct_throw:
    print("ERROR: did not get exception when checking not equals against invalid operand type as second argument")

correct_throw = False
try:
    5 != ua1
except TypeError:
    correct_throw = True
    print("Correctly got exception when checking not equals against invalid operand type as first argument")

if not correct_throw:
    print("ERROR: did not get exception when checking not equals against invalid operand type as first argument")


# Once expections are enabled in UnitAlgebra (instead of the fatal()
# that is currently called), we need to test doing math and comparison
# operations on this with mismatch units to make sure that they throw
# exceptions in the proper instances (i.e. add, sub and compares other
# than == and != will throw an exception if the units don't match.



#Check conversions to int (in which case it rounds to nearest integer)
#and floats.  There are two ways to do it:
# 1 - use the built in int() and float() operations
# 2 - use the getRoundedValue() and getFloatValue() functions
#     defined on UnitAlgebra
print("")
print("Conversion to int:")
ua3 = UnitAlgebra("1GHz")
print("'%s' to long = %d"%(ua3,int(ua3)))
print("'%s'.getRoundedValue() = %d"%(ua3, ua3.getRoundedValue()))

print("")
print("Conversion to float:")
ua3 = UnitAlgebra("1.77s")
print("'%s' to float = %f"%(ua3,float(ua3)))
print("'%s'.getFloatValue() = %f"%(ua3, ua3.getFloatValue()))

print("")
print("Conversion to float:")
ua3 = UnitAlgebra("1.77s")
print("'%s' to float = %f"%(ua3,float(ua3)))
print("'%s'.getFloatValue() = %f"%(ua3, ua3.getFloatValue()))

print("")
print("Conversion to bool:")
ua3 = UnitAlgebra("0ns")
print("bool(%s) = %r"%(ua3,bool(ua3)))
ua3 = UnitAlgebra("5ns")
print("bool(%s) = %r"%(ua3,bool(ua3)))

# Test the remaining functions that haven't been tested elsewhere
print("")
print("isValueZero():")
ua3 = UnitAlgebra("0ns")
print("'%s'.isValueZero() = %r"%(ua3,ua3.isValueZero()))
ua3 = UnitAlgebra("5ns")
print("'%s'.isValueZero() = %r"%(ua3,ua3.isValueZero()))

print("")
print("Negate:")
print("-%s = %s"%(ua1,-ua1))

print("")
print("Invert:")
print("'%s'.invert() = %s"%(ua1,ua1.invert()))

print("")
print("hasUnits():")
print("'%s'.hasUnits(\"ns\") = %r"%(ua1,ua1.hasUnits("ns")))
print("'%s'.hasUnits(\"s\") = %r"%(ua1,ua1.hasUnits("s")))
print("'%s'.hasUnits(\"Hz\") = %r"%(ua1,ua1.hasUnits("Hz")))

