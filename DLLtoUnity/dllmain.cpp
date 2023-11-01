#include <stdlib.h>
#include <string>


//server
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#pragma comment(lib, "ws2_32.lib")
//It follows the main server code, making use of the above definitions.
//static volatile UA_Boolean running = true;


//client
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <signal.h>

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

//функции клиента:
//1. int OPC_ClientConnect (url) // "opc.tcp://localhost:4840"
//2. int OPC_ClientWriteValueDouble (description, value) //(char*)"the.answer", double
//3. double OPC_ClientReadValueDouble (description) //(char*)"the.answer"
//4. int OPC_ClientUpdate () - обновление клиента
//5. int OPC_ClientDelete() - выключение клиента


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
    UA_Client_delete(client); // Disconnects the client internally 
    return 0;
}




































//--------------OLD and TEST------------------------------------------------------------

/*
tutorial_server_object - инстансы
tutorial_server_variabletype - массивы
tutorial_server_monitoreditems - подписка сервера
tutorial_server_method_async
tutorial_server_method - vtnjls / bynthfrwbb
tutorial_server_events - Triggering an event
tutorial_datatypes - строки / массивы / числа
server_inheritance - примеры объектов
tutorial_client_firststeps - чтение переменной
tutorial_client_events - подписка на мониторинг переменной и события
client_subscription_loop - подписка
client_connect
client_connect_loop - автоподключение
client_async - вызов метода
client - чтение списка всего на сервере
UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "paula", "paula123");
pubsub_realtime - реалтайм
*/


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

