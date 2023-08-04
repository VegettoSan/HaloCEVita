using UnityEngine;
using System.Collections;
using System;
using UnityEngine.UI;
using TouchControlsKit;

[RequireComponent(typeof(AudioSource))]
[RequireComponent (typeof (CharacterController))]
[RequireComponent(typeof(FP_Input))]
[RequireComponent(typeof(FP_CameraLook))]
[RequireComponent(typeof(FP_FootSteps))]
public class FP_Controller : MonoBehaviour
{
    public bool canControl = true;
    public float gravity = 20.0f;
	public float walkSpeed = 6.0f;
    public float runSpeed = 11.0f;
    public float jumpForce = 8.0f;
    public float crouchSpeed = 2.0F;
    public float crouchHeight = 1.0F;

    public KeyCode crouchKey = KeyCode.LeftControl;
    public KeyCode runKey = KeyCode.LeftShift;
    public KeyCode jumpKey = KeyCode.Space;

    public bool airControl = true;
    public bool canCrouch = true;
    public bool canJump = true;
    public bool canRun = true;

    public float MandoSpeed = 1f;

    [HideInInspector]
    public CharacterController controller;

    private Vector3 moveDirection;
    private Vector3 contactPoint;
    private Vector3 hitNormal;
    private AudioSource JumpLandSource;
    private FP_FootSteps footSteps;
	private Transform myTransform;
    private FP_Input playerInput;
	private RaycastHit hit;


    private bool playerControl = false;
	public bool isCrouching = false;
	public bool grounded = false;
    private bool sliding = false;
    private bool crouch = false;
	public bool jump = false;
    private bool run = false;

	public int antiBunnyHopFactor = 1;
	public int jumpTimer;
	public int landTimer;
	public int jumpState;
	public int runState;

    private float antiBumpFactor = 0.75F;
    private float inputModifyFactor;
    private float slideSpeed = 2.0F;
    private float minCrouchHeight;
    public float inputX, inputZ;
    private float fallStartLevel;
    private float defaultHeight;
    private float rayDistance;
    private float slideLimit;
    private float speed;
    private string surfaceTag;
    

    void Awake()
    {
        controller = GetComponent<CharacterController>();
        playerInput = GetComponent<FP_Input>();
        footSteps = GetComponent<FP_FootSteps>();
    }
	
	void Start() 
    {
		defaultHeight = controller.height;
        minCrouchHeight = crouchHeight > controller.radius * 2 ? crouchHeight : controller.radius * 2;
		myTransform = transform;
		speed = walkSpeed;
		rayDistance = controller.height * 0.5F + controller.radius;
		slideLimit = controller.slopeLimit - 0.1F;
		jumpTimer = antiBunnyHopFactor;
        JumpLandSource = gameObject.AddComponent<AudioSource>();
	}
	
	void FixedUpdate()
    {
		// If both horizontal and vertical are used simultaneously, limit speed (if allowed), so the total doesn't exceed normal move speed
		inputModifyFactor = (inputX != 0.0F && inputZ != 0.0F)? 0.7071F : 1.0F;
		
		if (grounded) {
			sliding = false;
			// See if surface immediately below should be slid down. We use this normally rather than a ControllerColliderHit point,
			// because that interferes with step climbing amongst other annoyances
			if (Physics.Raycast(myTransform.position, -Vector3.up, out hit, rayDistance)) {
				if (Vector3.Angle(hit.normal, Vector3.up) > slideLimit && CanSlide())
					sliding = true;
			}
			// However, just raycasting straight down from the center can fail when on steep slopes
			// So if the above raycast didn't catch anything, raycast down from the stored ControllerColliderHit point instead
			else {
				Physics.Raycast(contactPoint + Vector3.up, -Vector3.up, out hit, rayDistance);
				if (Vector3.Angle(hit.normal, Vector3.up) > slideLimit && CanSlide())
					sliding = true;
			}

            speed = isCrouching || !CanStand() ? crouchSpeed : run ? canRun ? runSpeed : walkSpeed : walkSpeed;
			
			// If sliding (and it's allowed), or if we're on an object tagged "Slide", get a vector pointing down the slope we're on
			if (sliding) 
            {
				hitNormal = hit.normal;
				moveDirection = new Vector3(hitNormal.x, -hitNormal.y, hitNormal.z);
				Vector3.OrthoNormalize (ref hitNormal, ref moveDirection);
				moveDirection *= slideSpeed;
				playerControl = false;
			}
			// Otherwise recalculate moveDirection directly from axes, adding a bit of -y to avoid bumping down inclines
			else
            {
				moveDirection = new Vector3(inputX * inputModifyFactor, -antiBumpFactor, inputZ * inputModifyFactor);
				moveDirection = myTransform.TransformDirection(moveDirection) * speed;
				playerControl = true;
			}
			
			// Jump! But only if canJump, the jump button has been released and player has been grounded for a given number of frames
			if (!jump)
				jumpTimer++;
			else if (canJump && jumpTimer >= antiBunnyHopFactor) 
            {
				moveDirection.y = jumpForce;
				jumpTimer = 0;
			}
		}
		else 
        {
			// If air control is allowed, check movement but don't touch the y component
			if (airControl && playerControl)
            {
				moveDirection.x = inputX * speed * inputModifyFactor;
				moveDirection.z = inputZ * speed * inputModifyFactor;
				moveDirection = myTransform.TransformDirection(moveDirection);
			}
		}
		
		// Apply gravity
		moveDirection.y -= gravity * Time.deltaTime;
		// Move the controller, and set grounded true or false depending on whether we're standing on something
		grounded = (controller.Move(moveDirection * Time.deltaTime) & CollisionFlags.Below) != 0;
    }

    void Update()
    {
        if (!canControl)
            return;

        switch (playerInput.UseMobileInput)
        {
            case true:
			runState = playerInput.Run() && canRun && !isCrouching ? 1 : 0;
			inputX = InputManager.GetAxis("Joystick","IzqX");
			inputZ = InputManager.GetAxis("Joystick","IzqY");
			crouch = InputManager.GetButton("Crouch");
			//run = InputManager.GetButton("Run");
			jump = InputManager.GetButton("Jump");
                /*runState = playerInput.Run() && canRun && !isCrouching ? 1 : 0;
                inputX = playerInput.MoveInput().x;
                inputZ = playerInput.MoveInput().z + runState;
                crouch = playerInput.Crouch();
                run = playerInput.Run();
                jump = playerInput.Jump();*/
            break;
            case false:
                inputX = Input.GetAxis(PlayerPrefs.GetString("MovimientoX")) * MandoSpeed;
                inputZ = Input.GetAxis(PlayerPrefs.GetString("MovimientoY")) * MandoSpeed;

                crouch = Input.GetButton(PlayerPrefs.GetString("PBL"));
				//run = Input.GetButton("Boton10");
				jump = Input.GetButton(PlayerPrefs.GetString("A"));
                /*crouch = Input.GetKey(crouchKey);
                run = Input.GetKey(runKey);
                jump = Input.GetKey(jumpKey);*/
            break;
        }

        if (jumpState == 0 && CanStand() && jump && jumpTimer >= antiBunnyHopFactor)
        {
            PlaySound(footSteps.jumpSound, JumpLandSource);
            jumpState++;
        }

        if ((Mathf.Abs((transform.position - contactPoint).magnitude) > 2))
            landTimer = 1;

        isCrouching = crouch && canCrouch;

        if (grounded)
        {
            if (isCrouching)
            {
                controller.center = Vector3.Lerp(controller.center, new Vector3(controller.center.x, -(defaultHeight - minCrouchHeight) / 2, controller.center.z), 15 * Time.deltaTime);
                controller.height = Mathf.Lerp(controller.height, minCrouchHeight, 15 * Time.deltaTime);
            }
            else
            {
                if (CanStand())
                {
                    controller.center = Vector3.Lerp(controller.center, Vector3.zero, 15 * Time.deltaTime);
                    controller.height = Mathf.Lerp(controller.height, defaultHeight, 15 * Time.deltaTime);
                }
            }
        }
    }

	void OnControllerColliderHit (ControllerColliderHit hit) {
		if (!IsGrounded () && landTimer == 1)
            PlaySound(footSteps.landSound, JumpLandSource);
		landTimer = 0;
        jumpState = 0;
		contactPoint = hit.point;
        surfaceTag = hit.collider.tag;
	}

	void PlaySound(AudioClip audio, AudioSource source)
	{
		source.clip = audio;
		if (audio)
			source.Play ();
	}

	public bool IsGrounded()
	{
		return grounded;
	}

    public bool IsCrouching()
    {
        return crouch;
    }

    public bool IsRunning()
    {
        return run;
    }

	private bool CanStand()
	{
        RaycastHit hitAbove = new RaycastHit();
        return !Physics.SphereCast(controller.bounds.center, controller.radius, Vector3.up, out hitAbove,
                                   controller.height / 2 + 0.5F);
	}

	private bool CanSlide()
	{
		return new Vector3 (controller.velocity.x, 0, controller.velocity.z).magnitude < walkSpeed/2;
	}

    public string SurfaceTag()
    {
        return surfaceTag;
    }

    public void ChangeMobileInput()
    {
        playerInput.UseMobileInput = !playerInput.UseMobileInput;
    }
}