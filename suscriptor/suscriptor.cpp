#include <iostream>
#include <cstring>
#include <MQTTAsync.h>

#define QOS         1

volatile MQTTAsync_token delivered_token;

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Mensaje recibido en el tema: " << topicName << std::endl;
    std::cout << "Contenido: " << std::string((char*)message->payload, message->payloadlen) << std::endl;
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onConnect(void* context, MQTTAsync_successData* response) {
    std::cout << "Conexión exitosa" << std::endl;
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_subscribe(client, "bombillo", QOS, NULL);
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    std::cout << "Error al conectar al broker: " << response->message << std::endl;
}

int main(int argc, char* argv[]) {
    std::string host;
    int port;
    std::string topic;

    bool connected = false;
    while (!connected) {
        // Solicitar la entrada del usuario para el host, puerto y tópico
        std::cout << "Ingrese el host: ";
        std::cin >> host;
        std::cout << "Ingrese el puerto: ";
        std::cin >> port;
        std::cout << "Ingrese el tópico: ";
        std::cin >> topic;

        // Configurar el cliente MQTT y el broker MQTT
        MQTTAsync client;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        MQTTAsync_create(&client, ("tcp://" + host + ":" + std::to_string(port)).c_str(), "broker_example", MQTTCLIENT_PERSISTENCE_NONE, NULL);
        MQTTAsync_setCallbacks(client, NULL, NULL, messageArrived, NULL);
        conn_opts.onSuccess = onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = client;

        int rc;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
            std::cout << "Error al iniciar la conexión al broker: " << MQTTAsync_strerror(rc) << std::endl;
            MQTTAsync_destroy(&client);
        } else {
            connected = true;
            while (1) {
                // Mantener el programa en ejecución
            }
            MQTTAsync_destroy(&client);
        }
    }

    return 0;
}