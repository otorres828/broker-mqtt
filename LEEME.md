# BROKER MQTT

Este proyecto fue realizado por Oliver Torres, en el Mayo, 2023. Proyecto realizado para la materia de Sistemas Distribuidos en la universidad Catolica Andres Bello, Venezuela - Ciudad Guayana

# Descripcion

El proyecto corresponde a un broker mqtt realizado con sockets y dos clientes (publicadores y suscriptores) realizado con la libreria  MQTTAsync.h de **Paho Mqtt**. 

El proyecto fue realizado en un entorno Linux.

## GUIA INICIAL

Para el uso de los clientes publicador y suscriptor, es necesario tener instalado la libreria de **[Paho Mqtt](https://github.com/eclipse/paho.mqtt.cpp)**

## Software de terceros

Para probar los clientes y suscriptores puedes hacer uso de un broker de terceros como [hivemq](https://www.mqtt-dashboard.com/)
host: broker.hivemq.com
puerto: 1883

Para probar el broker puedes utilizar Node-Red. Para instalar node-red debes de asegurarte de tener node js y npm isntalado en tu sistema. Ejecuta los siguientes comandos

```
sudo apt update
sudo apt install nodejs
//para verificar que tienes instalado node correctamente
node -v
```
Ahora instalamos npm
```
sudo apt install npm
//para verificar que tienes instalado node correctamente
npm -v
```
Instalamos node-red
```
sudo npm install -g --unsafe-perm node-red
```
Una vez tengas instalado node -red lo ejecutamos
```
node-red
```
## Compilación y Ejecución del Suscriptor

Una vez hayas descargado o clonado el repositorio. Parate en la raiz de la carpeta principal y ejecuta los siguientes comandos en la terminal.

    cd publicador
   si quiere volver a compilar

      g++ -o publicador publicador.cpp -lpaho-mqtt3a

Ejecutas el programa

	./publicador

Una vez hayas ejecutado el publicador te pedira el host, puerto, topic y payload.

## Compilación y Ejecución del Suscriptor

Ejecuta los siguientes comandos en la terminal.

    cd suscriptor
   si quiere volver a compilar

    g++ -o suscriptor suscriptor.cpp -lpaho-mqtt3a

Ejecutas el programa

	./suscriptor

   Una vez ejecutado el suscriptor te pedira el host y puerto al que te quieres conectar.
   
## Compilación y Ejecución del Broker
Ejecuta los siguientes comandos en la terminal.

    cd broker
   si quiere volver a compilar

    g++ broker.cpp -o broker -pthread


Ejecutas el programa

	./broker

   Una vez ejecutado el broker te pedira el puerto al que deseas conectarte. Es importante recordar que el broker se esta ejecutando en **localhost**

