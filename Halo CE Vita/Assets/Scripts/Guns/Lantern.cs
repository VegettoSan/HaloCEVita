using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TouchControlsKit;

public class Lantern : MonoBehaviour
{
    public bool ActiveLantern;

    [SerializeField]
    Light _lantern;

    [SerializeField]
    Slider BarLantern;

    [SerializeField]
    float CadenceOff = 3f, CadenceOn = 3f;

    [SerializeField]
    float ValueLantern = 100f, ValueMax = 100f;


    void Start()
    {
        ActiveLantern = false;
    }

    
    void Update()
    {
        ValueLantern = Mathf.Clamp(ValueLantern, 0f, ValueMax);
        BarLantern.maxValue = ValueMax;
        BarLantern.value = ValueLantern;

        if (InputManager.GetButtonDown("Lantern"))
        {
            EnableLantern();
        }

        switch (ActiveLantern)
        {
            case true:
                if(ValueLantern <= 0f)
                {
                    ActiveLantern = false;
                }
                else if(ValueLantern > 0f)
                {
                    BarLantern.gameObject.SetActive(true);
                    _lantern.enabled = true;
                    ValueLantern -= Time.deltaTime * CadenceOff;
                }

                break;

            case false:

                BarLantern.gameObject.SetActive(false);
                _lantern.enabled = false;
                if(ValueLantern < ValueMax)
                {
                    ValueLantern += Time.deltaTime * CadenceOn;
                }

                break;
        }
    }

    public void EnableLantern()
    {
        ActiveLantern = !ActiveLantern;
    }
}
