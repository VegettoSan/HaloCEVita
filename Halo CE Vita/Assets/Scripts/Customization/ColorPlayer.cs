using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ColorPlayer : MonoBehaviour
{
    [Header("Spartan")]
    public Transform View;
    public Animator Anim;

    [Header("MATERIAL")]
    public Material Render;
    public Material RenderPlayer;

    [Header("COLORS")]
    public Color[] Colors;

    [Header("CUBEMAPS")]
    public Cubemap[] CubeMaps;

    [SerializeField]
    int NColor, NCubemap;

    private void Start()
    {
        NColor = PlayerPrefs.GetInt("Color", 0);
        NCubemap = PlayerPrefs.GetInt("CubemapVisor", 10);

        Render.SetColor("_ColorArmored", Colors[NColor]);
        RenderPlayer.SetColor("_ColorArmored", Colors[NColor]);
        Render.SetTexture("Cubemap_Visor", CubeMaps[NCubemap]);
    }

    void Update()
    {
        
    }

    #region Colors

    public void Color_1()
    {
        NColor = 0;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[0]);
        RenderPlayer.SetColor("_ColorArmored", Colors[0]);
    }
    public void Color_2()
    {
        NColor = 1;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[1]);
        RenderPlayer.SetColor("_ColorArmored", Colors[1]);
    }
    public void Color_3()
    {
        NColor = 2;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[2]);
        RenderPlayer.SetColor("_ColorArmored", Colors[2]);
    }
    public void Color_4()
    {
        NColor = 3;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[3]);
        RenderPlayer.SetColor("_ColorArmored", Colors[3]);
    }
    public void Color_5()
    {
        NColor = 4;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[4]);
        RenderPlayer.SetColor("_ColorArmored", Colors[4]);
    }
    public void Color_6()
    {
        NColor = 5;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[5]);
        RenderPlayer.SetColor("_ColorArmored", Colors[5]);
    }
    public void Color_7()
    {
        NColor = 6;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[6]);
        RenderPlayer.SetColor("_ColorArmored", Colors[6]);
    }
    public void Color_8()
    {
        NColor = 7;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[7]);
        RenderPlayer.SetColor("_ColorArmored", Colors[7]);
    }
    public void Color_9()
    {
        NColor = 8;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[8]);
        RenderPlayer.SetColor("_ColorArmored", Colors[8]);
    }
    public void Color_10()
    {
        NColor = 9;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[9]);
        RenderPlayer.SetColor("_ColorArmored", Colors[9]);
    }
    public void Color_11()
    {
        NColor = 10;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[10]);
        RenderPlayer.SetColor("_ColorArmored", Colors[10]);
    }
    public void Color_12()
    {
        NColor = 11;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[11]);
        RenderPlayer.SetColor("_ColorArmored", Colors[11]);
    }
    public void Color_13()
    {
        NColor = 12;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[12]);
        RenderPlayer.SetColor("_ColorArmored", Colors[12]);
    }
    public void Color_14()
    {
        NColor = 13;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[13]);
        RenderPlayer.SetColor("_ColorArmored", Colors[13]);
    }
    public void Color_15()
    {
        NColor = 14;
        PlayerPrefs.SetInt("Color", NColor);
        Render.SetColor("_ColorArmored", Colors[14]);
        RenderPlayer.SetColor("_ColorArmored", Colors[14]);
    }

    #endregion

    #region Cubemaps

    public void Cubemap_1()
    {
        NCubemap = 0;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[0]);
    }
    public void Cubemap_2()
    {
        NCubemap = 1;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[1]);
    }
    public void Cubemap_3()
    {
        NCubemap = 2;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[2]);
    }
    public void Cubemap_4()
    {
        NCubemap = 3;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[3]);
    }
    public void Cubemap_5()
    {
        NCubemap = 4;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[4]);
    }
    public void Cubemap_6()
    {
        NCubemap = 5;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[5]);
    }
    public void Cubemap_7()
    {
        NCubemap = 6;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[6]);
    }
    public void Cubemap_8()
    {
        NCubemap = 7;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[7]);
    }
    public void Cubemap_9()
    {
        NCubemap = 8;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[8]);
    }
    public void Cubemap_10()
    {
        NCubemap = 9;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[9]);
    }
    public void Cubemap_11()
    {
        NCubemap = 10;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        Render.SetTexture("Cubemap_Visor", CubeMaps[10]);
    }

    #endregion

    public void RandomColor()
    {
        int i = Random.Range(0, Colors.Length);
        Render.SetColor("_ColorArmored", Colors[i]);
        RenderPlayer.SetColor("_ColorArmored", Colors[i]);
        int c = Random.Range(0, CubeMaps.Length);
        Render.SetTexture("Cubemap_Visor", CubeMaps[c]);

        NCubemap = c;
        PlayerPrefs.SetInt("CubemapVisor", NCubemap);
        NColor = i;
        PlayerPrefs.SetInt("Color", NColor);
    }

    public void RotateView(float value)
    {
        View.rotation = Quaternion.Euler(0f, -value * 180, 0f);
    }

    public void OnZoom(bool enable)
    {
        Anim.SetBool("Zoom", enable);
    }
}
