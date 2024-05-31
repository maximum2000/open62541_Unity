/***************************************************************************
OPCUA_SERVER_DLL.cs  - Модель связи OPC UA с UNITY
-------------------
begin                : 13 ноября 2023
copyright            : (C) 2023 by Гаммер Максим Дмитриевич (maximum2000)
email                : maxim.gammer@yandex.ru
site				 : lcontent.ru 
org					 : ИП Гаммер Максим Дмитриевич
***************************************************************************/

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using AOT; // MonopInvokeCallback
using System.Runtime.InteropServices;
using System;
using System.Text;
using TMPro;
using UnityEngine.Events;

using System.Threading;
using System.Timers;


[System.Serializable]
public class MyMethodCallEvent : UnityEvent<int, string> { }

//вызываются при изменении переменной числовой
[System.Serializable]
public class MyDoubleChangeEvent : UnityEvent<string, string, double> { }

//вызываются при изменении переменной строковой
[System.Serializable]
public class MyStringChangeEvent : UnityEvent<string, string, string> { }

//
public class ChangeVariableClass
{
    public string objectName;
    public string VarName;
    public string ValueString;
    public double ValueDouble;
}




public class OPCUA_SERVER_DLL : MonoBehaviour
{


    //Обработчики call-back функций (функции ызываются из DLL)
    //1.RegisterDebugCallback - регистрация callback'ов
    //2. SendLog из DLL -> DebugLog в Unity = дебаг-вывод в unity
    //3. MethodCall из DLL -> MethodCall в Unity = для обработчика вызова метода'а
    //4. из DLL -> ValueChange в Unity = callback для обработчика изменения переменной по подписке (для клиента)
    //5. из DLL -> ValueChange в Unity = callback для обработчика изменения переменной по подписке (для сервера)

    //1.регистрация всех callbeck функций (привязка)
    public void RegisterCallback()
    {
        RegisterDebugCallback(DebugLog);
        RegisterMethodCallCallback(MethodCall);
        RegisterValueChangeCallback(ValueChange);
        RegisterServerValueChangeCallback(ServerValueChange);
    }

    //2. callback для Debug'а
    private delegate void DebugCallback(IntPtr message, int color, int size);
    [DllImport("DLL1", CallingConvention = CallingConvention.Cdecl)]
    private static extern void RegisterDebugCallback(DebugCallback callback);
    //вывод сообщения из debug'а DLL'ки
    private static void DebugLog(IntPtr message, int color, int size)
    {
        string debugString = Marshal.PtrToStringAnsi(message, size);
        Debug.Log("c# debug:" + debugString);
    }
    //end callback для Debug'а



    //3. callback для обработчика вызова метода'а
    private delegate void MethodCallCallback(IntPtr message, uint nodeid, int size);
    [DllImport("DLL1", CallingConvention = CallingConvention.Cdecl)]
    private static extern void RegisterMethodCallCallback(MethodCallCallback callback);
    //
    private static void MethodCall(IntPtr message, uint nodeid, int size)
    {
        string debugString = Marshal.PtrToStringAnsi(message, size);
        Debug.Log("c# MethodCall:" + debugString + ", id=" + nodeid);
        allEvent.Add(new KeyValuePair<uint, string>(nodeid, debugString));

    }
    //end callback для обработчика вызова метода'а


    //4. callback для обработчика изменения переменной по подписке (для клиента)
    private delegate void ValueChangeCallback(IntPtr message1, int size1, IntPtr message2, int size2, uint monid, double value, IntPtr strvalue, int strvaluesize);
    [DllImport("DLL1", CallingConvention = CallingConvention.Cdecl)]
    private static extern void RegisterValueChangeCallback(ValueChangeCallback callback);
    //
    private static void ValueChange(IntPtr message1, int size1, IntPtr message2, int size2, uint monid, double value, IntPtr strvalue, int strvaluesize)
    {
        string debugString1 = Marshal.PtrToStringAnsi(message1, size1);
        string debugString2 = Marshal.PtrToStringAnsi(message2, size2);
        string debugString3 = Marshal.PtrToStringAnsi(strvalue, strvaluesize);

        Debug.Log("c# ValueChange:" + debugString1 + "*" + debugString2 + "*" + "monId =" + monid + ", value=" + value + ", strvalue=" + debugString3);// ", name=" + subscriptions[monid]);

        KeyValuePair<string, string> request = new KeyValuePair<string, string>(debugString1, debugString2);
        Variables[request] = value;

        ChangeVariableClass temp = new ChangeVariableClass();
        temp.objectName = debugString1;
        temp.VarName = debugString2;
        temp.ValueDouble = value;
        temp.ValueString = debugString3;
        allVariableEvents.Add(temp);
    }
    //end callback для обработчика изменения переменной по подписке (для клиента)

    //6. callback для обработчика изменения переменной по подписке  (для сервера)
    private delegate void ServerValueChangeCallback(IntPtr message1, int size1, IntPtr message2, int size2, uint monid, double value);
    [DllImport("DLL1", CallingConvention = CallingConvention.Cdecl)]
    private static extern void RegisterServerValueChangeCallback(ServerValueChangeCallback callback);
    //
    private static void ServerValueChange(IntPtr message1, int size1, IntPtr message2, int size2, uint monid, double value)
    {
        string debugString1 = Marshal.PtrToStringAnsi(message1, size1);
        string debugString2 = Marshal.PtrToStringAnsi(message2, size2);

        Debug.Log("c# ServerValueChange:" + debugString1 + "*" + debugString2 + "*" + "NodeId =" + monid + ", value=" + value);// ", name=" + subscriptions[monid]);
    }
    //end callback для обработчика изменения переменной по подписке (для сервера)



    //функции сервера:
    //1. int OPC_ServerCreate () - создание сервера OPC
    //2. int OPC_ServerUpdate () - обновление сервера
    //3. int OPC_ServerAddVariableDouble (objectname, varname, type)  - добавить переменную (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ТИП (0-double/1-int), интервал обновления в мс, например 1000 - 1раз в сукунду
    //4. int OPC_ServerWriteValueDouble (objectname, varname, value)  - изменить переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
    //5. double OPC_ServerReadValueDouble (objectname, varname) - прочитать напрямую переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    //6. int OPC_ServerWriteValueString - изменить переменную типа STRING (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
    //7. int OPC_ServerReadValueString - прочитать напрямую переменную типа STRING (ВОЗРАЩАЕМОЕ ЗНАЧЕНИЕ, ВОЗВРАЩАЕМАЯ ДЛИННА,  ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    //8. int OPC_ServerShutdown - выключение сервера
    //9. int OPC_ServerCreateMethod (nodeID, name, displayName, description) - создание метода (УНИКАЛЬЫНЙ НОМЕР, ИМЯ МЕТОДА, НАЗВАНИЕ МЕТОДА, ОПИСАНИЕ)
    //10. int OPC_ServerCallMethod - вызов метода из сервера (nodeID, value) - (УНИКАЛЬЫНЙ НОМЕР, ЗНАЧЕНИЕ)
    //11. int OPC_ServerSubscription - Добавить подписку на изменение переменной для сервера(objectString, varNameString, interval) - (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ИНТЕРВАЛ 1000=1с)

    //1. int OPCserverCreate () - создание сервера OPC
    [DllImport("DLL1")]
    public static extern int OPC_ServerCreate();

    //2. int OPC_ServerUpdate () - обновление сервера
    [DllImport("DLL1")]
    public static extern int OPC_ServerUpdate();

    //3. int OPC_ServerAddVariable (objectname, varname, type)  - добавить переменную (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ТИП (0-double/1-int)
    [DllImport("DLL1")]
    public static extern int OPC_ServerAddVariable(StringBuilder objectString, StringBuilder descriptionString, StringBuilder displayNameString, int type, double samplingInterval);

    //4. int OPC_ServerWriteValueDouble (objectname, varname, value) - изменить переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
    [DllImport("DLL1")]
    public static extern int OPC_ServerWriteValueDouble(StringBuilder objectString, StringBuilder descriptionString, double value);

    //5. double OPC_ServerReadValueDouble (objectname, varname) - прочитать напрямую переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    [DllImport("DLL1")]
    public static extern double OPC_ServerReadValueDouble(StringBuilder objectString, StringBuilder descriptionString);

    //6. int OPC_ServerWriteValueString - изменить переменную типа STRING (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
    [DllImport("DLL1")]
    public static extern int OPC_ServerWriteValueString(StringBuilder objectString, StringBuilder descriptionString, StringBuilder value);

    //7. int OPC_ServerReadValueString - прочитать напрямую переменную типа STRING (ВОЗРАЩАЕМОЕ ЗНАЧЕНИЕ, ВОЗВРАЩАЕМАЯ ДЛИННА,  ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    [DllImport("DLL1")]
    public static extern int OPC_ServerReadValueString(StringBuilder returnString, int returnStringLength, StringBuilder objectString, StringBuilder descriptionString);

    //8. int OPC_ServerShutdown - выключение сервера
    [DllImport("DLL1")]
    public static extern int OPC_ServerShutdown();

    //9. int OPC_ServerCreateMethod (nodeID, name, displayName, description) - создание метода (УНИКАЛЬЫНЙ НОМЕР, ИМЯ МЕТОДА, НАЗВАНИЕ МЕТОДА, ОПИСАНИЕ)
    [DllImport("DLL1")]
    public static extern int OPC_ServerCreateMethod(uint nodeID, StringBuilder name, StringBuilder displayName, StringBuilder description);

    //10. int OPC_ServerCallMethod - вызов метода из сервера - (УНИКАЛЬЫНЙ НОМЕР, ЗНАЧЕНИЕ)
    [DllImport("DLL1")]
    public static extern int OPC_ServerCallMethod(uint nodeID, StringBuilder value);

    //11. int OPC_ServerSubscription(objectString, varNameString, interval) - (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ИНТЕРВАЛ 1000=1с)
    [DllImport("DLL1")]
    public static extern int OPC_ServerSubscription(StringBuilder objectString, StringBuilder varNameString, double interval);



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

    //1. int OPC_ClientConnect (url) - ПОДКЛЮЧЕНИЕ К СЕРВЕРУ "opc.tcp://localhost:4840"
    [DllImport("DLL1")]
    public static extern int OPC_ClientConnect(StringBuilder url);

    //2. int OPC_ClientWriteValueDouble (objectname, varname, value) - записать переменную типа DOUBLE (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
    [DllImport("DLL1")]
    public static extern int OPC_ClientWriteValueDouble(StringBuilder objectname, StringBuilder varname, double value);

    //3. double OPC_ClientReadValueDouble (objectname, varname) - ПРЯМОЕ чтение переменной типа DOUBLE (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    [DllImport("DLL1")]
    public static extern double OPC_ClientReadValueDouble(StringBuilder objectname, StringBuilder varname);

    //4. int OPC_ClientUpdate () - обновление клиента
    [DllImport("DLL1")]
    public static extern int OPC_ClientUpdate();

    //5. int OPC_ClientDelete() - выключение клиента
    [DllImport("DLL1")]
    public static extern int OPC_ClientDelete();

    //6. int OPC_ClientCallMethod(NodeID, value) - вызов метода по никальному ID (нгапример, NodeID=62541) и строковым параметров
    [DllImport("DLL1")]
    public static extern int OPC_ClientCallMethod(uint NodeID, StringBuilder value);

    //7. int OPC_UA_Client_Service_browse() - читает всю структуру иерархии дерева с сервера для возможности писать не только в корневые объекты, возвращает число прочитанных узлов
    [DllImport("DLL1")]
    public static extern int OPC_UA_Client_Service_browse();

    //8. int OPC_ClientSubscriptions - выполняет подписку на все перменные, ранее переданные через OPC_ClientSubscriptionAddVariable. Параметр - частота опроса, мсек, т.е. 1000=1с
    [DllImport("DLL1")]
    public static extern int OPC_ClientSubscriptions(double interval);

    //9. OPC_ClientSubscriptionAddVariable - добавляет в подписку (выполнять до OPC_ClientSubscriptions) одну переменную (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    [DllImport("DLL1")]
    public static extern void OPC_ClientSubscriptionAddVariable(StringBuilder objectname, StringBuilder varname);

    //10. OPC_ClientSubscriptionAddVariable - список серверов
    [DllImport("DLL1")]
    public static extern int OPC_Client_findServers();

    //11. OPC_Check_Object - int OPC_Check_Object - возвращает 1 если такой объект с таким параметром есть, иначе 0 (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ) (выполнять после OPC_ClientSubscriptions)
    [DllImport("DLL1")]
    public static extern int OPC_Check_Object(StringBuilder objectname, StringBuilder varname);





    //Поток и блокирующий мьютекс для сервера
    private Thread Server_Thread;
    private Mutex Server_mutexObj;

    //Поток и блокирующий мьютекс для клиента
    private Thread Client_Thread;
    private Mutex Client_mutexObj;

    //хранение буфера переменных для чтения и записи
    public Dictionary<KeyValuePair<string, string>, double> VariablesForRead;
    public Dictionary<KeyValuePair<string, string>, double> VariablesToWrite;
    //хранение переменных
    public static Dictionary<KeyValuePair<string, string>, double> Variables = new Dictionary<KeyValuePair<string, string>, double>();

    //флаг останова потоков сервера и клиента
    bool stop = false;


    //События на будующее
    public MyMethodCallEvent ReceiveMethodCallEvent;
    private static List<KeyValuePair<uint, string>> allEvent = new List<KeyValuePair<uint, string>>();

    public MyDoubleChangeEvent ReceiveDoubleChangeEvent;
    public MyStringChangeEvent ReceiveStringChangeEvent;
    private static List<ChangeVariableClass> allVariableEvents = new List<ChangeVariableClass>();


    //При удалении объекта
    void OnDestroy()
    {
        Debug.Log("OnDestroy!!!!");

        
        if (Server_mutexObj != null) Server_mutexObj.WaitOne();
        if (Client_mutexObj != null) Client_mutexObj.WaitOne();

        if (allEvent != null) allEvent.Clear();
        if (allVariableEvents != null) allVariableEvents.Clear();
        if (VariablesForRead != null) VariablesForRead.Clear();
        if (VariablesToWrite != null) VariablesToWrite.Clear();
        if (Variables != null) Variables.Clear();


        stop = true;



        if (clientConnected == true)
        {
            OPC_ClientDelete();
            Client_Thread.Abort();
            // Waiting for the thread to terminate.
            Client_Thread.Join();
            Debug.Log("Client_Thread is terminating");
        }

        if (serverConnected == true)
        {
            OPC_ServerShutdown();
            Server_Thread.Abort();
            // Waiting for the thread to terminate.
            Server_Thread.Join();
            Debug.Log("Server_Thread is terminating");
        }

        serverConnected = false;
        clientConnected = false;

        RegisterDebugCallback(null);
        RegisterMethodCallCallback(null);
        RegisterValueChangeCallback(null);
        RegisterServerValueChangeCallback(null);

        if (allEvent != null) allEvent.Clear();
        if (allVariableEvents != null) allVariableEvents.Clear();
        if (VariablesForRead != null) VariablesForRead.Clear();
        if (VariablesToWrite != null) VariablesToWrite.Clear();
        if (Variables != null) Variables.Clear();

        
        Debug.Log("Native library successfully unloaded.");

        Debug.Log(".");
    }

    //При старте объекта


    

    void Start()
    {
        return;
    }







    //Операции выполняемые в потоке клиента и сервера
    void Update()
    {
        //return;

        if (Server_mutexObj != null)
        {
            Server_mutexObj.WaitOne();
            //..........
            Server_mutexObj.ReleaseMutex();
        }

        if (Client_mutexObj != null)
        {
            Client_mutexObj.WaitOne();
            //..........
            for (int i=0; i < allEvent.Count; i++)
            {
                if (ReceiveMethodCallEvent != null)
                {
                    ReceiveMethodCallEvent.Invoke((int)allEvent[i].Key, allEvent[i].Value);
                }
            }
            allEvent.Clear();
            //
            for (int i = 0; i < allVariableEvents.Count; i++)
            {
                if ((allVariableEvents[i].ValueString != "")&&(ReceiveStringChangeEvent != null))
                {
                    ReceiveStringChangeEvent.Invoke(allVariableEvents[i].objectName, allVariableEvents[i].VarName, allVariableEvents[i].ValueString);
                }
                else if ((allVariableEvents[i].ValueString == "") && (ReceiveDoubleChangeEvent != null))
                {
                    ReceiveDoubleChangeEvent.Invoke(allVariableEvents[i].objectName, allVariableEvents[i].VarName, allVariableEvents[i].ValueDouble);
                }
            }
            allVariableEvents.Clear();
            
            //
            Client_mutexObj.ReleaseMutex();
        }
        return;
        

    }


    //--------------------------КЛИЕНТ-----------------------------------------
    bool clientConnected = false;
    //Подключение к клиенту, по умолчанию на 127.0.0.1
    public void ClientConnect(string IP = "opc.tcp://127.0.0.1:4840")
    {
        VariablesForRead = new Dictionary<KeyValuePair<string, string>, double>();
        VariablesToWrite = new Dictionary<KeyValuePair<string, string>, double>();
       

        //1. int OPC_ClientConnect (url) // "opc.tcp://localhost:4840"
        //string IP = "opc.tcp://127.0.0.1:4840";
        StringBuilder addr = new StringBuilder(IP, 100);
        int ret = OPC_ClientConnect(addr);
        Debug.Log("OPC_ClientConnect=" + ret.ToString());

        //2. StartClientThread();

        clientConnected = true;
    }


    //чтение значения переменной типа double (вызов с клиента) (имя объекта, имя переменной, удалить после возврата значения или продолжать запрашивать постоянно)
    public double getClientRead(string objectname, string varname, bool deleteAfrerRead=false)
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return 0;
        }
        if (Client_mutexObj == null) return 0;
        Client_mutexObj.WaitOne();
        KeyValuePair<string, string> request = new KeyValuePair<string, string>(objectname, varname);
        if (VariablesForRead.ContainsKey(request) == true)
        {
            double returnValue = VariablesForRead[request];
            //удаляем значение после прочтения, чтобы не запрашивать постоянно
            if (deleteAfrerRead == true) VariablesForRead.Remove(request);
            Client_mutexObj.ReleaseMutex();
            return returnValue;
        }
        else
        {
            VariablesForRead[request] = 0;
        }
        Client_mutexObj.ReleaseMutex();
        return 0;
    }

    //запись значения переменной типа double (вызов с клиента) (имя объекта, имя переменной, значение)
    public void setClientWrite(string objectname, string varname, double value)
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return;
        }

        if (Client_mutexObj == null) return;

        Client_mutexObj.WaitOne();
        KeyValuePair<string, string> request = new KeyValuePair<string, string>(objectname, varname);
        VariablesToWrite[request] = value;
        Client_mutexObj.ReleaseMutex();
        return;
    }

    //вызов метода (вызов с клиента) (УНИКАЛЬНЫЙ НОМЕР МЕТОДА, параметр)
    public void ClientCallMethod(uint NodeId, string value)
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return;
        }

        // = 62541
        // = "hello" 
        StringBuilder value2 = new StringBuilder(value, value.Length + 100);
        OPC_ClientCallMethod(NodeId, value2);
    }

    //Добавить переменную в список на получение обновлений (имя объекта, имя переменной)
    public void ClientSubscriptionAddVariable(string _objectname, string _varname)
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return;
        }

        StringBuilder objectname = new StringBuilder(_objectname, 100);
        StringBuilder varname = new StringBuilder(_varname, 100);
        OPC_ClientSubscriptionAddVariable(objectname, varname);
        return;
    }


    //возвращает 1 если такой объект с таким параметром есть, иначе 0 (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ) (выполнять после OPC_ClientSubscriptions)
    public int Check_ObjectVariable(string _objectname, string _varname)
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return;
        }

        StringBuilder objectname = new StringBuilder(_objectname, 100);
        StringBuilder varname = new StringBuilder(_varname, 100);
        
        return OPC_Check_Object(objectname, varname); 
    }


    public void Client_findServers()
    {
 

        OPC_Client_findServers();
        return;
    }
    

    //Выполнить подписку на все ранее указанные (ClientSubscriptionAddVariable) переменные  (вызов с клиента)
    public int ClientSubscription( double interval)
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return 0;
        }

        int returnvalue = OPC_ClientSubscriptions( interval);
        return returnvalue;
    }

    //выполнить запрос на чтение структуры СЕРВЕРА  (вызов с клиента). Необходимо если хотим обращаться по имени объекта/переменной
    public int UA_Client_Service_browse()
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return 0;
        }

        int returnvalue = OPC_UA_Client_Service_browse();
        return returnvalue;
    }

    bool upates = false;
    int sleep_value = 15;
    //Запуск потока клиента
    public void StartClientThread(bool _updates = false, int _sleep_value = 15)
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return;
        }

        sleep_value = _sleep_value;
        upates = _updates;
        Client_mutexObj = new Mutex();
        Client_Thread = new Thread(new ThreadStart(client_Thread_loop));
        Client_Thread.IsBackground = true;
        Client_Thread.Start();
    }

    //поток клиента
    private void client_Thread_loop()
    {
        while (stop == false)
        {
            Thread.Sleep(sleep_value);//15
            if (upates==true) OPC_ClientUpdate();

            Client_mutexObj.WaitOne();

            //чтение
            KeyValuePair<string, string>[] keys = new KeyValuePair<string, string>[VariablesForRead.Keys.Count];
            VariablesForRead.Keys.CopyTo(keys, 0);
            for (int i = 0; i < keys.Length; ++i)
            {
                StringBuilder objectname = new StringBuilder(keys[i].Key, 100);
                StringBuilder varname = new StringBuilder(keys[i].Value, 100);
                KeyValuePair<string, string> request = new KeyValuePair<string, string>(keys[i].Key, keys[i].Value);
                VariablesForRead[request] = OPC_ClientReadValueDouble(objectname, varname);
            }

            //запись
            foreach (var key in VariablesToWrite.Keys)
            {
                StringBuilder objectname = new StringBuilder(key.Key, 100);
                StringBuilder varname = new StringBuilder(key.Value, 100);
                int ret3 = OPC_ClientWriteValueDouble(objectname, varname, VariablesToWrite[key]);
            }
            VariablesToWrite.Clear();

            Client_mutexObj.ReleaseMutex();
        }
    }

    //возврат переменных пойманных в подписке на изменение в клиенте
    public double GetStoredVariable(string objname, string varname)
    {
        if (clientConnected == false)
        {
            Debug.Log("Client not connected!!! Stop!");
            return 0;
        }

        KeyValuePair<string, string> request = new KeyValuePair<string, string>(objname, varname);

        if (Variables.ContainsKey(request) == true)
        {
            return Variables[request];
        }
        else
        {
            Debug.Log("GetStoredVariable: variable=" + objname + "." + varname + " not found in stored....");
            return 0;
        }
    }


    //--------------------------СЕРВЕР-----------------------------------------
    bool serverConnected = false;
    //Создать сервер
    public void startServer()
    {
        OPC_ServerCreate();
        serverConnected = true;
    }

    //11. int OPC_ServerSubscription(objectString, varNameString, interval) - (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ИНТЕРВАЛ 1000=1с)
    public void ServerSubscription(string objectname, string name, double interval)
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return;
        }

        StringBuilder objectname2 = new StringBuilder(objectname, 100);
        StringBuilder varname = new StringBuilder(name, 100);

        OPC_ServerSubscription(objectname2, varname, interval);
    }

    //Добавить переменную (вызов из сервера) (имя объекта, имя переменной, название переменной, тип= 0- double 1-string, samplingInterval)
    public void ServerAddVariable(string objectname, string name, string discription, int type, double samplingInterval=50)
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return;
        }

        StringBuilder objectname2 = new StringBuilder(objectname, 100);
        StringBuilder varname = new StringBuilder(name, 100);
        StringBuilder vardiscription = new StringBuilder(discription, 100);
        OPC_ServerAddVariable(objectname2, varname, vardiscription, type, samplingInterval);
    }

    //Записать в переменную типа double(вызов из сервера) (имя объекта, имя переменной, значение)
    public void ServerWriteValueDouble(string objectname, string name, double value)
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return;
        }

        StringBuilder objectname2 = new StringBuilder(objectname, 100);
        StringBuilder varname = new StringBuilder(name, 100);
        OPC_ServerWriteValueDouble(objectname2, varname, value);
    }

    //Прочитать переменную типа double(вызов из сервера) (имя объекта, имя переменной)
    public double ServerReadValueDouble(string objectname, string name)
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return 0;
        }

        StringBuilder objectname2 = new StringBuilder(objectname, 100);
        StringBuilder varname = new StringBuilder(name, 100);
        return OPC_ServerReadValueDouble(objectname2, varname);
    }

    //Записать в переменную типа string(вызов из сервера) (имя объекта, имя переменной, значение)
    public void ServerWriteValueString(string objectString, string descriptionString, string value)
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return;
        }

        StringBuilder objectString2 = new StringBuilder(objectString, 100);
        StringBuilder descriptionString2 = new StringBuilder(descriptionString, 100);
        StringBuilder value2 = new StringBuilder(value, 100);
        OPC_ServerWriteValueString(objectString2, descriptionString2, value2);
    }

    //Прочитать переменную типа string (вызов из сервера) (имя объекта, имя переменной)
    public string ServerReadValueString(string objectString, string descriptionString)
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return "";
        }

        StringBuilder returnString = new StringBuilder("", 100);
        StringBuilder objectString2 = new StringBuilder(objectString, 100);
        StringBuilder descriptionString2 = new StringBuilder(descriptionString, 100);
        OPC_ServerReadValueString(returnString, returnString.Capacity, objectString2, descriptionString2);
        string myString2 = returnString.ToString();
        //Debug.Log("c# debugServerReadValueString: " + myString2);
        return myString2;
    }

    //Создать метод (вызов из сервера) (уникальный номер, имя метода, название, описание)
    public void ServerCreateMethod(uint nodeID, string name, string displayName, string description)
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return;
        }

        StringBuilder name2 = new StringBuilder(name, 100);
        StringBuilder displayName2 = new StringBuilder(displayName, 100);
        StringBuilder description2 = new StringBuilder(description, 100);

        OPC_ServerCreateMethod(nodeID, name2, displayName2, description2);
    }

    //Вызов метода (вызов из сервера) (уникальный номер, значение параметра)
    public void ServerCallMethod(uint nodeID, string value)
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return;
        }

        StringBuilder value2 = new StringBuilder(value, value.Length+ 100);
        OPC_ServerCallMethod(nodeID, value2);
    }

    

    //Остановить сервер
    public void stopServer()
    {
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return;
        }

        RegisterDebugCallback(null);
        RegisterMethodCallCallback(null);
        RegisterValueChangeCallback(null);
        Server_Thread.Abort();

        OPC_ServerShutdown();
    }

    //Запустить поток сервера
    public void StartServerThread()
    {
        Server_mutexObj = new Mutex();
        Server_Thread = new Thread(new ThreadStart(server_Thread_loop));
        Server_Thread.IsBackground = true;
        Server_Thread.Start();
    }

    //поток сервера
    private void server_Thread_loop()
    {
        while (stop == false)
        {
            //Thread.Sleep(15);
            OPC_ServerUpdate();

            Server_mutexObj.WaitOne();
            //zzz = testServerRead();
            Server_mutexObj.ReleaseMutex();
        }
    }

    //--------------------------СЕРВЕР END-------------------------------------
}

/*
public class SimplyOPCtest : MonoBehaviour
{
    public OPCUA_SERVER_DLL NET;
    int mode = -1;

    public GameObject My;

    public void StopServer()
    {
        NET.stopServer();
        mode = -1;
    }


    public void SelectDriller()
    {
        mode = 0;
        NET.RegisterCallback();

        NET.startServer();

        NET.ServerAddVariable("", "x", "x", 0);
        NET.ServerAddVariable("", "y", "y", 0);
        NET.ServerAddVariable("", "z", "z", 0);

        NET.ServerAddVariable("Men1", "x", "x",0);
        NET.ServerAddVariable("Men1", "y", "y", 0);
        NET.ServerAddVariable("Men1", "z", "z", 0);
        NET.ServerAddVariable("Men1", "r", "r", 0);

        NET.ServerCreateMethod(62541, "hello world", "Hello World", "Say `Hello World`");
        NET.ServerCreateMethod(62542, "A", "B", "C");

        NET.StartServerThread();
        NET.ServerCallMethod(62541, "Anna");
        NET.ServerCallMethod(62542, "Omega");

        NET.ServerAddVariable("AAA", "Men1.x", "Men1.x", 0);
        NET.ServerAddVariable("AAA", "Men1.y", "Men1.y", 0);
        NET.ServerAddVariable("AAA", "Men1.s", "Men1.s", 1);

        NET.ServerWriteValueDouble("AAA", "Men1.y", 12);
        Debug.Log("ServerWriteValueDouble=" + NET.ServerReadValueDouble("AAA", "Men1.y"));

        NET.ServerWriteValueString("AAA", "Men1.s", "Максим Гаммер");
        Debug.Log("ServerReadValueString=" + NET.ServerReadValueString("AAA", "Men1.s"));

        {
            NET.ServerAddVariable("V-448", "TI448-A", "TI448-A", 0);
            NET.ServerWriteValueDouble("V-448", "TI448-A", 241.452127789);

            NET.ServerAddVariable("V-448", "PDI448-B", "PDI448-B", 0);
            NET.ServerWriteValueDouble("V-448", "PDI448-B", 149.97412210);
            //

            NET.ServerAddVariable("V-449", "TI449-A", "TI449-A", 0);
            NET.ServerWriteValueDouble("V-449", "TI449-A", 39.94973481);

            NET.ServerAddVariable("V-449", "PDI449-B", "PDI449-B", 0);
            NET.ServerWriteValueDouble("V-449", "PDI449-B", 117.6398164);
        }

        



        NET.ClientConnect();
        Debug.Log("UA_Client_Service_browse=" + NET.UA_Client_Service_browse());
        //
        NET.ClientCallMethod(62541, "Hello from maximum2000 !");

        //подписка
        {
            NET.ClientSubscriptionAddVariable("AAA", "Men1.x");
            NET.ClientSubscriptionAddVariable("AAA", "Men1.y");
            NET.ClientSubscriptionAddVariable("", "x");
            NET.ClientSubscriptionAddVariable("", "y");
            int tempNumber = NET.ClientSubscription(100);
        }
        
        NET.setClientWrite("AAA", "Men1.x", 123456789);
        Debug.Log("test=" + NET.getClientRead("AAA", "Men1.x"));

        //!!! Внимание !!! запускать поток клиента можно ТОЛЬКО после подписок и всего остального
        NET.StartClientThread();


        //!!! Внимание !!! читать напрямую и писать напрямую уже только после  запуска потока клиента
        NET.setClientWrite("AAA", "Men1.x", 123456789);
        Debug.Log("test=" + NET.getClientRead("AAA", "Men1.x"));
    }

    public void SelectDrillerHelper1()
    {
        NET.ClientConnect();
        mode = 1;
   
    }

    public void SelectDrillerHelper2()
    {
        NET.ClientConnect();
        mode = 2;
   
    }


    // Start is called before the first frame update
    void Start()
    {
        
    }

    public void Write (string objectname, string varname, double value)
    {
        if (mode == -1) return;
        NET.setClientWrite(objectname, varname, value);
    }

    // Update is called once per frame
    void Update()
    {
        if (mode == -1) return;

        //читаю всех
        float x1 = (float)NET.getClientRead("Men1", "x");
        float y1 = (float)NET.getClientRead("Men1", "y");
        float z1 = (float)NET.getClientRead("Men1", "z");
        float r1 = (float)NET.getClientRead("Men1", "r");

        if (mode==0)
        {
            //скрываю 0го, передвигаю первого и второго
            //Men0.SetActive(false);
            NET.setClientWrite("Men1", "x", (double)My.transform.localPosition.x);
            NET.setClientWrite("Men1", "y", (double)My.transform.localPosition.y);
            NET.setClientWrite("Men1", "z", (double)My.transform.localPosition.z);
            NET.setClientWrite("Men1", "r", (double)My.transform.localEulerAngles.y);

            //NET.test();
        }

        if (mode == 1)
        {
        }

        if (mode == 2)
        {
        }


    }
}
*/


