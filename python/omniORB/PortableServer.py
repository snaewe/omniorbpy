# -*- Mode: Python; -*-
#                            Package   : omniORBpy
# PortableServer.py          Created on: 1999/09/22
#                            Author    : Duncan Grisby (dpg1)
#
#    Copyright (C) 1999 AT&T Laboratories Cambridge
#
#    This file is part of the omniORBpy library
#
#    The omniORBpy library is free software; you can redistribute it
#    and/or modify it under the terms of the GNU Lesser General
#    Public License as published by the Free Software Foundation;
#    either version 2.1 of the License, or (at your option) any later
#    version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public
#    License along with this library; if not, write to the Free
#    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
#    MA 02111-1307, USA
#
#
# Description:
#    Fake PortableServer module interfacing with the omniORB BOA


# $Id$

# $Log$
# Revision 1.2  1999/09/24 09:22:01  dpg1
# Added copyright notices.
#
# Revision 1.1  1999/09/22 15:46:11  dpg1
# Fake POA implemented.
#

import _omnipy
import omniORB
from omniORB import CORBA


# native Servant
class Servant:
    def _this(self):
        try:
            return self._objref
        except AttributeError:
            orb = CORBA.ORB_init()
            poa = orb.resolve_initial_references("RootPOA")
            poa.activate_object(self)
            return self._objref


# ObjectId
_d_ObjectId  = (omniORB.tcInternal.tv_sequence, omniORB.tcInternal.tv_octet, 0)
_ad_ObjectId = (omniORB.tcInternal.tv_alias,
                "IDL:omg.org/PortableServer/ObjectId:1.0", "ObjectId",
                (omniORB.tcInternal.tv_sequence,
                 omniORB.tcInternal.tv_octet, 0))
_tc_ObjectId = omniORB.tcInternal.createTypeCode(_ad_ObjectId)


# exception ForwardRequest
class ForwardRequest (CORBA.UserException):
    _NP_RepositoryId = "IDL:omg.org/PortableServer/ForwardRequest:1.0"

    def __init__(self, forward_reference):
        self.forward_reference = forward_reference

_d_ForwardRequest  = (omniORB.tcInternal.tv_except, ForwardRequest,
                      ForwardRequest._NP_RepositoryId, "ForwardRequest",
                      "forward_reference", CORBA._d_Object)
_tc_ForwardRequest = omniORB.tcInternal.createTypeCode(_d_ForwardRequest)
omniORB.registerType(ForwardRequest._NP_RepositoryId,
                     _d_ForwardRequest, _tc_ForwardRequest)

# interface POAManager
class POAManager :
    _NP_RepositoryId = "IDL:omg.org/PortableServer/POAManager:1.0"
    _nil = CORBA.Object._nil

    # exception AdapterInactive
    class AdapterInactive (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POAManager/AdapterInactive:1.0"

    _d_AdapterInactive  = (omniORB.tcInternal.tv_except, AdapterInactive,
                           AdapterInactive._NP_RepositoryId, "AdapterInactive")
    _tc_AdapterInactive = omniORB.tcInternal.createTypeCode(_d_AdapterInactive)
    omniORB.registerType(AdapterInactive._NP_RepositoryId,
                         _d_AdapterInactive, _tc_AdapterInactive)
    
    # enum State
    HOLDING    = omniORB.EnumItem("HOLDING", 0)
    ACTIVE     = omniORB.EnumItem("ACTIVE", 1)
    DISCARDING = omniORB.EnumItem("DISCARDING", 2)
    INACTIVE   = omniORB.EnumItem("INACTIVE", 3)
    State = omniORB.Enum("IDL:omg.org/PortableServer/POAManager/State:1.0",
                         (HOLDING, ACTIVE, DISCARDING, INACTIVE))
    
    _d_State  = (omniORB.tcInternal.tv_enum, State._NP_RepositoryId,
                 "State", State._items)
    _tc_State = omniORB.tcInternal.createTypeCode(_d_State)
    omniORB.registerType(State._NP_RepositoryId, _d_State, _tc_State)

    def __init__(self, poa):
        self.__active = 0
        self.__poa    = poa

    def activate(self):
        _omnipy.implIsReady(self.__poa, 0, 1)
        self.__active = 1

    def hold_requests(self, wait_for_completion):
        raise CORBA.NO_IMPLEMENT()
    
    def discard_requests(self, wait_for_completion):
        raise CORBA.NO_IMPLEMENT()
    
    def deactivate(self, etherialize_objects, wait_for_completion):
        raise CORBA.NO_IMPLEMENT()

    def get_state(self):
        if self.__active:
            return POAManager.ACTIVE
        else:
            return POAManager.HOLDING

    __methods__ = ["activate", "hold_requests", "discard_requests",
                   "deactivate", "get_state"] + CORBA.Object.__methods__


# interface POA
class POA :
    """Fake POA implementation. Most operations not available."""
    
    _NP_RepositoryId = "IDL:omg.org/PortableServer/POA:2.3"
    _nil = CORBA.Object._nil

    # exception AdapterAlreadyExists
    class AdapterAlreadyExists (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/AdapterAlreadyExists:1.0"
    
    _d_AdapterAlreadyExists  = (omniORB.tcInternal.tv_except,
                                AdapterAlreadyExists,
                                AdapterAlreadyExists._NP_RepositoryId,
                                "AdapterAlreadyExists")
    _tc_AdapterAlreadyExists = omniORB.tcInternal.createTypeCode(\
        _d_AdapterAlreadyExists)
    omniORB.registerType(AdapterAlreadyExists._NP_RepositoryId,
                         _d_AdapterAlreadyExists, _tc_AdapterAlreadyExists)
    
    # exception AdapterNonExistent
    class AdapterNonExistent (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/AdapterNonExistent:1.0"
    
    _d_AdapterNonExistent  = (omniORB.tcInternal.tv_except,
                              AdapterNonExistent,
                              AdapterNonExistent._NP_RepositoryId,
                              "AdapterNonExistent")
    _tc_AdapterNonExistent = omniORB.tcInternal.createTypeCode(\
        _d_AdapterNonExistent)
    omniORB.registerType(AdapterNonExistent._NP_RepositoryId,
                         _d_AdapterNonExistent, _tc_AdapterNonExistent)
    
    # exception InvalidPolicy
    class InvalidPolicy (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/InvalidPolicy:1.0"
    
        def __init__(self, index):
            self.index = index
    
    _d_InvalidPolicy  = (omniORB.tcInternal.tv_except,
                         InvalidPolicy,
                         InvalidPolicy._NP_RepositoryId,
                         "InvalidPolicy",
                         "index", omniORB.tcInternal.tv_ushort)
    _tc_InvalidPolicy = omniORB.tcInternal.createTypeCode(_d_InvalidPolicy)
    omniORB.registerType(InvalidPolicy._NP_RepositoryId,
                         _d_InvalidPolicy, _tc_InvalidPolicy)
    
    # exception NoServant
    class NoServant (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/NoServant:1.0"
    
    _d_NoServant  = (omniORB.tcInternal.tv_except, NoServant,
                     NoServant._NP_RepositoryId, "NoServant")
    _tc_NoServant = omniORB.tcInternal.createTypeCode(_d_NoServant)
    omniORB.registerType(NoServant._NP_RepositoryId,
                         _d_NoServant, _tc_NoServant)
    
    # exception ObjectAlreadyActive
    class ObjectAlreadyActive (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/ObjectAlreadyActive:1.0"

    _d_ObjectAlreadyActive  = (omniORB.tcInternal.tv_except,
                               ObjectAlreadyActive,
                               ObjectAlreadyActive._NP_RepositoryId,
                               "ObjectAlreadyActive")
    _tc_ObjectAlreadyActive = omniORB.tcInternal.createTypeCode(\
        _d_ObjectAlreadyActive)
    omniORB.registerType(ObjectAlreadyActive._NP_RepositoryId,
                         _d_ObjectAlreadyActive, _tc_ObjectAlreadyActive)
    
    # exception ObjectNotActive
    class ObjectNotActive (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/ObjectNotActive:1.0"

    _d_ObjectNotActive  = (omniORB.tcInternal.tv_except, ObjectNotActive,
                           ObjectNotActive._NP_RepositoryId, "ObjectNotActive")
    _tc_ObjectNotActive = omniORB.tcInternal.createTypeCode(_d_ObjectNotActive)
    omniORB.registerType(ObjectNotActive._NP_RepositoryId,
                         _d_ObjectNotActive, _tc_ObjectNotActive)
    
    # exception ServantAlreadyActive
    class ServantAlreadyActive (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/ServantAlreadyActive:1.0"

    _d_ServantAlreadyActive  = (omniORB.tcInternal.tv_except,
                                ServantAlreadyActive,
                                ServantAlreadyActive._NP_RepositoryId,
                                "ServantAlreadyActive")
    _tc_ServantAlreadyActive = omniORB.tcInternal.createTypeCode(\
        _d_ServantAlreadyActive)
    omniORB.registerType(ServantAlreadyActive._NP_RepositoryId,
                         _d_ServantAlreadyActive, _tc_ServantAlreadyActive)
    
    # exception ServantNotActive
    class ServantNotActive (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/ServantNotActive:1.0"

    _d_ServantNotActive  = (omniORB.tcInternal.tv_except, ServantNotActive,
                            ServantNotActive._NP_RepositoryId,
                            "ServantNotActive")
    _tc_ServantNotActive = omniORB.tcInternal.createTypeCode(\
        _d_ServantNotActive)
    omniORB.registerType(ServantNotActive._NP_RepositoryId,
                         _d_ServantNotActive, _tc_ServantNotActive)
    
    # exception WrongAdapter
    class WrongAdapter (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/WrongAdapter:1.0"

    _d_WrongAdapter  = (omniORB.tcInternal.tv_except, WrongAdapter,
                        WrongAdapter._NP_RepositoryId, "WrongAdapter")
    _tc_WrongAdapter = omniORB.tcInternal.createTypeCode(_d_WrongAdapter)
    omniORB.registerType(WrongAdapter._NP_RepositoryId,
                         _d_WrongAdapter, _tc_WrongAdapter)
    
    # exception WrongPolicy
    class WrongPolicy (CORBA.UserException):
        _NP_RepositoryId = "IDL:omg.org/PortableServer/POA/WrongPolicy:1.0"

    _d_WrongPolicy  = (omniORB.tcInternal.tv_except, WrongPolicy,
                       WrongPolicy._NP_RepositoryId, "WrongPolicy")
    _tc_WrongPolicy = omniORB.tcInternal.createTypeCode(_d_WrongPolicy)
    omniORB.registerType(WrongPolicy._NP_RepositoryId,
                         _d_WrongPolicy, _tc_WrongPolicy)


    def __init__(self, orb, argv):
        _omnipy.BOA_init(self, orb, argv, "omniORB2_BOA")
        self.__orb     = orb
        self.__manager = POAManager(self)

    def create_POA(self, adapter_name, a_POAManager, policies):
        raise CORBA.NO_IMPLEMENT()

    def find_POA(self, adapter_name, activate_it):
        raise CORBA.NO_IMPLEMENT()

    def destroy(self, etherialize_objects, wait_for_completion):
        raise CORBA.NO_IMPLEMENT()

    def create_thread_policy(self, value):
        raise CORBA.NO_IMPLEMENT()

    def create_lifespan_policy(self, value):
        raise CORBA.NO_IMPLEMENT()

    def create_id_uniqueness_policy(self, value):
        raise CORBA.NO_IMPLEMENT()

    def create_id_assignment_policy(self, value):
        raise CORBA.NO_IMPLEMENT()

    def create_implict_activation_policy(self, value):
        raise CORBA.NO_IMPLEMENT()

    def create_servant_retention_policy(self, value):
        raise CORBA.NO_IMPLEMENT()

    def create_request_processing_policy(self, value):
        raise CORBA.NO_IMPLEMENT()

    def _get_the_name(self):
        return "RootPOA(BOA)"

    def _get_the_parent(self):
        return self._nil

    def _get_the_children(self):
        raise CORBA.NO_IMPLEMENT()

    def _get_the_POAManager(self):
        return self.__manager

    def _get_the_activator(self):
        raise CORBA.NO_IMPLEMENT()

    def _set_the_activator(self, value):
        raise CORBA.NO_IMPLEMENT()

    def get_servant_manager(self):
        raise CORBA.NO_IMPLEMENT()

    def set_servant_manager(self, imgr):
        raise CORBA.NO_IMPLEMENT()

    def get_servant(self):
        raise CORBA.NO_IMPLEMENT()

    def set_servant(self, p_servant):
        raise CORBA.NO_IMPLEMENT()

    def activate_object(self, p_servant):
        _omnipy.objectIsReady(self, p_servant)
        return _omnipy.referenceToId(p_servant._objref)

    def activate_object_with_id(self, id, p_servant):
        raise CORBA.NO_IMPLEMENT()

    def deactivate_object(self, oid):
        raise CORBA.NO_IMPLEMENT()

    def create_reference(self, intf):
        raise CORBA.NO_IMPLEMENT()

    def create_reference_with_id(self, oid, intf):
        raise CORBA.NO_IMPLEMENT()

    def servant_to_id(self, p_servant):
        return _omnipy.referenceToId(p_servant._objref)

    def servant_to_reference(self, p_servant):
        return p_servant._this()

    def reference_to_servant(self, reference):
        try:
            return _omnipy.referenceToServant(reference)
        except TypeError:
            raise POA.WrongAdapter()

    def reference_to_id(self, reference):
        return _omnipy.referenceToId(reference)

    def id_to_servant(self, oid):
        raise CORBA.NO_IMPLEMENT()

    def id_to_reference(self, oid):
        raise CORBA.NO_IMPLEMENT()

    __methods__ = ["create_POA",
                   "find_POA",
                   "destroy",
                   "create_thread_policy",
                   "create_lifespan_policy",
                   "create_id_uniqueness_policy",
                   "create_id_assignment_policy",
                   "create_implict_activation_policy",
                   "create_servant_retention_policy",
                   "create_request_processing_policy",
                   "_get_the_name",
                   "_get_the_parent",
                   "_get_the_children",
                   "_get_the_POAManager",
                   "_get_the_activator",
                   "_set_the_activator",
                   "get_servant_manager",
                   "set_servant_manager",
                   "get_servant",
                   "set_servant",
                   "activate_object",
                   "activate_object_with_id",
                   "deactivate_object",
                   "create_reference",
                   "create_reference_with_id",
                   "servant_to_id",
                   "servant_to_reference",
                   "reference_to_servant",
                   "reference_to_id",
                   "id_to_servant",
                   "id_to_reference"] + CORBA.Object.__methods__

