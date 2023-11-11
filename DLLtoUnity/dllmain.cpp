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
#include <sstream>
#include <map>

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

class ObjectNodeVariables
{
public:

    UA_NodeId nodeId;
    //связи имя имя_атрибута -> nodeID
    std::map < std::string, UA_NodeId> VariableNode_NameID;
};
//связи имя объекта_имя ->  nodeID для создаваемых сервером объектов (для сервера)
std::map < std::string, ObjectNodeVariables*> ObjectNodes;

//связи имя объекта_имя ->  nodeID для прочитанных клиентом объектов (для клиента), см. OPC_UA_Client_Service_browse
std::map < std::string, ObjectNodeVariables*> ClientObjectNodes;

//функции общие
// RegisterDebugCallback - регистрация callback'ов
//1. SendLog - дебаг-вывод в unity

//функции сервера:
//1. int OPC_ServerCreate () - создание сервера
//2. int OPC_ServerUpdate () - обновление сервера
//3. int OPC_ServerAddVariableDouble ( objectname, description, displayName) // (char*)"the.answer", (char*)"the answer" - добавление переменной
//4. int OPC_ServerWriteValueDouble (objectname, description, value) //(char*)"the.answer", double - запись значения переменной
//5. double OPC_ServerReadValueDouble (objectname, description) //(char*)"the.answer" - чтение переменной
//6. int OPC_ServerWriteValueString (objectname, description, value) 
//7. double OPC_ServerReadValueString (objectname, description) 
//8. int OPC_ServerShutdown - выключение сервера 
//9. int OPC_ServerCreateMethod(unsigned int nodeID, char* name, char* displayName, char* description) - создание метода и обработчик метода
//10. int OPC_ServerCallMethod(unsigned int NodeId, char* value)  - Вызов метода  из сервера 


//Про методы: например можно сделать метод - создать игрока передать туда имя и тип и в ответ получить ок или не ок, можно и в ответ имя получить
// UA_NODEID_NUMERIC / UA_NODEID_STRING 0 - NAMESPACE NS0



//функции клиента:
//1. int OPC_ClientConnect (url) // "opc.tcp://localhost:4840" - подключение к серверу
//2. int OPC_ClientWriteValueDouble (description, value) //(char*)"the.answer", double - запись значения переменной
//3. double OPC_ClientReadValueDouble (description) //(char*)"the.answer" - чтение значения переменной
//4. int OPC_ClientUpdate () - обновление клиента
//5. int OPC_ClientDelete() - выключение клиента
//6. int OPC_ClientCallMethod (unsigned int NodeId, char* value) - вызов метода
//7. int OPC_ClientSubscription(char* varname, double interval) - подписка на изменение значения переменной
//8. int OPC_UA_Client_Service_browse() - читает всю структуру иерархии дерева с сервера для возможности писать не только в корневые объекты, возвращает число прочитанных узлов


//сделать:
//!!!!
//1. ПОДПИСКА >1 ПЕРЕМЕННЫХ НЕ  РАБОТАЕТ, нужно подписывать сразу на несколько за один вызов на 1 callback 
    //(open62541/tests/client/check_client_subscriptions.c)
    //https://github.com/open62541/open62541/issues/2094
    //!!!
//2. tutorial_server_object - объектыи инстансы                                                                    +-

 
    


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

// Call this function from a Untiy script
extern "C"
{
    typedef void(*MethodCallCallback)(const char* message, unsigned int nodeid, int size);
    static MethodCallCallback callbackMethodCallFunction = nullptr;
    __declspec(dllexport) void RegisterMethodCallCallback(MethodCallCallback callback);
}
void RegisterMethodCallCallback(MethodCallCallback callback)
{
    callbackMethodCallFunction = callback;
}
//nothrow
void SendMethodCall(const std::wstring& str, const unsigned int& nodeid)
{
    std::string s(str.begin(), str.end());
    const char* tmsg = s.c_str();
    if (callbackMethodCallFunction != nullptr)
    {
        callbackMethodCallFunction(tmsg, (unsigned int)nodeid, (int)strlen(tmsg));
    }
}
//SendMethodCall(L"Hello word", 62541);

// Call this function from a Untiy script
extern "C"
{
    typedef void(*ValueChangeCallback)(unsigned int monID, double value);
    static ValueChangeCallback callbackValueChangeFunction = nullptr;
    __declspec(dllexport) void RegisterValueChangeCallback(ValueChangeCallback callback);
}
void RegisterValueChangeCallback(ValueChangeCallback callback)
{
    callbackValueChangeFunction = callback;
}
//nothrow
void SendValueChange(const unsigned int& monID, const double& value)
{
    if (callbackValueChangeFunction != nullptr)
    {
        callbackValueChangeFunction((unsigned int)monID, (double)value);
    }
}
//SendValueChange(1000, 3.141516);


 
 

//---------------------------------SERVER---------------------------------------

//1. int OPCserverCreate () - создание сервера OPC
extern "C" __declspec(dllexport) int OPC_ServerCreate()
{
    SendLog(L"debug DLL:OPCserverCreate ...", 0);

    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_ServerConfig* config = UA_Server_getConfig(server);
    config->applicationDescription.applicationName =  UA_LOCALIZEDTEXT((char*)"en - US", (char*)"LContent OPC server");
    config->applicationDescription.productUri = UA_STRING((char*)"https://LContent.ru");
    //config->applicationDescription.applicationUri = UA_STRING((char*)"urn:unconfigured:application");
    config->buildInfo.manufacturerName = UA_STRING((char*)"LContent.ru");
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

//3. int OPC_ServerAddVariable (description, displayName) // (char*)"the.answer", (char*)"the answer"
//type==0 - double
//type==1 - string
extern "C" __declspec(dllexport)  int OPC_ServerAddVariable(char* objectString, char* descriptionString, char* displayNameString, int type)
{
    std::string objectName(objectString);
    std::string browseObjectName(objectString);

    //если это атрибут объекта
    if (objectName!="")
    {
        //если этого объекта еще нет - создаем и запоминаем
        if (ObjectNodes.count(objectName) == 0)
        {
            UA_NodeId ObjectNodeId; // get the nodeid assigned by the server
            UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
            oAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)objectName.c_str()); //(char*)"Pump (Manual)"
            UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, (char*)browseObjectName.c_str()), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &ObjectNodeId);
            //UA_NODEID_NUMERIC(1, 0)
            ObjectNodeVariables* newObjectNode = new ObjectNodeVariables;
            newObjectNode->nodeId = ObjectNodeId;
            ObjectNodes[objectName] = newObjectNode;
        }
        //теперь создаем атрибут в объекте
        UA_NodeId ObjectNodeId = ObjectNodes[objectName]->nodeId;
        std::string VariableName(descriptionString);
        UA_NodeId VariableNodeId;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        if (type == 0)
        {
            UA_Double myDouble = 0;
            UA_Variant_setScalar(&attr.value, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
            attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
        }
        else
        {
            UA_String stringValue = UA_STRING((char*)"");
            UA_Variant_setScalar(&attr.value, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
            attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
        }
        attr.description = UA_LOCALIZEDTEXT((char*)"en - US", (char*)VariableName.c_str());
        attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)VariableName.c_str()); //(char*)"MotorRPM"
        
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Server_addVariableNode(server, UA_NODEID_NULL, ObjectNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, (char*)VariableName.c_str()), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, &VariableNodeId);
        ObjectNodes[objectName]->VariableNode_NameID[VariableName] = VariableNodeId;
    }
    //если это НЕ атрибут объекта
    else
    {
        //просто создаем переменную в корне        
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        if (type == 0)
        {
            UA_Double myDouble = 0;
            UA_Variant_setScalar(&attr.value, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
        }
        else
        {
            UA_String stringValue = UA_STRING((char*)"");
            UA_Variant_setScalar(&attr.value, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
        }
        attr.description = UA_LOCALIZEDTEXT((char*)"en - US", descriptionString);   //(char*)"the.answer"
        attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", displayNameString);     //(char*)"the answer"
        attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        //Add the variable node to the information model
        UA_NodeId myIntegerNodeId = UA_NODEID_STRING(0, descriptionString);
        UA_QualifiedName myDoubleName = UA_QUALIFIEDNAME(0, displayNameString);
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId, myDoubleName, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
    }
    return 0;
}

//4. int OPC_ServerWriteValueDouble (description, value) //(char*)"the.answer", double
extern "C" __declspec(dllexport)  int OPC_ServerWriteValueDouble(char* objectString, char* descriptionString, double value)
{
    UA_NodeId myDoubleNodeId;
    std::string objectName(objectString);
    std::string attributeName(descriptionString);

    //если это атрибут объекта
    if (objectName != "")
    {
        myDoubleNodeId = ObjectNodes[objectName]->VariableNode_NameID[attributeName];
    }
    else
    {
        myDoubleNodeId = UA_NODEID_STRING(1, descriptionString);
    }

    

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
extern "C" __declspec(dllexport) double OPC_ServerReadValueDouble(char* objectString, char* descriptionString)
{
    UA_NodeId myDoubleNodeId;
    std::string objectName(objectString);
    std::string attributeName(descriptionString);

    //если это атрибут объекта
    if (objectName != "")
    {
        myDoubleNodeId = ObjectNodes[objectName]->VariableNode_NameID[attributeName];
    }
    else
    {
        myDoubleNodeId = UA_NODEID_STRING(0, descriptionString);
    }

    UA_Variant out;
    UA_Variant_init(&out);
    UA_Server_readValue(server, myDoubleNodeId, &out);
    double value = *(UA_Double*)out.data;
    /* Clean up */
    UA_Variant_clear(&out);
    return value;
}



//6. int OPC_ServerWriteValueString (objectname, description, value) 
extern "C" __declspec(dllexport)  int OPC_ServerWriteValueString(char* objectString, char* descriptionString, char* value)
{
    UA_NodeId myDoubleNodeId;
    std::string objectName(objectString);
    std::string attributeName(descriptionString);
    std::string value2(value);

    //если это атрибут объекта
    if (objectName != "")
    {
        myDoubleNodeId = ObjectNodes[objectName]->VariableNode_NameID[attributeName];
    }
    else
    {
        myDoubleNodeId = UA_NODEID_STRING(1, descriptionString);
    }

    UA_String stringValue = UA_STRING((char*)value);
    UA_Variant myVar;
    UA_Variant_init(&myVar);
    UA_Variant_setScalar(&myVar, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, myDoubleNodeId, myVar);

    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = myDoubleNodeId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.status = UA_STATUSCODE_BADNOTCONNECTED;
    wv.value.hasStatus = true;
    UA_Server_write(server, &wv);

    wv.value.hasStatus = false;
    wv.value.value = myVar;
    wv.value.hasValue = true;
    UA_Server_write(server, &wv);

    return 0;
}


//7. double OPC_ServerReadValueString (objectname, description) 
extern "C" __declspec(dllexport) int OPC_ServerReadValueString(char* returnString, int returnStringLength, char* objectString, char* descriptionString)
{
    UA_NodeId myDoubleNodeId;
    std::string objectName(objectString);
    std::string attributeName(descriptionString);

    //если это атрибут объекта
    if (objectName != "")
    {
        myDoubleNodeId = ObjectNodes[objectName]->VariableNode_NameID[attributeName];
    }
    else
    {
        myDoubleNodeId = UA_NODEID_STRING(0, descriptionString);
    }

    UA_Variant out;
    UA_Variant_init(&out);
    UA_Server_readValue(server, myDoubleNodeId, &out);
    //double value = *(UA_Double*)out.data;

    UA_String uaString = *(UA_String*)out.data;

    char* convert = (char*)UA_malloc(sizeof(char) * uaString.length + 1);
    memcpy(convert, uaString.data, uaString.length);
    convert[uaString.length] = '\0';

    std::stringstream cls2;
    cls2 << convert;
    std::string svalue = cls2.str();

    //пишем в специальную переменну на сторону c#
    strcpy_s(returnString, returnStringLength, svalue.c_str());
    
    UA_Variant_clear(&out);
    
    return 0;
}



//8. int OPC_ServerShutdown
extern "C" __declspec(dllexport) int OPC_ServerShutdown()
{
    SendLog(L"debug DLL:OPC_ServerShutdown... ", 0);
    UA_Server_delete(server);
    UA_StatusCode retval = UA_Server_run_shutdown(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;

    //UA_Server_run_shutdown
    //UA_Server_delete(server);
    //This is to shut down the network before you delete it.  
    //UA_Job* jobs;
    //nl.getJobs(&nl, &jobs, 100);
    //nl.stop(&nl, &jobs);
    ////now you can delete the network layer.
    //nl.deleteMembers(&nl);
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
extern "C" __declspec(dllexport) int OPC_ClientWriteValueDouble(char* object2, char* description, double _value)
{
    UA_NodeId myDoubleNodeId;
    std::string objectName(object2);
    std::string attributeName(description);

    //если это атрибут объекта
    if (objectName != "")
    {
        myDoubleNodeId = ClientObjectNodes[objectName]->VariableNode_NameID[attributeName];
    }
    else
    {
        myDoubleNodeId = UA_NODEID_STRING(0, description);
    }

    //UA_NodeId myDoubleNodeId = UA_NODEID_STRING(0, description);

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
extern "C" __declspec(dllexport) double OPC_ClientReadValueDouble(char* object2, char* description)
{
    UA_NodeId myDoubleNodeId;
    std::string objectName(object2);
    std::string attributeName(description);

    //если это атрибут объекта
    if (objectName != "")
    {
        myDoubleNodeId = ClientObjectNodes[objectName]->VariableNode_NameID[attributeName];
    }
    else
    {
        myDoubleNodeId = UA_NODEID_STRING(0, description);
    }


    // Read the value attribute of the node. UA_Client_readValueAttribute is a
    // wrapper for the raw read service available as UA_Client_Service_read. 
    UA_Variant value; // Variants can hold scalar values and arrays of any type 
    UA_Variant_init(&value);

    // NodeId of the variable holding the current time 
    //UA_NodeId myDoubleNodeId = UA_NODEID_STRING(0, description); //(char*)"the.answer"
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





//-----------------------------------Регистрация метода из сервера и обработчик вызова метода--------------------------------
//tutorial_server_method.c
static UA_StatusCode helloWorldMethodCallback(UA_Server* server,
    const UA_NodeId* sessionId, void* sessionHandle,
    const UA_NodeId* methodId, void* methodContext,
    const UA_NodeId* objectId, void* objectContext,
    size_t inputSize, const UA_Variant* input,
    size_t outputSize, UA_Variant* output)
{
    UA_String* inputStr = (UA_String*)input->data;
    UA_String tmp = UA_STRING_ALLOC("Hello ");
    if (inputStr->length > 0) {
        tmp.data = (UA_Byte*)UA_realloc(tmp.data, tmp.length + inputStr->length);
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&tmp);
    //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Hello World was called");
    SendLog(L"debug DLL:Hello World was called", 0);


    char* convert = (char*)UA_malloc(sizeof(char) * inputStr->length + 1);
    memcpy(convert, inputStr->data, inputStr->length);
    convert[inputStr->length] = '\0';

    std::wstringstream cls;
    cls << convert;
    std::wstring name = cls.str();

    SendMethodCall(name, methodId->identifier.numeric);


    return UA_STATUSCODE_GOOD;
}

static void addHelloWorldMethod(UA_Server* server, unsigned int nodeID, char* name, char* displayName, char* description)
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
    helloAttr.description = UA_LOCALIZEDTEXT((char*)"en-US", description);  //4 (char*)"Say `Hello World`"
    helloAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", displayName);  //3 (char*)"Hello World"
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(0, nodeID),           //1 unsigned int //62541
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, name),                      //2      (char*)"hello world"
        helloAttr, &helloWorldMethodCallback,
        1, &inputArgument, 1, &outputArgument, NULL, NULL);
}

extern "C" __declspec(dllexport) int OPC_ServerCreateMethod(unsigned int nodeID, char* name, char* displayName, char* description)
{
    addHelloWorldMethod(server, nodeID, name, displayName, description);
    return 0;
}
//-----------------------------------Регистрация метода и обработчик вызова метода--------------------------------


//-----------------------------------Вызов метода  из сервера-----------------------------------------------------
extern "C" __declspec(dllexport) int OPC_ServerCallMethod(unsigned int NodeId, char* value)
{
    
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"A String");
    inputArgument.name = UA_STRING((char*)"MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;


    UA_Variant* inputArguments = (UA_Variant*)UA_calloc(1, (sizeof(UA_Variant)));
    UA_String _value = UA_STRING(value);
    UA_Variant_setScalar(&inputArguments[0], &_value, &UA_TYPES[UA_TYPES_STRING]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, NodeId);
    UA_CallMethodResult response = UA_Server_call(server, &callMethodRequest);
       
    UA_CallMethodResult_clear(&response);
    return 0;
}
//-----------------------------------Вызов метода  из сервера-----------------------------------------------------

//-----------------------------------Вызов метода  из клиента-----------------------------------------------------
extern "C" __declspec(dllexport) int OPC_ClientCallMethod(unsigned int NodeId, char* value)
{
#ifdef UA_ENABLE_METHODCALLS
    /* Call a remote method */
    UA_Variant input;
    //UA_String argString = UA_STRING((char*)"Hello Server");
    UA_String argString = UA_STRING(value);
    UA_Variant_init(&input);
    UA_Variant_setScalarCopy(&input, &argString, &UA_TYPES[UA_TYPES_STRING]);
    size_t outputSize;
    UA_Variant* output;
    //UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),UA_NODEID_NUMERIC(1, 62541), 1, &input, &outputSize, &output);  //62541 unsigned int 

    UA_StatusCode retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, NodeId), 1, &input, &outputSize, &output);
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
//-----------------------------------Вызов метода  из клиента-----------------------------------------------------



//подписка на мониторинг переменной и события
//tutorial_client_events
#ifdef UA_ENABLE_SUBSCRIPTIONS
static void handler_TheAnswerChanged(UA_Client* client, UA_UInt32 subId, void* subContext, UA_UInt32 monId, void* monContext, UA_DataValue* value) 
{
    SendLog(L"Monitoring .. handler_TheAnswerChanged", 0);

    if (value->hasValue == true)
    {
        if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DOUBLE]))
        {
            UA_Double severity = *(UA_Double*)value->value.data;
            SendValueChange(monId, severity);
            SendLog(L"Notification1 double", 0);
        }
        else if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]))
        {
            UA_LocalizedText* lt = (UA_LocalizedText*)value->value.data;
            SendLog(L"Notification1 text", 0);
        }
    }

}
#endif

extern "C" __declspec(dllexport) unsigned int OPC_ClientSubscription(char* object, char* varname, double interval)
{
    // The first publish request should return the initial value of the variable
    //UA_Client_run_iterate(client, 1000);

    SendLog(L"OPC_ClientSubscription", 0);

    // Create a subscription
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedPublishingInterval = interval;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);

    UA_UInt32 subId = response.subscriptionId;
    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
    {
        //printf("Create subscription succeeded, id %u\n", subId);
        SendLog(L"Create subscription succeeded", 0);
    }

    UA_NodeId myDoubleNodeId;
    std::string objectName(object);
    std::string attributeName(varname);

    //если это атрибут объекта
    if (objectName != "")
    {
        myDoubleNodeId = ClientObjectNodes[objectName]->VariableNode_NameID[attributeName];
    }
    else
    {
        myDoubleNodeId = UA_NODEID_STRING(0, varname);
    }

    //UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(0, varname));
    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(myDoubleNodeId);

    UA_MonitoredItemCreateResult monResponse = UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH,monRequest, NULL, handler_TheAnswerChanged, NULL);
    
    if (monResponse.statusCode == UA_STATUSCODE_GOOD)
    {
        SendLog(L"Monitoring .. ok", 0);
        //printf("Monitoring 'the.answer', id %u\n", monResponse.monitoredItemId);
    }
    else
    {
        SendLog(L"Monitoring .. false", 0);
    }

    // The first publish request should return the initial value of the variable
    UA_Client_run_iterate(client, 1000);

    UA_UInt32 monId = monResponse.monitoredItemId;
    return monId;

    //UA_MonitoredItemCreateResult_clear(&result);
    //UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    //UA_Array_delete(filter.selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
}

//----------------------------------------подписка на мониторинг переменной и события------------------------------------------------



//--------------------------------------------Чтение структуры------------------------------------------------------------------

void Client_Service_browse_recursive(UA_NodeId browse_node, std::string ParentName)
{
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = browse_node; // UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); // browse objects folder
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; // return everything
    //bReq.nodesToBrowse[0].includeSubtypes = UA_TRUE;
    //bReq.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT); //ТОЛЬКО МЕТОДЫ
    //bReq.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0, ); //ТОЛЬКО каталоги ..UA_NS0ID_ORGANIZES UA_NS0ID_FOLDERTYPE UA_NS0ID_HASCHILD UA_NS0ID_OBJECTNODE UA_NS0ID_HIERARCHICALREFERENCES
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    //printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
    SendLog(L"NAMESPACE, NODEID, BROWSE NAME, DISPLAY NAME", 0);
    for (size_t i = 0; i < bResp.resultsSize; ++i)
    {
        for (size_t j = 0; j < bResp.results[i].referencesSize; ++j)
        {
            UA_ReferenceDescription* ref = &(bResp.results[i].references[j]);

            //тип
            if (ref->nodeClass == UA_NODECLASS_OBJECT)
            {
                SendLog(L"Group!!!!", 0);
            }
            if (ref->nodeClass == UA_NODECLASS_METHOD)
            {
                SendLog(L"Method!!!!", 0);
            }
                
            //тут за идентификатор, т.е. чем определяется переменная - именем или строкой, может быть и так и так
            if (ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC)
            {
                //printf("%-9u %-16u %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
                //    ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
                //    ref->browseName.name.data, (int)ref->displayName.text.length,
                //    ref->displayName.text.data);
                UA_UInt16 a = ref->nodeId.nodeId.namespaceIndex;
                UA_UInt32 b = ref->nodeId.nodeId.identifier.numeric;
                int c = (int)ref->browseName.name.length;
                UA_Byte* d = ref->browseName.name.data;
                int e = (int)ref->displayName.text.length;
                UA_Byte* f = ref->displayName.text.data;

                {
                    UA_UInt32 numeric_id = ref->nodeId.nodeId.identifier.numeric;

                    char* convert = (char*)UA_malloc(sizeof(char) * ref->displayName.text.length + 1);
                    memcpy(convert, ref->displayName.text.data, ref->displayName.text.length);
                    convert[ref->displayName.text.length] = '\0';

                    std::wstring sub = L"";
                    if (ParentName != "") sub = L"---->";

                    std::wstringstream cls;
                    cls << sub << convert << "=" << numeric_id;
                    std::wstring displayName = cls.str();

                    //ИСКЛЮЧИТЬ! BaseObjectType=58
                    std::stringstream cls2;
                    cls2 << convert;
                    std::string flushName = cls2.str();

                    if ((flushName == "BaseObjectType") || (numeric_id == 58) || (flushName == "Server")) continue;

                    SendLog(displayName, 0);
                    if (ParentName != "")
                    {
                        //если этого объекта еще нет - создаем и запоминаем
                        if (ClientObjectNodes.count(ParentName) == 0)
                        {
                            ObjectNodeVariables* newObjectNode = new ObjectNodeVariables;
                            newObjectNode->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
                            ClientObjectNodes[ParentName] = newObjectNode;
                        }
                        //теперь создаем атрибут в объекте
                        ClientObjectNodes[ParentName]->VariableNode_NameID[flushName] = UA_NODEID_NUMERIC(0,numeric_id);
                    }


                    if (ref->nodeClass == UA_NODECLASS_OBJECT)
                    {
                        std::string parentname(convert);
                        Client_Service_browse_recursive(ref->nodeId.nodeId, parentname);
                    }
                }
            }
            else if (ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING)
            {
                //printf("%-9u %-16.*s %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
                //    (int)ref->nodeId.nodeId.identifier.string.length,
                //    ref->nodeId.nodeId.identifier.string.data,
                //    (int)ref->browseName.name.length, ref->browseName.name.data,
                //    (int)ref->displayName.text.length, ref->displayName.text.data);

                char* convert = (char*)UA_malloc(sizeof(char) * ref->displayName.text.length + 1);
                memcpy(convert, ref->displayName.text.data, ref->displayName.text.length);
                convert[ref->displayName.text.length] = '\0';

                char* convert1 = (char*)UA_malloc(sizeof(char) * (int)ref->nodeId.nodeId.identifier.string.length + 1);
                memcpy(convert1, ref->nodeId.nodeId.identifier.string.data, (int)ref->nodeId.nodeId.identifier.string.length);
                convert1[(int)ref->nodeId.nodeId.identifier.string.length] = '\0';

                std::wstring sub = L"";
                if (ParentName != "") sub = L"---->";

                std::wstringstream cls;
                cls << sub  << convert << "=" << convert1;
                std::wstring displayName = cls.str();

                std::stringstream cls2;
                cls2 << convert;
                std::string flushName = cls2.str();

                std::stringstream cls3;
                cls3 << convert1;
                std::string flushName2 = cls3.str();

                //ИСКЛЮЧИТЬ! BaseObjectType=58
                if ((flushName == "BaseObjectType")|| (flushName2 == "58") || (flushName == "Server")) continue;
                SendLog(displayName, 0);

                if (ParentName != "")
                {
                    //если этого объекта еще нет - создаем и запоминаем
                    if (ClientObjectNodes.count(ParentName) == 0)
                    {
                        ObjectNodeVariables* newObjectNode = new ObjectNodeVariables;
                        newObjectNode->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
                        ClientObjectNodes[ParentName] = newObjectNode;
                    }
                    //теперь создаем атрибут в объекте
                    ClientObjectNodes[ParentName]->VariableNode_NameID[flushName] = UA_NODEID_STRING(0, (char*)flushName.c_str());
                }

                if (ref->nodeClass == UA_NODECLASS_OBJECT)
                {
                    std::string parentname(convert);
                    Client_Service_browse_recursive(ref->nodeId.nodeId, parentname);
                }
            }
        }
    }
    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
}

//читает всю структуру иерархии дерева с сервера для возможности писать не только в корневые объекты, возвращает число прочитанных узлов
extern "C" __declspec(dllexport)  int OPC_UA_Client_Service_browse()
{
    //чистим дерево связей
    ClientObjectNodes.clear();
    SendLog(L"Browsing nodes in objects folder", 0);

    Client_Service_browse_recursive(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), ""); // browse objects folder);

    //(L"ClientObjectNodes.count = " + std::wstring(ClientObjectNodes.size()), 0);
    

    return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------































//--------------OLD and TEST------------------------------------------------------------


 /*
 //tutorial_server_object.c
//UA_NodeId pumpRpmId;
static void manuallyDefinePump(UA_Server* server)
{
    std::string pumpName = "Pump (Manual)";

    {
        UA_NodeId pumpId; // get the nodeid assigned by the server
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        oAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)pumpName.c_str()); //(char*)"Pump (Manual)"
        UA_Server_addObjectNode(server, UA_NODEID_NULL,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, (char*)"Pump (Manual)"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
            oAttr, NULL, &pumpId);

        ObjectNodeVariables* newObjectNode = new ObjectNodeVariables;
        newObjectNode->nodeId = pumpId;
        ObjectNodes[pumpName] = newObjectNode;
    }

    {
        std::string VariableName = "ManufacturerName";
        UA_NodeId VariableNodeId;
        UA_NodeId ObjectNodeId = ObjectNodes[pumpName]->nodeId;


        UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
        UA_String manufacturerName = UA_STRING((char*)"Pump King Ltd.");
        UA_Variant_setScalar(&mnAttr.value, &manufacturerName, &UA_TYPES[UA_TYPES_STRING]);
        mnAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)VariableName.c_str()); //"ManufacturerName"
        UA_Server_addVariableNode(server, UA_NODEID_NULL, ObjectNodeId, //pumpId
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char*)VariableName.c_str()),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), mnAttr, NULL, &VariableNodeId); //, NULL)
        ObjectNodes[pumpName]->VariableNode_NameID[VariableName] = VariableNodeId;
    }

    {
        std::string VariableName = "ModelName";
        UA_NodeId VariableNodeId;
        UA_NodeId ObjectNodeId = ObjectNodes[pumpName]->nodeId;

        UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
        UA_String modelName = UA_STRING((char*)"Mega Pump 3000");
        UA_Variant_setScalar(&modelAttr.value, &modelName, &UA_TYPES[UA_TYPES_STRING]);
        modelAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)VariableName.c_str());
        UA_Server_addVariableNode(server, UA_NODEID_NULL, ObjectNodeId, //pumpId
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char*)VariableName.c_str()),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), modelAttr, NULL, &VariableNodeId); //, NULL)
        ObjectNodes[pumpName]->VariableNode_NameID[VariableName] = VariableNodeId;
    }

    {
        std::string VariableName = "Status";
        UA_NodeId VariableNodeId;
        UA_NodeId ObjectNodeId = ObjectNodes[pumpName]->nodeId;

        UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
        UA_Boolean status = true;
        UA_Variant_setScalar(&statusAttr.value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
        statusAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)VariableName.c_str()); //(char*)"Status"
        UA_Server_addVariableNode(server, UA_NODEID_NULL, ObjectNodeId, //pumpId
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char*)VariableName.c_str()),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr, NULL, &VariableNodeId);
        ObjectNodes[pumpName]->VariableNode_NameID[VariableName] = VariableNodeId;
    }

    {
        std::string VariableName = "MotorRPM";
        UA_NodeId VariableNodeId;
        UA_NodeId ObjectNodeId = ObjectNodes[pumpName]->nodeId;

        UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
        UA_Double rpm = 50.1;
        UA_Variant_setScalar(&rpmAttr.value, &rpm, &UA_TYPES[UA_TYPES_DOUBLE]);
        rpmAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)VariableName.c_str()); //(char*)"MotorRPM"
        rpmAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Server_addVariableNode(server, UA_NODEID_NULL, ObjectNodeId, //pumpId
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char*)VariableName.c_str()),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), rpmAttr, NULL, &VariableNodeId); //&pumpRpmId
        ObjectNodes[pumpName]->VariableNode_NameID[VariableName] = VariableNodeId;
    }
}
extern "C" __declspec(dllexport)  int OPC_TestObjectPump()
{
    manuallyDefinePump(server);
    

    //иначе client из примеров - чтение списка всего на сервере и оттуда определяем ID - browseAll
    //или получаем через вызов метода 


    std::string objectName = "Pump (Manual)";
    std::string VariableName = "MotorRPM";
    UA_NodeId pumpRpmId = ObjectNodes[objectName]->VariableNode_NameID[VariableName];

    //запись из сервера
    if (false)
    {
        UA_NodeId myDoubleNodeId = pumpRpmId;
        // Write a different double value 
UA_Double myDouble = 456.789;
UA_Variant myVar;
UA_Variant_init(&myVar);
UA_Variant_setScalar(&myVar, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
UA_Server_writeValue(server, myDoubleNodeId, myVar);
// Set the status code of the value to an error code. The function
 // UA_Server_write provides access to the raw service. The above
 // UA_Server_writeValue is syntactic sugar for writing a specific node
 // attribute with the write service. 
UA_WriteValue wv;
UA_WriteValue_init(&wv);
wv.nodeId = myDoubleNodeId;
wv.attributeId = UA_ATTRIBUTEID_VALUE;
wv.value.status = UA_STATUSCODE_BADNOTCONNECTED;
wv.value.hasStatus = true;
UA_Server_write(server, &wv);
// Reset the variable to a good statuscode with a value 
wv.value.hasStatus = false;
wv.value.value = myVar;
wv.value.hasValue = true;
UA_Server_write(server, &wv);
    }

    //запись из клиента
    if (true)
    {
        UA_NodeId myDoubleNodeId = pumpRpmId;
        UA_Double myDouble = 321.777;
        UA_Variant* myVariant = UA_Variant_new();
        UA_Variant_setScalarCopy(myVariant, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
        UA_Client_writeValueAttribute(client, myDoubleNodeId, myVariant);
        UA_Variant_delete(myVariant);
    }

    //browseAll();


    return 0;
}
 
 */
 

    //UA_Server_readObjectProperty
    //UA_Server_writeObjectProperty
/*
    //defineObjectTypes(server);
    //addPumpObjectInstance(server, (char *)"pump2");
    //addPumpObjectInstance(server, (char*)"pump3");
    //addPumpTypeConstructor(server);
    //addPumpObjectInstance(server, (char*)"pump4");
    //addPumpObjectInstance(server, (char*)"pump5");

// predefined identifier for later use
UA_NodeId pumpTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

static void defineObjectTypes(UA_Server *server)
{
    // Define the object type for "Device"
    UA_NodeId deviceTypeId; // get the nodeid assigned by the server
    UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"DeviceType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, (char*)"DeviceType"), dtAttr,
                                NULL, &deviceTypeId);

    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    mnAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"ManufacturerName");
    UA_NodeId manufacturerNameId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, (char*)"ManufacturerName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), mnAttr, NULL, &manufacturerNameId);
    // Make the manufacturer name mandatory //
    UA_Server_addReference(server, manufacturerNameId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);


    UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
    modelAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"ModelName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, (char*)"ModelName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), modelAttr, NULL, NULL);

    // Define the object type for "Pump" //
    UA_ObjectTypeAttributes ptAttr = UA_ObjectTypeAttributes_default;
    ptAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"PumpType");
    UA_Server_addObjectTypeNode(server, pumpTypeId,
                                deviceTypeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, (char*)"PumpType"), ptAttr,
                                NULL, NULL);

    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    statusAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"Status");
    statusAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId statusId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, (char*)"Status"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr, NULL, &statusId);
    // Make the status variable mandatory //
    UA_Server_addReference(server, statusId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

    UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
    rpmAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"MotorRPM");
    rpmAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, (char*)"MotorRPMs"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), rpmAttr, NULL, NULL);
}



static void addPumpObjectInstance(UA_Server *server, char *name)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", name);
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, name),
                            pumpTypeId, // this refers to the object type identifier
                            oAttr, NULL, NULL);
}


static UA_StatusCode pumpTypeConstructor(UA_Server* server, const UA_NodeId* sessionId, void* sessionContext, const UA_NodeId* typeId, void* typeContext, const UA_NodeId* nodeId, void** nodeContext)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "New pump created");

    // Find the NodeId of the status child variable
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = UA_QUALIFIEDNAME(1, (char*)"Status");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = *nodeId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if (bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1)  return bpr.statusCode;

    // Set the status value
    UA_Boolean status = true;
    UA_Variant value;
    UA_Variant_setScalar(&value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_clear(&bpr);

    // At this point we could replace the node context ..

    return UA_STATUSCODE_GOOD;
}

static void addPumpTypeConstructor(UA_Server* server)
{
    UA_NodeTypeLifecycle lifecycle;
    lifecycle.constructor = pumpTypeConstructor;
    lifecycle.destructor = NULL;
    UA_Server_setNodeTypeLifecycle(server, pumpTypeId, lifecycle);
}
*/


//Вывод вструктуры всего на сервере
/*
void browseAll()
{
    SendLog(L"Browsing nodes in objects folder", 0);

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); // browse objects folder
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; // return everything
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    //printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
    SendLog(L"NAMESPACE, NODEID, BROWSE NAME, DISPLAY NAME", 0);
    for (size_t i = 0; i < bResp.resultsSize; ++i)
    {
        for (size_t j = 0; j < bResp.results[i].referencesSize; ++j)
        {
            UA_ReferenceDescription* ref = &(bResp.results[i].references[j]);
            if (ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC)
            {
                //printf("%-9u %-16u %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
                //    ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
                //    ref->browseName.name.data, (int)ref->displayName.text.length,
                //    ref->displayName.text.data);
                UA_UInt16 a = ref->nodeId.nodeId.namespaceIndex;
                UA_UInt32 b = ref->nodeId.nodeId.identifier.numeric;
                int c = (int)ref->browseName.name.length;
                UA_Byte* d = ref->browseName.name.data;
                int e = (int)ref->displayName.text.length;
                UA_Byte* f = ref->displayName.text.data;

                char* convert = (char*)UA_malloc(sizeof(char) * ref->displayName.text.length + 1);
                memcpy(convert, ref->displayName.text.data, ref->displayName.text.length);
                convert[ref->displayName.text.length] = '\0';
                std::wstringstream cls;
                cls << convert;
                std::wstring displayName = cls.str();

                SendLog(displayName, 0);
            }
            else if (ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING)
            {
                printf("%-9u %-16.*s %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
                    (int)ref->nodeId.nodeId.identifier.string.length,
                    ref->nodeId.nodeId.identifier.string.data,
                    (int)ref->browseName.name.length, ref->browseName.name.data,
                    (int)ref->displayName.text.length, ref->displayName.text.data);
            }
        }
    }
    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
}
*/


/*
//https://github.com/open62541/open62541/issues/3799
UA_StatusCode find_datavariable_nodeid(UA_Server* server, const UA_NodeId object_id, const UA_QualifiedName name, UA_NodeId* node_id)
{
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = name;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = object_id;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    UA_StatusCode rc = bpr.statusCode;
    if (rc == UA_STATUSCODE_GOOD && bpr.targetsSize < 1)
        rc = UA_STATUSCODE_BADNOMATCH;
    if (rc == UA_STATUSCODE_GOOD)
        *node_id = bpr.targets[0].targetId.nodeId;

    UA_BrowsePathResult_deleteMembers(&bpr);

    return rc;
}
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
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(0, (char*)"AAA");
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

    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(0, (char*)"double.matrix");
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
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(0, (char*)"AAA");

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
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(0, (char*)"AAA");
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
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(0, description);//(char*)"AAA"
    retval = UA_Client_readValueAttribute(client2, myIntegerNodeId, &value);

    

    double p = -4;
    if (retval == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) //UA_TYPES_INT32
    {
        p = *(UA_Double*)value.data; //UA_Int32
    }




    {
        UA_NodeId myDoubleNodeId = UA_NODEID_STRING(0, description);

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

