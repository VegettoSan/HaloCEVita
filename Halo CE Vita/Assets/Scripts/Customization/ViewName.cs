using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ViewName : MonoBehaviour
{
    [SerializeField]
    string FirstText;
    [SerializeField]
    Text text;
    void Update()
    {
        text.text = FirstText + PlayerPrefs.GetString("NamePlayer", "No Name");
    }
}
