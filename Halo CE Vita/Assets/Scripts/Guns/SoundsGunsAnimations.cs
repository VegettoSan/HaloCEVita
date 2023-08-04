using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SoundsGunsAnimations : MonoBehaviour
{
    public AudioClip Sound0, Sound1, Sound2, Sound3, Sound4, Sound5, Sound6, Sound7;
    public AudioSource Source;
    
    public void Sound_0()
    {
        Source.PlayOneShot(Sound0);
    }
    public void Sound_1()
    {
        Source.PlayOneShot(Sound1);
    }
    public void Sound_2()
    {
        Source.PlayOneShot(Sound2);
    }
    public void Sound_3()
    {
        Source.PlayOneShot(Sound3);
    }
    public void Sound_4()
    {
        Source.PlayOneShot(Sound4);
    }
    public void Sound_5()
    {
        Source.PlayOneShot(Sound5);
    }
    public void Sound_6()
    {
        Source.PlayOneShot(Sound6);
    }
    public void Sound_7()
    {
        Source.PlayOneShot(Sound7);
    }
}
