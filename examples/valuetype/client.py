#!/usr/bin/env python

import CORBA, ValueTest, Derived

def main(args):
    orb = CORBA.ORB_init(args)

    obj = orb.string_to_object(args[1])
    obj = obj._narrow(ValueTest.Test)

    v1 = ValueTest.One("hello", 123)
    v2 = ValueTest.One("test", 42)
    v3 = ValueTest.Two(None, None)
    v4 = ValueTest.Two(v1, None)
    v5 = ValueTest.Two(v1, v2)
    v6 = ValueTest.Two(v1, v1)
    v7 = Derived.Three("abc", 456, "more")

    r1 = obj.op1(v1)
    r2 = obj.op1(v2)
    r3 = obj.op1(None)

    obj.op2(v1, v2)
    obj.op2(None, v1)
    obj.op2(v1, None)
    obj.op2(None, None)
    obj.op2(v1, v1)

    r4 = obj.op3(v3)
    r5 = obj.op3(v4)
    r6 = obj.op3(v5)
    r7 = obj.op3(v6)
    r8 = obj.op1(v7)
    obj.op2(v7, v7)
    obj.op2(v1, v7)
    obj.op2(v7, v1)

    r9 = obj.op4("Hello")
    r10 = obj.op4(None)

    orb.destroy()


if __name__ == "__main__":
    import sys
    main(sys.argv)
