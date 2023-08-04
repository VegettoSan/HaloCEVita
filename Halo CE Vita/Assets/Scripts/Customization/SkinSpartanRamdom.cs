using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TouchControlsKit;

public class SkinSpartanRamdom : MonoBehaviour
{
    [Header("RENDER")]
    public Renderer[] Renders;

    [Header("COLORS")]
    public Color[] Colors;

    [Header("CUBEMAPS")]
    public Cubemap[] CubeMaps; 

    void Start()
    {
        RamdomSkin();
    }

    public void RamdomSkin()
    {
        RandomColor();
        RandomCubemap();
    }

    void RandomColor()
    {
        int i = Random.Range(0, Colors.Length);
        foreach(Renderer M in Renders)
        {
            M.material.SetColor("_ColorArmored", Colors[i]);
        }
    }
    void RandomCubemap()
    {
        int i = Random.Range(0, CubeMaps.Length);
        foreach (Renderer M in Renders)
        {
            M.material.SetTexture("Cubemap_Visor", CubeMaps[i]);
        }
    }
}
