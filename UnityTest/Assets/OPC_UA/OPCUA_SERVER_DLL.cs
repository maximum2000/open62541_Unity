﻿using System.Collections;
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
    public static extern int testClient();


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

    
    //-----------------------------------------------------------------------------------------------

    public void RegisterCallback()
    {
        //регистрация всех callbeck функций (привязка)
        {
            RegisterDebugCallback(DebugLog);
           
        }


        
    }

   

    public void startServer()
    {
        //StringBuilder str2 = new StringBuilder(FedarationName.text, 100);
        //string filename = Application.streamingAssetsPath + "/fdd_test.xml";
        //StringBuilder str3 = new StringBuilder(filename, 100);
        //CreateFederationExecution(str2, str2.Capacity, str3);
        //string myString2 = str2.ToString();
        //Debug.Log("c# debug:" + "CreateFederationExecution: " + myString2);

        int a = 1;
        int b = 1;
        testServerCreate(a, b);


    }

    public void  testExampleClientFunction()
    {
        Debug.Log("testClient=" + testClient().ToString());
    }



    void OnDestroy()
    {
        RegisterDebugCallback(null);
        
    }





    void Start()
    {
    }

        // Start is called before the first frame update
    public void StartThread()
    {
        allEvent = new List<int>();

        mutexObj = new Mutex();
        My_Thread = new Thread(new ThreadStart(Thread_loop));
        My_Thread.IsBackground = true;
        My_Thread.Start();

        // mutexObj.Dispose();
        // My_Thread.Abort();

    }

    // Update is called once per frame
    void Update()
    {
        if (mutexObj == null) return;

        mutexObj.WaitOne();
        ReadIntTest.text = zzz.ToString();
        if (writeToggle.isOn == false)
        {
            slider.value = zzz;
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

    int zzz = 0;
    private void Thread_loop()
    {
        while (stop==false)
        {
            //evokeCallback(0.1);
            //getObjectClassHandle();



            if (writeToggle.isOn == true)
            {
                int a = (int)slider.value;
                testServerWrite(a);
            }

            testServerUpdate();


            mutexObj.WaitOne();
            zzz = testServerRead();
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


