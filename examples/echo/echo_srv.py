#!/usr/bin/env python

import sys
from omniORB import CORBA, PortableServer

# Import the skeletons for the Example module
import POA__GlobalIDL

# Initialise the ORB
orb = CORBA.ORB_init(sys.argv, CORBA.ORB_ID)

# Find the POA
poa = orb.resolve_initial_references("RootPOA")

# Activate the POA
poaManager = poa._get_the_POAManager()
poaManager.activate()

# Define an implementation of the Echo interface
class Echo_i (POA__GlobalIDL.Echo):
    def echoString(self, mesg):
        print "echoString() called with message:", mesg
        return mesg

# Create an instance of it
ei = Echo_i()

# Create an object reference, and implicitly activate the object
eo = ei._this()

# Print out the IOR
print orb.object_to_string(eo)

# Everything is running now, but if this thread drops out of the end
# of the file, the process will exit. orb.run() just blocks for ever
orb.run()
