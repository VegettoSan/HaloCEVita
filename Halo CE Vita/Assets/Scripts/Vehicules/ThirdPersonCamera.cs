using UnityEngine;

public class ThirdPersonCamera : MonoBehaviour
{
    public Transform player; // Referencia al jugador
    public float cameraDistance = 5f; // Distancia de la cámara al jugador
    public float cameraHeight = 2f; // Altura de la cámara respecto al jugador
    public float cameraAngle = 45f; // Ángulo de la cámara respecto al suelo
    public float mouseSensitivity = 3f; // Sensibilidad del mouse

    private Vector3 cameraOffset; // Distancia inicial de la cámara al jugador

    void Start()
    {
        cameraOffset = new Vector3(0f, cameraHeight, -cameraDistance); // Calculamos la distancia inicial de la cámara
        transform.position = player.position + cameraOffset; // Movemos la cámara a su posición inicial
        transform.LookAt(player.position); // Hacemos que la cámara mire hacia el jugador
    }

    void LateUpdate()
    {
        float mouseX = Input.GetAxis("Mouse X") * mouseSensitivity; // Leemos el movimiento del mouse en el eje X
        float mouseY = Input.GetAxis("Mouse Y") * mouseSensitivity; // Leemos el movimiento del mouse en el eje Y

        // Rotamos la cámara según el movimiento del mouse
        transform.RotateAround(player.position, Vector3.up, mouseX);
        transform.RotateAround(player.position, transform.right, -mouseY);

        // Calculamos la posición de la cámara según la distancia, altura y ángulo configurados
        Vector3 cameraPosition = player.position + Quaternion.Euler(cameraAngle, transform.eulerAngles.y, 0f) * cameraOffset;

        // Comprobamos si hay un objeto en medio de la cámara y el jugador
        RaycastHit hit;
        if (Physics.Linecast(player.position, cameraPosition, out hit))
        {
            cameraPosition = hit.point + (hit.normal * 0.2f); // Ajustamos la posición de la cámara para evitar el objeto
        }

        transform.position = cameraPosition; // Movemos la cámara a su nueva posición
        transform.LookAt(player.position); // Hacemos que la cámara mire hacia el jugador
    }
}