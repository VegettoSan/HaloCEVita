using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RotateObject : MonoBehaviour
{
    [SerializeField]
    float Speed = 1f;

    [SerializeField]
    float X, Y, Z;
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        float T = Time.deltaTime * Speed;
        transform.Rotate(T * X, T * Y, T * Z, Space.Self);
    }
}
