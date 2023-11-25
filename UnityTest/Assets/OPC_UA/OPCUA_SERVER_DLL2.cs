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
public class MyMethodCallEvent2 : UnityEvent<int, string> { }

//Для альтернативного вызова функций из DLL (иначе Unity не отпустит DLL)
//см. https://stackoverflow.com/questions/12822781/from-a-c-sharp-method-how-to-call-and-run-a-dll-where-the-dll-name-comes-from
internal static class NativeWinAPI
{
    //[DllImport("kernel32.dll")]
    //internal static extern IntPtr LoadLibrary(string dllToLoad);

    //[DllImport("kernel32.dll")]
    //internal static extern bool FreeLibrary(IntPtr hModule);

    //[DllImport("kernel32.dll")]
    //internal static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);

    [DllImport("kernel32", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool FreeLibrary(IntPtr hModule);

    [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr LoadLibrary(string lpFileName);

    [DllImport("kernel32")]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);
}


public class OPCUA_SERVER_DLL2 : MonoBehaviour
{
    IntPtr hLibrary = IntPtr.Zero;

    //Обработчики call-back функций (функции ызываются из DLL)
    //1.RegisterDebugCallback - регистрация callback'ов
    //2. SendLog из DLL -> DebugLog в Unity = дебаг-вывод в unity
    //3. MethodCall из DLL -> MethodCall в Unity = для обработчика вызова метода'а
    //4. из DLL -> ValueChange в Unity = callback для обработчика изменения переменной по подписке (для клиента)
    //5. из DLL -> ValueChange в Unity = callback для обработчика изменения переменной по подписке (для сервера)

    //1.регистрация всех callbeck функций (привязка)
    public void RegisterCallback()
    {
        //string projectFolder = Path.Combine(Application.dataPath, "../");
#if UNITY_EDITOR
        string DLLFileName = Application.dataPath +  "/Plugins/Dll1.dll";
        Debug.Log("DLLFileName=" + DLLFileName);
#else
        string DLLFileName = Application.dataPath +  "/Plugins/x86_64/Dll1.dll";
#endif
        hLibrary = NativeWinAPI.LoadLibrary(DLLFileName);

        if (hLibrary != IntPtr.Zero) // DLL is loaded successfully
        {
            Debug.Log("hLibrary != IntPtr.Zero");

            {
                IntPtr pointerToRegisterDebugCallback = NativeWinAPI.GetProcAddress(hLibrary, "RegisterDebugCallback");
                if (pointerToRegisterDebugCallback != IntPtr.Zero)
                {
                    Debug.Log("RegisterDebugCallback != IntPtr.Zero");
                    RegisterDebugCallback = (MyFunctionDelegate_RegisterDebugCallback)Marshal.GetDelegateForFunctionPointer(pointerToRegisterDebugCallback, typeof(MyFunctionDelegate_RegisterDebugCallback));
                    //function_RegisterDebugCallback(DebugLog); 
                }
                else
                {
                    Debug.Log("RegisterDebugCallback == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToRegisterMethodCallCallback = NativeWinAPI.GetProcAddress(hLibrary, "RegisterMethodCallCallback");
                if (pointerToRegisterMethodCallCallback != IntPtr.Zero)
                {
                    Debug.Log("RegisterMethodCallCallback != IntPtr.Zero");
                    RegisterMethodCallCallback = (MyFunctionDelegate_MethodCallCallback)Marshal.GetDelegateForFunctionPointer(pointerToRegisterMethodCallCallback, typeof(MyFunctionDelegate_MethodCallCallback));
                }
                else
                {
                    Debug.Log("RegisterMethodCallCallback == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToRegisterValueChangeCallback = NativeWinAPI.GetProcAddress(hLibrary, "RegisterValueChangeCallback");
                if (pointerToRegisterValueChangeCallback != IntPtr.Zero)
                {
                    Debug.Log("RegisterValueChangeCallback != IntPtr.Zero");
                    RegisterValueChangeCallback = (MyFunctionDelegate_RegisterValueChangeCallback)Marshal.GetDelegateForFunctionPointer(pointerToRegisterValueChangeCallback, typeof(MyFunctionDelegate_RegisterValueChangeCallback));
                }
                else
                {
                    Debug.Log("RegisterValueChangeCallback == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToRegisterServerValueChangeCallback = NativeWinAPI.GetProcAddress(hLibrary, "RegisterServerValueChangeCallback");
                if (pointerToRegisterServerValueChangeCallback != IntPtr.Zero)
                {
                    Debug.Log("RegisterServerValueChangeCallback != IntPtr.Zero");
                    RegisterServerValueChangeCallback = (MyFunctionDelegate_RegisterServerValueChangeCallback)Marshal.GetDelegateForFunctionPointer(pointerToRegisterServerValueChangeCallback, typeof(MyFunctionDelegate_RegisterServerValueChangeCallback));
                }
                else
                {
                    Debug.Log("RegisterServerValueChangeCallback == IntPtr.Zero");
                }
                
            }
            ////////////////
            {
                IntPtr pointerToOPC_ServerCreate = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerCreate");
                if (pointerToOPC_ServerCreate != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerCreate != IntPtr.Zero");
                    OPC_ServerCreate = (MyFunctionDelegate_OPC_ServerCreate)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerCreate, typeof(MyFunctionDelegate_OPC_ServerCreate));
                }
                else
                {
                    Debug.Log("OPC_ServerCreate == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerUpdate = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerUpdate");
                if (pointerToOPC_ServerUpdate != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerUpdate != IntPtr.Zero");
                    OPC_ServerUpdate = (MyFunctionDelegate_OPC_ServerUpdate)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerUpdate, typeof(MyFunctionDelegate_OPC_ServerUpdate));
                }
                else
                {
                    Debug.Log("OPC_ServerUpdate == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerAddVariable = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerAddVariable");
                if (pointerToOPC_ServerAddVariable != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerAddVariable != IntPtr.Zero");
                    OPC_ServerAddVariable = (MyFunctionDelegate_OPC_ServerAddVariable)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerAddVariable, typeof(MyFunctionDelegate_OPC_ServerAddVariable));
                }
                else
                {
                    Debug.Log("OPC_ServerAddVariable == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerWriteValueDouble = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerWriteValueDouble");
                if (pointerToOPC_ServerWriteValueDouble != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerWriteValueDouble != IntPtr.Zero");
                    OPC_ServerWriteValueDouble = (MyFunctionDelegate_OPC_ServerWriteValueDouble)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerWriteValueDouble, typeof(MyFunctionDelegate_OPC_ServerWriteValueDouble));
                }
                else
                {
                    Debug.Log("OPC_ServerWriteValueDouble == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerReadValueDouble = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerReadValueDouble");
                if (pointerToOPC_ServerReadValueDouble != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerReadValueDouble != IntPtr.Zero");
                    OPC_ServerReadValueDouble = (MyFunctionDelegate_OPC_ServerReadValueDouble)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerReadValueDouble, typeof(MyFunctionDelegate_OPC_ServerReadValueDouble));
                }
                else
                {
                    Debug.Log("OPC_ServerReadValueDouble == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerWriteValueString = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerWriteValueString");
                if (pointerToOPC_ServerWriteValueString != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerWriteValueString != IntPtr.Zero");
                    OPC_ServerWriteValueString = (MyFunctionDelegate_OPC_ServerWriteValueString)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerWriteValueString, typeof(MyFunctionDelegate_OPC_ServerWriteValueString));
                }
                else
                {
                    Debug.Log("OPC_ServerWriteValueString == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerReadValueString = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerReadValueString");
                if (pointerToOPC_ServerReadValueString != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerReadValueString != IntPtr.Zero");
                    OPC_ServerReadValueString = (MyFunctionDelegate_OPC_ServerReadValueString)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerReadValueString, typeof(MyFunctionDelegate_OPC_ServerReadValueString));
                }
                else
                {
                    Debug.Log("OPC_ServerReadValueString == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerShutdown = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerShutdown");
                if (pointerToOPC_ServerShutdown != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerShutdown != IntPtr.Zero");
                    OPC_ServerShutdown = (MyFunctionDelegate_OPC_ServerShutdown)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerShutdown, typeof(MyFunctionDelegate_OPC_ServerShutdown));
                }
                else
                {
                    Debug.Log("OPC_ServerShutdown == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerCreateMethod = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerCreateMethod");
                if (pointerToOPC_ServerCreateMethod != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerCreateMethod != IntPtr.Zero");
                    OPC_ServerCreateMethod = (MyFunctionDelegate_OPC_ServerCreateMethod)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerCreateMethod, typeof(MyFunctionDelegate_OPC_ServerCreateMethod));
                }
                else
                {
                    Debug.Log("OPC_ServerCreateMethod == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerCallMethod = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerCallMethod");
                if (pointerToOPC_ServerCallMethod != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerCallMethod != IntPtr.Zero");
                    OPC_ServerCallMethod = (MyFunctionDelegate_OPC_ServerCallMethod)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerCallMethod, typeof(MyFunctionDelegate_OPC_ServerCallMethod));
                }
                else
                {
                    Debug.Log("OPC_ServerCallMethod == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ServerSubscription = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ServerSubscription");
                if (pointerToOPC_ServerSubscription != IntPtr.Zero)
                {
                    Debug.Log("OPC_ServerSubscription != IntPtr.Zero");
                    OPC_ServerSubscription = (MyFunctionDelegate_OPC_ServerSubscription)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ServerSubscription, typeof(MyFunctionDelegate_OPC_ServerSubscription));
                }
                else
                {
                    Debug.Log("OPC_ServerSubscription == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ClientConnect = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ClientConnect");
                if (pointerToOPC_ClientConnect != IntPtr.Zero)
                {
                    Debug.Log("OPC_ClientConnect != IntPtr.Zero");
                    OPC_ClientConnect = (MyFunctionDelegate_OPC_ClientConnect)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ClientConnect, typeof(MyFunctionDelegate_OPC_ClientConnect));
                }
                else
                {
                    Debug.Log("OPC_ClientConnect == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ClientWriteValueDouble = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ClientWriteValueDouble");
                if (pointerToOPC_ClientWriteValueDouble != IntPtr.Zero)
                {
                    Debug.Log("OPC_ClientWriteValueDouble != IntPtr.Zero");
                    OPC_ClientWriteValueDouble = (MyFunctionDelegate_OPC_ClientWriteValueDouble)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ClientWriteValueDouble, typeof(MyFunctionDelegate_OPC_ClientWriteValueDouble));
                }
                else
                {
                    Debug.Log("OPC_ClientWriteValueDouble == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ClientReadValueDouble = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ClientReadValueDouble");
                if (pointerToOPC_ClientReadValueDouble != IntPtr.Zero)
                {
                    Debug.Log("OPC_ClientReadValueDouble != IntPtr.Zero");
                    OPC_ClientReadValueDouble = (MyFunctionDelegate_OPC_ClientReadValueDouble)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ClientReadValueDouble, typeof(MyFunctionDelegate_OPC_ClientReadValueDouble));
                }
                else
                {
                    Debug.Log("OPC_ClientReadValueDouble == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ClientUpdate = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ClientUpdate");
                if (pointerToOPC_ClientUpdate != IntPtr.Zero)
                {
                    Debug.Log("OPC_ClientUpdate != IntPtr.Zero");
                    OPC_ClientUpdate = (MyFunctionDelegate_OPC_ClientUpdate)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ClientUpdate, typeof(MyFunctionDelegate_OPC_ClientUpdate));
                }
                else
                {
                    Debug.Log("OPC_ClientUpdate == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ClientDelete = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ClientDelete");
                if (pointerToOPC_ClientDelete != IntPtr.Zero)
                {
                    Debug.Log("OPC_ClientDelete != IntPtr.Zero");
                    OPC_ClientDelete = (MyFunctionDelegate_OPC_ClientDelete)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ClientDelete, typeof(MyFunctionDelegate_OPC_ClientDelete));
                }
                else
                {
                    Debug.Log("OPC_ClientDelete == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ClientCallMethod = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ClientCallMethod");
                if (pointerToOPC_ClientCallMethod != IntPtr.Zero)
                {
                    Debug.Log("OPC_ClientCallMethod != IntPtr.Zero");
                    OPC_ClientCallMethod = (MyFunctionDelegate_OPC_ClientCallMethod)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ClientCallMethod, typeof(MyFunctionDelegate_OPC_ClientCallMethod));
                }
                else
                {
                    Debug.Log("OPC_ClientCallMethod == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_UA_Client_Service_browse = NativeWinAPI.GetProcAddress(hLibrary, "OPC_UA_Client_Service_browse");
                if (pointerToOPC_UA_Client_Service_browse != IntPtr.Zero)
                {
                    Debug.Log("OPC_UA_Client_Service_browse != IntPtr.Zero");
                    OPC_UA_Client_Service_browse = (MyFunctionDelegate_OPC_UA_Client_Service_browse)Marshal.GetDelegateForFunctionPointer(pointerToOPC_UA_Client_Service_browse, typeof(MyFunctionDelegate_OPC_UA_Client_Service_browse));
                }
                else
                {
                    Debug.Log("OPC_UA_Client_Service_browse == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ClientSubscriptions = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ClientSubscriptions");
                if (pointerToOPC_ClientSubscriptions != IntPtr.Zero)
                {
                    Debug.Log("OPC_ClientSubscriptions != IntPtr.Zero");
                    OPC_ClientSubscriptions = (MyFunctionDelegate_OPC_ClientSubscriptions)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ClientSubscriptions, typeof(MyFunctionDelegate_OPC_ClientSubscriptions));
                }
                else
                {
                    Debug.Log("OPC_ClientSubscriptions == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_ClientSubscriptionAddVariable = NativeWinAPI.GetProcAddress(hLibrary, "OPC_ClientSubscriptionAddVariable");
                if (pointerToOPC_ClientSubscriptionAddVariable != IntPtr.Zero)
                {
                    Debug.Log("OPC_ClientSubscriptionAddVariable != IntPtr.Zero");
                    OPC_ClientSubscriptionAddVariable = (MyFunctionDelegate_OPC_ClientSubscriptionAddVariable)Marshal.GetDelegateForFunctionPointer(pointerToOPC_ClientSubscriptionAddVariable, typeof(MyFunctionDelegate_OPC_ClientSubscriptionAddVariable));
                }
                else
                {
                    Debug.Log("OPC_ClientSubscriptionAddVariable == IntPtr.Zero");
                }
            }
            {
                IntPtr pointerToOPC_Client_findServers = NativeWinAPI.GetProcAddress(hLibrary, "OPC_Client_findServers");
                if (pointerToOPC_Client_findServers != IntPtr.Zero)
                {
                    Debug.Log("OPC_Client_findServers != IntPtr.Zero");
                    OPC_Client_findServers = (MyFunctionDelegate_OPC_Client_findServers)Marshal.GetDelegateForFunctionPointer(pointerToOPC_Client_findServers, typeof(MyFunctionDelegate_OPC_Client_findServers));
                }
                else
                {
                    Debug.Log("OPC_Client_findServers == IntPtr.Zero");
                }
            }


            
            //и сам вызов функций
            RegisterDebugCallback(DebugLog);
            RegisterMethodCallCallback(MethodCall);
            RegisterValueChangeCallback(ValueChange);
            RegisterServerValueChangeCallback(ServerValueChange);
        }
        else
        {
            
            var errorCode = System.Runtime.InteropServices.Marshal.GetLastWin32Error();

            //throw new System.SystemException($"System failed to load library '{resource}' with error {errorCode}");
            

            Debug.Log("hLibrary == IntPtr.Zero , errorCode=" + errorCode);
        }
    }

    //При удалении объекта
    void OnDestroy()
    {
        Debug.Log("OnDestroy!!!!");

        if (hLibrary == IntPtr.Zero) return;
        if (Server_mutexObj != null) Server_mutexObj.WaitOne();
        if (Client_mutexObj != null) Client_mutexObj.WaitOne();

        if (allEvent != null) allEvent.Clear();
        if (VariablesForRead != null) VariablesForRead.Clear();
        if (VariablesToWrite != null) VariablesToWrite.Clear();
        if (Variables != null) Variables.Clear();


        stop = true;

        

        if (clientConnected==true)
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
        if (VariablesForRead != null) VariablesForRead.Clear();
        if (VariablesToWrite != null) VariablesToWrite.Clear();
        if (Variables != null) Variables.Clear();

        bool ok = NativeWinAPI.FreeLibrary(hLibrary);
        Debug.Log(ok
                      ? "Native library successfully unloaded."
                      : "Native library could not be unloaded.");

        hLibrary = IntPtr.Zero;

        //if (Server_mutexObj != null) Server_mutexObj.ReleaseMutex();
        //if (Client_mutexObj != null) Client_mutexObj.ReleaseMutex();
        Debug.Log(".");
    }

    //2. callback для Debug'а
    private delegate void DebugCallback(IntPtr message, int color, int size);
    //[DllImport("DLL1", CallingConvention = CallingConvention.Cdecl)]
    //private static extern void RegisterDebugCallback(DebugCallback callback);
    delegate void MyFunctionDelegate_RegisterDebugCallback(DebugCallback callback);
    MyFunctionDelegate_RegisterDebugCallback RegisterDebugCallback = null;
    //вывод сообщения из debug'а DLL'ки
    private static void DebugLog(IntPtr message, int color, int size)
    {
        string debugString = Marshal.PtrToStringAnsi(message, size);
        Debug.Log("c# debug:" + debugString);
    }
    //end callback для Debug'а



    //3. callback для обработчика вызова метода'а
    private delegate void MethodCallCallback(IntPtr message, uint nodeid, int size);
    //[DllImport("DLL1", CallingConvention = CallingConvention.Cdecl)]
    //private static extern void RegisterMethodCallCallback(MethodCallCallback callback);
    delegate void MyFunctionDelegate_MethodCallCallback(MethodCallCallback callback);
    MyFunctionDelegate_MethodCallCallback RegisterMethodCallCallback = null;
    private static void MethodCall(IntPtr message, uint nodeid, int size)
    {
        string debugString = Marshal.PtrToStringAnsi(message, size);
        Debug.Log("c# MethodCall:" + debugString + ", id=" + nodeid);
        allEvent.Add(new KeyValuePair<uint, string>(nodeid, debugString));

    }
    //end callback для обработчика вызова метода'а


    //4. callback для обработчика изменения переменной по подписке (для клиента)
    private delegate void ValueChangeCallback(IntPtr message1, int size1, IntPtr message2, int size2, uint monid, double value);
    //[DllImport("DLL1", CallingConvention = CallingConvention.Cdecl)]
    //private static extern void RegisterValueChangeCallback(ValueChangeCallback callback);
    delegate void MyFunctionDelegate_RegisterValueChangeCallback(ValueChangeCallback callback);
    MyFunctionDelegate_RegisterValueChangeCallback RegisterValueChangeCallback = null;
    private static void ValueChange(IntPtr message1, int size1, IntPtr message2, int size2, uint monid, double value)
    {
        string debugString1 = Marshal.PtrToStringAnsi(message1, size1);
        string debugString2 = Marshal.PtrToStringAnsi(message2, size2);

        Debug.Log("c# ValueChange:" + debugString1 + "*" + debugString2 + "*" + "monId =" + monid + ", value=" + value);// ", name=" + subscriptions[monid]);

        KeyValuePair<string, string> request = new KeyValuePair<string, string>(debugString1, debugString2);
        Variables[request] = value;
    }
    //end callback для обработчика изменения переменной по подписке (для клиента)

    //6. callback для обработчика изменения переменной по подписке  (для сервера)
    private delegate void ServerValueChangeCallback(IntPtr message1, int size1, IntPtr message2, int size2, uint monid, double value);
    //[DllImport("DLL1", CallingConvention = CallingConvention.Cdecl)]
    //private static extern void RegisterServerValueChangeCallback(ValueChangeCallback callback);
    delegate void MyFunctionDelegate_RegisterServerValueChangeCallback(ServerValueChangeCallback callback);
    MyFunctionDelegate_RegisterServerValueChangeCallback RegisterServerValueChangeCallback = null;
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
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerCreate();
    delegate int MyFunctionDelegate_OPC_ServerCreate();
    MyFunctionDelegate_OPC_ServerCreate OPC_ServerCreate = null;

    //2. int OPC_ServerUpdate () - обновление сервера
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerUpdate();
    delegate int MyFunctionDelegate_OPC_ServerUpdate();
    MyFunctionDelegate_OPC_ServerUpdate OPC_ServerUpdate = null;

    //3. int OPC_ServerAddVariable (objectname, varname, type)  - добавить переменную (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ТИП (0-double/1-int)
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerAddVariable(StringBuilder objectString, StringBuilder descriptionString, StringBuilder displayNameString, int type, double samplingInterval);
    delegate int MyFunctionDelegate_OPC_ServerAddVariable(StringBuilder objectString, StringBuilder descriptionString, StringBuilder displayNameString, int type, double samplingInterval);
    MyFunctionDelegate_OPC_ServerAddVariable OPC_ServerAddVariable = null;

    //4. int OPC_ServerWriteValueDouble (objectname, varname, value) - изменить переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerWriteValueDouble(StringBuilder objectString, StringBuilder descriptionString, double value);
    delegate int MyFunctionDelegate_OPC_ServerWriteValueDouble(StringBuilder objectString, StringBuilder descriptionString, double value);
    MyFunctionDelegate_OPC_ServerWriteValueDouble OPC_ServerWriteValueDouble = null;

    //5. double OPC_ServerReadValueDouble (objectname, varname) - прочитать напрямую переменную типа DOUBLE (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    //[DllImport("DLL1")]
    //public static extern double OPC_ServerReadValueDouble(StringBuilder objectString, StringBuilder descriptionString);
    delegate double MyFunctionDelegate_OPC_ServerReadValueDouble(StringBuilder objectString, StringBuilder descriptionString);
    MyFunctionDelegate_OPC_ServerReadValueDouble OPC_ServerReadValueDouble = null;

    //6. int OPC_ServerWriteValueString - изменить переменную типа STRING (ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerWriteValueString(StringBuilder objectString, StringBuilder descriptionString, StringBuilder value);
    delegate int MyFunctionDelegate_OPC_ServerWriteValueString(StringBuilder objectString, StringBuilder descriptionString, StringBuilder value);
    MyFunctionDelegate_OPC_ServerWriteValueString OPC_ServerWriteValueString = null;

    //7. int OPC_ServerReadValueString - прочитать напрямую переменную типа STRING (ВОЗРАЩАЕМОЕ ЗНАЧЕНИЕ, ВОЗВРАЩАЕМАЯ ДЛИННА,  ИМЯОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerReadValueString(StringBuilder returnString, int returnStringLength, StringBuilder objectString, StringBuilder descriptionString);
    delegate int MyFunctionDelegate_OPC_ServerReadValueString(StringBuilder returnString, int returnStringLength, StringBuilder objectString, StringBuilder descriptionString);
    MyFunctionDelegate_OPC_ServerReadValueString OPC_ServerReadValueString = null;

    //8. int OPC_ServerShutdown - выключение сервера
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerShutdown();
    delegate int MyFunctionDelegate_OPC_ServerShutdown();
    MyFunctionDelegate_OPC_ServerShutdown OPC_ServerShutdown = null;

    //9. int OPC_ServerCreateMethod (nodeID, name, displayName, description) - создание метода (УНИКАЛЬЫНЙ НОМЕР, ИМЯ МЕТОДА, НАЗВАНИЕ МЕТОДА, ОПИСАНИЕ)
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerCreateMethod(uint nodeID, StringBuilder name, StringBuilder displayName, StringBuilder description);
    delegate int MyFunctionDelegate_OPC_ServerCreateMethod(uint nodeID, StringBuilder name, StringBuilder displayName, StringBuilder description);
    MyFunctionDelegate_OPC_ServerCreateMethod OPC_ServerCreateMethod = null;

    //10. int OPC_ServerCallMethod - вызов метода из сервера - (УНИКАЛЬЫНЙ НОМЕР, ЗНАЧЕНИЕ)
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerCallMethod(uint nodeID, StringBuilder value);
    delegate int MyFunctionDelegate_OPC_ServerCallMethod(uint nodeID, StringBuilder value);
    MyFunctionDelegate_OPC_ServerCallMethod OPC_ServerCallMethod = null;

    //11. int OPC_ServerSubscription(objectString, varNameString, interval) - (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ИНТЕРВАЛ 1000=1с)
    //[DllImport("DLL1")]
    //public static extern int OPC_ServerSubscription(StringBuilder objectString, StringBuilder varNameString, double interval);
    delegate int MyFunctionDelegate_OPC_ServerSubscription(StringBuilder objectString, StringBuilder varNameString, double interval);
    MyFunctionDelegate_OPC_ServerSubscription OPC_ServerSubscription = null;



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
    //[DllImport("DLL1")]
    //public static extern int OPC_ClientConnect(StringBuilder url);
    delegate int MyFunctionDelegate_OPC_ClientConnect(StringBuilder url);
    MyFunctionDelegate_OPC_ClientConnect OPC_ClientConnect = null;

    //2. int OPC_ClientWriteValueDouble (objectname, varname, value) - записать переменную типа DOUBLE (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ, ЗНАЧЕНИЕ)
    //[DllImport("DLL1")]
    //public static extern int OPC_ClientWriteValueDouble(StringBuilder objectname, StringBuilder varname, double value);
    delegate int MyFunctionDelegate_OPC_ClientWriteValueDouble(StringBuilder objectname, StringBuilder varname, double value);
    MyFunctionDelegate_OPC_ClientWriteValueDouble OPC_ClientWriteValueDouble = null;

    //3. double OPC_ClientReadValueDouble (objectname, varname) - ПРЯМОЕ чтение переменной типа DOUBLE (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    //[DllImport("DLL1")]
    //public static extern double OPC_ClientReadValueDouble(StringBuilder objectname, StringBuilder varname);
    delegate double MyFunctionDelegate_OPC_ClientReadValueDouble(StringBuilder objectname, StringBuilder varname);
    MyFunctionDelegate_OPC_ClientReadValueDouble OPC_ClientReadValueDouble = null;

    //4. int OPC_ClientUpdate () - обновление клиента
    //[DllImport("DLL1")]
    //public static extern int OPC_ClientUpdate();
    delegate int MyFunctionDelegate_OPC_ClientUpdate();
    MyFunctionDelegate_OPC_ClientUpdate OPC_ClientUpdate = null;

    //5. int OPC_ClientDelete() - выключение клиента
    //[DllImport("DLL1")]
    //public static extern int OPC_ClientDelete();
    delegate int MyFunctionDelegate_OPC_ClientDelete();
    MyFunctionDelegate_OPC_ClientDelete OPC_ClientDelete = null;

    //6. int OPC_ClientCallMethod(NodeID, value) - вызов метода по никальному ID (нгапример, NodeID=62541) и строковым параметров
    //[DllImport("DLL1")]
    //public static extern int OPC_ClientCallMethod(uint NodeID, StringBuilder value);
    delegate int MyFunctionDelegate_OPC_ClientCallMethod(uint NodeID, StringBuilder value);
    MyFunctionDelegate_OPC_ClientCallMethod OPC_ClientCallMethod = null;

    //7. int OPC_UA_Client_Service_browse() - читает всю структуру иерархии дерева с сервера для возможности писать не только в корневые объекты, возвращает число прочитанных узлов
    //[DllImport("DLL1")]
    //public static extern int OPC_UA_Client_Service_browse();
    delegate int MyFunctionDelegate_OPC_UA_Client_Service_browse();
    MyFunctionDelegate_OPC_UA_Client_Service_browse OPC_UA_Client_Service_browse = null;

    //8. int OPC_ClientSubscriptions - выполняет подписку на все перменные, ранее переданные через OPC_ClientSubscriptionAddVariable. Параметр - частота опроса, мсек, т.е. 1000=1с
    //[DllImport("DLL1")]
    //public static extern int OPC_ClientSubscriptions(double interval);
    delegate int MyFunctionDelegate_OPC_ClientSubscriptions(double interval);
    MyFunctionDelegate_OPC_ClientSubscriptions OPC_ClientSubscriptions = null;

    //9. OPC_ClientSubscriptionAddVariable - добавляет в подписку (выполнять до OPC_ClientSubscriptions) одну переменную (ИМЯ ОБЪЕКТА, ИМЯ ПЕРЕМЕННОЙ)
    //[DllImport("DLL1")]
    //public static extern void OPC_ClientSubscriptionAddVariable(StringBuilder objectname, StringBuilder varname);
    delegate void MyFunctionDelegate_OPC_ClientSubscriptionAddVariable(StringBuilder objectname, StringBuilder varname);
    MyFunctionDelegate_OPC_ClientSubscriptionAddVariable OPC_ClientSubscriptionAddVariable = null;


    //10. OPC_ClientSubscriptionAddVariable - список серверов
    //[DllImport("DLL1")]
    //public static extern int OPC_Client_findServers();
    delegate void MyFunctionDelegate_OPC_Client_findServers(); //(int i);
    MyFunctionDelegate_OPC_Client_findServers OPC_Client_findServers = null;




    //Поток и блокирующий мьютекс для сервера
    private System.Threading.Thread Server_Thread;
    private Mutex Server_mutexObj;

    //Поток и блокирующий мьютекс для клиента
    private System.Threading.Thread Client_Thread;
    private Mutex Client_mutexObj;

    //хранение буфера переменных для чтения и записи
    public Dictionary<KeyValuePair<string, string>, double> VariablesForRead;
    public Dictionary<KeyValuePair<string, string>, double> VariablesToWrite;
    //хранение переменных
    public static Dictionary<KeyValuePair<string, string>, double> Variables;

    //флаг останова потоков сервера и клиента
    bool stop = false;


    //События на будующее
    public MyMethodCallEvent2 ReceiveMethodCallEvent;
    public static List<KeyValuePair<uint, string>> allEvent = new List<KeyValuePair<uint, string>>();


   

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
            Client_mutexObj.ReleaseMutex();
        }
        return;
        

    }


    //--------------------------КЛИЕНТ-----------------------------------------
    bool clientConnected = false;
    //Подключение к клиенту, по умолчанию на 127.0.0.1
    public void ClientConnect(string IP = "opc.tcp://127.0.0.1:4840")
    {
        if (hLibrary == IntPtr.Zero) return;

        VariablesForRead = new Dictionary<KeyValuePair<string, string>, double>();
        VariablesToWrite = new Dictionary<KeyValuePair<string, string>, double>();
        Variables = new Dictionary<KeyValuePair<string, string>, double>();


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

    public void Client_findServers()
    {
        if (hLibrary == IntPtr.Zero)
        {
            Debug.Log("DLL not loaded!!! Stop!");
            return;
        }

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
        Client_Thread = new System.Threading.Thread(new System.Threading.ThreadStart(client_Thread_loop));
        Client_Thread.IsBackground = true;
        Client_Thread.Start();
    }

    //поток клиента
    private void client_Thread_loop()
    {
        while (stop == false)
        {
            System.Threading.Thread.Sleep(sleep_value);//15
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
        if (hLibrary == IntPtr.Zero) return;
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
        if (serverConnected == false)
        {
            Debug.Log("Server not created!!! Stop!");
            return;
        }

        Server_mutexObj = new Mutex();
        Server_Thread = new System.Threading.Thread(new System.Threading.ThreadStart(server_Thread_loop));
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


