using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Destroy : MonoBehaviour
{
    [SerializeField] float Seconds;
    void Update()
    {
        DestroyObj(Seconds);
    }
    public void DestroyObj(float time)
    {
        Destroy(this.gameObject, time);
    }
}
