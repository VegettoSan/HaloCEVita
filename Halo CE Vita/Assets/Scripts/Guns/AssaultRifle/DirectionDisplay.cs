using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DirectionDisplay : MonoBehaviour
{
    [SerializeField] Transform Player;
    [SerializeField] private Renderer Rifle;
    [SerializeField] private Texture[] Directions;

    private float D;
    void Start()
    {
        Direction();
    }

    // Update is called once per frame
    void Update()
    {
        Direction();
    }

    void Direction()
    {
        D = Player.localEulerAngles.y;

        if (D >= 0f && D <= 44.9f)
        {
            Rifle.materials[1].SetTexture("_BaseMap", Directions[0]);
        }
        if (D >= 45f && D <= 89.9f)
        {
            Rifle.materials[1].SetTexture("_BaseMap", Directions[1]);
        }
        if (D >= 90 && D <= 134.9f)
        {
            Rifle.materials[1].SetTexture("_BaseMap", Directions[2]);
        }
        if (D >= 135 && D <= 179.9f)
        {
            Rifle.materials[1].SetTexture("_BaseMap", Directions[3]);
        }
        if (D >= 180 && D <= 224.9f)
        {
            Rifle.materials[1].SetTexture("_BaseMap", Directions[4]);
        }
        if (D >= 225 && D <= 269.9f)
        {
            Rifle.materials[1].SetTexture("_BaseMap", Directions[5]);
        }
        if (D >= 270 && D <= 314.9f)
        {
            Rifle.materials[1].SetTexture("_BaseMap", Directions[6]);
        }
        if (D >= 315 && D <= 360)
        {
            Rifle.materials[1].SetTexture("_BaseMap", Directions[7]);
        }
    }
}
