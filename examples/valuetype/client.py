#!/usr/bin/env python

import CORBA, ValueTest, ValueTest__POA, Derived


class Three_i(ValueTest.Three):
    def test(self):
        print "test local call"
        return "value"

class Four_i(ValueTest__POA.Four):
    def test(self):
        print "test callback"
        return "object"


def main(args):
    orb = CORBA.ORB_init(args)
    poa = orb.resolve_initial_references("RootPOA")
    poa._get_the_POAManager().activate()

    orb.register_value_factory(CORBA.id(ValueTest.Three), Three_i)

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

    r9  = obj.op4("Hello")
    r10 = obj.op4(None)

    # Abstract interface test
    obj.op5(None)

    fi = Four_i()
    fo = fi._this()

    obj.op5(fo)

    t = Three_i("experiment")
    obj.op5(t)

    orb.destroy()


if __name__ == "__main__":
    import sys
    main(sys.argv)
