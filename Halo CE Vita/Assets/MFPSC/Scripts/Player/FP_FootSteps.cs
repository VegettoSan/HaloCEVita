using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FP_FootSteps : MonoBehaviour 
{
    public AudioClip jumpSound, landSound;
    public List<Footsteps> footsteps = new List<Footsteps>();
    private FP_Controller playerController;
    private int randomStep;

    void Start()
    {
        playerController = GetComponent<FP_Controller>();
    }

    public void PlayFootstepSounds(AudioSource audioSource)
    {
        for (int i = 0; i < footsteps.Count; i++)
        {
            if (footsteps[i].SurfaceTag == playerController.SurfaceTag())
            {
                // pick & play a random footstep sound from the array,
                // excluding sound at index 0
                randomStep = Random.Range(1, footsteps[i].stepSounds.Length);
                audioSource.clip = footsteps[i].stepSounds[randomStep];
                audioSource.Play();

                // move picked sound to index 0 so it's not picked next time
                footsteps[i].stepSounds[randomStep] = footsteps[i].stepSounds[0];
                footsteps[i].stepSounds[0] = audioSource.clip;
            }

			if (footsteps[i].Terreno == playerController.SurfaceTag())
			{
				// pick & play a random footstep sound from the array,
				// excluding sound at index 0
				randomStep = Random.Range(1, footsteps[i].TerrenoSounds.Length);
				audioSource.clip = footsteps[i].TerrenoSounds[randomStep];
				audioSource.Play();

				// move picked sound to index 0 so it's not picked next time
				footsteps[i].TerrenoSounds[randomStep] = footsteps[i].TerrenoSounds[0];
				footsteps[i].TerrenoSounds[0] = audioSource.clip;
			}

			if (footsteps[i].Metal == playerController.SurfaceTag())
			{
				// pick & play a random footstep sound from the array,
				// excluding sound at index 0
				randomStep = Random.Range(1, footsteps[i].MetalSounds.Length);
				audioSource.clip = footsteps[i].MetalSounds[randomStep];
				audioSource.Play();

				// move picked sound to index 0 so it's not picked next time
				footsteps[i].MetalSounds[randomStep] = footsteps[i].MetalSounds[0];
				footsteps[i].MetalSounds[0] = audioSource.clip;
			}

			if (footsteps[i].Agua == playerController.SurfaceTag())
			{
				// pick & play a random footstep sound from the array,
				// excluding sound at index 0
				randomStep = Random.Range(1, footsteps[i].WaterSounds.Length);
				audioSource.clip = footsteps[i].WaterSounds[randomStep];
				audioSource.Play();

				// move picked sound to index 0 so it's not picked next time
				footsteps[i].WaterSounds[randomStep] = footsteps[i].WaterSounds[0];
				footsteps[i].WaterSounds[0] = audioSource.clip;
			}

        }
    }

    public void ResetFootstepSounds(AudioSource audioSource)
    {
        for (int i = 0; i < footsteps.Count; i++)
        {
            if (footsteps[i].SurfaceTag == playerController.SurfaceTag())
            {
                audioSource.clip =  footsteps[i].stepSounds[0];
                audioSource.Play();
            }

			if (footsteps[i].Terreno == playerController.SurfaceTag())
			{
				audioSource.clip =  footsteps[i].TerrenoSounds[0];
				audioSource.Play();
			}

        }
    }
}



