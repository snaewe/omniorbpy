#!/usr/bin/env python

# Example of using a ServantActivator.
#
# This program behaves just like the echo server, except that the
# servant object is not activated until the first request on the
# object.
#
# If you run with a -l command line argument, the program will make a
# number of local calls to the object, showing that the activator runs
# in the local, as well as the remote, case.


import sys, time
from omniORB import CORBA, PortableServer, POA_PortableServer

import _GlobalIDL, POA__GlobalIDL

class Echo_i (POA__GlobalIDL.Echo):
    def __init__(self):
        print "Echo_i created."

    def __del__(self):
        print "Echo_i deleted."

    def echoString(self, mesg):
        print "echoString() called with message:", mesg
        return mesg

class ServantActivator_i (POA_PortableServer.ServantActivator):
    def incarnate(self, oid, poa):
        print "incarnate(): oid:", oid, "poa:", poa._get_the_name()
        ei = Echo_i()
        return ei

    def etherealize(self, oid, poa, serv, cleanup_in_progress,
                    remaining_activations):
        print "etherealize(): oid:", oid, "poa:", poa._get_the_name()


# Initialise the ORB and activate the root POA.
orb = CORBA.ORB_init(sys.argv, CORBA.ORB_ID)
poa = orb.resolve_initial_references("RootPOA")
poaManager = poa._get_the_POAManager()
poaManager.activate()

# Create a child POA with the right policies for a ServantActivator
ps = [poa.create_id_assignment_policy(PortableServer.USER_ID),
      poa.create_servant_retention_policy(PortableServer.RETAIN),
      poa.create_request_processing_policy(PortableServer.USE_SERVANT_MANAGER)]

child = poa.create_POA("MyPOA", poaManager, ps)

# Create the ServantActivator and set it as the child's ServantManager
sai = ServantActivator_i()
sao = sai._this()
child.set_servant_manager(sao)

# Create an object reference with no servant
eo = child.create_reference_with_id("MyEcho", CORBA.id(_GlobalIDL.Echo))
print orb.object_to_string(eo)

# Run, or do some local calls...
if not (len(sys.argv) > 1 and sys.argv[1] == "-l"):
    orb.run()

time.sleep(1)

print "Calling..."

# On this invocation, the servant will be activated
print eo.echoString("Hello from same address space")
time.sleep(1)

# This invocation uses the local case optimisation, since the servant
# is now active
print eo.echoString("Hello again")
time.sleep(1)

# Deactivating the object causes a call to etherealize(), and the
# servant is deleted.
print "Deactivating the object..."
child.deactivate_object("MyEcho")
print "Deactivated."
time.sleep(1)

# This invocation activates the servant again
print eo.echoString("Hello again again")
time.sleep(1)

# Destroying the child POA causes the servant to be etherealized again
print "Destroying POA..."
child.destroy(1, 1)
print "Destroyed."
