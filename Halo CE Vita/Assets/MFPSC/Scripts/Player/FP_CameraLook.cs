using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System;
using TouchControlsKit;

[RequireComponent(typeof(FP_Controller))]
[RequireComponent(typeof(FP_FootSteps))]
[RequireComponent(typeof(FP_Input))]

public class FP_CameraLook : MonoBehaviour
{
    public Transform PlayerHead;                //Player head transform;
    public float LookXSensitivity = 2.0f, LookYSensitivity = 2.0f;        //Look senstivity;
    public float ShootSensitivity = 1.0f;       //Shoot sensetivity, used when shoot button is pressed;
	//public Slider SensibilidadLook;
    public float SenciblilidadXMando = 1.0f;
    public float SenciblilidadYMando = 1.0f;
    public bool InvertedMX, InvertedMY, InvertedX, InvertedY;

    //public Text NLook;
    //public Text MLook;
    [Range(-35, -90)]
    public float minimumY = -60.0F;             //Minimum look angle;
    [Range(35, 90)]
    public float maximumY = 60.0F;              //Maximum look angle;
    public float Smooth = 25;                   //Look smoothing;


    private Vector2 lookAt;
    private float sensitivity;
    [HideInInspector]
    public float rotationY = 0.0F;

    // Head bobbing is controlled by a sine wave and the character's speed, modulated by a handful of values.
    // This script also controls sound effects for footsteps, landing and jumping
    // (because the footsteps are tied to the head bob cycle)
    // When jumping or landing, the head also moves and tilts based on some simple springy calculations.
    public HeadBob headBob;

    // private vars:
    private Vector3 originalLocalPos;							// the original local position of this gameobject at Start

    private float nextStepTime = 0.5f;									// the time at which the next footstep sound is due to occur
    private float headBobCycle = 0;								// the current position through the headbob cycle
    private float headBobFade = 0;								// the current amount to which the head bob position is being applied or not (it is faded out when the character is not moving)

    // Fields for simple spring calculation:
    private float springPos = 0;
    private float springVelocity = 0;
    private float springElastic = 1.1f;
    private float springDampen = 0.8f;
    private float springVelocityThreshold = 0.05f;
    private float springPositionThreshold = 0.05f;


    private Vector3 prevPosition;								// the position from last frame
    private Vector3 prevVelocity = Vector3.zero;				// the velocity from last frame
    private Vector3 velocity, velocityChange;
    private bool prevGrounded = true;							// whether the character was grounded last frame

    private float flatVelocity, strideLengthen, bobFactor, bobSwayFactor, speedHeightFactor, xPos, yPos, xTilt, zTilt, stepVolume;
    private float InputX, InputY;

    private AudioSource audioSource;

    private FP_Input playerInput;
    private FP_Controller playerController;
    private FP_FootSteps footSteps;

	public bool ActMira;

    void Awake()
    {
        //Cursor.visible = false;
        //Cursor.lockState = CursorLockMode.Locked;
        headBob.MainCamera.tag = "MainCamera";
    }

    // Use this for initialization
    void Start()
    {
        playerController = GetComponent<FP_Controller>();
        footSteps = GetComponent<FP_FootSteps>();
        playerInput = GetComponent<FP_Input>();

        originalLocalPos = headBob.MainCamera.localPosition;

        if (GetComponent<AudioSource>() == null)
        {
            // we automatically add an audiosource, if one has not been manually added.
            // (if you want to control the rolloff or other audio settings, add an audiosource manually)
            gameObject.AddComponent<AudioSource>();
        }

        prevPosition = transform.position;
        audioSource = GetComponent<AudioSource>();

		//SensibilidadLook.value = PlayerPrefs.GetFloat ("SencibilidadTouch", 2f);

    }

    void Update()
    {
        SenciblilidadXMando = PlayerPrefs.GetFloat("LookXGamePad", 0.5f);
        SenciblilidadYMando = PlayerPrefs.GetFloat("LookYGamePad", 0.5f);
        InvertedMX = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedXGamepad", 0));
        InvertedMY = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedYGamepad", 0));

        LookXSensitivity = PlayerPrefs.GetFloat("LookX", 0.5f);
        LookYSensitivity = PlayerPrefs.GetFloat("LookY", 0.5f);
        InvertedX = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedX", 0));
        InvertedY = Convert.ToBoolean(PlayerPrefs.GetInt("InvertedY", 0));

        /*if (!ActMira) {

			LookSensitivity = SensibilidadLook.value;

			SensibilidadLook.minValue = 0.5f;
			SensibilidadLook.maxValue = 6f;

			NLook.text = SensibilidadLook.value.ToString ();



        }

		if (ActMira) {

			LookSensitivity = 0.5f;

		}

		PlayerPrefs.SetFloat ("SencibilidadTouch", SensibilidadLook.value);*/

        //PlayerPrefs.SetFloat ("SencibilidadTouch", LookSensitivity);
        //PlayerPrefs.SetFloat ("SencibilidadShoot", ShootSensitivity);

        if (!playerController.canControl)
            return;

        switch (playerInput.UseMobileInput)
        {
            case true:

                switch (InvertedX)
                {
                    case true:

                        InputX = -(InputManager.GetAxis("Touch", "DerX") * 10f * LookXSensitivity) +
                    -(InputManager.GetAxis("Disparo", "MouseX") / 10f);

                        break;

                    case false:

                        InputX = (InputManager.GetAxis("Touch", "DerX") * 10f * LookXSensitivity) +
                    (InputManager.GetAxis("Disparo", "MouseX") / 10f);

                        break;
                }

                switch (InvertedY)
                {
                    case true:

                        InputY = -(InputManager.GetAxis("Touch", "DerY") * 10f * LookYSensitivity) +
                    -(InputManager.GetAxis("Disparo", "MouseY") / 10f);

                        break;

                    case false:

                        InputY = (InputManager.GetAxis("Touch", "DerY") * 10f * LookYSensitivity) +
                    (InputManager.GetAxis("Disparo", "MouseY") / 10f);

                        break;
                }

                /*InputX = playerInput.LookInput().x + playerInput.ShotInput().x;
                InputY = playerInput.LookInput().y + playerInput.ShotInput().y;*/
                break;
            case false:

                switch (InvertedMX)
                {
                    case true:

                        InputX = -Input.GetAxis(PlayerPrefs.GetString("VistaX")) * 10 * SenciblilidadXMando;

                        break;

                    case false:

                        InputX = Input.GetAxis(PlayerPrefs.GetString("VistaX")) * 10 * SenciblilidadXMando;

                        break;
                }

                switch (InvertedMY)
                {
                    case true:

                        InputY = -Input.GetAxis(PlayerPrefs.GetString("VistaY")) * 10 * SenciblilidadYMando;

                        break;

                    case false:

                        InputY = Input.GetAxis(PlayerPrefs.GetString("VistaY")) * 10 * SenciblilidadYMando;

                        break;
                }

                break;
        }

        sensitivity = playerInput.Shoot() ? ShootSensitivity : LookXSensitivity;

        PlayerHead.localPosition = Vector3.Lerp(PlayerHead.localPosition, new Vector3(
        PlayerHead.localPosition.x, playerController.controller.center.y + playerController.controller.height / 2 - 0.25F, PlayerHead.localPosition.z), 0.15F * 3);

        lookAt.x = Mathf.Lerp(lookAt.x, InputX, Smooth * Time.deltaTime);
        lookAt.y = Mathf.Lerp(lookAt.y, InputY, Smooth * Time.deltaTime);

        transform.Rotate(0.0F, lookAt.x * (sensitivity / 10), 0.0F);
        rotationY += lookAt.y * (sensitivity / 10);
        rotationY = Mathf.Clamp(rotationY, minimumY, maximumY);
        PlayerHead.localEulerAngles = new Vector3(-rotationY, PlayerHead.localEulerAngles.y, 0.0F);
    }

    // Update is called once per frame
    void FixedUpdate()
    {
        //control footsteps volume based on player move speed;
        // we use the actual distance moved as the velocity since last frame, rather than reading
        //the rigidbody's velocity, because this prevents the 'running against a wall' effect.
        velocity = (transform.position - prevPosition) / Time.deltaTime;
        velocityChange = velocity - prevVelocity;
        prevPosition = transform.position;
        prevVelocity = velocity;

        // vertical head position "spring simulation" for jumping/landing impacts
        springVelocity -= velocityChange.y;							// input to spring from change in character Y velocity
        springVelocity -= springPos * springElastic;					// elastic spring force towards zero position
        springVelocity *= springDampen;								// damping towards zero velocity
        springPos += springVelocity * Time.deltaTime;				// output to head Y position
        springPos = Mathf.Clamp(springPos, -.3f, .3f);			// clamp spring distance

        // snap spring values to zero if almost stopped:
        if ((Mathf.Abs(springVelocity) < springVelocityThreshold && Mathf.Abs(springPos) < springPositionThreshold))
        {
            springVelocity = 0;
            springPos = 0;
        }

        // head bob cycle is based on "flat" velocity (i.e. excluding Y)
        flatVelocity = new Vector3(velocity.x, 0, velocity.z).magnitude;

        // lengthen stride based on speed (so run bobbing isn't lots of little steps)
        strideLengthen = 1 + (flatVelocity * headBob.strideSpeedLengthen);

        // increment cycle
        headBobCycle += (flatVelocity / strideLengthen) * (Time.deltaTime / headBob.BobFrequency);

        // actual bobbing and swaying values calculated using Sine wave
        bobFactor = Mathf.Sin(headBobCycle * Mathf.PI * 2);
        bobSwayFactor = Mathf.Sin(headBobCycle * Mathf.PI * 2 + Mathf.PI * .5f); // sway is offset along the sin curve by a quarter-turn in radians
        bobFactor = 1 - (bobFactor * .5f + 1); // bob value is brought into 0-1 range and inverted
        bobFactor *= bobFactor;	// bob value is biased towards 0

        // fade head bob effect to zero if not moving
        if (new Vector3(velocity.x, 0, velocity.z).magnitude < 0.1f)
        {
            headBobFade = Mathf.Lerp(headBobFade, 0, Time.deltaTime);
        }
        else
        {
            headBobFade = Mathf.Lerp(headBobFade, 1, Time.deltaTime);
        }

        // height of bob is exaggerated based on speed
        speedHeightFactor = 1 + (flatVelocity * headBob.heightSpeedMultiplier);

        // finally, set the position and rotation values
        xPos = -headBob.BobSideMovement * bobSwayFactor;
        yPos = springPos * headBob.jumpLandMove + bobFactor * headBob.BobHeight * headBobFade * speedHeightFactor;
        xTilt = -springPos * headBob.jumpLandTilt;
        zTilt = bobSwayFactor * headBob.BobSwayAngle * headBobFade;

        headBob.MainCamera.localPosition = originalLocalPos + new Vector3(xPos, yPos, 0);
        headBob.MainCamera.localRotation = Quaternion.Euler(xTilt, 0, zTilt);

        // Play audio clips based on leaving ground/landing and head bob cycle
        if (playerController.IsGrounded())
        {
            headBob.MainCamera.localRotation = Quaternion.Euler(xTilt, 0, zTilt);
            if (!prevGrounded)
            {
                if (headBobCycle > nextStepTime)
                {
                    nextStepTime = headBobCycle + .5f;
                    footSteps.ResetFootstepSounds(audioSource);
                }
            }
            else
            {
                if (headBobCycle > nextStepTime)
                {
                    // time for next footstep sound:
                    nextStepTime = headBobCycle + .5f;
                    // play footstep sounds
                    footSteps.PlayFootstepSounds(audioSource);

                }
            }
            prevGrounded = true;
        }
        else
            prevGrounded = false;
    }

    //public Vector2 LookInput()
    //{
    //    return new Vector2(InputX, InputY);
    //}

	/*public void AjutesDefault() {

		SensibilidadLook.value = 2f;

		PlayerPrefs.DeleteKey ("SencibilidadTouch");

	}

    public void CamSencibilidadMando(float sencibilidad)
    {
        SenciblilidadMando = sencibilidad;
        MLook.text = Mathf.RoundToInt(sencibilidad).ToString();
    }*/
}