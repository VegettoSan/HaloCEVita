using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class NumberDisplay : MonoBehaviour
{
    [SerializeField] private Renderer Rifle;
    [SerializeField] private Texture[] Numbers;
    private GunsHuman Balds;
    void Start()
    {
        Balds = GetComponent<GunsHuman>();
        Counter();
    }

    // Update is called once per frame
    void Update()
    {
        Counter();
    }

    void Counter()
    {
        switch (Balds.Balas)
        {
            case 0:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[0]);

                break;
            case 1:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[1]);

                break;
            case 2:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[2]);

                break;
            case 3:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[3]);

                break;
            case 4:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[4]);

                break;
            case 5:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[5]);

                break;
            case 6:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[6]);

                break;
            case 7:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[7]);

                break;
            case 8:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[8]);

                break;
            case 9:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[0]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[9]);

                break;
            case 10:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[0]);

                break;
            case 11:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[1]);

                break;
            case 12:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[2]);

                break;
            case 13:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[3]);

                break;
            case 14:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[4]);

                break;
            case 15:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[5]);

                break;
            case 16:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[6]);

                break;
            case 17:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[7]);

                break;
            case 18:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[8]);

                break;
            case 19:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[1]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[9]);

                break;
            case 20:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[0]);

                break;
            case 21:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[1]);

                break;
            case 22:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[2]);

                break;
            case 23:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[3]);

                break;
            case 24:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[4]);

                break;
            case 25:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[5]);

                break;
            case 26:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[6]);

                break;
            case 27:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[7]);

                break;
            case 28:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[8]);

                break;
            case 29:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[2]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[9]);

                break;
            case 30:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[0]);

                break;
            case 31:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[1]);

                break;
            case 32:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[2]);

                break;
            case 33:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[3]);

                break;
            case 34:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[4]);

                break;
            case 35:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[5]);

                break;
            case 36:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[6]);

                break;
            case 37:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[7]);

                break;
            case 38:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[8]);

                break;
            case 39:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[3]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[9]);

                break;
            case 40:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[0]);

                break;
            case 41:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[1]);

                break;
            case 42:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[2]);

                break;
            case 43:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[3]);

                break;
            case 44:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[4]);

                break;
            case 45:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[5]);

                break;
            case 46:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[6]);

                break;
            case 47:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[7]);

                break;
            case 48:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[8]);

                break;
            case 49:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[4]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[9]);

                break;
            case 50:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[0]);

                break;
            case 51:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[1]);

                break;
            case 52:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[2]);

                break;
            case 53:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[3]);

                break;
            case 54:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[4]);

                break;
            case 55:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[5]);

                break;
            case 56:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[6]);

                break;
            case 57:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[7]);

                break;
            case 58:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[8]);

                break;
            case 59:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[5]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[9]);

                break;
            case 60:

                Rifle.materials[3].SetTexture("_BaseMap", Numbers[6]);
                Rifle.materials[4].SetTexture("_BaseMap", Numbers[0]);

                break;
        }
    }
}
