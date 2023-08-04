using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class PlayerManager : MonoBehaviour
{
    public static PlayerManager include;
    public GameObject Player, BodyDead, CanvasCount;
    GameObject[] RespawnPositions;
    public TMP_Text text;
    public int Seconds;
    [SerializeField] AudioSource Source;
    [SerializeField] AudioClip CountSound, RespawnSound;

    int n;

    private void Awake()
    {
        if (include != null)
        {
            Destroy(this.gameObject);
        }
        else if (include == null)
        {
            include = this;
        }
    }
    void Start()
    {
        if (RespawnPositions == null)
        {
            RespawnPositions = GameObject.FindGameObjectsWithTag("PointRespawnPlayer");
        }
    }

    // Update is called once per frame
    void Update()
    {

    }

    public void StartCount()
    {
        n = Seconds + 1;
        EnableControllersTouch.instance.Enabled(false);
        CanvasCount.SetActive(true);
        Instantiate(BodyDead, Player.transform.position, Player.transform.rotation);
        StartCoroutine("Counter");
    }
    public void EndCount()
    {
        n = Seconds + 1;
        int i = Random.Range(0, RespawnPositions.Length);
        PlayerHealth H = Player.GetComponent<PlayerHealth>();
        H.HealthStatistics.Health = H.HealthStatistics.MaxHealth;
        H.HealthStatistics.Shield = H.HealthStatistics.MaxShield;
        Player.transform.position = RespawnPositions[i].transform.position;
        Player.SetActive(true);

        CanvasCount.SetActive(false);
        EnableControllersTouch.instance.Enabled(true);
    }
    IEnumerator Counter()
    {
        //text.text = "0";
        for (int i = 0; i < Seconds; i++)
        {
            n--;
            Source.PlayOneShot(CountSound);
            text.text = "REAPARECIENDO EN " + n.ToString();
            yield return new WaitForSeconds(1f);
        }
        text.text = "REAPARECIENDO...";
        yield return new WaitForSeconds(0.3f);
        Source.PlayOneShot(RespawnSound);
        EndCount();
        yield break;
    }
}
