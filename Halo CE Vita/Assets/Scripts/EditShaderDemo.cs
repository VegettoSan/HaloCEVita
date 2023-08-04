using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class EditShaderDemo : MonoBehaviour {

	public Renderer render;

	public Color[] Colors;
	public Cubemap[] Cubemaps;
	void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
        if (Input.GetKeyDown(KeyCode.Joystick1Button0) || Input.GetKeyDown(KeyCode.X))
        {
			RandomSkin();
		}
	}

	public void RandomSkin()
    {
		int c = Random.Range(0, Colors.Length);
        int i = Random.Range(0, Cubemaps.Length);

		render.material.SetColor("_ColorArmadura", Colors[c]);
		render.material.SetTexture("_CubemapVisor", Cubemaps[i]);
	}
}
