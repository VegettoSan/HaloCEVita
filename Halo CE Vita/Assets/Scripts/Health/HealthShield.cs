using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class HealthShield : MonoBehaviour
{
    [SerializeField] private string TagNormal;
    public Transform Pelvis;

    [SerializeField] private SkinSpartanRamdom Skin;

    public MonoBehaviour[] Scripts;
    public DemoAi Controller;

    private Rigidbody[] Ragdolls;
    [SerializeField] private Animator Anim;

    public int Health = 100, MaxHealth = 100;
    public float Shield = 200f, MaxShield = 200f, CandenceShield = 3f, smoothTime = 1f;

    public Renderer[] Model;

    public bool Active;

    [Range(0f, 1f)]
    public float ActiveShield = 0f;

    float Timer = 0f;
    public float TimeMax = 3f;
    void Start()
    {
        Ragdolls = Controller.Agent.gameObject.GetComponentsInChildren<Rigidbody>();
        Skin = GetComponent<SkinSpartanRamdom>();
        if (Health > 0)
        {
            Controller.Spine.tag = TagNormal;
            EnableRagdoll(false);
        }
        else if (Health <= 0)
        {
            Controller.Spine.tag = "null";
            EnableRagdoll(true);
        }

    }

    void Update()
    {
        ActiveShield = Mathf.Clamp(ActiveShield, 0f, 1f);
        Health = Mathf.Clamp(Health, 0, MaxHealth);
        Shield = Mathf.Clamp(Shield, 0f, MaxShield);

        foreach (Renderer R in Model)
        {
            R.material.SetFloat("Active_Shield", ActiveShield);
        }

        if (Health > 0)
        {
            Controller.Spine.tag = TagNormal;
            EnableRagdoll(false);
        }
        else if (Health <= 0)
        {
            Controller.Spine.tag = "null";
            Invoke("DeleteObject", 3f);
            //DeleteObject();
            EnableRagdoll(true);
            Shield = 0f;
            Timer = 0f;
            ActiveShield = 0f;
            Active = false;
            return;
        }
        if (Shield > 0f)
        {
            switch (Active)
            {
                case true:

                    if (Shield > 0)
                    {
                        ActiveShield = 1f;
                        Timer += Time.deltaTime * 1f;

                        if (Timer >= TimeMax)
                        {
                            //ActiveShield = 0f;
                            if (Shield < MaxShield)
                            {
                                Shield += Time.deltaTime * CandenceShield;
                                if (Shield >= MaxShield)
                                {
                                    Active = false;
                                }
                            }
                        }
                    }

                    break;

                case false:
                    float velocity = 0f;
                    float value = Mathf.SmoothDamp(Model[0].material.GetFloat("Active_Shield"),
                            Active ? 1 : 0, ref velocity, smoothTime);

                    ActiveShield = value;

                    //ActiveShield = 0f;
                    Timer = 0f;

                    break;
            }
        }
    }

    public void Damage(int total)
    {
        if (Shield > 0)
        {
            Shield -= total;
            switch (Active)
            {
                case true:

                    Timer = 0f;

                    break;

                case false:

                    Active = true;

                    break;
            }
        }
        else if (Shield <= 0)
        {
            ActiveShield = 0f;
            Health -= total;

            if (Health <= 0)
            {
                Invoke("DeleteObject", 4f);
                EnableRagdoll(true);
                Shield = 0f;
                Timer = 0f;
                ActiveShield = 0f;
                Active = false;
            }
        }
    }

    public void EnableRagdoll(bool enable)
    {
        Anim.enabled = !enable;
        Controller.Agent.enabled = !enable;
        foreach (Rigidbody R in Ragdolls)
        {
            R.isKinematic = !enable;
        }
        if (Scripts.Length != 0)
        {
            foreach (MonoBehaviour M in Scripts)
            {
                M.enabled = !enable;
            }
        }
    }

    void DeleteObject()
    {
        ResetRotationPelvis();
        Skin.RamdomSkin();
        Controller.Agent.gameObject.SetActive(false);
    }

    public void ResetRotationPelvis()
    {
        Pelvis.localPosition = new Vector3(0, 1, 0);
        Pelvis.localEulerAngles = new Vector3(0, -180, -90);
    }
}
