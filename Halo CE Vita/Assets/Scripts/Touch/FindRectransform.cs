using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class FindRectransform : MonoBehaviour {

	RectTransform Ui;

	public float PosX, PosY, TamX, TamY;

	void Start () {

		Ui = GetComponent<RectTransform> ();
		
	}

	void Update () {

		PosX = Ui.localPosition.x;
		PosY = Ui.localPosition.y;
		TamX = Ui.localScale.x;
		TamY = Ui.localScale.y;
		
	}
}
