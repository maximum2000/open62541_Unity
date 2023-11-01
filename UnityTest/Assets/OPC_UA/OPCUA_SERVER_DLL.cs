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


[System.Serializable]
public class MyInteractionEvent : UnityEvent<int> { }


public class OPCUA_SERVER_DLL : MonoBehaviour
{

    //функции общие
    // RegisterDebugCallback - регистрация callback'ов
    //1. SendLog - дебаг-вывод в unity

    //callback для Debug'а
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

    public void RegisterCallback()
    {
        //регистрация всех callbeck функций (привязка)
        {
            RegisterDebugCallback(DebugLog);
        }
    }

    //функции сервера:
    //1. int OPC_ServerCreate ()
    //2. int OPC_ServerUpdate ()
    //3. int OPC_ServerAddVariableDouble (description, displayName) // (char*)"the.answer", (char*)"the answer"
    //4. int OPC_ServerWriteValueDouble (description, value) //(char*)"the.answer", double
    //5. double OPC_ServerReadValueDouble (description) //(char*)"the.answer"
    //6. int OPC_ServerShutdown - выключение сервера

    //1. int OPCserverCreate () - создание сервера OPC
    [DllImport("DLL1")]
    public static extern int OPC_ServerCreate();

    //2. int OPC_ServerUpdate () - обновление сервера
    [DllImport("DLL1")]
    public static extern int OPC_ServerUpdate();

    //3. int OPC_ServerAddVariableDouble (description, displayName) 
    [DllImport("DLL1")]
    public static extern int OPC_ServerAddVariableDouble(StringBuilder descriptionString, StringBuilder displayNameString);

    //4. int OPC_ServerWriteValueDouble (description, value) 
    [DllImport("DLL1")]
    public static extern int OPC_ServerWriteValueDouble(StringBuilder descriptionString, double value);

    //5. double OPC_ServerReadValueDouble (description) 
    [DllImport("DLL1")]
    public static extern double OPC_ServerReadValueDouble(StringBuilder descriptionString);

    //6. int OPC_ServerShutdown
    [DllImport("DLL1")]
    public static extern int OPC_ServerShutdown();

    //функции клиента:
    //1. int OPC_ClientConnect (url) // "opc.tcp://localhost:4840"
    //2. int OPC_ClientWriteValueDouble (description, value) //(char*)"the.answer", double
    //3. double OPC_ClientReadValueDouble (description) //(char*)"the.answer"
    //4. int OPC_ClientUpdate () - обновление клиента
    //5. int OPC_ClientDelete() - выключение клиента

    //1. int OPC_ClientConnect (url) // "opc.tcp://localhost:4840"
    [DllImport("DLL1")]
    public static extern int OPC_ClientConnect(StringBuilder url);

    //2. int OPC_ClientWriteValueDouble (description, value) 
    [DllImport("DLL1")]
    public static extern int OPC_ClientWriteValueDouble(StringBuilder description, double value);

    //3. double OPC_ClientReadValueDouble (description) 
    [DllImport("DLL1")]
    public static extern double OPC_ClientReadValueDouble(StringBuilder description);

    //4. int OPC_ClientUpdate () - обновление клиента
    [DllImport("DLL1")]
    public static extern int OPC_ClientUpdate();

    //5. int OPC_ClientDelete() - выключение клиента
    [DllImport("DLL1")]
    public static extern int OPC_ClientDelete();


    ////////////////////////////////////////////////






    public Slider slider;

    /// Background thread for TcpServer workload.  
    private Thread My_Thread;
    private Mutex mutexObj;
    bool stop = false;


    public MyInteractionEvent ReceiveInteractionEvent;
    public static List<int> allEvent;
    //public  Action<int> HealthChangedAction;


    //имя класса для создаваемого объекта
    public TMP_InputField IP;
    public TMP_InputField ReadIntTest;
    public Toggle writeToggle;



    //-------------------------тестовое-----------------------------
    //
    [DllImport("DLL1")]
    public static extern int testServerCreate(int a, int b);
    //public static extern int GetObjectClassHandle(StringBuilder className, int length);

    [DllImport("DLL1")]
    public static extern int testServerUpdate();

    [DllImport("DLL1")]
    public static extern int testServerRead();

    [DllImport("DLL1")]
    public static extern int testServerWrite(int a);

    [DllImport("DLL1")]
    public static extern double testClient(StringBuilder description);
    //-------------------------тестовое-----------------------------


    void OnDestroy()
    {
        RegisterDebugCallback(null);
        // My_Thread.Abort();
    }

    void Start()
    {

    }



    public void testExampleClientFunction()
    {
        StringBuilder varzname = new StringBuilder("AAA", 100);

        Debug.Log("testClient=" + testClient(varzname).ToString());

        //1. int OPC_ClientConnect (url) // "opc.tcp://localhost:4840"
        StringBuilder addr = new StringBuilder("opc.tcp://localhost:4840", 100);
        int ret = OPC_ClientConnect(addr);
        Debug.Log("OPC_ClientConnect=" + ret.ToString());

         //4. int OPC_ClientUpdate () - обновление клиента
         OPC_ClientUpdate();

        //3. double OPC_ClientReadValueDouble (description) //(char*)"the.answer"
        StringBuilder varname = new StringBuilder("AAA", 100);
        Debug.Log("OPC_ClientReadValueDouble=" + OPC_ClientReadValueDouble(varname).ToString());

         OPC_ClientUpdate();

        //2. int  (description, value) //(char*)"the.answer", double
        StringBuilder var2name = new StringBuilder("AAA", 100);
        int ret3 = OPC_ClientWriteValueDouble(var2name, 321);
        Debug.Log("OPC_ClientWriteValueDouble=" + ret3.ToString());

        OPC_ClientUpdate();


        //Debug.Log("OPC_ClientUpdate=" + ret4.ToString());
        
        //5. int OPC_ClientDelete() - выключение клиента
        int ret5 = OPC_ClientDelete();
        Debug.Log("OPC_ClientDelete=" + ret5.ToString());
    }





    public void startServer()
    {
        //StringBuilder str2 = new StringBuilder(FedarationName.text, 100);
        //string filename = Application.streamingAssetsPath + "/fdd_test.xml";
        //StringBuilder str3 = new StringBuilder(filename, 100);
        //CreateFederationExecution(str2, str2.Capacity, str3);
        //string myString2 = str2.ToString();
        //Debug.Log("c# debug:" + "CreateFederationExecution: " + myString2);

        //int a = 1;
        //int b = 1;
        //testServerCreate(a, b);

        OPC_ServerCreate();

        StringBuilder var1name = new StringBuilder("AAA", 100);
        StringBuilder var1discription = new StringBuilder("AAAdiscription", 100);
        OPC_ServerAddVariableDouble(var1name, var1discription);

        StringBuilder var2name = new StringBuilder("b123", 100);
        StringBuilder var2discription = new StringBuilder("b123discription", 100);
        OPC_ServerAddVariableDouble(var2name, var2discription);
    }

   



    

        // Start is called before the first frame update
    public void StartThread()
    {
        allEvent = new List<int>();
        //
        mutexObj = new Mutex();
        My_Thread = new Thread(new ThreadStart(Thread_loop));
        My_Thread.IsBackground = true;
        My_Thread.Start();
    }

    // Update is called once per frame
    void Update()
    {
        if (mutexObj == null) return;

        mutexObj.WaitOne();
        ReadIntTest.text = zzz.ToString();
        if (writeToggle.isOn == false)
        {
            slider.value = (float)zzz;
        }
        mutexObj.ReleaseMutex();

        return;

        GetReadedBytes();

        //List<bool> allEvent
        for (int i=0; i < allEvent.Count; i++)
        {
            int z = allEvent[i];
            if (ReceiveInteractionEvent != null)
            {
                ReceiveInteractionEvent.Invoke(z);
            }
        }
            
    }

    double zzz = 0;
    private void Thread_loop()
    {
        while (stop==false)
        {
            //evokeCallback(0.1);

            if (writeToggle.isOn == true)
            {
                double value = (double)slider.value;
                //testServerWrite(a);
                StringBuilder var1name = new StringBuilder("AAA", 100);
                OPC_ServerWriteValueDouble(var1name, value);
            }

            //testServerUpdate();
            OPC_ServerUpdate();

            mutexObj.WaitOne();
            //zzz = testServerRead();
            StringBuilder var2name = new StringBuilder("AAA", 100);
            zzz = OPC_ServerReadValueDouble(var2name);
            mutexObj.ReleaseMutex();
        }
    }

    public List<int> GetReadedBytes()
    {
        if (stop == true)
        {
            return new List<int>();
        }

        mutexObj.WaitOne();
        List<int> ReadedBytesDeepCopy = new List<int>();
        foreach (byte b in allEvent)
        {
            ReadedBytesDeepCopy.Add(b);
        }
        allEvent.Clear();
        mutexObj.ReleaseMutex();
        return ReadedBytesDeepCopy;
    }



  

}


