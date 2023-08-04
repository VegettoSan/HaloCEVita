using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class FP_FastTurn : MonoBehaviour
{
    public float turnSpeed = 5.5F;
    public float turnAngle = 180;
    public Button leftTurn, rightTurn;
    private Transform thisT;
    public static bool turn;
    private Quaternion targetRotation;
	// Use this for initialization
	void Start () 
    {
        thisT = transform;
        leftTurn.onClick.AddListener(LeftTurn);
        rightTurn.onClick.AddListener(RightTurn);
	}
	
	// Update is called once per frame
	void Update ()
    {
        if (Input.GetKeyDown(KeyCode.Q))
            LeftTurn();
        else if (Input.GetKeyDown(KeyCode.E))
            RightTurn();

        if (thisT.rotation != targetRotation)
        {
            if (turn == true)
                thisT.rotation = Quaternion.RotateTowards(thisT.rotation, targetRotation, turnSpeed * 100 * Time.deltaTime);
        }
        else
            turn = false;
	}


    void LeftTurn()
    {
        targetRotation = Quaternion.AngleAxis(turnAngle, transform.up) * thisT.rotation;
        turn = true;
    }

    void RightTurn()
    {
        targetRotation = Quaternion.AngleAxis(-turnAngle, transform.up) * thisT.rotation;
        turn = true;
    }
}