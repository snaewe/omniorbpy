#!/usr/bin/env python

import sys

# Import the CORBA module
from omniORB import CORBA

# Import the stubs for the Example module
import Example

# Initialise the ORB
orb = CORBA.ORB_init(sys.argv, CORBA.ORB_ID)

# Get the IOR of an Echo object from the command line (without
# checking that the arguments are sensible!)
ior = sys.argv[1]

# Convert the IOR to an object reference
object = orb.string_to_object(ior)

# Invoke the echoString operation
message = "Hello from Python"
result  = object.echoString(message)
print "I said '" + result + "'. The object said '" + result + "'"
