using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DemoAim : MonoBehaviour
{
    [SerializeField] DemoAi Ai;
    [SerializeField] bool Looked, Debug;
    [SerializeField] float H, V, smoothTime, smoothRotation;
    [SerializeField] Animator Anim;
    [SerializeField] Transform Look, Target;
    [SerializeField] Vector2 Axis;
    Vector3 v;
    float velocityX, velocityY;

    [Header("Horizontal")]
    [Space(1)]
    [SerializeField] float minH_1;
    [SerializeField] float maxH_1;
    [SerializeField] float minH_08;
    [SerializeField] float maxH_08;
    [SerializeField] float minH_06;
    [SerializeField] float maxH_06;
    [SerializeField] float minH_04;
    [SerializeField] float maxH_04;
    [SerializeField] float minH_02;
    [SerializeField] float maxH_02;
    [SerializeField] float minH_0;
    [SerializeField] float maxH_0;
    [SerializeField] float minH02;
    [SerializeField] float maxH02;
    [SerializeField] float minH04;
    [SerializeField] float maxH04;
    [SerializeField] float minH06;
    [SerializeField] float maxH06;
    [SerializeField] float minH08;
    [SerializeField] float maxH08;
    [SerializeField] float minH1;
    [SerializeField] float maxH1;

    [Header("Vertical")]
    [Space(1)]
    [SerializeField] float minV_1;
    [SerializeField] float maxV_1;
    [SerializeField] float minV_08;
    [SerializeField] float maxV_08;
    [SerializeField] float minV_06;
    [SerializeField] float maxV_06;
    [SerializeField] float minV_04;
    [SerializeField] float maxV_04;
    [SerializeField] float minV_02;
    [SerializeField] float maxV_02;
    [SerializeField] float minV_0;
    [SerializeField] float maxV_0;
    [SerializeField] float minV02;
    [SerializeField] float maxV02;
    [SerializeField] float minV04;
    [SerializeField] float maxV04;
    [SerializeField] float minV06;
    [SerializeField] float maxV06;
    [SerializeField] float minV08;
    [SerializeField] float maxV08;
    [SerializeField] float minV1;
    [SerializeField] float maxV1;

    int LayerAim;
    void Start()
    {
        LayerAim = Anim.GetLayerIndex("aim");
    }

    // Update is called once per frame
    void Update()
    {
        if (!Debug)
        {
            if (Ai.Target != null)
            {
                Looked = true;
                if (Ai.Target != Ai.Spine)
                {
                    if (Ai.Stop)
                    {
                        if (Ai.DistanciaIR <= Ai.Distancia && Ai.DistanciaIR > Ai.DistanciaAtacar)
                        {
                            Anim.SetLayerWeight(LayerAim, 0.5f);
                        }
                        if (Ai.DistanciaIR <= Ai.DistanciaAtacar && Ai.DistanciaIR >
                            Ai.DistanciaEvade)
                        {
                            Anim.SetLayerWeight(LayerAim, 1f);
                            SetRotation();
                        }
                        if (Ai.DistanciaIR < Ai.DistanciaEvade)
                        {
                            Anim.SetLayerWeight(LayerAim, 1f);
                            SetRotation();
                        }
                    }
                }
                else if (Ai.Target == Ai.Spine)
                {
                    Anim.SetLayerWeight(LayerAim, 0);
                }
            }
            else if (Ai.Target == null)
            {
                Looked = false;
                Anim.SetLayerWeight(LayerAim, 0);
            }
        }
        Axis = new Vector2(Look.localEulerAngles.y, Look.localEulerAngles.x);
        if (Looked)
        {
            Target = Ai.Target.transform;

            if (Target != null)
            {
                Look.LookAt(Target.position);
            }

            H = Mathf.Clamp(H, -1f, 1f);
            V = Mathf.Clamp(V, -1f, 1f);

            aim(H, V);
            SetAim();
            
        }
        
    }

    void aim(float h, float v)
    {
        Anim.SetFloat("H", h);
        Anim.SetFloat("V", v);
    }

    void SetAim()
    {
        //Rigth
        if (Axis.x >= 280f && Axis.x < 360f)
        {
            float _h = Mathf.SmoothDamp(H, 1f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x >= 270f && Axis.x < 280f)
        {
            float _h = Mathf.SmoothDamp(H, 0.8f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x >= 264f && Axis.x < 270f)
        {
            float _h = Mathf.SmoothDamp(H, 0.6f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x >= 254f && Axis.x < 264f)
        {
            float _h = Mathf.SmoothDamp(H, 0.4f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x >= 245f && Axis.x < 254f)
        {
            float _h = Mathf.SmoothDamp(H, 0.2f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x >= 236f && Axis.x < 245f)
        {
            float _h = Mathf.SmoothDamp(H, 0f, ref velocityX, smoothTime);
            H = _h;
        }

        //Left
        if (Axis.x < 236f && Axis.x > 225f)
        {
            float _h = Mathf.SmoothDamp(H, 0f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x <= 225f && Axis.x > 218f)
        {
            float _h = Mathf.SmoothDamp(H, -0.2f, ref velocityX, smoothTime);
            H = _h;
        }
        if(Axis.x <= 218f && Axis.x > 204f)
        {
            float _h = Mathf.SmoothDamp(H, -0.4f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x <= 204f && Axis.x > 189f)
        {
            float _h = Mathf.SmoothDamp(H, -0.6f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x <= 189f && Axis.x > 181f)
        {
            float _h = Mathf.SmoothDamp(H, -0.8f, ref velocityX, smoothTime);
            H = _h;
        }
        if (Axis.x <= 181f && Axis.x > 180f)
        {
            float _h = Mathf.SmoothDamp(H, -1f, ref velocityX, smoothTime);
            H = _h;
        }

        //Up
         if (Axis.y <= 303f && Axis.y > 270f)
         {
             float _v = Mathf.SmoothDamp(V, 1f, ref velocityY, smoothTime);
             V = _v;
         }
         if (Axis.y <= 313f && Axis.y > 303f)
         {
             float _v = Mathf.SmoothDamp(V, 0.8f, ref velocityY, smoothTime);
             V = _v;
         }
         if (Axis.y <= 321f && Axis.y > 313f)
         {
             float _v = Mathf.SmoothDamp(V, 0.6f, ref velocityY, smoothTime);
             V = _v;
         }
         if (Axis.y <= 330f && Axis.y > 321f)
         {
             float _v = Mathf.SmoothDamp(V, 0.4f, ref velocityY, smoothTime);
             V = _v;
         }
         if (Axis.y <= 337f && Axis.y > 330f)
         {
             float _v = Mathf.SmoothDamp(V, 0.2f, ref velocityY, smoothTime);
             V = _v;
         }
         if (Axis.y < 342f && Axis.y > 337f)
         {
             float _v = Mathf.SmoothDamp(V, 0f, ref velocityY, smoothTime);
             V = _v;
         }

        //Down
        if (Axis.y >= 342f && Axis.y < 354f)
        {
            float _v = Mathf.SmoothDamp(V, 0f, ref velocityY, smoothTime);
            V = _v;
        }
        if (Axis.y >= 354f && Axis.y < 360f)
        {
            float _v = Mathf.SmoothDamp(V, -0.2f, ref velocityY, smoothTime);
            V = _v;
        }
        if (Axis.y >= 1f && Axis.y < 6f)
        {
            float _v = Mathf.SmoothDamp(V, -0.4f, ref velocityY, smoothTime);
            V = _v;
        }
        if (Axis.y >= 6f && Axis.y < 17f)
        {
            float _v = Mathf.SmoothDamp(V, -0.6f, ref velocityY, smoothTime);
            V = _v;
        }
        if (Axis.y >= 17f && Axis.y < 28f)
        {
            float _v = Mathf.SmoothDamp(V, -0.8f, ref velocityY, smoothTime);
            V = _v;
        }
        if (Axis.y >= 28f && Axis.y < 90f)
        {
            float _v = Mathf.SmoothDamp(V, -1f, ref velocityY, smoothTime);
            V = _v;
        }
    }

    void SetRotation()
    {
        if (Axis.x > 340f && Axis.x < 360f)
        {
            //Ai.Agent.gameObject.transform.EulerAngles += new Vector3(0f, Time.deltaTime * smoothRotation, 0f);
            Ai.Agent.gameObject.transform.eulerAngles += new Vector3(0f, 45f, 0f);
            //Ai.Agent.gameObject.transform.Rotate(0f, Time.deltaTime * smoothRotation, 0f);
        }
        if (Axis.x > 0f && Axis.x < 40f)
        {
            //Ai.Agent.gameObject.transform.EulerAngles += new Vector3(0f, Time.deltaTime * smoothRotation, 0f);
            Ai.Agent.gameObject.transform.eulerAngles += new Vector3(0f, 45f, 0f);
            //Ai.Agent.gameObject.transform.Rotate(0f, Time.deltaTime * smoothRotation, 0f);
        }

        if (Axis.x < 200f && Axis.x > 180f)
        {
            //Ai.Agent.gameObject.transform.EulerAngles -= new Vector3(0f, Time.deltaTime * smoothRotation, 0f);
            Ai.Agent.gameObject.transform.eulerAngles -= new Vector3(0f, 45f, 0f);
            //Ai.Agent.gameObject.transform.Rotate(0f, -Time.deltaTime * smoothRotation, 0f);
        }
        if (Axis.x < 180f && Axis.x > 140f)
        {
            //Ai.Agent.gameObject.transform.EulerAngles -= new Vector3(0f, Time.deltaTime * smoothRotation, 0f);
            Ai.Agent.gameObject.transform.eulerAngles -= new Vector3(0f, 45f, 0f);
            //Ai.Agent.gameObject.transform.Rotate(0f, -Time.deltaTime * smoothRotation, 0f);
        }
    }
}
