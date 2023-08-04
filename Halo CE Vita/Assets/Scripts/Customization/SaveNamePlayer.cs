using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class SaveNamePlayer : MonoBehaviour
{
    string Name;

    [SerializeField]
    InputField Field;

    [SerializeField]
    GameObject MenuCanvas, NameCanvas, FirstOk, Ok, Back;
    void Start()
    {
        Name = PlayerPrefs.GetString("NamePlayer", "");

        if(Name == "")
        {
            Field.text = "";
            MenuCanvas.SetActive(false);
            NameCanvas.SetActive(true);
            FirstOk.SetActive(true);
            Ok.SetActive(false);
            Back.SetActive(false);
        }
        else if (Name != "")
        {
            Field.text = Name;
            MenuCanvas.SetActive(true);
            NameCanvas.SetActive(false);
            FirstOk.SetActive(false);
            Ok.SetActive(true);
            Back.SetActive(true);
        }
    }

    private void FixedUpdate()
    {
        if (Field.text == "")
        {
            if (Name == "")
            {
                FirstOk.SetActive(false);
                Ok.SetActive(false);
                Back.SetActive(false);
            }
            else if (Name != "")
            {
                FirstOk.SetActive(false);
                Ok.SetActive(false);
                Back.SetActive(true);
            }
        }
        else if (Field.text != "")
        {
            if (Name == "")
            {
                FirstOk.SetActive(true);
                Ok.SetActive(false);
                Back.SetActive(false);
            }
            else if (Name != "")
            {
                FirstOk.SetActive(false);
                Ok.SetActive(true);
                Back.SetActive(true);
            }
        }
    }

    public void SaveName()
    {
        Name = Field.text;
        PlayerPrefs.SetString("NamePlayer", Name);
    }
}
