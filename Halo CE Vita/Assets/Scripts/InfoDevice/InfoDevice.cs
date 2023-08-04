using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class InfoDevice : MonoBehaviour
{
    public Text Text;
    string Info;

    string NameGamepad;
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        DeviceInfo();
    }

    void DeviceInfo()
    {
        float Battery = SystemInfo.batteryLevel * 100f;

        if (Input.GetJoystickNames().Length > 0)
        {
            if (Input.GetJoystickNames()[0] == "")
            {
                NameGamepad = "No conectado";
            }
            else if (Input.GetJoystickNames()[0] != "")
            {
                NameGamepad = Input.GetJoystickNames()[0];
            }
        }
        else if (Input.GetJoystickNames().Length == 0)
        {

            NameGamepad = "No conectado";

        }

            Info = "BATERIA: " + Battery.ToString() + "%" + "\n\n"
            + "MODELO: " + SystemInfo.deviceModel.ToString() + "\n\n"
            + "NOMBRE: " + SystemInfo.deviceName.ToString() + "\n\n"
            + "TIPO: " + SystemInfo.deviceType.ToString() + "\n\n"
            + "OS: " + SystemInfo.operatingSystem.ToString() + "\n\n"
            + "PROCESADOR: " + SystemInfo.processorType.ToString() + "\n\n"
            + "RAM: " + SystemInfo.systemMemorySize.ToString() + " MB" + "\n\n"
            + "GRAFICA: " + SystemInfo.graphicsDeviceName.ToString() + "\n\n"
            + "MANDO: " + NameGamepad + "\n\n";

        Text.text = Info;
    }
}
