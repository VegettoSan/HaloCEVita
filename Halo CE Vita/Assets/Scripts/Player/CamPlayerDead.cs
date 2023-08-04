using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CamPlayerDead : MonoBehaviour
{
    [SerializeField] bool Active;
    [SerializeField] Transform Target, Camera, Point;
    [SerializeField] LayerMask Layers;
    [SerializeField] Vector3 Offset;
    [SerializeField] float Distance;
    void Start()
    {

    }

    // Update is called once per frame
    void Update()
    {
        if (Active)
        {
            transform.position = Target.position;
            Camera.LookAt(Target);
        }
        RaycastHit Hit;

        if (Physics.Raycast(transform.position, transform.forward, out Hit, Distance, Layers))
        {
            Camera.localPosition = new Vector3(Offset.x, Offset.y, Hit.distance + Offset.z);
        }
        else
        {
            Camera.localPosition = new Vector3(Offset.x, Offset.y, Distance);
        }
    }
}
