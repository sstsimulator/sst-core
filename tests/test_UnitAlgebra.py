# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
from sst import UnitAlgebra


ua1 = UnitAlgebra("15ns")
ua2 = UnitAlgebra("10ns")

# Test all the math functions
print("Simple math operations:")

#addition
ua3 = ua1 + ua2
print("%s + %s = %s"%(ua1.bestSI(), ua2.bestSI(), ua3.bestSI()))

#subtraction
ua3 = ua1 - ua2
print("%s - %s = %s"%(ua1.bestSI(), ua2.bestSI(), ua3.bestSI()))

#multiplication
ua3 = ua1 * ua2
print("%s * %s = %s"%(ua1.bestSI(), ua2.bestSI(), ua3.bestSI()))

#division
ua3 = ua1 / ua2
print("%s / %s = %s"%(ua1.bestSI(), ua2.bestSI(), ua3.bestSI()))

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
print(ua3.bestSI())
print(ua4.bestSI())

print("Subtraction:")

ua3 = ua1
ua4 = ua3
ua3_id_before = id(ua3)
ua3 -= ua2
if id(ua3) == ua3_id_before:
    print("ERROR: -= operator returned the same object")
print(ua3.bestSI())
print(ua4.bestSI())

print("Multiplication:")

ua3 = ua1
ua4 = ua3
ua3_id_before = id(ua3)
ua3 *= ua2
if id(ua3) == ua3_id_before:
    print("ERROR: *= operator returned the same object")
print(ua3.bestSI())
print(ua4.bestSI())

print("Division:")

ua3 = ua1
ua4 = ua3
ua3_id_before = id(ua3)
ua3 /= ua2
if id(ua3) == ua3_id_before:
    print("ERROR: /= operator returned the same object")
print(ua3.bestSI())
print(ua4.bestSI())

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

print("%s > %s = %r"%(ua1.bestSI(),ua2.bestSI(),ua1 > ua2))
print("%s >= %s = %r"%(ua1.bestSI(),ua2.bestSI(),ua1 >= ua2))
print("%s < %s = %r"%(ua1.bestSI(),ua2.bestSI(),ua1 < ua2))
print("%s <= %s = %r"%(ua1.bestSI(),ua2.bestSI(),ua1 <= ua2))
print("%s == %s = %r"%(ua1.bestSI(),ua2.bestSI(),ua1 == ua2))
print("%s != %s = %r"%(ua1.bestSI(),ua2.bestSI(),ua1 != ua2))
print("")
print("%s > %s = %r"%(ua1.bestSI(),ua3.bestSI(),ua1 > ua3))
print("%s >= %s = %r"%(ua1.bestSI(),ua3.bestSI(),ua1 >= ua3))
print("%s < %s = %r"%(ua1.bestSI(),ua3.bestSI(),ua1 < ua3))
print("%s <= %s = %r"%(ua1.bestSI(),ua3.bestSI(),ua1 <= ua3))
print("%s == %s = %r"%(ua1.bestSI(),ua3.bestSI(),ua1 == ua3))
print("%s != %s = %r"%(ua1.bestSI(),ua3.bestSI(),ua1 != ua3))


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


# Test doing math and comparison operations on this with mismatch units to make
# sure that they throw exceptions in the proper instances (i.e. add, sub and
# compares other than == and != will throw an exception if the units don't match.
try:
    UnitAlgebra("1 m")
except ValueError as e:
    assert str(e) == "Invalid unit type: "

ua4 = UnitAlgebra("1s")
ua5 = UnitAlgebra("4 events")
try:
    ua1 + ua4
except TypeError as e:
    assert str(e) == ""
try:
    ua1 + ua5
except TypeError as e:
    assert str(e) == "Attempting to add UnitAlgebra values with non-matching units: s, events"
try:
    ua1 < ua5
except TypeError as e:
    assert str(e) == "Attempting to compare UnitAlgebra values with non-matching units: s, events"

#Check conversions to int (in which case it rounds to nearest integer)
#and floats.  There are two ways to do it:
# 1 - use the built in int() and float() operations
# 2 - use the getRoundedValue() and getFloatValue() functions
#     defined on UnitAlgebra
print("")
print("Conversion to int:")
ua3 = UnitAlgebra("1GHz")
print("'%s' to long = %d"%(ua3.bestSI(),int(ua3)))
print("'%s'.getRoundedValue() = %d"%(ua3.bestSI(), ua3.getRoundedValue()))

print("")
print("Conversion to float:")
ua3 = UnitAlgebra("1.77s")
print("'%s' to float = %f"%(ua3.bestSI(),float(ua3)))
print("'%s'.getFloatValue() = %f"%(ua3.bestSI(), ua3.getFloatValue()))

print("")
print("Conversion to float:")
ua3 = UnitAlgebra("1.77s")
print("'%s' to float = %f"%(ua3.bestSI(),float(ua3)))
print("'%s'.getFloatValue() = %f"%(ua3.bestSI(), ua3.getFloatValue()))

print("")
print("Conversion to bool:")
ua3 = UnitAlgebra("0ns")
print("bool(%s) = %r"%(ua3.bestSI(),bool(ua3)))
ua3 = UnitAlgebra("5ns")
print("bool(%s) = %r"%(ua3.bestSI(),bool(ua3)))

# Test the remaining functions that haven't been tested elsewhere
print("")
print("isValueZero():")
ua3 = UnitAlgebra("0ns")
print("'%s'.isValueZero() = %r"%(ua3.bestSI(),ua3.isValueZero()))
ua3 = UnitAlgebra("5ns")
print("'%s'.isValueZero() = %r"%(ua3.bestSI(),ua3.isValueZero()))

print("")
print("Negate:")
print("-%s = %s"%(ua1.bestSI(),(-ua1).bestSI()))

print("")
print("Invert:")
print("'%s'.invert() = %s"%(ua1.bestSI(),ua1.invert().bestSI()))

print("")
print("hasUnits():")
print("'%s'.hasUnits(\"ns\") = %r"%(ua1.bestSI(),ua1.hasUnits("ns")))
print("'%s'.hasUnits(\"s\") = %r"%(ua1.bestSI(),ua1.hasUnits("s")))
print("'%s'.hasUnits(\"Hz\") = %r"%(ua1.bestSI(),ua1.hasUnits("Hz")))

# Check creation from int and float types
print("")
print("Initialization from numeric arguments:")
ua3 = UnitAlgebra(2)
print("From int = 2: %s"%(ua3))
ua3 = UnitAlgebra(9.443)
print("From float = 9.443: %s"%(ua3))

print("")
print("Printing:")
ua3 = UnitAlgebra("1024MiB")
ua3 = ua3 - UnitAlgebra("1B")
print("%s = %s = %s = %s = %s = %s\n"%(ua3, ua3.precision(), ua3.precision(4), ua3.bestSI(), ua3.bestSI(2), ua3.bestSI(0)))
