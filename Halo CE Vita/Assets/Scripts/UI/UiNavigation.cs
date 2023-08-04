using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UiNavigation : MonoBehaviour
{
    public GameObject Obj;


    public void PrimerBoton(GameObject FirstObject)
    {
        EventSystem.current.SetSelectedGameObject(null);

        EventSystem.current.SetSelectedGameObject(FirstObject);
    }
}