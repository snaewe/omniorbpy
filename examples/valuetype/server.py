#!/usr/bin/env python

import CORBA, ValueTest, ValueTest__POA

class Three_i(ValueTest.Three):
    def test(self):
        print "test local call"
        return "value"


class Test_i (ValueTest__POA.Test):

    def op1(self, a):
        if a:
            print "op1:", a, a.s, a.l
        else:
            print "op1:", a
        return a

    def op2(self, a, b):
        if a:
            print "op2: a:", a, a.s, a.l
        else:
            print "op2: a:", a
        if b:
            print "op2: b:", b, b.s, b.l
        else:
            print "op2: b:", b

    def op3(self, a):
        if a:
            print "op3:", a, a.a, a.b
        else:
            print "op3:", a
        return a

    def op4(self, a):
        print "op4:", a
        return a

    def op5(self, a):
        print "op5:", a
        if a is not None:
            print a.test()


def main(args):
    orb = CORBA.ORB_init(args)
    poa = orb.resolve_initial_references("RootPOA")

    orb.register_value_factory(CORBA.id(ValueTest.Three), Three_i)

    ti = Test_i()
    to = ti._this()

    print orb.object_to_string(to)

    poa._get_the_POAManager().activate()
    orb.run()

if __name__ == "__main__":
    import sys
    main(sys.argv)
