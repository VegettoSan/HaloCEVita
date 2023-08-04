using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class EditFOV : MonoBehaviour
{
    [SerializeField]
    Camera Camera;
    [SerializeField]
    Text text;
    [SerializeField]
    Slider BarFov;
    void Start()
    {
        Camera.fieldOfView = PlayerPrefs.GetFloat("FOV", 40f);
        if (BarFov != null && text != null)
        {
            BarFov.value = PlayerPrefs.GetFloat("FOV", 40f);
            text.text = BarFov.value.ToString();
        }
    }

    public void SaveFov(float value)
    {
        value = Mathf.Clamp(value, 40f, 60f);
        PlayerPrefs.SetFloat("FOV", value);
        int i = (int)value; ;
        text.text = i.ToString();

        Camera.fieldOfView = value;
    }

    public void DefaultFov(float value)
    {
        value = Mathf.Clamp(value, 40f, 60f);
        PlayerPrefs.SetFloat("FOV", value);
        int i = (int)value; ;
        text.text = i.ToString();
        BarFov.value = value;

        Camera.fieldOfView = value;
    }
}
