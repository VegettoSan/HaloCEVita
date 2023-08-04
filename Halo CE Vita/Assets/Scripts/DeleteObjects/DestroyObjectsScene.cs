using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DestroyObjectsScene : MonoBehaviour
{
    public static DestroyObjectsScene instance;
    public GameObject Delete, DContent;

    [SerializeField] private Transform[] Content;

    [SerializeField] int CountChield;

    [SerializeField] float MaxTime;
    [SerializeField] float Timer;

    private void Awake()
    {
        if (instance != null && instance != this)
        {
            Destroy(gameObject);
            return;
        }
        instance = this;
    }
    void Start()
    {
        if(Delete == null)
        {
            AssingDelete();
        }
    }

    // Update is called once per frame
    void Update()
    {
        CountChield = Delete.transform.childCount;
        if (Delete.transform.childCount > 0)
        {
            Content = Delete.GetComponentsInChildren<Transform>();
            Timer += Time.deltaTime * 1f;
            if (Timer >= MaxTime)
            {
                for (int i = 1; i < Content.Length; i++)
                {
                    Destroy(Content[i].gameObject);
                    Content.GetLength(0);
                    Timer = 0f;
                    //AssingDelete();
                }
            }
            /*if (Timer >= MaxTime + 1f)
            {
                //AssingDelete();
                Timer = 0f;
            }*/
        }
        else if (Delete.transform.childCount == 0)
        {
            //Content = null;
            Timer = 0f;
        }
    }

    void AssingDelete()
    {
        var D = Instantiate(DContent, this.transform.position, this.transform.rotation, this.transform);
        Delete = D.gameObject;
    }
}
