using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;

public class DemoAi : MonoBehaviour
{
	public GameObject Spine;

	[SerializeField] LayerMask Layers;
	public string[] EnemysTags;
	public GameObject Target;
	public bool WithTarget = false, Pvp = false;

	public float Speed = 5f, SpeedReverse = 5f;
	public NavMeshAgent Agent;
	public Animator Anim;

	public float DistanciaIR;
	public float Distancia = 30f;
	public float DistanciaFire = 15f;
	public float DistanciaAtacar = 10f;
	public float DistanciaEvade = 5f;

	public bool Evadir, Stop, Fire;
	public RaycastHit Hit;
	[SerializeField] Transform Look;
	[SerializeField] private GameObject Ai;

	void Start()
	{

	}

	// Update is called once per frame
	void Update()
	{
        if (Input.GetKeyDown(KeyCode.Space))
        {
			Target = null;
			WithTarget = false;
		}

		if (WithTarget)
		{
			if (Target != null)
			{
				if (Target.tag == "null" || Target == Spine)
				{
					WithTarget = false;
					Target = null;

					Fire = false;
				}
				else if (Target != Spine)
				{

					DistanciaIR = Vector3.Distance(transform.position, Target.transform.position);

					if (DistanciaIR > Distancia)
					{
						Target = null;
						WithTarget = false;
					}
					if (DistanciaIR <= Distancia && DistanciaIR > DistanciaAtacar)
					{
						Fire = false;
						Evadir = false;
						Stop = false;
						Agent.speed = Speed;
						Agent.SetDestination(Target.transform.position);
						Anim.SetBool("Idle", false);
						Anim.SetBool("Run", true);
						Anim.SetBool("RunBack", false);
					}
					/*if (DistanciaIR <= DistanciaAtacar && DistanciaIR > DistanciaEvade)
					{
						Evadir = false;
						Stop = true;
						Agent.speed = 0f;
						Anim.SetBool("Idle", true);
						Anim.SetBool("Run", false);
						Anim.SetBool("RunBack", false);
					}
					if (DistanciaIR <= DistanciaEvade)
					{
						Evadir = true;
						Stop = true;
						EvadeReverse();
					}*/


					Debug.DrawRay(Look.position, Look.forward * DistanciaIR, Color.green, 0.1f);
					if (Physics.Raycast(Look.position, Look.forward, out Hit, DistanciaIR, Layers))
					{
						if (Hit.collider.tag == Target.tag || Hit.collider.tag == Target.tag + "Parts")
						{
							Stop = true;
							if (DistanciaIR <= DistanciaFire && DistanciaIR > DistanciaAtacar)
							{
								Fire = true;
								Evadir = false;
								Agent.speed = 0f;
								Anim.SetBool("Idle", false);
								Anim.SetBool("Run", true);
								Anim.SetBool("RunBack", false);
							}
							if (DistanciaIR <= DistanciaAtacar && DistanciaIR > DistanciaEvade)
							{
								Fire = true;
								Evadir = false;
								Agent.speed = Speed;
								Anim.SetBool("Idle", true);
								Anim.SetBool("Run", false);
								Anim.SetBool("RunBack", false);
							}
							if (DistanciaIR <= DistanciaEvade)
							{
								Fire = true;
								Evadir = true;
								EvadeReverse();
							}
						}
						if (Hit.collider.tag == "Untagged" || Hit.collider.tag == "Metal" ||
							Hit.collider.tag == "Ground" || Hit.collider.tag == "Water" ||
							Hit.collider.tag == "null" || Hit.collider.tag == null)
						{
							Fire = false;
							Evadir = false;
							Stop = false;
							Agent.speed = Speed;
							Agent.SetDestination(Target.transform.position);
							Anim.SetBool("Idle", false);
							Anim.SetBool("Run", true);
							Anim.SetBool("RunBack", false);
						}
					}
					//
				}
			}
			else if (Target == null || !WithTarget)
			{
				Agent.speed = 0f;
				Anim.SetBool("Idle", true);
				Anim.SetBool("Run", false);
				Anim.SetBool("RunBack", false);
			}
		}
		else if (!WithTarget)
		{
			Agent.speed = 0f;
			Anim.SetBool("Idle", true);
			Anim.SetBool("Run", false);
			Anim.SetBool("RunBack", false);
			foreach (string tag in EnemysTags)
			{
				foreach (GameObject enemi in GameObject.FindGameObjectsWithTag(tag))
				{
					if (Target == null)
					{
						switch (Pvp)
						{
							case true:

								if (enemi != Spine)
								{
									Target = enemi;
								}
								else if (enemi == Spine)
								{
									Agent.speed = 0f;
									Anim.SetBool("Idle", true);
									Anim.SetBool("Run", false);
									Anim.SetBool("RunBack", false);
									RandomTarget(tag);
								}
								break;

							case false:

								Target = enemi;

								break;
						}
					}
					else
					{

						if (Vector3.Distance(transform.position, enemi.transform.position) < Vector3.Distance(transform.position, Target.transform.position))
						{

							switch (Pvp)
							{
								case true:

									if (enemi != Spine)
									{
										Target = enemi;
									}
									else if (enemi == Spine)
									{
										RandomTarget(tag);
									}

									break;

								case false:

									Target = enemi;

									break;
							}

						}
					}
				}
			}

			WithTarget = true;

		}
		else
		{



		}
	}

	void RandomTarget(string tag)
    {
		int i = Random.Range(0, GameObject.FindGameObjectsWithTag(tag).Length);
		Target = GameObject.FindGameObjectsWithTag(tag)[i];

	}

	void EvadeReverse()
	{
		if (Evadir)
		{
			Agent.speed = 0.05f;
			Anim.SetBool("Idle", false);
			Anim.SetBool("Run", false);
			Anim.SetBool("RunBack", true);
			Ai.transform.Translate(Vector3.back * SpeedReverse * Time.deltaTime);
		}
	}
}
