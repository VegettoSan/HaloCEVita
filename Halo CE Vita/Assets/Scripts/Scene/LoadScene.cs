using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;

public class LoadScene : MonoBehaviour {

	Button ButtonLoad;  // Button que vas ha usar para cambiar la Scene
	//public string Scene; // Name of the Scene
	public BarraDeCargaEscena ScriptBarra;

	public void LoadSceneFuntion (string Scene) {

		ScriptBarra.Scene = Scene;


	}

	public void Salir () {

		Application.Quit ();


	}

}
