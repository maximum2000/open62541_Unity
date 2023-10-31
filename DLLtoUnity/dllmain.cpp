//M:\GIT2\open62541_Unity\open62541 - 1.3.8\examples\tutorial_client_firststeps.c

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <stdlib.h>
#include <string>









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







//nothrow
extern "C" __declspec(dllexport) int bopen(int a, int b)
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




/*


#include <stdlib.h>

int main(void) {
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

*/

/**
 * Compilation is similar to the server example.
 *
 * .. code-block:: bash
 *
 *     $ gcc -std=c99 open62541.c myClient.c -o myClient
 *
 * In a MinGW environment, the Winsock library must be added.
 *
 * .. code-block:: bash
 *
 *    $ gcc -std=c99 open62541.c myClient.c -lws2_32 -o myClient.exe
 *
 * Further tasks
 * ^^^^^^^^^^^^^
 *
 * - Try to connect to some other OPC UA server by changing
 *   ``opc.tcp://localhost:4840`` to an appropriate address (remember that the
 *   queried node is contained in any OPC UA server).
 *
 * - Try to set the value of the variable node (ns=1,i="the.answer") containing
 *   an ``Int32`` from the example server (which is built in
 *   :doc:`tutorial_server_firststeps`) using "UA_Client_write" function. The
 *   example server needs some more modifications, i.e., changing request types.
 *   The answer can be found in ``examples/client.c``. */



