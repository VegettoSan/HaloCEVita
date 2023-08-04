using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TouchControlsKit;

public class UiPosition_Size : MonoBehaviour {

	public RectTransform UI;
	public Vector3 MoverY;
	public Vector3 MoverX;
	public Vector3 Tamaño;

	public float PosX, PosY, TamX, TamY;

	public bool InGame;

	SaveUIPosition Save;

	void Start () {

		if (InGame) {

			Destroy(this.GetComponent<UiPosition_Size>());
			Destroy(this.GetComponent<SaveUIPosition>());

		}

		if (UI == null) {

			UI = GetComponent<RectTransform> ();

		}
		
	}

	void Update () {

		if (Input.GetKey (KeyCode.UpArrow)) {

			MoverArriba ();

		}

		if (Input.GetKey (KeyCode.DownArrow)) {

			MoverAbajo ();

		}

		if (Input.GetKey (KeyCode.LeftArrow)) {

			MoverIzquierda ();

		}

		if (Input.GetKey (KeyCode.RightArrow)) {

			MoverDerecha ();

		}

		if (Input.GetKeyDown (KeyCode.W)) {

			Aumentar ();

		}

		if (Input.GetKeyDown (KeyCode.S)) {

			Disminuir ();

		}

		if (Input.GetKeyDown (KeyCode.E)) {

			Default ();

		}
		
	}

	public void FreeMove() {

		this.gameObject.transform.position = new Vector3(Input.mousePosition.x, Input.mousePosition.y);

	}

	public void MoverArriba() {

		UI.localPosition += MoverY;

	}

	public void MoverAbajo() {

		UI.localPosition -= MoverY;

	}

	public void MoverDerecha() {

		UI.localPosition += MoverX;

	}

	public void MoverIzquierda() {

		UI.localPosition -= MoverX;

	}

	public void Aumentar() {

		UI.localScale += Tamaño;

	}

	public void Disminuir() {

		UI.localScale -= Tamaño;

	}

	public void Default() {

		UI.localPosition = new Vector3 (PosX, PosY);
		UI.localScale = new Vector3 (TamX, TamY);

	}
}
