using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Spawner : MonoBehaviour {

	[System.Serializable]
	public class ScripsHealth
    {
		public bool HealthShield;
    }
	[Header("HEALTH")]
	[SerializeField] ScripsHealth HealthScripts;

	[Header("POSITIONS")]
	[SerializeField] Transform[] PocionesSpawn;
	//[SerializeField] private float Timer = 0f;
	[SerializeField] float Tiempo = 15f;

	[SerializeField] private GameObject[] TotalBots;
	[SerializeField] string Tag;

	[Range(1,10)]
	public int MaxBots;

    private void Awake()
    {
		TotalBots = GameObject.FindGameObjectsWithTag(Tag);
	}
    private void Start()
    {
		StartCoroutine("Counter");
	}

    void Update ()
	{
		TotalBots = GameObject.FindGameObjectsWithTag(Tag);
	}

	IEnumerator Counter()
	{
		for (int i = 0; i < i + 1; i++)
		{
			if (TotalBots.Length == 0 || TotalBots.Length < MaxBots)
			{
				Instanciar();
			}
			yield return new WaitForSeconds(Tiempo);
		}
		yield break;
	}

	void Instanciar()
	{
		/*if (Timer >= Tiempo)
		{
			int a = Random.Range(0, PocionesSpawn.Length);
			GameObject Obj = ObjectPool.Instance.RequestObj();
			Obj.transform.position = PocionesSpawn[a].position;
			UseScriptsHealth(Obj);
			Timer = 0f;
		}*/
		int a = Random.Range(0, PocionesSpawn.Length);
		GameObject Obj = ObjectPool.Instance.RequestObj();
		Obj.transform.position = PocionesSpawn[a].position;
		UseScriptsHealth(Obj);
		//Timer = 0f;
	}

	void UseScriptsHealth(GameObject obj)
    {
        if (HealthScripts.HealthShield)
        {
			HealthShield _hs = obj.GetComponentInChildren<HealthShield>();
			if(_hs != null)
            {
				_hs.Health = _hs.MaxHealth;
				_hs.Shield = _hs.MaxShield;
				_hs.ResetRotationPelvis();
				_hs.Controller.Agent.enabled = true;
			}
		}
    }
}