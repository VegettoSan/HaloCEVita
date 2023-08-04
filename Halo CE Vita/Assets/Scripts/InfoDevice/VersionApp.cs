using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class VersionApp : MonoBehaviour
{
    Text Texto;
    public string TextStart = "ver ";
    void Start()
    {
        Texto = GetComponent<Text>();
        Texto.text = TextStart + Application.version.ToString();
    }

    // Update is called once per frame
    void Update()
    {
        
    }
}
