using System.Collections;
using System.Collections.Generic;
using System;
using UnityEngine;
using UnityEngine.UI;
using TouchControlsKit;
using TMPro;

public class GunsHuman : MonoBehaviour
{
    public bool Enabled_Fire = true;

    public Animator Anim;

    public LayerMask Layers;

    public Transform Point;
    public Transform Origen;
    public Transform[] OrigenesBalas;

    public Sprite[] BulletsSprites;
    public Image ImageBulletCounter;

    public bool AutomaticFire;
    public int Balas, MaxBalas, Cargador, MaxCargador, Damage = 10;
    public Text CargadorText;
    public float Distancia = 500f, Force = 50f;

    public float Cadencia = 10f;
    float SiguienteDisparo;

    bool Disparo;

    public AudioSource Source;
    public bool SoundFireEnabled;
    public AudioClip SoundFire, SoundNoAmmo;

    public Animator ReloadWarning, NoAmmoWarning;

    //GamePad//

    public FP_Input Inputt;
    
    void Start()
    {
        Anim.SetBool("Idle", true);
        Inputt = GetComponentInParent<FP_Input>();
    }

    
    void Update()
    {
        Balas = Mathf.Clamp(Balas, 0, MaxBalas);
        Cargador = Mathf.Clamp(Cargador, 0, MaxCargador);
        CargadorText.text = Cargador.ToString();

        CounterBullets();

        if (Balas == 0 && Cargador == 0)
        {
            Anim.SetBool("Idle", true);
            Anim.SetBool("Fire", false);
            Anim.SetBool("Posing", false);
            return;
        }
        if (Cargador > 0)
        {
            if (Balas <= MaxBalas / 4)
            {
                ReloadWarning.SetBool("Enabled", true);
            }
            else if (Balas > MaxBalas / 3)
            {
                ReloadWarning.SetBool("Enabled", false);
            }
        }
        else if (Cargador <= 0)
        {
            ReloadWarning.SetBool("Enabled", false);
        }

            switch (Inputt.UseMobileInput)
        {
            case true:

                if(InputManager.GetButton("Fire"))
                {
                    if (AutomaticFire)
                    {
                        Shooter();
                    }
                }
                else
                {
                    Stop();
                }

                if (InputManager.GetButtonDown("Fire"))
                {
                    if(Balas <= 0)
                    {
                        Source.PlayOneShot(SoundNoAmmo);
                    }

                    if(Balas <= 0 && Cargador <= 0)
                    {
                        NoAmmoWarning.SetTrigger("Enabled");
                    }
                    if (!AutomaticFire)
                    {
                        Shooter();
                    }
                }

                if (InputManager.GetButtonDown("Reload"))
                {
                    Reload();
                }
                    break;

            case false:

                break;
        }
    }
    public void Fire()
    {
        
        Balas--;
        OrigenBala();
        RaycastHit Hit;
        if (SoundFireEnabled)
        {
            Source.PlayOneShot(SoundFire);
        }
        Anim.SetBool("Idle", false);
        Anim.SetBool("Fire", true);
        Anim.SetBool("Posing", false);

        if (Physics.Raycast(Origen.position, Origen.forward, out Hit, Distancia, Layers))
        {
            Point.position = Hit.point;
            TargetHits(Hit);
        }
    }

    public void Shooter()
    {
        if (Enabled_Fire)
        {
            switch (AutomaticFire)
            {
                case true:

                    Disparo = true;

                    if (Balas > 0)
                    {
                        if (Time.time >= SiguienteDisparo)
                        {
                            SiguienteDisparo = Time.time + 1 / Cadencia;
                            if (Disparo)
                            {
                                Fire();
                            }
                        }
                    }
                    else if (Balas == 0)
                    {
                        Stop();
                    }

                    break;

                case false:

                    if (Balas > 0)
                    {
                        Fire();
                    }
                    else if (Balas == 0)
                    {
                        Stop();
                    }

                    break;
            }
        }
    }
    public void Stop()
    {
        Disparo = false;
        SiguienteDisparo = 0f;
        Anim.SetBool("Idle", true);
        Anim.SetBool("Fire", false);
        Anim.SetBool("Posing", false);
    }

    public void Reload()
    {
        int Rest = MaxBalas - Balas;

        if (Balas < MaxBalas && Cargador > 0)
        {
            Anim.SetTrigger("Reload");
            Anim.SetBool("Idle", false);
            Anim.SetBool("Fire", false);
            Anim.SetBool("Posing", false);

            if (Cargador <= MaxCargador && Cargador > 0)
            {
                if (Cargador >= MaxBalas)
                {
                    if (Balas < MaxBalas)
                    {
                        Balas += Rest;
                        Cargador -= Rest;
                    }
                    else if (Balas == 0)
                    {
                        Balas += MaxBalas;
                        Cargador -= MaxBalas;
                    }
                }
                if (Cargador < MaxBalas)
                {
                    if (Rest < Cargador)
                    {
                        Balas += Rest;
                        Cargador -= Rest;
                    }
                    else if (Rest >= Cargador)
                    {
                        Balas += Cargador;

                        Cargador = 0;
                    }

                    if (Balas < MaxBalas)
                    {
                       if (Balas == 0)
                        {
                            Balas += Cargador;
                            Cargador = 0;
                        }
                    }
                }

                if (Cargador == Balas)
                {
                    if(Rest < Cargador)
                    {
                        Balas += Rest;
                        Cargador -= Rest;
                    }
                    else if (Rest >= Cargador)
                    {
                        Balas += Cargador;

                        Cargador = 0;
                    }
                }
            }
        }
    }

    void CounterBullets()
    {
        int i = Balas;
        ImageBulletCounter.sprite = BulletsSprites[i];
    }

    void OrigenBala()
    {

        int OringBald = UnityEngine.Random.Range(0, OrigenesBalas.Length);

        Origen = OrigenesBalas[OringBald];

    }

    void TargetHits(RaycastHit hit)
    {
        if (hit.rigidbody != null)
        {
            hit.rigidbody.AddForce(-hit.normal * Force);
        }
        if (hit.collider.tag == "Ground")
        {
            //Instantiate(Terreno, Hit.point, Quaternion.FromToRotation(Vector3.up, Hit.normal));
            //Instantiate(MarcaDeBala, Hit.point, Quaternion.FromToRotation(Vector3.up, Hit.normal));
        }
        if (hit.collider.tag == "DemoShield")
        {
            DemoShield Demo = hit.transform.GetComponent<DemoShield>();

            if (Demo != null)
            {
                Demo.Damage();
            }
        }
        if (hit.collider.tag == "SpartanEnemy" || hit.collider.tag == "Spartan" ||
            hit.collider.tag == "SpartanEnemyParts" || hit.collider.tag == "SpartanParts")
        {
            AiShieldParts Health = hit.transform.GetComponent<AiShieldParts>();

            if (Health != null)
            {
                Health.Damage(Damage);
            }
        }
    }

    public void ActiveEnabledFire()
    {
        Enabled_Fire = true;
    }

    public void DesactiveEnabledFire()
    {
        Enabled_Fire = false;
    }

    public void ShooterGamepad(string TriggerName)
    {
        if (Input.GetButton(TriggerName))
        {
            if (AutomaticFire)
            {
                Shooter();
            }
        }
        else
        {
            Stop();
        }

        if (Input.GetButtonDown(TriggerName))
        {
            if (Balas <= 0)
            {
                Source.PlayOneShot(SoundNoAmmo);
            }

            if (Balas <= 0 && Cargador <= 0)
            {
                NoAmmoWarning.SetTrigger("Enabled");
            }
            if (!AutomaticFire)
            {
                Shooter();
            }
        }
    }

    /*public void ShooterGamepadAxis(string TriggerName)
    {
        if (MyInput.GetAxisAsButtonNegative(TriggerName) || MyInput.GetAxisAsButtonPositive(TriggerName))
        {
            if (AutomaticFire)
            {
                MyInput.repeat = true;
                MyInput.wait = 0f;
                Shooter();
            }
            else if (!AutomaticFire)
            {
                MyInput.repeat = false;
                if (Balas <= 0)
                {
                    Source.PlayOneShot(SoundNoAmmo);
                }

                if (Balas <= 0 && Cargador <= 0)
                {
                    NoAmmoWarning.SetTrigger("Enabled");
                }

                Shooter();
            }
        }
        else
        {
            Stop();
        }
    }*/
}
