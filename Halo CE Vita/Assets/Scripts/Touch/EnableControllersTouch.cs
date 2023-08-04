using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TouchControlsKit;

public class EnableControllersTouch : TouchManagerBase
{
    public static EnableControllersTouch instance;
    private void Awake()
    {
        if (instance != null && instance != this)
        {
            Destroy(gameObject);
            return;
        }
        instance = this;
    }
    public void Enabled(bool enable)
    {
        SetActive(enable);
    }
}
