using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AiShieldParts : MonoBehaviour
{
    [SerializeField] HealthShield Health;

    public void Damage(int total)
    {
        Health.Damage(total);
    }
}
