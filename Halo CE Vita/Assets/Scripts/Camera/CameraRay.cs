using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class CameraRay : MonoBehaviour
{
    [SerializeField] LayerMask Layers;
    [SerializeField] float Distance;
    [SerializeField] Image[] Crossairs;
    [SerializeField] Color Normal, Friend, Enemy;
    [SerializeField] string[] EnemyTag, FriendTag; 
    void Start()
    {
        
    }

    void Update()
    {
        RaycastHit Hit;

        Debug.DrawRay(transform.position, transform.forward * Distance, Color.green, 0.1f);
        if (Physics.Raycast(transform.position, transform.forward, out Hit, Distance, Layers))
        {
            if (Hit.collider.tag == "Untagged" || Hit.collider.tag == "Metal" ||
                Hit.collider.tag == "Ground" || Hit.collider.tag == "Water" ||
                Hit.collider.tag == "null" ||
                Hit.collider.tag == null)
            {
                Crossaircolor(Normal);
            }
            if (Hit.collider.tag == "Grunt" || Hit.collider.tag == "Elite" ||
                 Hit.collider.tag == "Jackal" || Hit.collider.tag == "Flood" ||
                 Hit.collider.tag == "SpartanEnemy" || Hit.collider.tag == "SpartanEnemyParts")
            {
                Crossaircolor(Enemy);
            }
            if (Hit.collider.tag == "Marine" || Hit.collider.tag == "Spartan")
            {
                Crossaircolor(Friend);
            }
        }
    }

    void Crossaircolor(Color color)
    {
        foreach(Image Cross in Crossairs)
        {
            Cross.color = color;
        }
    }
}
