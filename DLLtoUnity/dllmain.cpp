/***************************************************************************
dllmain.cpp  -  OPC UA for Unity C#
used in company projects https://Lcontent.ru
based on OpenRTI
-------------------
begin                : 01 ноября 2023
copyright            : (C) 2023 by Гаммер Максим Дмитриевич (maximum2000)
email                : Maxim.Gammer@yandex.ru
***************************************************************************/

#include <stdlib.h>
#include <string>


//server
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <open62541/client_subscriptions.h>
//#pragma comment(lib, "ws2_32.lib")
//It follows the main server code, making use of the above definitions.
//static volatile UA_Boolean running = true;


//client
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <signal.h>

#ifdef _MSC_VER
#pragma warning(disable:4996) // warning C4996: 'UA_Client_Subscriptions_addMonitoredEvent': was declared deprecated
#pragma warning(disable:4244) // warning C4244

#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif



//функции общие
// RegisterDebugCallback - регистрация callback'ов
//1. SendLog - дебаг-вывод в unity

//функции сервера:
//1. int OPC_ServerCreate ()
//2. int OPC_ServerUpdate ()
//3. int OPC_ServerAddVariableDouble (description, displayName) // (char*)"the.answer", (char*)"the answer"
//4. int OPC_ServerWriteValueDouble (description, value) //(char*)"the.answer", double
//5. double OPC_ServerReadValueDouble (description) //(char*)"the.answer"
//6. int OPC_ServerShutdown - выключение сервера
//7. int OPC_ServerCreateMethod() - создание метода и обработчик метода

//функции клиента:
//1. int OPC_ClientConnect (url) // "opc.tcp://localhost:4840"
//2. int OPC_ClientWriteValueDouble (description, value) //(char*)"the.answer", double
//3. double OPC_ClientReadValueDouble (description) //(char*)"the.answer"
//4. int OPC_ClientUpdate () - обновление клиента
//5. int OPC_ClientDelete() - выключение клиента
// 
//6. int OPC_ClientCallMethod - вызов метода
// int OPC_ClientSubscription() - подписка


//сделать:
//tutorial_server_object - объектыи инстансы                                                                    +-
    //tutorial_client_events - подписка на мониторинг переменной и события                                      +-
 //tutorial_server_method (tutorial_server_method_async) - методы                                               +-
    //client  - вызов метода                                                                                    +-
    


//интересно:
//UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "paula", "paula123");    -
//tutorial_server_monitoreditems - подписка на изменение сервера (callback)                                     -
//pubsub_realtime - реалтайм                                                                                    -
//tutorial_datatypes - строки / массивы / числа                                                                 -
//server_inheritance - примеры объектов                                                                         -
//tutorial_server_variabletype - массивы                                                                        -
//client - чтение списка всего на сервере                                                                       -
//client_find_servers - поиск сервера                                                                           -
//client_subscription_loop - обработка закрытия сервера                                                         -
//client_connect_loop - автоподключение                                                                         -
//client - создание объектов, ччтение состава сервера и т.д.                                                    -
//tutorial_server_events - триггеры и события                                                                   -



//глобальные переменные:
UA_Server* server;
UA_Boolean waitInternal = false;


//---------------------------------DEBUG---------------------------------------
// https://github.com/mkowalik92/UnityNativePluginPractice
// Call this function from a Untiy script
extern "C"
{
    typedef void(*DebugCallback)(const char* message, int color, int size);
    static DebugCallback callbackFunction = nullptr;
    __declspec(dllexport) void RegisterDebugCallback(DebugCallback callback);
}
void RegisterDebugCallback(DebugCallback callback)
{
    callbackFunction = callback;
}
//nothrow
void SendLog(const std::wstring& str, const int& color)
{
    std::string s(str.begin(), str.end());
    const char* tmsg = s.c_str();
    if (callbackFunction != nullptr)
    {
        callbackFunction(tmsg, (int)color, (int)strlen(tmsg));
    }
}
//SendLog(L"debug DLL:connectionLost", 0);
 
 
 

//---------------------------------SERVER---------------------------------------

//1. int OPCserverCreate () - создание сервера OPC
extern "C" __declspec(dllexport) int OPC_ServerCreate()
{
    SendLog(L"debug DLL:OPCserverCreate ...", 0);

    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_ServerConfig* config = UA_Server_getConfig(server);
    config->verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT;
    #ifdef UA_ENABLE_WEBSOCKET_SERVER
        UA_ServerConfig_addNetworkLayerWS(UA_Server_getConfig(server), 7681, 0, 0, NULL, NULL);
    #endif
    UA_StatusCode retval = UA_Server_run_startup(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        SendLog(L"debug DLL:OPCserverCreate... false", 0);
        UA_Server_delete(server);
        retval = UA_Server_run_shutdown(server);
        return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    SendLog(L"debug DLL:OPC_ServerCreate... ok", 0);
    return 0;
}

//2. int OPC_ServerUpdate () - обновление сервера
extern "C" __declspec(dllexport) int OPC_ServerUpdate()
{
    UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);
    return 0;
}

//3. int OPC_ServerAddVariableDouble (description, displayName) // (char*)"the.answer", (char*)"the answer"
extern "C" __declspec(dllexport)  int OPC_ServerAddVariableDouble(char* descriptionString, char* displayNameString)
{
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Double myDouble = 0;
    UA_Variant_setScalar(&attr.value, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT((char*)"en - US", descriptionString);   //(char*)"the.answer"
    attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", displayNameString);     //(char*)"the answer"
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, descriptionString);
    UA_QualifiedName myDoubleName = UA_QUALIFIEDNAME(1, displayNameString);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,parentReferenceNodeId, myDoubleName, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
    return 0;
}

//4. int OPC_ServerWriteValueDouble (description, value) //(char*)"the.answer", double
extern "C" __declspec(dllexport)  int OPC_ServerWriteValueDouble(char* descriptionString, double value)
{
    UA_NodeId myDoubleNodeId = UA_NODEID_STRING(1, descriptionString);

    /* Write a different double value */
    UA_Double myDouble = value;
    UA_Variant myVar;
    UA_Variant_init(&myVar);
    UA_Variant_setScalar(&myVar, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, myDoubleNodeId, myVar);

    /* Set the status code of the value to an error code. The function
     * UA_Server_write provides access to the raw service. The above
     * UA_Server_writeValue is syntactic sugar for writing a specific node
     * attribute with the write service. */
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = myDoubleNodeId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.status = UA_STATUSCODE_BADNOTCONNECTED;
    wv.value.hasStatus = true;
    UA_Server_write(server, &wv);

    /* Reset the variable to a good statuscode with a value */
    wv.value.hasStatus = false;
    wv.value.value = myVar;
    wv.value.hasValue = true;
    UA_Server_write(server, &wv);

    return 0;
}



//5. double OPC_ServerReadValueDouble (description) //(char*)"the.answer"
extern "C" __declspec(dllexport) double OPC_ServerReadValueDouble(char* descriptionString)
{
    UA_NodeId myDoubleNodeId = UA_NODEID_STRING(1, descriptionString);
    UA_Variant out;
    UA_Variant_init(&out);
    UA_Server_readValue(server, myDoubleNodeId, &out);
    double value = *(UA_Double*)out.data;
    /* Clean up */
    UA_Variant_clear(&out);
    return value;
}

//6. int OPC_ServerShutdown
extern "C" __declspec(dllexport) int OPC_ServerShutdown()
{
    SendLog(L"debug DLL:OPC_ServerShutdown... ", 0);
    UA_Server_delete(server);
    UA_StatusCode retval = UA_Server_run_shutdown(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}



//---------------------------------CLIENT---------------------------------------
UA_Client* client;
//1. int OPC_ClientConnect (url) // "opc.tcp://localhost:4840"
extern "C" __declspec(dllexport) int OPC_ClientConnect(char* url)
{
    SendLog(L"debug DLL:OPC_ClientConnect... ", 0);
    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, url); //"opc.tcp://localhost:4840"
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_Client_delete(client);
        SendLog(L"debug DLL:OPC_ClientConnect... false", 0);
        return (int)retval;
    }
    SendLog(L"debug DLL:OPC_ClientConnect... ok", 0);
    return 0;
}

//2. int OPC_ClientWriteValueDouble (description, value) //(char*)"the.answer", double
extern "C" __declspec(dllexport) int OPC_ClientWriteValueDouble(char* description, double _value)
{
    UA_NodeId myDoubleNodeId = UA_NODEID_STRING(1, description);

    //Write a different double value
    UA_Double myDouble = _value;
    //UA_Variant myVar;
    //UA_Variant_init(&myVar);
    //UA_Variant_setScalar(&myVar, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
    //UA_Client_writeValueAttribute(client, myDoubleNodeId, &myVar);

    UA_Variant* myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Client_writeValueAttribute(client, myDoubleNodeId, myVariant);
    UA_Variant_delete(myVariant);


    return 0;
}

//3. double OPC_ClientReadValueDouble (description) //(char*)"the.answer"
extern "C" __declspec(dllexport) double OPC_ClientReadValueDouble(char* description)
{
    // Read the value attribute of the node. UA_Client_readValueAttribute is a
    // wrapper for the raw read service available as UA_Client_Service_read. 
    UA_Variant value; // Variants can hold scalar values and arrays of any type 
    UA_Variant_init(&value);

    // NodeId of the variable holding the current time 
    UA_NodeId myDoubleNodeId = UA_NODEID_STRING(1, description); //(char*)"the.answer"
    UA_StatusCode retval = UA_Client_readValueAttribute(client, myDoubleNodeId, &value);
    if (retval == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE]))
    {
        double val = *(UA_Double*)value.data;
        // Clean up 
        UA_Variant_clear(&value);
        return val;
    }
    SendLog(L"debug DLL:5", 0);

    // Clean up 
    UA_Variant_clear(&value);
    return -1;
}

//4. int OPC_ClientUpdate () - обновление клиента
extern "C" __declspec(dllexport) int OPC_ClientUpdate()
{
    UA_UInt16 timeout = UA_Client_run_iterate(client, waitInternal);
    return 0;
}

//5. int OPC_ClientDelete()
extern "C" __declspec(dllexport) int OPC_ClientDelete()
{
    UA_Client_disconnect(client);
    UA_Client_delete(client); // Disconnects the client internally 
    return 0;
}












//---------------------в разработке-------------------------------
//tutorial_client_events - подписка на мониторинг переменной и события                                      +
/* Create a subscription */

#ifdef UA_ENABLE_SUBSCRIPTIONS

static void handler_events(UA_Client* client, UA_UInt32 subId, void* subContext,  UA_UInt32 monId, void* monContext, size_t nEventFields, UA_Variant* eventFields) 
{
    //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Notification");
    SendLog(L"Notification", 0);

    /* The context should point to the monId on the stack */
    UA_assert(*(UA_UInt32*)monContext == monId);

    for (size_t i = 0; i < nEventFields; ++i)
    {
        if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_UINT16])) 
        {
            UA_UInt16 severity = *(UA_UInt16*)eventFields[i].data;
            //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Severity: %u", severity);
        }
        else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) 
        {
            UA_LocalizedText* lt = (UA_LocalizedText*)eventFields[i].data;
            //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Message: '%.*s'", (int)lt->text.length, lt->text.data);
        }
        else 
        {
            #ifdef UA_ENABLE_TYPEDESCRIPTION
                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Don't know how to handle type: '%s'", eventFields[i].type->typeName);
            #else
                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Don't know how to handle type, enable UA_ENABLE_TYPEDESCRIPTION for typename");
            #endif
        }
    }
}

const size_t nSelectClauses = 2;

static UA_SimpleAttributeOperand*  setupSelectClauses(void) 
{
    UA_SimpleAttributeOperand* selectClauses = (UA_SimpleAttributeOperand*) UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if (!selectClauses)return NULL;

    for (size_t i = 0; i < nSelectClauses; ++i) 
    {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
    }

    selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[0].browsePathSize = 1;
    selectClauses[0].browsePath = (UA_QualifiedName*)  UA_Array_new(selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if (!selectClauses[0].browsePath) 
    {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");

    selectClauses[1].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[1].browsePathSize = 1;
    selectClauses[1].browsePath = (UA_QualifiedName*) UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if (!selectClauses[1].browsePath) 
    {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[1].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");

    return selectClauses;
}
#endif

extern "C" __declspec(dllexport) int OPC_ClientSubscription()
{
    /*
    #ifdef UA_ENABLE_SUBSCRIPTIONS
    static void
    handler_TheAnswerChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                                UA_UInt32 monId, void *monContext, UA_DataValue *value) {
        printf("The Answer has changed!\n");
    }
    #endif
    // Create a subscription 
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
        NULL, NULL, NULL);

    UA_UInt32 subId = response.subscriptionId;
    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        printf("Create subscription succeeded, id %u\n", subId);

    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(1, "the.answer"));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
            UA_TIMESTAMPSTORETURN_BOTH,
            monRequest, NULL, handler_TheAnswerChanged, NULL);
    if (monResponse.statusCode == UA_STATUSCODE_GOOD)
        printf("Monitoring 'the.answer', id %u\n", monResponse.monitoredItemId);


    // The first publish request should return the initial value of the variable 
    UA_Client_run_iterate(client, 1000);
    */

    SendLog(L"Create subscription succeeded", 0);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,  NULL, NULL, NULL);
    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) 
    {
        return EXIT_FAILURE;
    }
    UA_UInt32 subId = response.subscriptionId;
    //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Create subscription succeeded, id %u", subId);
    SendLog(L"Create subscription succeeded", 0);

    /* Add a MonitoredItem */
    //UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(1, "the.answer"));

    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, 2253); // UA_NODEID_STRING(1, (char*)"the.answer"); // UA_NODEID_NUMERIC(0, 2253); // Root->Objects->Server
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = setupSelectClauses();
    filter.selectClausesSize = nSelectClauses;

    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.data = &filter;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];

    UA_UInt32 monId = 0;

    UA_MonitoredItemCreateResult result =  UA_Client_MonitoredItems_createEvent(client, subId, UA_TIMESTAMPSTORETURN_BOTH, item, &monId, handler_events, NULL);

    if (result.statusCode != UA_STATUSCODE_GOOD) 
    {
        //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Could not add the MonitoredItem with" ); //UA_StatusCode_name(retval)
        SendLog(L"Could not add the MonitoredItem", 0);
        goto cleanup;
    }
    else 
    {
        //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Monitoring 'Root->Objects->Server', id %u", response.subscriptionId);
        SendLog(L"Monitoring Root->Objects->Server", 0);
    }

    monId = result.monitoredItemId;




    /* Delete the subscription */
cleanup:
    //UA_MonitoredItemCreateResult_clear(&result);
    //UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    //UA_Array_delete(filter.selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);

#endif
    return 0;
}

//---------------------в разработке-------------------------------


//--------------------В РАЗРАБОТКЕ--------------------------------
//tutorial_server_object.c
/*
static void manuallyDefinePump(UA_Server *server)
{
    UA_NodeId pumpId; // get the nodeid assigned by the server
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Pump (Manual)");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Pump (Manual)"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &pumpId);

    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    UA_String manufacturerName = UA_STRING("Pump King Ltd.");
    UA_Variant_setScalar(&mnAttr.value, &manufacturerName, &UA_TYPES[UA_TYPES_STRING]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ManufacturerName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), mnAttr, NULL, NULL);

    UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
    UA_String modelName = UA_STRING("Mega Pump 3000");
    UA_Variant_setScalar(&modelAttr.value, &modelName, &UA_TYPES[UA_TYPES_STRING]);
    modelAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ModelName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ModelName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), modelAttr, NULL, NULL);

    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    UA_Boolean status = true;
    UA_Variant_setScalar(&statusAttr.value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
    statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Status"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr, NULL, NULL);

    UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
    UA_Double rpm = 50.0;
    UA_Variant_setScalar(&rpmAttr.value, &rpm, &UA_TYPES[UA_TYPES_DOUBLE]);
    rpmAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorRPM");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "MotorRPMs"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), rpmAttr, NULL, NULL);
}


// predefined identifier for later use
UA_NodeId pumpTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

static void
defineObjectTypes(UA_Server *server) {
    // Define the object type for "Device"
    UA_NodeId deviceTypeId; // get the nodeid assigned by the server
    UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DeviceType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "DeviceType"), dtAttr,
                                NULL, &deviceTypeId);

    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_NodeId manufacturerNameId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ManufacturerName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), mnAttr, NULL, &manufacturerNameId);
    // Make the manufacturer name mandatory //
    UA_Server_addReference(server, manufacturerNameId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);


    UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
    modelAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ModelName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ModelName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), modelAttr, NULL, NULL);

    // Define the object type for "Pump" //
    UA_ObjectTypeAttributes ptAttr = UA_ObjectTypeAttributes_default;
    ptAttr.displayName = UA_LOCALIZEDTEXT("en-US", "PumpType");
    UA_Server_addObjectTypeNode(server, pumpTypeId,
                                deviceTypeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "PumpType"), ptAttr,
                                NULL, NULL);

    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    statusAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId statusId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Status"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr, NULL, &statusId);
    // Make the status variable mandatory //
    UA_Server_addReference(server, statusId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

    UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
    rpmAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorRPM");
    rpmAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "MotorRPMs"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), rpmAttr, NULL, NULL);
}



static void
addPumpObjectInstance(UA_Server *server, char *name) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, name),
                            pumpTypeId, // this refers to the object type identifier 
oAttr, NULL, NULL);
}


static UA_StatusCode
pumpTypeConstructor(UA_Server* server,
    const UA_NodeId* sessionId, void* sessionContext,
    const UA_NodeId* typeId, void* typeContext,
    const UA_NodeId* nodeId, void** nodeContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "New pump created");

    // Find the NodeId of the status child variable
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = UA_QUALIFIEDNAME(1, "Status");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = *nodeId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr =
        UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if (bpr.statusCode != UA_STATUSCODE_GOOD ||
        bpr.targetsSize < 1)
        return bpr.statusCode;

    // Set the status value
    UA_Boolean status = true;
    UA_Variant value;
    UA_Variant_setScalar(&value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_clear(&bpr);

    // At this point we could replace the node context ..

    return UA_STATUSCODE_GOOD;
}

static void
addPumpTypeConstructor(UA_Server* server) {
    UA_NodeTypeLifecycle lifecycle;
    lifecycle.constructor = pumpTypeConstructor;
    lifecycle.destructor = NULL;
    UA_Server_setNodeTypeLifecycle(server, pumpTypeId, lifecycle);
}

// It follows the main server code, making use of the above definitions. 

static volatile UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server* server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    manuallyDefinePump(server);
    defineObjectTypes(server);
    addPumpObjectInstance(server, "pump2");
    addPumpObjectInstance(server, "pump3");
    addPumpTypeConstructor(server);
    addPumpObjectInstance(server, "pump4");
    addPumpObjectInstance(server, "pump5");

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
*/
//

//--------------------В РАЗРАБОТКЕ--------------------------------



//--------------------В РАЗРАБОТКЕ--------------------------------
//tutorial_server_method.c

static UA_StatusCode helloWorldMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) 
{
    UA_String *inputStr = (UA_String*)input->data;
    UA_String tmp = UA_STRING_ALLOC("Hello ");
    if(inputStr->length > 0) {
        tmp.data = (UA_Byte *)UA_realloc(tmp.data, tmp.length + inputStr->length);
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&tmp);
    //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Hello World was called");
    SendLog(L"debug DLL:Hello World was called", 0);

    return UA_STATUSCODE_GOOD;
}

static void addHelloWorldMethod(UA_Server *server) 
{
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"A String");
    inputArgument.name = UA_STRING((char*)"MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"A String");
    outputArgument.name = UA_STRING((char*)"MyOutput");
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"Say `Hello World`");
    helloAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"Hello World");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, (char*)"hello world"),
                            helloAttr, &helloWorldMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
}

extern "C" __declspec(dllexport) int OPC_ServerCreateMethod()
{
    addHelloWorldMethod(server);
    return 0;
}
//--------------------В РАЗРАБОТКЕ--------------------------------




//--------------------В РАЗРАБОТКЕ--------------------------------
extern "C" __declspec(dllexport) int OPC_ClientCallMethod()
{
#ifdef UA_ENABLE_METHODCALLS
    /* Call a remote method */
    UA_Variant input;
    UA_String argString = UA_STRING((char*)"Hello Server");
    UA_Variant_init(&input);
    UA_Variant_setScalarCopy(&input, &argString, &UA_TYPES[UA_TYPES_STRING]);
    size_t outputSize;
    UA_Variant* output;
    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),UA_NODEID_NUMERIC(1, 62541), 1, &input, &outputSize, &output);
    if (retval == UA_STATUSCODE_GOOD) 
    {
        //printf("Method call was successful, and %lu returned values available.\n",      (unsigned long)outputSize);
        SendLog(L"debug DLL:Method call was successful", 0);
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    }
    else 
    {
        //printf("Method call was unsuccessful, and %x returned values available.\n", retval);
        SendLog(L"debug DLL:Method call was unsuccessful", 0);
    }
    UA_Variant_clear(&input);
#endif
    return 0;
}
//--------------------В РАЗРАБОТКЕ--------------------------------


















//--------------OLD and TEST------------------------------------------------------------




static void addVariable(UA_Server* server)
{
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT((char*)"en - US", (char*)"theanswer");
    attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"theanswer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, (char*)"AAA");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, (char*)"AAA");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
        parentReferenceNodeId, myIntegerName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}


static void addMatrixVariable(UA_Server* server) 
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)(char*)"DoubleMatrix");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    //Set the variable value constraints 
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.valueRank = UA_VALUERANK_TWO_DIMENSIONS;
    UA_UInt32 arrayDims[2] = { 2,2 };
    attr.arrayDimensions = arrayDims;
    attr.arrayDimensionsSize = 2;

    // Set the value. The array dimensions need to be the same for the value.
    UA_Double zero[4] = { 0.0, 0.0, 0.0, 0.0 };
    UA_Variant_setArray(&attr.value, zero, 4, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.value.arrayDimensions = arrayDims;
    attr.value.arrayDimensionsSize = 2;

    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, (char*)"double.matrix");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, (char*)"doublematrix");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
        parentReferenceNodeId, myIntegerName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
}


//Now we change the value with the write service. This uses the same service
//implementation that can also be reached over the network by an OPC UA client.
static void writeVariable(UA_Server* server, int value) 
{
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, (char*)"AAA");

    /* Write a different integer value */
    UA_Int32 myInteger = value;
    UA_Variant myVar;
    UA_Variant_init(&myVar);
    UA_Variant_setScalar(&myVar, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, myIntegerNodeId, myVar);

    /* Set the status code of the value to an error code. The function
     * UA_Server_write provides access to the raw service. The above
     * UA_Server_writeValue is syntactic sugar for writing a specific node
     * attribute with the write service. */
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = myIntegerNodeId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.status = UA_STATUSCODE_BADNOTCONNECTED;
    wv.value.hasStatus = true;
    UA_Server_write(server, &wv);

    /* Reset the variable to a good statuscode with a value */
    wv.value.hasStatus = false;
    wv.value.value = myVar;
    wv.value.hasValue = true;
    UA_Server_write(server, &wv);
}

extern "C" __declspec(dllexport) int testServerCreate(int a, int b)
{
    SendLog(L"debug DLL:testServer", 0);

    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_ServerConfig* config = UA_Server_getConfig(server);
    config->verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT;
    //config->applicationDescription.applicationName
    #ifdef UA_ENABLE_WEBSOCKET_SERVER
        UA_ServerConfig_addNetworkLayerWS(UA_Server_getConfig(server), 7681, 0, 0, NULL, NULL);
    #endif

    SendLog(L"debug DLL:addVariable", 0);
    addVariable(server);
    addMatrixVariable(server);
    writeVariable(server,54321);

    SendLog(L"debug DLL:UA_Server_run_startup", 0);
    UA_StatusCode retval = UA_Server_run_startup(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        goto cleanup;
    }

    return 0;

    /*
    retval = UA_Server_run_shutdown(server);
    */

cleanup:
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

extern "C" __declspec(dllexport) int testServerUpdate()
{
    //writeVariable(server, a);
    UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);
    return 0;
}



extern "C" __declspec(dllexport) int testServerWrite(int a)
{
    writeVariable(server, a);
    return 0;
}

extern "C" __declspec(dllexport) int testServerRead()
{
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, (char*)"AAA");
    UA_Variant out;
    UA_Variant_init(&out);
    UA_Server_readValue(server, myIntegerNodeId, &out);
    int p = *(UA_Int32*)out.data;
    /* Clean up */
    UA_Variant_clear(&out);
    
    return p;
}






//-------------------------------------------------------------------------------------------------------------



extern "C" __declspec(dllexport) double testClient(char* description)
{
    UA_Client* client2 = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client2));
    UA_StatusCode retval = UA_Client_connect(client2, "opc.tcp://localhost:4840");
    if (retval != UA_STATUSCODE_GOOD) 
    {
        UA_Client_delete(client2);
        return (int)retval;
    }

    // Read the value attribute of the node. UA_Client_readValueAttribute is a
    // wrapper for the raw read service available as UA_Client_Service_read. 
    UA_Variant value; // Variants can hold scalar values and arrays of any type 
    UA_Variant_init(&value);

    // NodeId of the variable holding the current time 
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, description);//(char*)"AAA"
    retval = UA_Client_readValueAttribute(client2, myIntegerNodeId, &value);

    

    double p = -4;
    if (retval == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) //UA_TYPES_INT32
    {
        p = *(UA_Double*)value.data; //UA_Int32
    }




    {
        UA_NodeId myDoubleNodeId = UA_NODEID_STRING(1, description);

        //Write a different double value
        UA_Double myDouble = 125;
        //UA_Variant myVar;
        //UA_Variant_init(&myVar);
        //UA_Variant_setScalar(&myVar, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
        //UA_Client_writeValueAttribute(client, myDoubleNodeId, &myVar);

        UA_Variant* myVariant = UA_Variant_new();
        UA_Variant_setScalarCopy(myVariant, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
        UA_Client_writeValueAttribute(client2, myDoubleNodeId, myVariant);
        UA_Variant_delete(myVariant);
    }









    // Clean up 
    UA_Variant_clear(&value);
    UA_Client_delete(client2); // Disconnects the client internally 
    return p;
}

