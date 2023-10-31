#include <stdlib.h>
#include <string>


//server
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#pragma comment(lib, "ws2_32.lib")


//client
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <signal.h>










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


//-----------------------------------------------------------------------------------------



static void addVariable(UA_Server* server)
{
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT((char*)"en - US", (char*)"the answer");
    attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"the answer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, (char*)"the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, (char*)"the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
        parentReferenceNodeId, myIntegerName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}


static void addMatrixVariable(UA_Server* server) 
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)(char*)"Double Matrix");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Set the variable value constraints */
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.valueRank = UA_VALUERANK_TWO_DIMENSIONS;
    UA_UInt32 arrayDims[2] = { 2,2 };
    attr.arrayDimensions = arrayDims;
    attr.arrayDimensionsSize = 2;

    /* Set the value. The array dimensions need to be the same for the value. */
    UA_Double zero[4] = { 0.0, 0.0, 0.0, 0.0 };
    UA_Variant_setArray(&attr.value, zero, 4, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.value.arrayDimensions = arrayDims;
    attr.value.arrayDimensionsSize = 2;

    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, (char*)"double.matrix");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, (char*)"double matrix");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
        parentReferenceNodeId, myIntegerName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
}

/**
 * Now we change the value with the write service. This uses the same service
 * implementation that can also be reached over the network by an OPC UA client.
 */

static void writeVariable(UA_Server* server, int value) 
{
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, (char*)"the.answer");

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

/**
 * Note how we initially set the DataType attribute of the variable node to the
 * NodeId of the Int32 data type. This forbids writing values that are not an
 * Int32. The following code shows how this consistency check is performed for
 * every write.
 */

static void writeWrongVariable(UA_Server* server) 
{
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, (char*)"the.answer");

    /* Write a string */
    UA_String myString = UA_STRING((char*)"test");
    UA_Variant myVar;
    UA_Variant_init(&myVar);
    UA_Variant_setScalar(&myVar, &myString, &UA_TYPES[UA_TYPES_STRING]);
    UA_StatusCode retval = UA_Server_writeValue(server, myIntegerNodeId, myVar);
    printf("Writing a string returned statuscode %s\n", UA_StatusCode_name(retval));
}

/** It follows the main server code, making use of the above definitions. */

static volatile UA_Boolean running = true;
//static void stopHandler(int sign) {
//    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
//    running = false;
//}




UA_Server* server;
UA_Boolean waitInternal = false;

extern "C" __declspec(dllexport) int testServerCreate(int a, int b)
{
   // signal(SIGINT, stopHandler);
   // signal(SIGTERM, stopHandler);
    
   SendLog(L"debug DLL:testServer", 0);

    //UA_Server* server = UA_Server_new();
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_ServerConfig* config = UA_Server_getConfig(server);
    config->verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT;
    #ifdef UA_ENABLE_WEBSOCKET_SERVER
        UA_ServerConfig_addNetworkLayerWS(UA_Server_getConfig(server), 7681, 0, 0, NULL, NULL);
    #endif

    SendLog(L"debug DLL:addVariable", 0);
    addVariable(server);
    addMatrixVariable(server);
    writeVariable(server,54321);
    writeWrongVariable(server);

                    //UA_StatusCode retval = UA_Server_run(server, &running);
                    //UA_Server_delete(server);
                    //return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;

    /* Should the server networklayer block (with a timeout) until a message
       arrives or should it return immediately? */
    //UA_Boolean waitInternal = false;


    SendLog(L"debug DLL:UA_Server_run_startup", 0);
    UA_StatusCode retval = UA_Server_run_startup(server);
    if (retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    //!!!!
    return 0;

    /*
    while (running) 
    {
        // timeout is the maximum possible delay (in millisec) until the next
        //   _iterate call. Otherwise, the server might miss an internal timeout
        //   or cannot react to messages with the promised responsiveness.
            //   If multicast discovery server is enabled, the timeout does not not consider new input data (requests) on the mDNS socket.
            //   It will be handled on the next call, which may be too late for requesting clients.
            //  if needed, the select with timeout on the multicast socket server->mdnsSocket (see example in mdnsd library)
        
        UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);

        // Now we can use the max timeout to do something else. In this case, we just sleep. (select is used as a platform-independent sleep function.) 
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = timeout * 1000;
        select(0, NULL, NULL, NULL, &tv);
    }
    retval = UA_Server_run_shutdown(server);
    */

cleanup:
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

extern "C" __declspec(dllexport) int testServerUpdate(int a, int b)
{
    writeVariable(server, a);
    UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);
    return 0;
}
















//-------------------------------------------------------------------------------------------------------------

//nothrow
extern "C" __declspec(dllexport) int testClient(int a, int b)
{
    UA_Client* client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    // Read the value attribute of the node. UA_Client_readValueAttribute is a
    // wrapper for the raw read service available as UA_Client_Service_read. 
    UA_Variant value; // Variants can hold scalar values and arrays of any type 
    UA_Variant_init(&value);

    // NodeId of the variable holding the current time 
    const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if (retval == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime*)value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "date is: %u-%u-%u %u:%u:%u.%03u\n",
            dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }

    // Clean up 
    UA_Variant_clear(&value);
    UA_Client_delete(client); // Disconnects the client internally 
    return EXIT_SUCCESS;
}



























/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

 /**
  * Building a Simple Client
  * ------------------------
  * You should already have a basic server from the previous tutorials. open62541
  * provides both a server- and clientside API, so creating a client is as easy as
  * creating a server. Copy the following into a file `myClient.c`: */

/*
//подключиться к серверу федерации
int MyConnect(std::wstring IP)
{
    IP = L"";
    LastErrorString = L"";
    FederationName = L"";
    FederateName = L"";

    try
    {
        ambassador->connect(L"rti://127.0.0.1"); //thread:// L"rti://127.0.0.1"
    }
    catch (const rti1516e::Exception& e)
    {
        LastErrorString = e.what();
        std::wcout << L"rti1516e::Exception: \"" << LastErrorString << L"\"" << std::endl;
        return 1;
    }
    catch (...)
    {
        LastErrorString = L"Unknown Exception!";
        std::wcout << LastErrorString << std::endl;
        return 1;
    }
    return 0;
}
//первичное подключение к серверу (RTInode)
int Connect(char* myString, int length)
{
    std::wstringstream cls1;
    cls1 << myString;
    IP  = cls1.str();

    //OpenRTI::RTI1516ESimpleAmbassador ambassador;
    ambassador = new OpenRTI::RTI1516EAmbassadorLContent();
    ambassador->setUseDataUrlObjectModels(false);

    

int ret = MyConnect(IP);
if (ret == 1)
{
    std::string s(LastErrorString.begin(), LastErrorString.end());
    strcpy_s(myString, length, s.c_str());
    return 1;
}

//std::cout << "Max Gammer Test 2" << std::endl;
std::string s("ok");
strcpy_s(myString, length, s.c_str());
return 0;
}
*/




