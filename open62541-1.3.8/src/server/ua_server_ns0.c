/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Thomas Bender
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Henrik Norrman
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#include "open62541/namespace0_generated.h"
#include "open62541/nodeids.h"
#include "open62541/types.h"
#include "open62541/types_generated.h"

#include "ua_server_internal.h"
#include "ua_session.h"
#include "ua_subscription.h"

static UA_StatusCode
addNode_raw(UA_Server *server, UA_NodeClass nodeClass,
            UA_UInt32 nodeId, char *name, void *attributes,
            const UA_DataType *attributesType) {
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.nodeClass = nodeClass;
    item.requestedNewNodeId.nodeId = UA_NODEID_NUMERIC(0, nodeId);
    item.browseName = UA_QUALIFIEDNAME(0, name);
    UA_ExtensionObject_setValueNoDelete(&item.nodeAttributes,
                                        attributes, attributesType);
    return AddNode_raw(server, &server->adminSession, NULL, &item, NULL);
}

static UA_StatusCode
addNode_finish(UA_Server *server, UA_UInt32 nodeId,
               UA_UInt32 parentNodeId, UA_UInt32 referenceTypeId) {
    const UA_NodeId sourceId = UA_NODEID_NUMERIC(0, nodeId);
    const UA_NodeId refTypeId = UA_NODEID_NUMERIC(0, referenceTypeId);
    const UA_ExpandedNodeId targetId = UA_EXPANDEDNODEID_NUMERIC(0, parentNodeId);
    UA_StatusCode retval = UA_Server_addReference(server, sourceId, refTypeId, targetId, false);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    return AddNode_finish(server, &server->adminSession, &sourceId);
}

static UA_StatusCode
addObjectNode(UA_Server *server, char* name, UA_UInt32 objectid,
              UA_UInt32 parentid, UA_UInt32 referenceid, UA_UInt32 type_id) {
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", name);
    return UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(0, objectid),
                                   UA_NODEID_NUMERIC(0, parentid),
                                   UA_NODEID_NUMERIC(0, referenceid),
                                   UA_QUALIFIEDNAME(0, name),
                                   UA_NODEID_NUMERIC(0, type_id),
                                   object_attr, NULL, NULL);
}

static UA_StatusCode
addReferenceTypeNode(UA_Server *server, char* name, char *inverseName, UA_UInt32 referencetypeid,
                     UA_Boolean isabstract, UA_Boolean symmetric, UA_UInt32 parentid) {
    UA_ReferenceTypeAttributes reference_attr = UA_ReferenceTypeAttributes_default;
    reference_attr.displayName = UA_LOCALIZEDTEXT("", name);
    reference_attr.isAbstract = isabstract;
    reference_attr.symmetric = symmetric;
    if(inverseName)
        reference_attr.inverseName = UA_LOCALIZEDTEXT("", inverseName);
    return UA_Server_addReferenceTypeNode(server, UA_NODEID_NUMERIC(0, referencetypeid),
                                   UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                   UA_QUALIFIEDNAME(0, name), reference_attr, NULL, NULL);
}

/***************************/
/* Bootstrap NS0 hierarchy */
/***************************/

/* Creates the basic nodes which are expected by the nodeset compiler to be
 * already created. This is necessary to reduce the dependencies for the nodeset
 * compiler. */
static UA_StatusCode
UA_Server_createNS0_base(UA_Server *server) {
    /* Bootstrap ReferenceTypes. The order of these is important for the
     * ReferenceTypeIndex. The ReferenceTypeIndex is created with the raw node.
     * The ReferenceTypeSet of subtypes for every ReferenceType is created
     * during the call to AddNode_finish. */
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_ReferenceTypeAttributes references_attr = UA_ReferenceTypeAttributes_default;
    references_attr.displayName = UA_LOCALIZEDTEXT("", "References");
    references_attr.isAbstract = true;
    references_attr.symmetric = true;
    references_attr.inverseName = UA_LOCALIZEDTEXT("", "References");
    ret |= addNode_raw(server, UA_NODECLASS_REFERENCETYPE, UA_NS0ID_REFERENCES, "References",
                       &references_attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]);

    UA_ReferenceTypeAttributes hassubtype_attr = UA_ReferenceTypeAttributes_default;
    hassubtype_attr.displayName = UA_LOCALIZEDTEXT("", "HasSubtype");
    hassubtype_attr.isAbstract = false;
    hassubtype_attr.symmetric = false;
    hassubtype_attr.inverseName = UA_LOCALIZEDTEXT("", "SubtypeOf");
    ret |= addNode_raw(server, UA_NODECLASS_REFERENCETYPE, UA_NS0ID_HASSUBTYPE, "HasSubtype",
                       &hassubtype_attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]);

    UA_ReferenceTypeAttributes aggregates_attr = UA_ReferenceTypeAttributes_default;
    aggregates_attr.displayName = UA_LOCALIZEDTEXT("", "Aggregates");
    aggregates_attr.isAbstract = true;
    aggregates_attr.symmetric = false;
    aggregates_attr.inverseName = UA_LOCALIZEDTEXT("", "AggregatedBy");
    ret |= addNode_raw(server, UA_NODECLASS_REFERENCETYPE, UA_NS0ID_AGGREGATES, "Aggregates",
                       &aggregates_attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]);

    ret |= addReferenceTypeNode(server, "HierarchicalReferences", NULL,
                         UA_NS0ID_HIERARCHICALREFERENCES, true, false, UA_NS0ID_REFERENCES);

    ret |= addReferenceTypeNode(server, "NonHierarchicalReferences", NULL,
                         UA_NS0ID_NONHIERARCHICALREFERENCES, true, true, UA_NS0ID_REFERENCES);

    ret |= addReferenceTypeNode(server, "HasChild", NULL, UA_NS0ID_HASCHILD,
                         true, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "Organizes", "OrganizedBy", UA_NS0ID_ORGANIZES,
                         false, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasEventSource", "EventSourceOf", UA_NS0ID_HASEVENTSOURCE,
                         false, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasModellingRule", "ModellingRuleOf", UA_NS0ID_HASMODELLINGRULE,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasEncoding", "EncodingOf", UA_NS0ID_HASENCODING,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasDescription", "DescriptionOf", UA_NS0ID_HASDESCRIPTION,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasTypeDefinition", "TypeDefinitionOf", UA_NS0ID_HASTYPEDEFINITION,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "GeneratesEvent", "GeneratedBy", UA_NS0ID_GENERATESEVENT,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    /* Complete bootstrap of Aggregates */
    ret |= addNode_finish(server, UA_NS0ID_AGGREGATES, UA_NS0ID_HASCHILD, UA_NS0ID_HASSUBTYPE);

    /* Complete bootstrap of HasSubtype */
    ret |= addNode_finish(server, UA_NS0ID_HASSUBTYPE, UA_NS0ID_HASCHILD, UA_NS0ID_HASSUBTYPE);

    ret |= addReferenceTypeNode(server, "HasProperty", "PropertyOf", UA_NS0ID_HASPROPERTY,
                         false, false, UA_NS0ID_AGGREGATES);

    ret |= addReferenceTypeNode(server, "HasComponent", "ComponentOf", UA_NS0ID_HASCOMPONENT,
                         false, false, UA_NS0ID_AGGREGATES);

    ret |= addReferenceTypeNode(server, "HasNotifier", "NotifierOf", UA_NS0ID_HASNOTIFIER,
                         false, false, UA_NS0ID_HASEVENTSOURCE);

    ret |= addReferenceTypeNode(server, "HasOrderedComponent", "OrderedComponentOf",
                         UA_NS0ID_HASORDEREDCOMPONENT, false, false, UA_NS0ID_HASCOMPONENT);

    ret |= addReferenceTypeNode(server, "HasInterface", "InterfaceOf",
                         UA_NS0ID_HASINTERFACE, false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    /**************/
    /* Data Types */
    /**************/

    /* Bootstrap BaseDataType */
    UA_DataTypeAttributes basedatatype_attr = UA_DataTypeAttributes_default;
    basedatatype_attr.displayName = UA_LOCALIZEDTEXT("", "BaseDataType");
    basedatatype_attr.isAbstract = true;
    ret |= addNode_raw(server, UA_NODECLASS_DATATYPE, UA_NS0ID_BASEDATATYPE, "BaseDataType",
                       &basedatatype_attr, &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES]);

    /*****************/
    /* VariableTypes */
    /*****************/

    UA_VariableTypeAttributes basevar_attr = UA_VariableTypeAttributes_default;
    basevar_attr.displayName = UA_LOCALIZEDTEXT("", "BaseVariableType");
    basevar_attr.isAbstract = true;
    basevar_attr.valueRank = UA_VALUERANK_ANY;
    basevar_attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    ret |= addNode_raw(server, UA_NODECLASS_VARIABLETYPE, UA_NS0ID_BASEVARIABLETYPE, "BaseVariableType",
                       &basevar_attr, &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES]);

    UA_VariableTypeAttributes bdv_attr = UA_VariableTypeAttributes_default;
    bdv_attr.displayName = UA_LOCALIZEDTEXT("", "BaseDataVariableType");
    bdv_attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    bdv_attr.valueRank = UA_VALUERANK_ANY;
    ret |= UA_Server_addVariableTypeNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                                         UA_NODEID_NULL, UA_QUALIFIEDNAME(0, "BaseDataVariableType"),
                                         UA_NODEID_NULL, bdv_attr, NULL, NULL);

    UA_VariableTypeAttributes prop_attr = UA_VariableTypeAttributes_default;
    prop_attr.displayName = UA_LOCALIZEDTEXT("", "PropertyType");
    prop_attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    prop_attr.valueRank = UA_VALUERANK_ANY;
    ret |= UA_Server_addVariableTypeNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                                         UA_NODEID_NULL, UA_QUALIFIEDNAME(0, "PropertyType"),
                                         UA_NODEID_NULL, prop_attr, NULL, NULL);

    /***************/
    /* ObjectTypes */
    /***************/

    UA_ObjectTypeAttributes baseobj_attr = UA_ObjectTypeAttributes_default;
    baseobj_attr.displayName = UA_LOCALIZEDTEXT("", "BaseObjectType");
    ret |= addNode_raw(server, UA_NODECLASS_OBJECTTYPE, UA_NS0ID_BASEOBJECTTYPE, "BaseObjectType",
                       &baseobj_attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES]);

    UA_ObjectTypeAttributes folder_attr = UA_ObjectTypeAttributes_default;
    folder_attr.displayName = UA_LOCALIZEDTEXT("", "FolderType");
    ret |= UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                       UA_NODEID_NULL, UA_QUALIFIEDNAME(0, "FolderType"),
                                       folder_attr, NULL, NULL);

    /******************/
    /* Root and below */
    /******************/

    ret |= addObjectNode(server, "Root", UA_NS0ID_ROOTFOLDER, 0, 0, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Objects", UA_NS0ID_OBJECTSFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Types", UA_NS0ID_TYPESFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "ReferenceTypes", UA_NS0ID_REFERENCETYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addNode_finish(server, UA_NS0ID_REFERENCES, UA_NS0ID_REFERENCETYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "DataTypes", UA_NS0ID_DATATYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addNode_finish(server, UA_NS0ID_BASEDATATYPE, UA_NS0ID_DATATYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "VariableTypes", UA_NS0ID_VARIABLETYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addNode_finish(server, UA_NS0ID_BASEVARIABLETYPE, UA_NS0ID_VARIABLETYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "ObjectTypes", UA_NS0ID_OBJECTTYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addNode_finish(server, UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_OBJECTTYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "EventTypes", UA_NS0ID_EVENTTYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Views", UA_NS0ID_VIEWSFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    /* Add BaseEventType */
    UA_ObjectTypeAttributes eventtype_attr = UA_ObjectTypeAttributes_default;
    eventtype_attr.displayName = UA_LOCALIZEDTEXT("", "BaseEventType");
    ret |= addNode_raw(server, UA_NODECLASS_OBJECTTYPE, UA_NS0ID_BASEEVENTTYPE, "BaseEventType",
                       &eventtype_attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES]);
    ret |= addNode_finish(server, UA_NS0ID_BASEEVENTTYPE, UA_NS0ID_EVENTTYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    if(ret != UA_STATUSCODE_GOOD)
        ret = UA_STATUSCODE_BADINTERNALERROR;

    return ret;
}

/****************/
/* Data Sources */
/****************/

static UA_StatusCode
readStatus(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    if(sourceTimestamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }

    void *data = NULL;

    UA_assert(nodeId->identifierType == UA_NODEIDTYPE_NUMERIC);

    switch(nodeId->identifier.numeric) {
    case UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN: {
        UA_UInt32 *shutdown = UA_UInt32_new();
        if(!shutdown)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(server->endTime != 0)
            *shutdown = (UA_UInt32)((server->endTime - UA_DateTime_now()) / UA_DATETIME_SEC);
        value->value.data = shutdown;
        value->value.type = &UA_TYPES[UA_TYPES_UINT32];
        value->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }

    case UA_NS0ID_SERVER_SERVERSTATUS_STATE: {
        UA_ServerState *state = UA_ServerState_new();
        if(!state)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(server->endTime != 0)
            *state = UA_SERVERSTATE_SHUTDOWN;
        value->value.data = state;
        value->value.type = &UA_TYPES[UA_TYPES_SERVERSTATE];
        value->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }

    case UA_NS0ID_SERVER_SERVERSTATUS: {
        UA_ServerStatusDataType *statustype = UA_ServerStatusDataType_new();
        if(!statustype)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        statustype->startTime = server->startTime;
        statustype->currentTime = UA_DateTime_now();

        statustype->state = UA_SERVERSTATE_RUNNING;
        statustype->secondsTillShutdown = 0;
        if(server->endTime != 0) {
            statustype->state = UA_SERVERSTATE_SHUTDOWN;
            statustype->secondsTillShutdown = (UA_UInt32)
                ((server->endTime - UA_DateTime_now()) / UA_DATETIME_SEC);
        }

        value->value.data = statustype;
        value->value.type = &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE];
        value->hasValue = true;
        return UA_BuildInfo_copy(&server->config.buildInfo, &statustype->buildInfo);
    }

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO:
        value->value.type = &UA_TYPES[UA_TYPES_BUILDINFO];
        data = &server->config.buildInfo;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.productUri;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.manufacturerName;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.productName;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.softwareVersion;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.buildNumber;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE:
        value->value.type = &UA_TYPES[UA_TYPES_DATETIME];
        data = &server->config.buildInfo.buildDate;
        break;

    default:
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINTERNALERROR;
        return UA_STATUSCODE_GOOD;
    }

    value->value.data = UA_new(value->value.type);
    if(!value->value.data) {
        value->value.type = NULL;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    value->hasValue = true;
    return UA_copy(data, value->value.data, value->value.type);
}

#ifdef UA_GENERATED_NAMESPACE_ZERO
static UA_StatusCode
readServiceLevel(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext, UA_Boolean includeSourceTimeStamp,
                 const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BYTE];
    value->value.arrayLength = 0;
    UA_Byte *byte = UA_Byte_new();
    *byte = 255;
    value->value.data = byte;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readAuditing(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext, UA_Boolean includeSourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
    value->value.arrayLength = 0;
    UA_Boolean *boolean = UA_Boolean_new();
    *boolean = false;
    value->value.data = boolean;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
readNamespaces(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeid, void *nodeContext, UA_Boolean includeSourceTimeStamp,
               const UA_NumericRange *range,
               UA_DataValue *value) {
    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode retval;
    retval = UA_Variant_setArrayCopy(&value->value, server->namespaces,
                                     server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeNamespaces(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeid, void *nodeContext, const UA_NumericRange *range,
                const UA_DataValue *value) {
    /* Check the data type */
    if(!value->hasValue ||
       value->value.type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Check that the variant is not empty */
    if(!value->value.data)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* TODO: Writing with a range is not implemented */
    if(range)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_String *newNamespaces = (UA_String*)value->value.data;
    size_t newNamespacesSize = value->value.arrayLength;

    /* Test if we append to the existing namespaces */
    if(newNamespacesSize <= server->namespacesSize)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

    /* Test if the existing namespaces are unchanged */
    for(size_t i = 0; i < server->namespacesSize; ++i) {
        if(!UA_String_equal(&server->namespaces[i], &newNamespaces[i]))
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add namespaces */
    for(size_t i = server->namespacesSize; i < newNamespacesSize; ++i)
        addNamespace(server, newNamespaces[i]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readCurrentTime(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeid, void *nodeContext, UA_Boolean sourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_DateTime currentTime = UA_DateTime_now();
    UA_StatusCode retval = UA_Variant_setScalarCopy(&value->value, &currentTime,
                                                    &UA_TYPES[UA_TYPES_DATETIME]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = currentTime;
    }
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_GENERATED_NAMESPACE_ZERO
static UA_StatusCode
readMinSamplingInterval(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeid, void *nodeContext, UA_Boolean includeSourceTimeStamp,
               const UA_NumericRange *range,
               UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode retval;
    UA_Duration minInterval;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    minInterval = server->config.samplingIntervalLimits.min;
#else
    minInterval = 0.0;
#endif
    retval = UA_Variant_setScalarCopy(&value->value, &minInterval, &UA_TYPES[UA_TYPES_DURATION]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}
#endif

#if defined(UA_GENERATED_NAMESPACE_ZERO) && defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
static UA_StatusCode
readMonitoredItems(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
                   void *objectContext, size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    /* Return two empty arrays by default */
    UA_Variant_setArray(&output[0], UA_Array_new(0, &UA_TYPES[UA_TYPES_UINT32]),
                        0, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setArray(&output[1], UA_Array_new(0, &UA_TYPES[UA_TYPES_UINT32]),
                        0, &UA_TYPES[UA_TYPES_UINT32]);

    /* Get the Session */
    UA_LOCK(&server->serviceMutex);
    UA_Session *session = UA_Server_getSessionById(server, sessionId);
    if(!session) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(inputSize == 0 || !input[0].data) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }

    /* Get the Subscription */
    UA_UInt32 subscriptionId = *((UA_UInt32*)(input[0].data));
    UA_Subscription *subscription = UA_Server_getSubscriptionById(server, subscriptionId);
    if(!subscription) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }

    /* The Subscription is not attached to this Session */
    if(subscription->session != session) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    }

    /* Count the MonitoredItems */
    UA_UInt32 sizeOfOutput = 0;
    UA_MonitoredItem* monitoredItem;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        ++sizeOfOutput;
    }
    if(sizeOfOutput == 0) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_GOOD;
    }

    /* Allocate the output arrays */
    UA_UInt32 *clientHandles = (UA_UInt32*)
        UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    if(!clientHandles) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_UInt32 *serverHandles = (UA_UInt32*)
        UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    if(!serverHandles) {
        UA_UNLOCK(&server->serviceMutex);
        UA_free(clientHandles);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Fill the array */
    UA_UInt32 i = 0;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        clientHandles[i] = monitoredItem->parameters.clientHandle;
        serverHandles[i] = monitoredItem->monitoredItemId;
        ++i;
    }
    UA_Variant_setArray(&output[0], serverHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setArray(&output[1], clientHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);

    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}
#endif /* defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS) */

UA_StatusCode
writeNs0VariableArray(UA_Server *server, UA_UInt32 id, void *v,
                      size_t length, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, id), var);
}

#ifndef UA_GENERATED_NAMESPACE_ZERO
static UA_StatusCode
addVariableNode(UA_Server *server, char* name, UA_UInt32 variableid,
                UA_UInt32 parentid, UA_UInt32 referenceid,
                UA_Int32 valueRank, UA_UInt32 dataType) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", name);
    attr.dataType = UA_NODEID_NUMERIC(0, dataType);
    attr.valueRank = valueRank;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    return UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(0, variableid),
                                     UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NUMERIC(0, referenceid),
                                     UA_QUALIFIEDNAME(0, name),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                     attr, NULL, NULL);
}

/* A minimal server object that is not complete and does not use the mandated
 * references to a server type. To be used on very constrained devices. */
static UA_StatusCode
UA_Server_minimalServerObject(UA_Server *server) {
    /* Server */
    UA_StatusCode retval = addObjectNode(server, "Server", UA_NS0ID_SERVER, UA_NS0ID_OBJECTSFOLDER,
                                         UA_NS0ID_ORGANIZES, UA_NS0ID_BASEOBJECTTYPE);

    /* Use a valuerank of -2 for now. The array is added later on and the valuerank set to 1. */
    retval |= addVariableNode(server, "ServerArray", UA_NS0ID_SERVER_SERVERARRAY,
                              UA_NS0ID_SERVER, UA_NS0ID_HASPROPERTY,
                              UA_VALUERANK_ANY, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "NamespaceArray", UA_NS0ID_SERVER_NAMESPACEARRAY,
                              UA_NS0ID_SERVER, UA_NS0ID_HASPROPERTY,
                              UA_VALUERANK_ANY, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "ServerStatus", UA_NS0ID_SERVER_SERVERSTATUS,
                              UA_NS0ID_SERVER, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "CurrentTime", UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME,
                              UA_NS0ID_SERVER_SERVERSTATUS, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "State", UA_NS0ID_SERVER_SERVERSTATUS_STATE,
                              UA_NS0ID_SERVER_SERVERSTATUS, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "BuildInfo", UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO,
                              UA_NS0ID_SERVER_SERVERSTATUS, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "ProductUri", UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "ManufacturerName",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "ProductName",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "SoftwareVersion",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "BuildNumber",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "BuildDate",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    return retval;
}

#else

static UA_StatusCode
writeNs0Variable(UA_Server *server, UA_UInt32 id, void *v, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, v, type);
    return UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, id), var);
}

static void
addModellingRules(UA_Server *server) {
    /* Test if the ModellingRules folder was added. (Only for the full ns0.) */
    UA_NodeClass mrnc = UA_NODECLASS_UNSPECIFIED;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES),
                                                   &mrnc);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* Add ExposesItsArray */
    UA_Server_addReference(server,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_EXPOSESITSARRAY),
                           true);

    /* Add Mandatory */
    UA_Server_addReference(server,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                           true);


    /* Add MandatoryPlaceholder */
    UA_Server_addReference(server,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORYPLACEHOLDER),
                           true);

    /* Add Optional */
    UA_Server_addReference(server,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_OPTIONAL),
                           true);

    /* Add OptionalPlaceholder */
    UA_Server_addReference(server,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_OPTIONALPLACEHOLDER),
                           true);
}

#endif

/* Initialize the nodeset 0 by using the generated code of the nodeset compiler.
 * This also initialized the data sources for various variables, such as for
 * example server time. */
UA_StatusCode
UA_Server_initNS0(UA_Server *server) {
    /* Initialize base nodes which are always required an cannot be created
     * through the NS compiler */
    server->bootstrapNS0 = true;
    UA_StatusCode retVal = UA_Server_createNS0_base(server);

#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* Load nodes and references generated from the XML ns0 definition */
    retVal |= namespace0_generated(server);
#else
    /* Create a minimal server object */
    retVal |= UA_Server_minimalServerObject(server);
#endif

    server->bootstrapNS0 = false;

    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Initialization of Namespace 0 failed with %s. "
                     "See previous outputs for any error messages.",
                     UA_StatusCode_name(retVal));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* NamespaceArray */
    UA_DataSource namespaceDataSource = {readNamespaces, writeNamespaces};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                                                   namespaceDataSource);
    retVal |= UA_Server_writeValueRank(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), 1);

    /* ServerArray */
    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERARRAY,
                                    &server->config.applicationDescription.applicationUri,
                                    1, &UA_TYPES[UA_TYPES_STRING]);
    retVal |= UA_Server_writeValueRank(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERARRAY), 1);

    /* ServerStatus */
    UA_DataSource serverStatus = {readStatus, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), serverStatus);

    /* StartTime will be sampled in UA_Server_run_startup()*/

    /* CurrentTime */
    UA_DataSource currentTime = {readCurrentTime, NULL};
    UA_NodeId currTime = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    retVal |= UA_Server_setVariableNode_dataSource(server, currTime, currentTime);
    retVal |= UA_Server_writeMinimumSamplingInterval(server, currTime, 100.0);

    /* State */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                                                   serverStatus);

    /* BuildInfo */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), serverStatus);

    /* BuildInfo - ProductUri */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI),
                                                   serverStatus);

    /* BuildInfo - ManufacturerName */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME),
                                                   serverStatus);

    /* BuildInfo - ProductName */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME),
                                                   serverStatus);

    /* BuildInfo - SoftwareVersion */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION),
                                                   serverStatus);

    /* BuildInfo - BuildNumber */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER),
                                                   serverStatus);

    /* BuildInfo - BuildDate */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE),
                                                   serverStatus);

#ifdef UA_GENERATED_NAMESPACE_ZERO

    /* SecondsTillShutdown */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN),
                                                   serverStatus);

    /* ShutDownReason */
    UA_LocalizedText shutdownReason;
    UA_LocalizedText_init(&shutdownReason);
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON,
                               &shutdownReason, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    /* ServiceLevel */
    UA_DataSource serviceLevel = {readServiceLevel, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL), serviceLevel);

    /* ServerDiagnostics - EnabledFlag */
#ifdef UA_ENABLE_DIAGNOSTICS
    UA_Boolean enabledFlag = true;
#else
    UA_Boolean enabledFlag = false;
#endif
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG,
                               &enabledFlag, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* According to Specification part-5 - pg.no-11(PDF pg.no-29), when the ServerDiagnostics is disabled the client
     * may modify the value of enabledFlag=true in the server. By default, this node have CurrentRead/Write access.
     * In CTT, Subscription_Minimum_1/002.js test will modify the above flag. This will not be a problem when build
     * configuration is set at UA_NAMESPACE_ZERO="REDUCED" as NodeIds will not be present. When UA_NAMESPACE_ZERO="FULL",
     * the test will fail. Hence made the NodeId as read only */
    retVal |= UA_Server_writeAccessLevel(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG),
                                         UA_ACCESSLEVELMASK_READ);

    /* Auditing */
    UA_DataSource auditing = {readAuditing, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING), auditing);

    /* Redundancy Support */
    UA_RedundancySupport redundancySupport = UA_REDUNDANCYSUPPORT_NONE;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT,
                               &redundancySupport, &UA_TYPES[UA_TYPES_REDUNDANCYSUPPORT]);

    /* Remove unused subtypes of ServerRedundancy */
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_CURRENTSERVERID), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANTSERVERARRAY), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_SERVERURIARRAY), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_SERVERNETWORKGROUPS), true);

    /* ServerCapabilities - LocaleIdArray */
    UA_LocaleId locale_en = UA_STRING("en");
    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY,
                                    &locale_en, 1, &UA_TYPES[UA_TYPES_LOCALEID]);

    /* ServerCapabilities - MaxBrowseContinuationPoints */
    UA_UInt16 maxBrowseContinuationPoints = UA_MAXCONTINUATIONPOINTS;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS,
                               &maxBrowseContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerProfileArray */
    UA_String profileArray[3];
    UA_UInt16 profileArraySize = 0;
#define ADDPROFILEARRAY(x) profileArray[profileArraySize++] = UA_STRING(x)
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/MicroEmbeddedDevice");
#ifdef UA_ENABLE_NODEMANAGEMENT
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NodeManagement");
#endif
#ifdef UA_ENABLE_METHODCALLS
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/Methods");
#endif
    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY,
                                    profileArray, profileArraySize, &UA_TYPES[UA_TYPES_STRING]);

    /* ServerCapabilities - MaxQueryContinuationPoints */
    UA_UInt16 maxQueryContinuationPoints = 0;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS,
                               &maxQueryContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerCapabilities - MaxHistoryContinuationPoints */
    UA_UInt16 maxHistoryContinuationPoints = 0;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS,
                               &maxHistoryContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerCapabilities - MinSupportedSampleRate */
    UA_DataSource samplingInterval = {readMinSamplingInterval, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE),
                                                   samplingInterval);

    /* ServerCapabilities - OperationLimits - MaxNodesPerRead */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREAD,
                               &server->config.maxNodesPerRead, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - maxNodesPerWrite */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERWRITE,
                               &server->config.maxNodesPerWrite, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerMethodCall */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERMETHODCALL,
                               &server->config.maxNodesPerMethodCall, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerBrowse */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERBROWSE,
                               &server->config.maxNodesPerBrowse, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerRegisterNodes */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREGISTERNODES,
                               &server->config.maxNodesPerRegisterNodes, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerTranslateBrowsePathsToNodeIds */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERTRANSLATEBROWSEPATHSTONODEIDS,
                               &server->config.maxNodesPerTranslateBrowsePathsToNodeIds, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerNodeManagement */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERNODEMANAGEMENT,
                               &server->config.maxNodesPerNodeManagement, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxMonitoredItemsPerCall */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXMONITOREDITEMSPERCALL,
                               &server->config.maxMonitoredItemsPerCall, &UA_TYPES[UA_TYPES_UINT32]);

    /* Remove unused operation limit components */
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERHISTORYREADDATA), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERHISTORYREADEVENTS), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERHISTORYUPDATEDATA), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERHISTORYUPDATEEVENTS), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXSTRINGLENGTH), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXARRAYLENGTH), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBYTESTRINGLENGTH), true);

    /* Remove not supported server configurations */
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_ESTIMATEDRETURNTIME), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_LOCALTIME), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_REQUESTSERVERSTATECHANGE), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_RESENDDATA), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SETSUBSCRIPTIONDURABLE), true);

#ifdef UA_ENABLE_DIAGNOSTICS
    /* ServerDiagnostics - ServerDiagnosticsSummary */
    UA_DataSource serverDiagSummary = {readDiagnostics, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - ServerViewCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SERVERVIEWCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - CurrentSessionCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CURRENTSESSIONCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - CumulatedSessionCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CUMULATEDSESSIONCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - SecurityRejectedSessionCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SECURITYREJECTEDSESSIONCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - RejectedSessionCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_REJECTEDSESSIONCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - SessionTimeoutCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SESSIONTIMEOUTCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - SessionAbortCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SESSIONABORTCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - CurrentSubscriptionCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CURRENTSUBSCRIPTIONCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - CumulatedSubscriptionCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CUMULATEDSUBSCRIPTIONCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - PublishingIntervalCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_PUBLISHINGINTERVALCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - SecurityRejectedRequestsCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SECURITYREJECTEDREQUESTSCOUNT), serverDiagSummary);

    /* ServerDiagnostics - ServerDiagnosticsSummary - RejectedRequestsCount */
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_REJECTEDREQUESTSCOUNT), serverDiagSummary);

    /* ServerDiagnostics - SubscriptionDiagnosticsArray */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_DataSource serverSubDiagSummary = {readSubscriptionDiagnosticsArray, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SUBSCRIPTIONDIAGNOSTICSARRAY), serverSubDiagSummary);
#endif

    /* ServerDiagnostics - SessionDiagnosticsSummary - SessionDiagnosticsArray */
    UA_DataSource sessionDiagSummary = {readSessionDiagnosticsArray, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY_SESSIONDIAGNOSTICSARRAY), sessionDiagSummary);

    /* ServerDiagnostics - SessionDiagnosticsSummary - SessionSecurityDiagnosticsArray */
    UA_DataSource sessionSecDiagSummary = {readSessionSecurityDiagnostics, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY_SESSIONSECURITYDIAGNOSTICSARRAY), sessionSecDiagSummary);

#else
    /* Removing these NodeIds make Server Object to be non-complaint with UA
     * 1.03 in CTT (Base Inforamtion/Base Info Core Structure/ 001.js) In the
     * 1.04 specification this has been resolved by allowing to remove these
     * static nodes as well */
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SAMPLINGINTERVALDIAGNOSTICSARRAY), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SUBSCRIPTIONDIAGNOSTICSARRAY), true);
#endif

#ifndef UA_ENABLE_PUBSUB
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE), true);
#endif

#ifndef UA_ENABLE_HISTORIZING
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_HISTORYSERVERCAPABILITIES), true);
#else
    /* ServerCapabilities - HistoryServerCapabilities - AccessHistoryDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_ACCESSHISTORYDATACAPABILITY,
                               &server->config.accessHistoryDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - MaxReturnDataValues */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_MAXRETURNDATAVALUES,
                               &server->config.maxReturnDataValues, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - HistoryServerCapabilities - AccessHistoryEventsCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_ACCESSHISTORYEVENTSCAPABILITY,
                               &server->config.accessHistoryEventsCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - MaxReturnEventValues */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_MAXRETURNEVENTVALUES,
                               &server->config.maxReturnEventValues, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - HistoryServerCapabilities - InsertDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_INSERTDATACAPABILITY,
                               &server->config.insertDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - InsertEventCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_INSERTEVENTCAPABILITY,
                               &server->config.insertEventCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - InsertAnnotationsCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_INSERTANNOTATIONCAPABILITY,
                               &server->config.insertAnnotationsCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - ReplaceDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_REPLACEDATACAPABILITY,
                               &server->config.replaceDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - ReplaceEventCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_REPLACEEVENTCAPABILITY,
                               &server->config.replaceEventCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - UpdateDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_UPDATEDATACAPABILITY,
                               &server->config.updateDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - UpdateEventCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_UPDATEEVENTCAPABILITY,
                               &server->config.updateEventCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - DeleteRawCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_DELETERAWCAPABILITY,
                               &server->config.deleteRawCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - DeleteEventCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_DELETEEVENTCAPABILITY,
                               &server->config.deleteEventCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - DeleteAtTimeDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_DELETEATTIMECAPABILITY,
                               &server->config.deleteAtTimeDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);
#endif

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    retVal |= UA_Server_setMethodNodeCallback(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS), readMonitoredItems);
#endif

    /* The HasComponent references to the ModellingRules are not part of the
     * Nodeset2.xml. So we add the references manually. */
    addModellingRules(server);

#endif /* UA_GENERATED_NAMESPACE_ZERO */

    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Initialization of Namespace 0 (after bootstrapping) "
                     "failed with %s. See previous outputs for any error messages.",
                     UA_StatusCode_name(retVal));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}
