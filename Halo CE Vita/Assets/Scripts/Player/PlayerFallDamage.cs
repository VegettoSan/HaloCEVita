using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerFallDamage : MonoBehaviour
{
    CharacterController Controller;
    PlayerHealth Health;
    [SerializeField] bool Grounded;
    [SerializeField] float Timer;

    void Start()
    {
        Controller = GetComponent<CharacterController>();
        Health = GetComponent<PlayerHealth>();
    }

    
    void Update()
    {
        Grounded = Controller.isGrounded;

        if (Grounded)
        {
            if (Timer >= 1.3f && Timer < 1.5f)
            {
                Health.Damage(50f);

                Timer = 0f;
            }
            if (Timer >= 1.5f && Timer < 1.7f)
            {
                Health.Damage(75f);

                Timer = 0f;
            }
            if (Timer >= 1.7f && Timer < 2f)
            {
                Health.Damage(150f);

                Timer = 0f;
            }
            if (Timer >= 2f && Timer < 2.3f)
            {
                Health.Damage(200f);

                Timer = 0f;
            }
            if (Timer >= 2.3f && Timer < 2.5f)
            {
                Health.HealthStatistics.Shield = 0f;
                Health.HealthStatistics.Health -= 25f;

                Health.Damage(0f);

                Timer = 0f;
            }
            if (Timer >= 2.5f && Timer < 2.7f)
            {
                Health.HealthStatistics.Shield = 0f;
                Health.HealthStatistics.Health -= 75f;

                Health.Damage(0f);

                Timer = 0f;
            }
            if (Timer >= 2.7f && Timer < 3f)
            {
                Health.HealthStatistics.Shield = 0f;
                Health.HealthStatistics.Health = 0f;

                Health.Damage(0f);

                Timer = 0f;
            }

            Timer = 0f;
        }
        else if (!Grounded)
        {
            Timer += Time.deltaTime * 1f;
        }
    }
}
