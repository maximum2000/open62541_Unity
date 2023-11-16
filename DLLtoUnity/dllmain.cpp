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
#include <vector>

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

class SubscriptionElementClass
{
public:
    std::string objectname = "";
    std::string variablename = "";
    //double interval = 1000;
};

//подписка для клиента
std::vector <SubscriptionElementClass*> allClientSubscription;
//unsigned int monID, 
std::map< unsigned int, SubscriptionElementClass*> allRegisteredSubscription;

//подписка для клиента
//NodeID, ....
std::map< unsigned int, SubscriptionElementClass*> allServerRegisteredSubscription;


//функции общие
// RegisterDebugCallback - регистрация callback'ов
//1. SendLog - дебаг-вывод в unity
//2. SendMethodCall - вызывается при сработке метода
//3. SendValueChange - сработка подписки на изменение (для клиента)
//4. ServerSendValueChange - - сработка подписки на изменение (для сервера)

//функции сервера:
//1. int OPC_ServerCreate () - создание сервера OPC
//2. int OPC_ServerUpdate () - обновление сервера
//3. int OPC_ServerAddVariable (objectname, varname, type)  - добавить переменную (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ТИП (0-double/1-int)
//4. int OPC_ServerWriteValueDouble (objectname, varname, value)  - изменить переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
//5. double OPC_ServerReadValueDouble (objectname, varname) - прочитать напрямую переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
//6. int OPC_ServerWriteValueString - изменить переменную типа STRING (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
//7. int OPC_ServerReadValueString - прочитать напрямую переменную типа STRING (ВОЗРАЩАЕМОЕ ЗНАЧЕНИЕ, ВОЗВРАЩАЕМАЯ ДЛИННА,  ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
//8. int OPC_ServerShutdown - выключение сервера
//9. int OPC_ServerCreateMethod (nodeID, name, displayName, description) - создание метода (УНИКАЛЬЫНЙ НОМЕР, ИМЯ МЕТОДА, НАЗВАНИЕ МЕТОДА, ОПИСАНИЕ)
//10. int OPC_ServerCallMethod - вызов метода из сервера (nodeID, value) - (УНИКАЛЬЫНЙ НОМЕР, ЗНАЧЕНИЕ)
//11. int OPC_ServerSubscription - Добавить подписку на изменение переменной для сервера(objectString, varNameString, interval) - (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ИНТЕРВАЛ 1000=1с)

//функции клиента:
//1. int OPC_ClientConnect (url) - ПОДКЛЮЧЕНИЕ К СЕРВЕРУ "opc.tcp://localhost:4840"
//2. int OPC_ClientWriteValueDouble (objectname, varname, value) - записать переменную типа DOUBLE (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
//3. double OPC_ClientReadValueDouble (objectname, varname) - ПРЯМОЕ чтение переменной типа DOUBLE (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
//4. int OPC_ClientUpdate () - обновление клиента
//5. int OPC_ClientDelete() - выключение клиента
//6. int OPC_ClientCallMethod(NodeID, value) - вызов метода по никальному ID (нгапример, NodeID=62541) и строковым параметров
//7. int OPC_UA_Client_Service_browse() - читает всю структуру иерархии дерева с сервера для возможности писать не только в корневые объекты, возвращает число прочитанных узлов
//8. int OPC_ClientSubscriptions - выполняет подписку на все перменные, ранее переданные через OPC_ClientSubscriptionAddVariable. Параметр - частота опроса, мсек, т.е. 1000=1с
//9. OPC_ClientSubscriptionAddVariable - добавляет в подписку (выполнять до OPC_ClientSubscriptions) одну переменную (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)



//сделать:
//tutorial_server_events - триггеры и события                                                                   -
//Про методы: например можно сделать метод - создать игрока передать туда имя и тип и в ответ получить ок или не ок, можно и в ответ имя получить
// UA_NODEID_NUMERIC / UA_NODEID_STRING 0 - NAMESPACE NS0


//интересно:
//UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "paula", "paula123");    -
//pubsub_realtime - реалтайм                                                                                    -
//tutorial_datatypes - строки / массивы / числа                                                                 -
//server_inheritance - примеры объектов                                                                         -
//tutorial_server_variabletype - массивы                                                                        -
//client_find_servers - поиск сервера                                                                           -
//client_subscription_loop - обработка закрытия сервера                                                         -
//client_connect_loop - автоподключение                                                                         -




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
    typedef void(*ValueChangeCallback)(const char* message1, int size1, const char* message2, int size2,  unsigned int monID, double value);
    static ValueChangeCallback callbackValueChangeFunction = nullptr;
    __declspec(dllexport) void RegisterValueChangeCallback(ValueChangeCallback callback);
}
void RegisterValueChangeCallback(ValueChangeCallback callback)
{
    callbackValueChangeFunction = callback;
}
//nothrow

void SendValueChange(const std::wstring& strObj, const std::wstring& strVar, const unsigned int& monID, const double& value)
{
    std::string s1(strObj.begin(), strObj.end());
    const char* tmsg1 = s1.c_str();
    std::string s2(strVar.begin(), strVar.end());
    const char* tmsg2 = s2.c_str();
    if (callbackValueChangeFunction != nullptr)
    {
        callbackValueChangeFunction(tmsg1, (int)strlen(tmsg1), tmsg2, (int)strlen(tmsg2), (unsigned int) monID, (double)value);
    }
}
//SendValueChange(1000, 3.141516);





// Call this function from a Untiy script
extern "C"
{
    typedef void(*ServerValueChangeCallback)(const char* message1, int size1, const char* message2, int size2, unsigned int monID, double value);
    static ServerValueChangeCallback ServerCallbackValueChangeFunction = nullptr;
    __declspec(dllexport) void RegisterServerValueChangeCallback(ServerValueChangeCallback callback);
}
void RegisterServerValueChangeCallback(ServerValueChangeCallback callback)
{
    ServerCallbackValueChangeFunction = callback;
}
//nothrow

void ServerSendValueChange(const std::wstring& strObj, const std::wstring& strVar, const unsigned int& monID, const double& value)
{
    std::string s1(strObj.begin(), strObj.end());
    const char* tmsg1 = s1.c_str();
    std::string s2(strVar.begin(), strVar.end());
    const char* tmsg2 = s2.c_str();
    if (ServerCallbackValueChangeFunction != nullptr)
    {
        ServerCallbackValueChangeFunction(tmsg1, (int)strlen(tmsg1), tmsg2, (int)strlen(tmsg2), (unsigned int)monID, (double)value);
    }
}
//ServerSendValueChange(1000, 3.141516);



 
 

//---------------------------------SERVER---------------------------------------

//1. int OPC_ServerCreate () - создание сервера OPC
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

    //
    UA_DurationRange publishingIntervalLimits;
    publishingIntervalLimits.min = 10.0;
    publishingIntervalLimits.max = 3600 * 1000.0;
    config->publishingIntervalLimits = publishingIntervalLimits;
    UA_DurationRange samplingIntervalLimits;
    samplingIntervalLimits.min = 10.0;
    samplingIntervalLimits.max = 24.0 * 3600.0 * 1000.0;
    config->samplingIntervalLimits = samplingIntervalLimits;
    //UA_Server* server = UA_Server_new(config);

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

//3. int OPC_ServerAddVariableDouble (objectname, varname, type)  - добавить переменную (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ТИП (0-double/1-int)
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

//4. int OPC_ServerWriteValueDouble (objectname, varname, value)  - изменить переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
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



//5. double OPC_ServerReadValueDouble (objectname, varname) - прочитать напрямую переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
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



//6. int OPC_ServerWriteValueString - изменить переменную типа STRING (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
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


//7. int OPC_ServerReadValueString - прочитать напрямую переменную типа STRING (ВОЗРАЩАЕМОЕ ЗНАЧЕНИЕ, ВОЗВРАЩАЕМАЯ ДЛИННА,  ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
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



//8. int OPC_ServerShutdown - выключение сервера
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

//9. int OPC_ServerCreateMethod (nodeID, name, displayName, description) - создание метода (УНИКАЛЬЫНЙ НОМЕР, ИМЯ МЕТОДА, НАЗВАНИЕ МЕТОДА, ОПИСАНИЕ)
extern "C" __declspec(dllexport) int OPC_ServerCreateMethod(unsigned int nodeID, char* name, char* displayName, char* description)
{
    addHelloWorldMethod(server, nodeID, name, displayName, description);
    return 0;
}
//-----------------------------------Регистрация метода и обработчик вызова метода--------------------------------


//-----------------------------------Вызов метода  из сервера-----------------------------------------------------
//10. int OPC_ServerCallMethod - вызов метода из сервера (nodeID, value) - (УНИКАЛЬЫНЙ НОМЕР, ЗНАЧЕНИЕ)
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

//Обработчик изменения переменной для сервера
static void serverDataChangeNotificationCallback(UA_Server* server, UA_UInt32 monitoredItemId, void* monitoredItemContext, const UA_NodeId* nodeId, void* nodeContext, UA_UInt32 attributeId, const UA_DataValue* value)
{
    //см. приходит nodeId !
    //SendLog(L"debug DLL:serverDataChangeNotificationCallback... ", 0);
    
    unsigned int t = nodeId->identifier.numeric;

    if (allServerRegisteredSubscription.count(t) > 0)
    {
        if (value->hasValue == true)
        {
            if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DOUBLE]))
            {
                UA_Double severity = *(UA_Double*)value->value.data;
                std::string oname = allServerRegisteredSubscription[t]->objectname;
                std::string vname = allServerRegisteredSubscription[t]->variablename;

                std::wstring woname(oname.begin(), oname.end());
                std::wstring wvname(vname.begin(), vname.end());
                ServerSendValueChange(woname, wvname, t, severity);
                //SendLog(L"Notification1 double", 0);
            }
            else if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]))
            {
                UA_LocalizedText* lt = (UA_LocalizedText*)value->value.data;
                SendLog(L"serverDataChangeNotificationCallback: Notification1 text", 0);
            }
        }
    }
    else
    {
        std::wstringstream ss1;
        ss1 << L"serverDataChangeNotificationCallback!";
        std::wstring str1 = ss1.str();
        SendLog(str1, 0);
    }
}

//11. Добавить подписку на изменение переменной для сервера //11. int OPC_ServerSubscription(objectString, varNameString, interval) - (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ИНТЕРВАЛ 1000=1с)
extern "C" __declspec(dllexport) int OPC_ServerSubscription(char* objectString, char* varNameString, double interval)
{
    SendLog(L"debug DLL:OPC_ServerSubscription... ", 0);

    UA_NodeId myDoubleNodeId;
    std::string objectName(objectString);
    std::string attributeName(varNameString);

    //если это атрибут объекта
    if (objectName != "")
    {
        myDoubleNodeId = ObjectNodes[objectName]->VariableNode_NameID[attributeName];
    }
    else
    {
        myDoubleNodeId = UA_NODEID_STRING(0, varNameString);
    }

    //UA_NodeId currentTimeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(myDoubleNodeId);
    monRequest.requestedParameters.samplingInterval = interval; // 100.0; /* 100 ms interval */
    UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE, monRequest, NULL, serverDataChangeNotificationCallback); //UA_TIMESTAMPSTORETURN_BOTH

    SubscriptionElementClass* temp = new SubscriptionElementClass();
    temp->objectname = objectName;
    temp->variablename = attributeName;

    allServerRegisteredSubscription[myDoubleNodeId.identifier.numeric] = temp;

    return 0;
}








//---------------------------------CLIENT---------------------------------------
UA_Client* client;

//1. int OPC_ClientConnect (url) - ПОДКЛЮЧЕНИЕ К СЕРВЕРУ "opc.tcp://localhost:4840"
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

//2. int OPC_ClientWriteValueDouble (objectname, varname, value) - записать переменную типа DOUBLE (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
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

//3. double OPC_ClientReadValueDouble (objectname, varname) - ПРЯМОЕ чтение переменной типа DOUBLE (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
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

//5. int OPC_ClientDelete() - выключение клиента
extern "C" __declspec(dllexport) int OPC_ClientDelete()
{
    UA_Client_disconnect(client);
    UA_Client_delete(client); // Disconnects the client internally 
    return 0;
}

//6. int OPC_ClientCallMethod(NodeID, value) - вызов метода по никальному ID (нгапример, NodeID=62541) и строковым параметров
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
                        ClientObjectNodes[ParentName]->VariableNode_NameID[flushName] = UA_NODEID_NUMERIC(0, numeric_id);
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
                cls << sub << convert << "=" << convert1;
                std::wstring displayName = cls.str();

                std::stringstream cls2;
                cls2 << convert;
                std::string flushName = cls2.str();

                std::stringstream cls3;
                cls3 << convert1;
                std::string flushName2 = cls3.str();

                //ИСКЛЮЧИТЬ! BaseObjectType=58
                if ((flushName == "BaseObjectType") || (flushName2 == "58") || (flushName == "Server")) continue;
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

//7. int OPC_UA_Client_Service_browse() - читает всю структуру иерархии дерева с сервера для возможности писать не только в корневые объекты, возвращает число прочитанных узлов
extern "C" __declspec(dllexport)  int OPC_UA_Client_Service_browse()
{
    //чистим дерево связей
    ClientObjectNodes.clear();
    SendLog(L"Browsing nodes in objects folder", 0);

    Client_Service_browse_recursive(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), ""); // browse objects folder);

    //(L"ClientObjectNodes.count = " + std::wstring(ClientObjectNodes.size()), 0);


    return 0;
}

//----------------------------------------подписка на мониторинг переменной и события для клиента------------------------------------------------
//Обработчик см. tutorial_client_events
#ifdef UA_ENABLE_SUBSCRIPTIONS
static void handler_TheAnswerChanged(UA_Client* client, UA_UInt32 subId, void* subContext, UA_UInt32 monId, void* monContext, UA_DataValue* value) 
{
    //SendLog(L"Monitoring .. handler_TheAnswerChanged", 0);

    if (allRegisteredSubscription.count(monId) > 0)
    {
        if (value->hasValue == true)
        {
            if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DOUBLE]))
            {
                UA_Double severity = *(UA_Double*)value->value.data;
                std::string oname = allRegisteredSubscription[monId]->objectname;
                std::string vname = allRegisteredSubscription[monId]->variablename;

                std::wstring woname(oname.begin(), oname.end());
                std::wstring wvname(vname.begin(), vname.end());
                SendValueChange(woname, wvname,  monId, severity);
                //SendLog(L"Notification1 double", 0);
            }
            else if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]))
            {
                UA_LocalizedText* lt = (UA_LocalizedText*)value->value.data;
                SendLog(L"Notification1 text", 0);
            }
        }
    }
    else
    {
        std::wstringstream ss1;
        ss1 << L"allRegisteredSubscription=" << monId << L" not found";
        std::wstring str1 = ss1.str();
        SendLog(str1, 0);
    }

}
#endif


//8. int OPC_ClientSubscriptions - выполняет подписку на все перменные, ранее переданные через OPC_ClientSubscriptionAddVariable. Параметр - частота опроса, мсек, т.е. 1000=1с
extern "C" __declspec(dllexport) int OPC_ClientSubscriptions(double interval)
{
    SendLog(L"OPC_ClientSubscriptions", 0);
    if (allClientSubscription.size() == 0) return 0;

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

    int count = (int)allClientSubscription.size();

    UA_MonitoredItemCreateRequest *items = new UA_MonitoredItemCreateRequest[count];
    UA_UInt32 *newMonitoredItemIds = new UA_UInt32[count];
    UA_Client_DataChangeNotificationCallback* callbacks = new UA_Client_DataChangeNotificationCallback[count];
    UA_Client_DeleteMonitoredItemCallback* deleteCallbacks = new UA_Client_DeleteMonitoredItemCallback[count];
    void** contexts = new void* [count];

    //UA_MonitoredItemCreateRequest items[2];
    //UA_UInt32 newMonitoredItemIds[2];
    //UA_Client_DataChangeNotificationCallback callbacks[2];
    //UA_Client_DeleteMonitoredItemCallback deleteCallbacks[2];
    //void* contexts[2];

    for (int i = 0; i < count; i++)
    {
        UA_NodeId myDoubleNodeId1;
        {
            std::string objectName(allClientSubscription[i]->objectname);
            std::string attributeName(allClientSubscription[i]->variablename);

            //если это атрибут объекта
            if (objectName != "")
            {
                myDoubleNodeId1 = ClientObjectNodes[objectName]->VariableNode_NameID[attributeName];
            }
            else
            {
                myDoubleNodeId1 = UA_NODEID_STRING(0, (char*)allClientSubscription[i]->variablename.c_str());
            }
        }

        //
        items[i] = UA_MonitoredItemCreateRequest_default(myDoubleNodeId1);
        callbacks[i] = handler_TheAnswerChanged;
        contexts[i] = NULL;
        deleteCallbacks[i] = NULL;
    }


    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = items;
    createRequest.itemsToCreateSize = count;
    UA_CreateMonitoredItemsResponse createResponse = UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts, callbacks, deleteCallbacks);

    if (createResponse.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
    {
        SendLog(L"OPC_ClientSubscriptions..ok", 0);
    }
    if (createResponse.results[0].statusCode == UA_STATUSCODE_GOOD)
    {
        //SendLog(L"ok2", 0);
    }

   
    
    // The first publish request should return the initial value of the variable
    //UA_Client_run_iterate(client, 1000);

    for (int i = 0; i < count; i++)
    {
        newMonitoredItemIds[i] = createResponse.results[i].monitoredItemId;
        std::wstringstream ss1;
        ss1 << L"OPC_ClientSubscription*" << allClientSubscription[i]->objectname.c_str() << "*" << allClientSubscription[i]->variablename.c_str() << "=" << newMonitoredItemIds[i];
        std::wstring str1 = ss1.str();
        SendLog(str1, 0);
        allRegisteredSubscription[newMonitoredItemIds[i]] = allClientSubscription[i];
    }

    // The first publish request should return the initial value of the variable
    UA_Client_run_iterate(client, 1000);
    

    UA_CreateMonitoredItemsResponse_deleteMembers(&createResponse);

    //allClientSubscription.clear();
   
    return 0;
}


//9. OPC_ClientSubscriptionAddVariable - добавляет в подписку (выполнять до OPC_ClientSubscriptions) одну переменную (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
extern "C" __declspec(dllexport) void OPC_ClientSubscriptionAddVariable(char* _objectname, char* _varname)
{
    std::string objectname(_objectname);
    std::string varname(_varname);

    SubscriptionElementClass* temp = new SubscriptionElementClass();
    temp->objectname = objectname;
    temp->variablename = varname;

    allClientSubscription.push_back(temp);
    return;
}

/*


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
*/
