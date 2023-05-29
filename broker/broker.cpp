#include <iostream>
#include <string>
#include <map>
#include <sys/socket.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
#include <thread>

using namespace std;

// Enumeración para los diferentes tipos de paquetes MQTT
enum class TipoDePaquete : uint8_t {
    CONNECT = 0x01,
    CONNACK = 0x02,
    PUBLISH = 0x03,
    PUBACK = 0x04,
    PUBREC = 0x05,
    PUBCOMP = 0x07,
    SUBSCRIBE = 0x08,
    SUBACK = 0x09,
    UNSUBSCRIBE = 0x0A,
    UNSUBACK = 0x0B,
    DISCONNECT = 0x0E
};

// Estructura para almacenar información sobre un cliente
struct ClientInfo {
    int socket_fd;
    bool conectado;
    string client_id;
    vector<string> subscriptions;
};

// Tabla hash para almacenar información sobre los clientes
unordered_map<int, ClientInfo> clients;

// Tabla hash para almacenar mensajes retenidos
unordered_map<string, string> retained_messages;

// Función para publicar un mensaje a todos los clientes suscritos a un topic
void publish(const string& topic, const string& message) {
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        int fd = it->first;
        ClientInfo& info = it->second;
        if (info.conectado) {
            for (const auto& subscription : info.subscriptions) {
                if (subscription == topic) {
                    uint8_t header = static_cast<uint8_t>(TipoDePaquete::PUBLISH) << 4 | 0x00;
                    uint8_t remaining_length = 2 + topic.size() + message.size();
                    if (remaining_length <= 127) {
                        std::cout << "Enviando paquete PUBLISH al cliente " << fd << " en el topic " << topic << " con el mensaje " << message << endl;
                        uint8_t buffer[4];
                        buffer[0] = header;
                        buffer[1] = remaining_length;
                        buffer[2] = static_cast<uint8_t>(topic.size() >> 8);
                        buffer[3] = topic.size() & 0xFF;
                        send(fd, buffer, 4, 0);
                        send(fd, topic.data(), topic.size(), 0);
                        send(fd, message.data(), message.size(), 0);
                    }
                    break;
                }
            }
        } else {
            std::cout << "Cliente no encontrado" << std::endl;
        }
    }
}

// Función para manejar un mensaje recibido de un cliente
void manejar_paquete(int cliente_id, TipoDePaquete type, const uint8_t* data, size_t size) {
    switch (type) {
        case TipoDePaquete::CONNECT: {
            std::cout<<"Un cliente se conecto"<<std::endl;
            // Parsear el paquete CONNECT
            uint16_t protocol_name_length = (data[0] << 8) | data[1];
            string protocol_name(reinterpret_cast<const char*>(data + 2), protocol_name_length);
            uint8_t protocol_level = data[2 + protocol_name_length];
            uint8_t connect_flags = data[2 + protocol_name_length + 1];
            uint16_t keep_alive = (data[2 + protocol_name_length + 2] << 8) | data[2 + protocol_name_length + 3];
            uint16_t client_id_length = (data[2 + protocol_name_length + 4] << 8) | data[2 + protocol_name_length + 5];
            string client_id(reinterpret_cast<const char*>(data + 2 + protocol_name_length + 6), client_id_length);
            // Comprobar si el ID de cliente ya está en uso
            if (clients.count(cliente_id) > 0 ) {
                // descoenctar el cliente
                uint8_t buffer[2];
                buffer[0] = static_cast<uint8_t>(TipoDePaquete::CONNACK) << 4;
                buffer[1] = 0x02;
                send(cliente_id, buffer, 2, 0);
                close(cliente_id);
                clients.erase(cliente_id);
                return;
            }
            // Guardar el id del cliente
            clients[cliente_id].socket_fd = cliente_id;
            clients[cliente_id].conectado = true;
            clients[cliente_id].client_id = client_id;
            // enviar un CONNACK
            uint8_t buffer[4];
            buffer[0] = static_cast<uint8_t>(TipoDePaquete::CONNACK) << 4;
            buffer[1] = 0x02;
            buffer[2] = 0x00;
            buffer[3] = 0x00;
            send(cliente_id, buffer, 4, 0);
            // los mensajes retenidos
            // for (auto it = retained_messages.begin(); it != retained_messages.end(); ++it) {
            //     const std::string& topic = it->first;
            //     publish(topic, it->second);            
            // }
            break;
        }
        case TipoDePaquete::PUBLISH: {
            std::cout<<"este paquete es de un publicador"<<std::endl;
            // Parsear el paquete PUBLISH
            uint16_t topic_length = (data[0] << 8) | data[1];
            string topic(reinterpret_cast<const char*>(data + 2), topic_length);
            if ((data[0] & 0x06) == 0x02) {
                // QoS 1 PUBLISH packet, send a PUBACK packet
                uint16_t packet_id = (data[2 + topic_length] << 8) | data[2 + topic_length + 1];
                uint8_t buffer[4];
                buffer[0] = static_cast<uint8_t>(TipoDePaquete::PUBACK) << 4;
                buffer[1] = 0x02;
                buffer[2] = packet_id >> 8;
                buffer[3] = packet_id & 0xFF;
                send(cliente_id, buffer, 4, 0);
            } 
            // Publicar el mensaje a todos los clientes suscritos al tema
            uint16_t message_length = size - 2 - topic_length;
            string message(reinterpret_cast<const char*>(data + 2 + topic_length), message_length);
            publish(topic, message);
            // Retain the message if the retain flag is set
            // if ((data[0] & 0x01) == 0x01) {
            //     retained_messages[topic] = message;
            // } else {
            //     retained_messages.erase(topic);
            // }
            break;
        }
        case TipoDePaquete::SUBSCRIBE: {
            std::cout<<"Un cliente se suscribio"<<std::endl;
            // Parsear el paquete SUBSCRIBE
            uint16_t packet_id = (data[0] << 8) | data[1];
            size_t index = 2;
            vector<pair<string, uint8_t>> subscriptions;
            while (index < size) {
                uint16_t topic_length = (data[index] << 8) | data[index + 1];
                string topic(reinterpret_cast<const char*>(data + index + 2), topic_length);
                uint8_t qos = data[index + 2 + topic_length];
                subscriptions.emplace_back(topic, qos);
                index += 3 + topic_length;
            }
            // Añadir las suscripciones al cliente
            for (auto it = subscriptions.begin(); it != subscriptions.end(); ++it) {
                const std::string& topic = it->first;
                auto& subscriptions = clients[cliente_id].subscriptions;
                if (find(subscriptions.begin(), subscriptions.end(), topic) == subscriptions.end()) {
                    subscriptions.push_back(topic);
                }
            }
            // Enviar un SUBACK 
            uint8_t buffer[3 + subscriptions.size()];
            buffer[0] = static_cast<uint8_t>(TipoDePaquete::SUBACK) << 4;
            buffer[1] = 2 + subscriptions.size();
            buffer[2] = packet_id >> 8;
            buffer[3] = packet_id & 0xFF;
            for (size_t i = 0; i < subscriptions.size(); i++) {
                buffer[4 + i] = subscriptions[i].second;
            }
            send(cliente_id, buffer, sizeof(buffer), 0);
            break;
        }
        case TipoDePaquete::UNSUBSCRIBE: {
             std::cout<<"Un cliente se des-suscribio"<<std::endl;
            // Parsear el paquete UNSUBSCRIBE
            uint16_t packet_id = (data[0] << 8) | data[1];
            size_t index = 2;
            vector<string> topics;
            while (index < size) {
                uint16_t topic_length = (data[index] << 8) | data[index + 1];
                string topic(reinterpret_cast<const char*>(data + index + 2), topic_length);
                topics.push_back(topic);
                index += 2 + topic_length;
            }
            // Eliminar las suscripciones del cliente
            for (auto& topic : topics) {
                auto& subscriptions = clients[cliente_id].subscriptions;
                subscriptions.erase(remove(subscriptions.begin(), subscriptions.end(), topic), subscriptions.end());
            }
            // Enviar un paquete UNSUBACK
            uint8_t buffer[2];
            buffer[0] = static_cast<uint8_t>(TipoDePaquete::UNSUBACK) << 4;
            buffer[1] = 0x02;
            buffer[2] = packet_id >> 8;
            buffer[3] = packet_id & 0xFF;
            send(cliente_id, buffer, 4, 0);
            break;
        }
        case TipoDePaquete::DISCONNECT: {
            std::cout<<"Un cliente se Desconecto"<<std::endl;
            // Cliente desconectado
            close(cliente_id);
            clients.erase(cliente_id);
            break;
        }
        default:
            // Tipo de paquete no válido, desconecte el cliente
            std::cout<<"No se pudo conectar, cliente desconectado"<<std::endl;
            close(cliente_id);
            clients.erase(cliente_id);
            break;
    }
}

void manejar_cliente(int cliente_id) {
    // Receive packets from the client
    uint8_t buffer[4096];
    size_t buffer_size = 0;
    while (true) {
        ssize_t bytes_received = recv(cliente_id, buffer + buffer_size, sizeof(buffer) - buffer_size, 0);
        if (bytes_received == -1) {
            cerr << "fallo al recibir los datos" << endl;
            break;
        } else if (bytes_received == 0) {
            // Cliente desconectado
            close(cliente_id);
            clients.erase(cliente_id);
            break;
        }
        buffer_size += bytes_received;
        // Parsear el paquete del buffer
        while (buffer_size > 0) {
            uint8_t fixed_header = buffer[0];
            uint8_t packet_type = fixed_header >> 4;
            uint32_t remaining_length = 0;
            size_t multiplier = 1;
            size_t i = 1;
            while ((buffer[i] & 0x80) != 0) {
                remaining_length += (buffer[i] & 0x7F) * multiplier;
                multiplier *= 128;
                i++;
            }
            remaining_length += (buffer[i] & 0x7F) * multiplier;
            if (buffer_size < i + 1 + remaining_length) {
                // Incomplete packet, wait for more data
                break;
            }
            // Handle the packet
            manejar_paquete(cliente_id, static_cast<TipoDePaquete>(packet_type), buffer + i + 1, remaining_length);
            // Remove the packet from the buffer
            memmove(buffer, buffer + i + 1 + remaining_length, buffer_size - i - 1 - remaining_length);
            buffer_size -= i + 1 + remaining_length;
        }
    }
}

int main() {
    // Crear un socket TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cerr << "fallo al crear el socket" << endl;
        return 1;
    }
    int port;
    std::cout << "Ingrese el puerto: ";
    std::cin >> port;

    // BROKER CONECTADO EN EL PUERTO
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        cerr << "Fallo al enlazar socket" << endl;
        close(server_fd);
        return 1;
    }
    // Escucha de conexiones entrantes
    if (listen(server_fd, SOMAXCONN) == -1) {
        cerr << "Fallo al escuchar el socket" << endl;
        close(server_fd);
        return 1;
    }
    std::mutex mutex; // Declarar un objeto mutex para proteger el acceso a la lista de clientes

    while (true) {
        // Esperar la conexion del cliente
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        int cliente_id = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
        if (cliente_id == -1) {
            cerr << "No se ha podido aceptar la conexión" << endl;
            continue;
        }
        // Proteger el acceso a la lista de clientes
        std::lock_guard<std::mutex> lock(mutex);

        //Crear un hilo para gestionar el cliente
        thread t(manejar_cliente,cliente_id);
        t.detach(); // Separar el hilo para que pueda funcionar de forma independiente
    }
    // Cerrar el socket del servidor
    close(server_fd);
    return 0;
}
