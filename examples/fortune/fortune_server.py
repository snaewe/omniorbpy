#!/usr/bin/env python

import sys, os
from omniORB import CORBA, PortableServer
import Fortune, Fortune__POA

FORTUNE_PATH = "/usr/games/fortune"

class CookieServer_i (Fortune__POA.CookieServer):
    def get_cookie(self):
        pipe   = os.popen(FORTUNE_PATH)
        cookie = pipe.read()
        if pipe.close():
            # An error occurred with the pipe
            raise Fortune.Failure("popen of fortune failed")
        return cookie

orb = CORBA.ORB_init(sys.argv + ["-ORBendpoint", "giop:tcp::14285"])
poa = orb.resolve_initial_references("omniINSPOA")

servant = CookieServer_i()
poa.activate_object_with_id("fortune", servant)

obj = servant._this()
print orb.object_to_string(obj)

poa._get_the_POAManager().activate()
orb.run()
