using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;

[RequireComponent(typeof(EventTrigger))]
[RequireComponent(typeof(CanvasGroup))]
public class FP_Lookpad : MonoBehaviour {

    private Vector2 touchInput, prevDelta, dragInput;
    private bool isPressed;
    private EventTrigger eventTrigger;
    private CanvasGroup canvasGroup;

    void Start()
    {
        SetupListeners();
        canvasGroup = GetComponent<CanvasGroup>();
        canvasGroup.alpha = 0;
    }

    // Update is called once per frame
    void Update()
    {
        touchInput = (dragInput - prevDelta) / Time.deltaTime;
        prevDelta = dragInput;
    }

    //Setup events;
    void SetupListeners()
    {
        eventTrigger = gameObject.GetComponent<EventTrigger>();

        var a = new EventTrigger.TriggerEvent();
        a.AddListener(data =>
        {
            var evData = (PointerEventData)data;
            data.Use();
            isPressed = true;
            prevDelta = dragInput = evData.position;
        });

        eventTrigger.triggers.Add(new EventTrigger.Entry { callback = a, eventID = EventTriggerType.PointerDown });


        var b = new EventTrigger.TriggerEvent();
        b.AddListener(data =>
        {
            var evData = (PointerEventData)data;
            data.Use();
            dragInput = evData.position;
        });

        eventTrigger.triggers.Add(new EventTrigger.Entry { callback = b, eventID = EventTriggerType.Drag });


        var c = new EventTrigger.TriggerEvent();
        c.AddListener(data =>
        {
            touchInput = Vector2.zero;
            isPressed = false;
        });

        eventTrigger.triggers.Add(new EventTrigger.Entry { callback = c, eventID = EventTriggerType.EndDrag });
    }

    //Returns drag vector;
    public Vector2 LookInput()
    {
        return touchInput * Time.deltaTime;
    }

    //Returns whether or not button is pressed
    public bool IsPressed()
    {
        return isPressed;
    }
}
