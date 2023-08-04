using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class EnabledButtons : MonoBehaviour
{
    public GameObject[] Buttons;
    public GameObject ReloadButton;

    public bool Guns_Human;
    public GunsHuman GunsHumans;

    public void ActiveButtons()
    {

        foreach (GameObject B in Buttons)
        {

            B.SetActive(true);

        }
    }

    public void DesactiveButtons()
    {

        foreach (GameObject B in Buttons)
        {

            B.SetActive(false);

        }
    }

    public void ActiveButtonReload()
    {
        ReloadButton.SetActive(true);
    }

    public void DesactiveButtonReload()
    {
        ReloadButton.SetActive(false);
    }

    public void EnabledFireGunshuman()
    {
        GunsHumans.ActiveEnabledFire();
    }
    public void DesnabledFireGunshuman()
    {
        GunsHumans.DesactiveEnabledFire();
    }
}
