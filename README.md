VPN
===
VPN stands for Virtual Private Network. This technology is used to create private virtual network connections across existing public network connections.

Our project was developed as a final work for the Technopark’s course «C/C++ preparatory program» (Подготовительный курс С/C++).
The project consists of two parts – client and server. The client sends requests to the server: to create a new network, to connect to the network, to send messages to other clients. The server receives these requests and acts accordingly. The process of message redirecting between the clients is carried out by the means of virtual TAP interfaces connected through a server-generated bridge. Сommand processing occurs in an asynchronous mode by using libevent library.

Installing
----------
    git clone https://github.com/theantialex/VPN.git
    cd vpn
    make

Running
-------
Server launching

    sudo ./server.out
Network creating (client side)

    sudo ./client.out create <network_name> <network_password> <server_ip_address>
Client connection

    sudo ./client.out connect <network_name> <network_password> <server_ip_address>
Client disconnection

    sudo ./client.out leave <network_name> <network_password> <server_ip_address>
Deleting network (client side)

    sudo ./client.out delete <network_name> <network_password> <server_ip_address>

Example with two clients communicating via netcat
-------------------------------------------------
When the server is running, network is created and two clients are connected to that network, you can use netcat for message exchange.

To start messaging

    netcat <other_client_ip_given_by_server> <port_number>
To wait for messages

    netcat -l -k <port_number>

Authors
-------
Anna Bushe - @AnnaBushe (Git & Telegram)

Alexandra Volynets - @TheAntiAlex (Git & Telegram)
