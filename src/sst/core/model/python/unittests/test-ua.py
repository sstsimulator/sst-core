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

print("")

print("Comparison functions:")
ua3 = UnitAlgebra("15ns")

print("%s > %s = %r"%(ua1,ua2,ua1 > ua2))
print("%s >= %s = %r"%(ua1,ua2,ua1 >= ua2))
print("%s < %s = %r"%(ua1,ua2,ua1 < ua2))
print("%s <= %s = %r"%(ua1,ua2,ua1 <= ua2))
print("")
print("%s > %s = %r"%(ua1,ua3,ua1 > ua3))
print("%s >= %s = %r"%(ua1,ua3,ua1 >= ua3))
print("%s < %s = %r"%(ua1,ua3,ua1 < ua3))
print("%s <= %s = %r"%(ua1,ua3,ua1 <= ua3))


print("")
print("Conversion to int:")
ua3 = UnitAlgebra("1GHz")
print("%s to long = %d"%(ua3,int(ua3)))
print("%s.getRoundedValue() = %d"%(ua3, ua3.getRoundedValue()))

print("")
print("Negate:")
print("-%s = %s"%(ua1,-ua1))

print("")
print("Invert:")
print("%s.invert() = %s"%(ua1,ua1.invert()))

print("")
print("hasUnits():")
print("%s hasUnits(\"ns\") = %r"%(ua1,ua1.hasUnits("ns")))
print("%s hasUnits(\"s\") = %r"%(ua1,ua1.hasUnits("s")))
print("")
print("%s hasUnits(\"Hz\") = %r"%(ua1,ua1.hasUnits("Hz")))
