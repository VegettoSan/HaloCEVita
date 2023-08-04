using System.Collections;
using System.Collections.Generic;
using System;
using UnityEngine;
using UnityEngine.UI;

public class PlayerHealth : MonoBehaviour
{
    [Serializable]
    public class Render
    {
        public Renderer[] Arms;
    }

    [Header("Renderer")]
    [SerializeField] Render Renders;

    [Serializable]
    public class HealthPlayer
    {
        public float Health = 100, MaxHealth = 100;
        public float Shield = 200, MaxShield = 200;
        public float CadenceRecharge = 5f;

        public Animator DamageHealth, DamageShield;

        public float smoothTime,smoothTimeDamage;
        public float velocity, velocityDamage;
    }

    [Header("Health Statistics")]
    [SerializeField]public HealthPlayer HealthStatistics;

    [Serializable]
    public class Sounds
    {
        public AudioSource Source;
        public AudioClip Hit, ReloadShield;
    }

    [Header("Sounds")]
    [SerializeField] Sounds HealthSounds;

    [Serializable]
    public class UIs
    {
        public Slider SliderShield, SliderHealth;
    }

    [Header("UI")]
    [SerializeField] UIs UiView;


    [SerializeField] bool Active;
    float Timer;
    [SerializeField] float StartShield_R = 3f;
    //[SerializeField] TimeActiveShield
    void Start()
    {
        UiView.SliderShield.maxValue = HealthStatistics.MaxShield;
        UiView.SliderHealth.maxValue = HealthStatistics.MaxHealth;
    }

    // Update is called once per frame
    void Update()
    {
        HealthStatistics.Health = Mathf.Clamp(HealthStatistics.Health, 0,
            HealthStatistics.MaxHealth);
        HealthStatistics.Shield = Mathf.Clamp(HealthStatistics.Shield, 0,
            HealthStatistics.MaxShield);

        UiView.SliderShield.value = HealthStatistics.Shield;
        UiView.SliderHealth.value = HealthStatistics.Health;

        if (HealthStatistics.Health <= 0)
        {
            PlayerManager.include.StartCount();
            this.gameObject.SetActive(false);
            return;
        }
        ResetDamageWarnings();

        if (HealthStatistics.Shield <= 0f)
        {
            Active = false;
        }
        if (HealthStatistics.Shield < HealthStatistics.MaxShield)
        {
            RechargeShield();
            HealthSounds.Source.loop = true;
        }
        else if (HealthStatistics.Shield >= HealthStatistics.MaxShield)
        {
            Timer = 0f;
            HealthSounds.Source.loop = false;
        }
        ArmsShield(Active ? 1 : 0);

        //Debug

        if (Input.GetKeyDown(KeyCode.S))
        {
            Damage(10f);
        }

        //
    }

    void ResetDamageWarnings()
    {
        float s = 0f;
        s = Mathf.Clamp(s, 0f, 1f);

        s = Mathf.SmoothDamp(HealthStatistics.DamageShield.GetFloat("Damage"),
                        0, ref HealthStatistics.velocityDamage,
                        HealthStatistics.smoothTimeDamage);
        HealthStatistics.DamageShield.SetFloat("Damage", s);
        float h = 0f;
        h = Mathf.Clamp(h, 0f, 1f);

        h = Mathf.SmoothDamp(HealthStatistics.DamageHealth.GetFloat("Damage"),
                        0, ref HealthStatistics.velocityDamage,
                        HealthStatistics.smoothTimeDamage);
        HealthStatistics.DamageHealth.SetFloat("Damage", h);
    }

    public void Damage(float value)
    {
        Timer = 0f;
        //HealthSounds.Source.Stop();
        HealthSounds.Source.PlayOneShot(HealthSounds.Hit);

        if(HealthStatistics.Shield > 0f)
        {
            HealthStatistics.DamageShield.SetFloat("Damage", 1f);
            HealthStatistics.Shield -= value;
        }
        else if (HealthStatistics.Shield <= 0f)
        {
            if(HealthStatistics.Health > 0f)
            {
                HealthStatistics.DamageHealth.SetFloat("Damage", 1f);
                HealthStatistics.Health -= value;
                Active = false;
            }
        }
    }

    void RechargeShield()
    {
        if (HealthStatistics.Shield > 0f)
        {
            Active = true;
        }
        Timer += Time.deltaTime * 1f;

        if(Timer >= StartShield_R)
        {
            Active = false;
            HealthStatistics.Shield += Time.deltaTime * HealthStatistics.CadenceRecharge;
            if (Timer <= StartShield_R + 0.1f)
            {
                HealthSounds.Source.clip = HealthSounds.ReloadShield;
                HealthSounds.Source.Play();
                HealthSounds.Source.loop = true;
            }
        }
    }

    void ArmsShield(float value)
    {
        value = Mathf.Clamp(value, 0f, 1f);

        foreach (Renderer R in Renders.Arms)
        {
            //R.material.SetFloat("Active_Shield", value);
            
            switch (Active)
            {
                case true:

                    R.material.SetFloat("Active_Shield", Active ? 1 : 0);

                    break;

                case false:

                    value = Mathf.SmoothDamp(R.material.GetFloat("Active_Shield"),
                        Active ? 1 : 0, ref HealthStatistics.velocity,
                        HealthStatistics.smoothTime);

                    R.material.SetFloat("Active_Shield", value);

                    break;
            }
        }
    }
}
