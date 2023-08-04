using UnityEngine;

public class ThirdPersonCamera : MonoBehaviour
{
    public Transform player; // Referencia al jugador
    public float cameraDistance = 5f; // Distancia de la c�mara al jugador
    public float cameraHeight = 2f; // Altura de la c�mara respecto al jugador
    public float cameraAngle = 45f; // �ngulo de la c�mara respecto al suelo
    public float mouseSensitivity = 3f; // Sensibilidad del mouse

    private Vector3 cameraOffset; // Distancia inicial de la c�mara al jugador

    void Start()
    {
        cameraOffset = new Vector3(0f, cameraHeight, -cameraDistance); // Calculamos la distancia inicial de la c�mara
        transform.position = player.position + cameraOffset; // Movemos la c�mara a su posici�n inicial
        transform.LookAt(player.position); // Hacemos que la c�mara mire hacia el jugador
    }

    void LateUpdate()
    {
        float mouseX = Input.GetAxis("Mouse X") * mouseSensitivity; // Leemos el movimiento del mouse en el eje X
        float mouseY = Input.GetAxis("Mouse Y") * mouseSensitivity; // Leemos el movimiento del mouse en el eje Y

        // Rotamos la c�mara seg�n el movimiento del mouse
        transform.RotateAround(player.position, Vector3.up, mouseX);
        transform.RotateAround(player.position, transform.right, -mouseY);

        // Calculamos la posici�n de la c�mara seg�n la distancia, altura y �ngulo configurados
        Vector3 cameraPosition = player.position + Quaternion.Euler(cameraAngle, transform.eulerAngles.y, 0f) * cameraOffset;

        // Comprobamos si hay un objeto en medio de la c�mara y el jugador
        RaycastHit hit;
        if (Physics.Linecast(player.position, cameraPosition, out hit))
        {
            cameraPosition = hit.point + (hit.normal * 0.2f); // Ajustamos la posici�n de la c�mara para evitar el objeto
        }

        transform.position = cameraPosition; // Movemos la c�mara a su nueva posici�n
        transform.LookAt(player.position); // Hacemos que la c�mara mire hacia el jugador
    }
}