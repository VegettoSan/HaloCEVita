using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;

[RequireComponent(typeof(CanvasGroup))]
[RequireComponent(typeof(EventTrigger))]
public class FP_Button : MonoBehaviour
{
    public Canvas myCanvas;                                     //Canvas object;
    public float defaultAlpha = 0.5F, activeAlpha = 1.0F;       //Alpha values;
    public bool Interactable = true;                            //Is button interactable or not;
    public bool Dynamic;                                        //Is button dynamic (moving with touch) or not;

    private bool isPressed, toggle, clicked, released;
    private CanvasGroup canvasGroup;
    private EventTrigger eventTrigger;
    private Vector2 touchInput, prevDelta, dragInput;
    private Vector2 defaultPos, targetPos;
    private RectTransform rect;

    void Start()
    {
        canvasGroup = GetComponent<CanvasGroup>();
        canvasGroup.alpha = defaultAlpha;
        rect = GetComponent<RectTransform>();
        defaultPos = rect.anchoredPosition;
        SetupListeners();
    }

    // Update is called once per frame
    void Update()
    {
        touchInput = (dragInput - prevDelta) / Time.deltaTime;
        prevDelta = dragInput;

        if (isPressed)
        {
            if (Interactable)
                canvasGroup.alpha = activeAlpha;
        }
        else
            canvasGroup.alpha = defaultAlpha;

        if(Dynamic)
        {
            if (isPressed)
            {
                RectTransformUtility.ScreenPointToLocalPointInRectangle(myCanvas.transform as RectTransform, dragInput,
                    myCanvas.worldCamera, out targetPos);

                rect.position = myCanvas.transform.TransformPoint(targetPos);
            }
            else
                rect.anchoredPosition = defaultPos;
        }
    }

    //Setup events listeners;
    void SetupListeners()
    {
        eventTrigger = gameObject.GetComponent<EventTrigger>();

        var a = new EventTrigger.TriggerEvent();
        a.AddListener(data =>
        {
            var evData = (PointerEventData)data;
            data.Use();
            isPressed = true;
            toggle = !toggle;
            prevDelta = dragInput = evData.position;
            StartCoroutine("WasClicked");
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
            StartCoroutine("WasReleased");
        });

        eventTrigger.triggers.Add(new EventTrigger.Entry { callback = c, eventID = EventTriggerType.PointerUp });
    }

    IEnumerator WasClicked()
    {
        clicked = true;
        yield return null;
        clicked = false;
    }

    IEnumerator WasReleased()
    {
        released = true;
        yield return null;
        released = false;
    }

    //Returns button drag vector;
    public Vector2 MoveInput()
    {
        return touchInput * Time.deltaTime;
    }

    //Returns whether or not button is pressed
    public bool IsPressed()
    {
        return isPressed;
    }

    //Fires once when button pressed
    public bool OnPress()
    {
        return clicked;
    }

    //Fires once when button released
    public bool OnRelease()
    {
        return released;
    }

    //Returns boolean as toggle
    public bool Toggle()
    {
        return toggle;
    }
}

