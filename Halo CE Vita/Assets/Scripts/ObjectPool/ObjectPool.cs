using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ObjectPool : MonoBehaviour
{
    [SerializeField] private GameObject Prefab;
    [SerializeField] private int poolSize = 10;
    [SerializeField] private List<GameObject> ObjectList;

    private static ObjectPool instance;
    public static ObjectPool Instance { get { return instance; } }

    void Awake()
    {
        if(instance == null)
        {
            instance = this;
        }
        else
        {
            Destroy(gameObject);
        }
    }

    void Start()
    {
        AddObjectToPool(poolSize);
    }

    private void AddObjectToPool(int amount)
    {
        for (int i = 0; i < amount; i++)
        {
            GameObject Obj = Instantiate(Prefab, this.transform);
            Obj.SetActive(false);
            ObjectList.Add(Obj);
        }
    }

    public GameObject RequestObj()
    {
        for(int i = 0; i < ObjectList.Count; i++)
        {
            if (!ObjectList[i].activeSelf)
            {
                ObjectList[i].SetActive(true);
                return ObjectList[i];
            }
        }
        AddObjectToPool(1);
        ObjectList[ObjectList.Count - 1].SetActive(true);
        return ObjectList[ObjectList.Count - 1];
    }
}
