using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CustomizeTouch : MonoBehaviour {

	public SaveUIPosition[] Saves;

	void Start () {

	}

	void Update () {
		
	}

	public void SavesLoad() {

		for (int i = 0; i < Saves.Length; i++) {

			Saves [i].CamLoad ();

		}
	}

	public void SavesSave() {

		for (int i = 0; i < Saves.Length; i++) {

			Saves [i].CamSave ();

		}
	}
}
