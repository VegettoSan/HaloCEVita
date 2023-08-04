using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LimitedFrame : MonoBehaviour
{

    public int FPS;

    private void Awake()
    {
        Application.targetFrameRate = PlayerPrefs.GetInt("fps", 30);
        FPS = PlayerPrefs.GetInt("fps", 30);
    }
    void Start()
    {

    }

    public void fps60()
    {
        Application.targetFrameRate = 60;
        FPS = 60;
        PlayerPrefs.SetInt("fps", FPS);
    }

    public void fps30()
    {
        Application.targetFrameRate = 30;
        FPS = 30;
        PlayerPrefs.SetInt("fps", FPS);
    }

    public void fps25()
    {
        Application.targetFrameRate = 25;
        FPS = 25;
        PlayerPrefs.SetInt("fps", FPS);
    }
}
