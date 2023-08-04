using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class TextRandom : MonoBehaviour
{
    [SerializeField]
    float Timer = 3f;
    [TextArea]
    public string[] Texts;
    Text Text;
    void Start()
    {
        Text = GetComponent<Text>();
        StartCoroutine("Counter");
    }

    IEnumerator Counter()
    {
        for (int i = 0; i < i + 1; i++)
        {
            int T = Random.Range(0, Texts.Length);
            Text.text = Texts[T];

            yield return new WaitForSeconds(Timer);
        }
    }
}
