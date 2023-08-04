using UnityEngine;
using UnityEditor;
using UnityEngine.UI;
using System.Collections;

[CustomEditor(typeof(FP_Controller))]
public class FP_Player_Inspector : Editor {

    private FP_Controller fP_Controller;
    MonoScript cntrlr;

    void OnEnable()
    {
        fP_Controller = (FP_Controller)target;
        cntrlr = MonoScript.FromMonoBehaviour((FP_Controller)target);
    }

    public override void OnInspectorGUI()
    {
        this.serializedObject.Update();
        Undo.RecordObject(fP_Controller, "Undo");

        cntrlr = (MonoScript)EditorGUILayout.ObjectField("Script", cntrlr, typeof(FP_Controller), false);
        fP_Controller.canControl = EditorGUILayout.Toggle("Can Control", fP_Controller.canControl);
        fP_Controller.walkSpeed = EditorGUILayout.FloatField("Walk Speed", fP_Controller.walkSpeed);
        fP_Controller.gravity = EditorGUILayout.FloatField("Gravity", fP_Controller.gravity);
        fP_Controller.airControl = EditorGUILayout.Toggle("Air Control", fP_Controller.airControl);

        EditorGUILayout.BeginHorizontal();

        fP_Controller.canRun = EditorGUILayout.ToggleLeft("Run", fP_Controller.canRun, GUILayout.Width(70));
        if(fP_Controller.canRun)
        {
            GUILayout.Label("Speed", GUILayout.Width(42.5F));
            fP_Controller.runSpeed = EditorGUILayout.FloatField("", fP_Controller.runSpeed, GUILayout.Width(50));
            EditorGUILayout.PropertyField(this.serializedObject.FindProperty("runKey"), GUIContent.none);
        }

        EditorGUILayout.EndHorizontal();

        EditorGUILayout.BeginHorizontal();

        fP_Controller.canJump = EditorGUILayout.ToggleLeft("Jump", fP_Controller.canJump, GUILayout.Width(70));
        if (fP_Controller.canJump)
        {
            GUILayout.Label("Force", GUILayout.Width(42.5F));
            fP_Controller.jumpForce = EditorGUILayout.FloatField("", fP_Controller.jumpForce, GUILayout.Width(50));
            EditorGUILayout.PropertyField(this.serializedObject.FindProperty("jumpKey"), GUIContent.none);
        }

        EditorGUILayout.EndHorizontal();

        EditorGUILayout.BeginHorizontal();

        fP_Controller.canCrouch = EditorGUILayout.ToggleLeft("Crouch", fP_Controller.canCrouch, GUILayout.Width(70));
        if (fP_Controller.canCrouch)
        {
            GUILayout.Label("Speed", GUILayout.Width(42.5F));
            fP_Controller.crouchSpeed = EditorGUILayout.FloatField("", fP_Controller.crouchSpeed, GUILayout.Width(50));
            EditorGUILayout.PropertyField(this.serializedObject.FindProperty("crouchKey"), GUIContent.none);
        }

        EditorGUILayout.EndHorizontal();

        if (fP_Controller.canCrouch)
        {
            fP_Controller.crouchHeight = EditorGUILayout.FloatField("Height", fP_Controller.crouchHeight);
        }

        this.serializedObject.ApplyModifiedProperties();
        if (GUI.changed)
            EditorUtility.SetDirty(fP_Controller);
    }

}
