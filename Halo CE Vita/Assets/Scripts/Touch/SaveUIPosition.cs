using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SaveUIPosition : MonoBehaviour {

	public Vector3 P;
	public Vector3 T;

	enum Modo { Save , Load }
	[SerializeField]
	Modo Mode;

	public string PosX, PosY, TamX, TamY;

	UiPosition_Size Script;

	void Start () {

		Script = GetComponent<UiPosition_Size> ();

		if (Mode == Modo.Load) {

			Load ();

		}
		
	}

	void Update () {

		P = new Vector3 (PlayerPrefs.GetFloat (PosX,Script.PosX), PlayerPrefs.GetFloat (PosY,Script.PosY));
		T = new Vector3 (PlayerPrefs.GetFloat (TamX,Script.TamX), PlayerPrefs.GetFloat (TamY,Script.TamY));

		if (Mode == Modo.Save) {

			Save ();

		}

		if (Mode == Modo.Load) {

			Load ();

		}
		
	}

	public void Load() {

		if (Script.UI != null) {

			Script.UI.localPosition = new Vector3 (PlayerPrefs.GetFloat (PosX,Script.PosX), PlayerPrefs.GetFloat (PosY,Script.PosY));
			Script.UI.localScale = new Vector3 (PlayerPrefs.GetFloat (TamX,Script.TamX), PlayerPrefs.GetFloat (TamY,Script.TamY));

		}

	}

	public void Save() {

		PlayerPrefs.SetFloat (PosX, Script.UI.localPosition.x);
		PlayerPrefs.SetFloat (PosY, Script.UI.localPosition.y);

		PlayerPrefs.SetFloat (TamX, Script.UI.localScale.x);
		PlayerPrefs.SetFloat (TamY, Script.UI.localScale.y);

	}

	public void CamSave() {

		Mode = Modo.Save;

	}

	public void CamLoad() {

		Mode = Modo.Load;

	}
}
