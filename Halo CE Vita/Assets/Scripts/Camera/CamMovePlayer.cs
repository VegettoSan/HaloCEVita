using System.Collections;
using System.Collections.Generic;
using System;
using UnityEngine;
using TouchControlsKit;

public class CamMovePlayer : MonoBehaviour
{
    FP_Input Controller;
    Animator Anim;
    float H, V;
    bool InvertedMX, InvertedMY, InvertedX, InvertedY;
    void Start()
    {
        Anim = GetComponent<Animator>();
        Controller = GetComponentInParent<FP_Input>();
    }

    void Update()
    {
        InvertedMX = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedXGamepad", 0));
        InvertedMY = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedYGamepad", 0));
        InvertedX = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedX", 0));
        InvertedY = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedY", 0));

        switch (Controller.UseMobileInput)
        {
            case true:

                switch (InvertedX)
                {
                    case true:

                        H = -InputManager.GetAxis("Touch", "DerX");

                        break;

                    case false:

                        H = InputManager.GetAxis("Touch", "DerX");

                        break;
                }

                switch (InvertedY)
                {
                    case true:

                        V = -InputManager.GetAxis("Touch", "DerY");

                        break;

                    case false:

                        V = InputManager.GetAxis("Touch", "DerY");

                        break;
                }

                break;

            case false:
                
                if(H != 0 || V != 0)
                {
                    MoveOn();
                }
                else if (H == 0 || V == 0)
                {
                    MoveOff();
                }

                switch (InvertedMX)
                {
                    case true:

                        H = -Input.GetAxis(PlayerPrefs.GetString("VistaX"));

                        break;

                    case false:

                        H = Input.GetAxis(PlayerPrefs.GetString("VistaX"));

                        break;
                }

                switch (InvertedMY)
                {
                    case true:

                        V = -Input.GetAxis(PlayerPrefs.GetString("VistaY"));

                        break;

                    case false:

                        V = Input.GetAxis(PlayerPrefs.GetString("VistaY"));

                        break;
                }

                break;
        }

        Anim.SetFloat("Horizontal", H);
        Anim.SetFloat("Vertical", V);
    }

    public void MoveOn()
    {
        Anim.SetBool("Move", true);
        //Anim.SetFloat("Horizontal", H);
        //Anim.SetFloat("Vertical", V);
    }

    public void MoveOff()
    {
        Anim.SetBool("Move", false);
        //Anim.SetFloat("Horizontal", 0);
        //Anim.SetFloat("Vertical", 0);
    }
}
