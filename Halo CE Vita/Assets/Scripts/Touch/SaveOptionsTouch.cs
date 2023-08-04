using System.Collections;
using System.Collections.Generic;
using System;
using UnityEngine;
using UnityEngine.UI;

public class SaveOptionsTouch : MonoBehaviour
{
    [SerializeField]
    Slider LookX, LookY;
    [SerializeField]
    Text TextX, TextY;

    [SerializeField]
    Toggle InvertedX, InvertedY, Vibration;
    void Start()
    {
        LookX.value = PlayerPrefs.GetFloat("LookX", 4f);
        float intX = LookX.value * 10;
        TextX.text = intX.ToString();
        LookY.value = PlayerPrefs.GetFloat("LookY", 4f);
        float intY = LookY.value * 10;
        TextY.text = intY.ToString();

        InvertedX.isOn = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedX", 0));
        InvertedY.isOn = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedY", 0));

        Vibration.isOn = Convert.ToBoolean(PlayerPrefs.GetInt("Vibration", 0));
    }

    public void XLook(float value)
    {
        float X = value * 10;
        TextX.text = X.ToString();
        PlayerPrefs.SetFloat("LookX", LookX.value);
    }
    public void YLook(float value)
    {
        float Y = value * 10;
        TextY.text = Y.ToString();
        PlayerPrefs.SetFloat("LookY", LookY.value);
    }

    public void XInverted(bool enable)
    {
        PlayerPrefs.SetInt("InvertedX", enable ? 1 : 0);
    }
    public void YInverted(bool enable)
    {
        PlayerPrefs.SetInt("InvertedY", enable ? 1 : 0);
    }
    public void VibrationEnable(bool enable)
    {
        PlayerPrefs.SetInt("Vibration", enable ? 1 : 0);
    }

    public void Defalut()
    {
        LookX.value = 4f;
        LookY.value = 4f;
        InvertedX.isOn = false;
        InvertedY.isOn = false;
        Vibration.isOn = false;

        XLook(4f);
        YLook(4f);
        XInverted(false);
        YInverted(false);
        VibrationEnable(false);
    }
}
