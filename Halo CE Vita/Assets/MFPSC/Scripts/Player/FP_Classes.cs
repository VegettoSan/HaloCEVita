using System;
using UnityEngine;
using UnityEngine.UI;
using System.Collections;

[Serializable]
public class HeadBob
{
    public Transform MainCamera;					   // the object to which the head-bob movement should be applied

    // these modulate the head bob movement
    [Range(0, 5)]
    public float BobFrequency = 1.5f;				// the base speed of the head bobbing (in cycles per metre)
    [Range(0, 1)]
    public float BobHeight = 0.3f;					// the height range of the head bob
    [Range(0, 3)]
    public float BobSwayAngle = 0.5f;				// the angle which the head tilts to left & right during the bob cycle
    [Range(0, 1)]
    public float BobSideMovement = 0.05f;			// the distance the head moves to left & right during the bob cycle
    [Range(0, 1)]
    public float heightSpeedMultiplier = 0.35f;		// the amount the bob height increases as the character's speed increases (for a good 'run' effect compared with walking)
    [Range(0, 1)]
    public float strideSpeedLengthen = 0.35f;			// the amount the stride lengthens based on speed (so that running isn't like a silly speedwalk!)

    // these control the amount of movement applied to the head when the character jumps or lands
    [Range(0, 5)]
    public float jumpLandMove = 1;
    [Range(0, 20)]
    public float jumpLandTilt = 10;
}

[Serializable]
public class Footsteps
{
    public string SurfaceTag = "Untagged";
	public string Metal = "Metal";
	public string Terreno = "Terreno";
	public string Agua = "Water";
    // audio clip references   
    public AudioClip[] stepSounds;      // an array of footstep sounds that will be randomly selected from.
	public AudioClip[] MetalSounds;
	public AudioClip[] TerrenoSounds;
	public AudioClip[] WaterSounds;
}