using System.Collections;
using System.Collections.Generic;
using UnityEngine;

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

        NET.ServerAddVariable("AAA", "Men1.x", "Men1.x", 0);
        NET.ServerAddVariable("AAA", "Men1.y", "Men1.y", 0);
        NET.ServerAddVariable("AAA", "Men1.s", "Men1.s", 1);

        NET.ServerCreateMethod(62541, "hello world", "Hello World", "Say `Hello World`");
        NET.ServerCreateMethod(62542, "A", "Name", "C");

        //Подписка
        NET.ServerSubscription("Men1", "x", 100.0);
        NET.ServerSubscription("Men1", "r", 100.0);

        //
        NET.StartServerThread();
        
        
        NET.ServerCallMethod(62541, "Anna");
        NET.ServerCallMethod(62542, "Omega");

        

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


        //!!
        NET.Client_findServers();


        NET.ClientConnect();
        Debug.Log("UA_Client_Service_browse=" + NET.UA_Client_Service_browse());
        //
        NET.ClientCallMethod(62541, "Hello from maximum2000 !");

        //подписка клиента
        {
            NET.ClientSubscriptionAddVariable("AAA", "Men1.x");
            NET.ClientSubscriptionAddVariable("AAA", "Men1.y");
            NET.ClientSubscriptionAddVariable("AAA", "Men1.s");
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

    //test
    public void _testReciveMethod(int nodeId, string message)
    {
        Debug.Log("_testReciveMethod=" + nodeId + " " + message);
    }
    public void _testChangeVariable(string objName, string varName, string value)
    {
        Debug.Log("_testChangeVariable (string) =" + objName + " " + varName + " " + value);
    }
    public void _testChangeVariable(string objName, string varName, double value)
    {
        Debug.Log("_testChangeVariable (double) =" + objName + " " + varName + " " + value);
    }

    //



    public void SelectDrillerHelper1()
    {
        NET.RegisterCallback();
        NET.ClientConnect();
        mode = 1;
   
    }

    public void SelectDrillerHelper2()
    {
        NET.RegisterCallback();
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
        //float r1 = (float)NET.getClientRead("Men1", "r");

        if (mode==0)
        {
            //скрываю 0го, передвигаю первого и второго
            //Men0.SetActive(false);
            NET.setClientWrite("Men1", "x", (double)My.transform.localPosition.x);
            NET.setClientWrite("Men1", "y", (double)My.transform.localPosition.y);
            NET.setClientWrite("Men1", "z", (double)My.transform.localPosition.z);
            //NET.setClientWrite("Men1", "r", (double)My.transform.localEulerAngles.y);

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
