using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DemoShield : MonoBehaviour
{
    public Renderer Model;

    public bool Active;

    [Range(0f, 1f)]
    public float ActiveShield = 0f;

    float Timer = 0f;
    public float TimeMax = 3f;
    void Start()
    {
        
    }

    
    void Update()
    {
        ActiveShield = Mathf.Clamp(ActiveShield, 0f, 1f);

        Model.material.SetFloat("Active_Shield", ActiveShield);

        switch (Active)
        {
            case true:

                ActiveShield = 1f;
                Timer += Time.deltaTime * 1f;

                if (Timer >= TimeMax)
                {
                    Active = false;
                }

                break;

            case false:

                ActiveShield = 0f;
                Timer = 0f;

                break;
        }
    }

    public void Damage()
    {
        switch (Active)
        {
            case true:

                Timer = 0f;

                break;

            case false:

                Active = true;

                break;
        }
    }
}
