using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerDamageDirections : MonoBehaviour
{
    [SerializeField] PlayerHealth Health;
    [SerializeField] GameObject Direction;
    [SerializeField] float TimeActive = 3f;
    float Timer;

    void Update()
    {
        if (Direction.activeSelf == true)
        {
            Timer += Time.deltaTime * 1f;
            if (Timer >= TimeActive)
            {
                Direction.SetActive(false);
            }
        }
        else if (Direction.activeSelf == false)
        {
            Timer = 0f;
        }
    }
    public void Damage(float value)
    {
        Timer = 0f;
        Direction.SetActive(true);
        Health.Damage(value);
    }
}
