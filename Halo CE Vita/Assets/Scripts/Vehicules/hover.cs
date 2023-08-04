using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class hover : MonoBehaviour
{
    Rigidbody hb;
    public float mult;
    public float moveForce;
    public float turnTorque;

    public GameObject referenceObject;
    public float maxAngleDifference;

    void Start()
    {
        hb = GetComponent<Rigidbody>();
    }

    public Transform[] anchors = new Transform[4];
    public RaycastHit[] hits = new RaycastHit[4];

    void FixedUpdate()
    {
        for (int i = 0; i < 4; i++)
            ApplyF(anchors[i], hits[i]);

        hb.AddForce(Input.GetAxis("Vertical") * moveForce * transform.forward);
        hb.AddForce(Input.GetAxis("Horizontal") * moveForce * transform.right);
        //hb.AddTorque(Input.GetAxis("Mouse X") * turnTorque * transform.up);
        float angleDifference = -Mathf.DeltaAngle(referenceObject.transform.localEulerAngles.y, transform.localEulerAngles.y);
        float torque = Mathf.Clamp(angleDifference, -maxAngleDifference, maxAngleDifference) * turnTorque;
        hb.AddTorque(torque * transform.up);

    }

    void ApplyF(Transform anchor, RaycastHit hit)
    {
        if (Physics.Raycast(anchor.position, -anchor.up, out hit))
        {
            float force = 0;
            force = Mathf.Abs(1 / (hit.point.y - anchor.position.y));
            hb.AddForceAtPosition(transform.up * force * mult, anchor.position, ForceMode.Acceleration);
        }
    }

}
