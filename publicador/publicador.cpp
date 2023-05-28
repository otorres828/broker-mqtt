#include <iostream>
#include <cstring>
#include <MQTTAsync.h>

#define QOS 1

std::string topic,payload;

void onConnect(void* context, MQTTAsync_successData* response) {
    std::cout << "Conexión exitosa" << std::endl;
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    pubmsg.payload = (void*) payload.c_str();
    pubmsg.payloadlen = payload.size();
    pubmsg.qos = 0;
    pubmsg.retained = 0;
    MQTTAsync_sendMessage(client, topic.c_str(), &pubmsg, NULL);
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    std::cerr << "Error al conectar al broker: " << response->message << std::endl;
}


int main(int argc, char* argv[]) {
    std::string host;
    int port;

    // Solicitar la entrada del usuario para el host, puerto y tópico
    std::cout << "Ingrese el host: ";
    std::cin >> host;
    std::cout << "Ingrese el puerto: ";
    std::cin >> port;
    std::cout << "Ingrese el tópico: ";
    std::cin >> topic;
    std::cout << "Ingrese el mensaje o payload: ";
    std::cin.ignore(); // Descarta cualquier caracter adicional en el búfer de entrada
    std::getline(std::cin, payload);
    std::cout << payload << std::endl;
    // Configurar el cliente MQTT y el broker MQTT
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_create(&client, ("tcp://" + host + ":" + std::to_string(port)).c_str(), "conectarse_broker", MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;
    int rc;
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        std::cerr << "Error al iniciar la conexión al broker: " << MQTTAsync_strerror(rc) << std::endl;
        MQTTAsync_destroy(&client);
        return 1;
    }

    MQTTAsync_subscribe(client, topic.c_str(), 0, NULL);

    // Esperar a que lleguen mensajes
    while (1) {
        // ...
    }

    MQTTAsync_disconnect(client, NULL);
    MQTTAsync_destroy(&client);
    return 0;
}