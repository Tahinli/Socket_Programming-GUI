#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
//Thread Values
int cConnection = !0;
int cRecv = !0;
int cSend = !0;
pthread_t connectionThreadID;
pthread_t recvThreadID;
pthread_t sendThreadID;
int connectionStatus;
socklen_t conlen = sizeof(connectionStatus);
//Client Window Decleration
        GtkWidget *cWindow;
        GtkWidget *cPane;
        GtkWidget *btConnect;
        GtkWidget *ipEntry;
        GtkWidget *portEntry;

//Client Chat Window Decleration
        GtkWidget *cOrderWindow;
        GtkWidget *cOrderPane;
        GtkWidget *cOrderRecvScrollableWindow;
        GtkWidget *cOrderSendScrollableWindow;
        GtkWidget *cOrderRecvTextView;
        GtkWidget *cOrderSendTextView;
//Socket Decleration
        int clientSocket;

//Funcs
int connectToServer(char ipAddress[], char portNumber[]);
int socketCloser(int aloneSocket);
int cCleaner(int aloneSocket);

//Threads
void *recvThread(void *vargp)
    {
        printf("I'm in recv thread\n");
        char recvBuffer[512];
        while(cRecv!=0)
            {
                //g_print("in recv\n");
                if(0>recv(clientSocket, recvBuffer, strlen(recvBuffer), 0))
                    {
                        perror("Recv Failed");
                    }
                else
                    {
                        printf("%s",recvBuffer);
                    }
                
            }
        pthread_exit(recvThread);
        printf("recvThread is broken\n");
    }
void *sendThread(void *vargp)
    {
        printf("I'm in send thread\n");
        int exit = 0;
        char sendBuffer[512];
        while(cSend!=0)
            {
                //printf("in send\n");
                fgets(sendBuffer, 512, stdin);
                if(0>send(clientSocket, sendBuffer, strlen(sendBuffer), 0))
                    {
                        perror("Send Failed");
                    }
            } 
        pthread_exit(sendThread);
        printf("sendThread is broken\n");
    }
void *connectionThread(void *vargp)
    {
        printf("I'm in Connection Thread \n");
        char ipAddress[15];
        char portNumber[12];
        strncpy(ipAddress, gtk_entry_get_text(GTK_ENTRY(ipEntry)),15);
        strncpy(portNumber, gtk_entry_get_text(GTK_ENTRY(portEntry)),12);
        //Socket Setup
        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        printf("Client Socket is = %d\n", clientSocket);
        if(0>clientSocket)
            {   
                printf("ERRNO = %d:%s\n", errno, strerror(errno));
                printf("Error ID = %d\n", clientSocket);
                perror("Socket Setup Failed");
                socketCloser(clientSocket);
                //return -1;
            }
        ipAddress[strlen(ipAddress)+1]='\0';
        portNumber[strlen(portNumber)+1]='\0';

        //Connection Information //93.190.8.248
        struct sockaddr_in server_Socket;
        server_Socket.sin_family = AF_INET;
        server_Socket.sin_addr.s_addr = inet_aton(ipAddress, (struct in_addr*)&server_Socket.sin_addr.s_addr);
        server_Socket.sin_port = htons(atoi(portNumber));
        socklen_t server_Socket_len = sizeof(server_Socket);
        
        if(0>inet_pton(AF_INET, ipAddress, &server_Socket.sin_addr.s_addr))
            {
                perror("Undefined IP Address");
                socketCloser(clientSocket);
                //return -1;
            }
        printf("IP is = %s and len = [%d]\n", ipAddress, strlen(ipAddress));
        printf("Port is = %s and len = [%d]\n", portNumber, strlen(portNumber));
        if(0>connect(clientSocket, (struct sockaddr*)&server_Socket, server_Socket_len))
            {
                perror("Connection Error");
                socketCloser(clientSocket);
                //return -1;
            }
        else
            {
                printf("Connected\n");
            }
        //Multithread
        cRecv = !0;
        cSend = !0;
        pthread_create(&recvThreadID, NULL, recvThread, NULL);
        pthread_create(&sendThreadID, NULL, sendThread, NULL);
        while(cConnection!=0)
            {
                //printf("%d\n",getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, &connectionStatus, &conlen));
                if(getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, &connectionStatus, &conlen))
                    {
                        perror("Connection is Corrupted");
                        cCleaner(clientSocket);
                    }
                
            }
    }
//Button Events
void cOrderWindowCloser(GtkWidget *widget, gpointer data)
    {
        //gtk_window_close(GTK_WINDOW(cOrderWindow));
        gtk_widget_destroy(GTK_WIDGET(cOrderWindow));
        if(!cCleaner(clientSocket))
            {
                printf("Cleaned Well\n");
            }
            
    }
void btConnectClick(GtkWidget *widget, gpointer data)
    {
        g_print("Connection Order\n");
        cConnection = !0;
        pthread_create(&connectionThreadID, NULL, connectionThread, NULL);

        //Chat Window Definition
        cOrderWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        cOrderPane = gtk_fixed_new();
        cOrderRecvScrollableWindow = gtk_scrolled_window_new(NULL, NULL);
        cOrderSendScrollableWindow = gtk_scrolled_window_new(NULL, NULL);
        cOrderRecvTextView = gtk_text_view_new();
        cOrderSendTextView = gtk_text_view_new();

        //Customization
            //Window
        gtk_window_set_position(GTK_WINDOW(cOrderWindow), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size(GTK_WINDOW(cOrderWindow), 400, 600);
        gtk_window_set_title(GTK_WINDOW(cOrderWindow), "Tahinli's Client");
        gtk_window_set_resizable(GTK_WINDOW(cOrderWindow), FALSE);
            //TextView
        gtk_widget_set_size_request(GTK_WIDGET(cOrderRecvScrollableWindow), 300, 300);
        gtk_widget_set_size_request(GTK_WIDGET(cOrderSendScrollableWindow), 300, 100);

        //Pane Set
        gtk_fixed_put(GTK_FIXED(cOrderPane), cOrderRecvScrollableWindow, 10, 10);
        gtk_fixed_put(GTK_FIXED(cOrderPane), cOrderSendScrollableWindow, 10, 400);


        //Epilogue
        gtk_container_add(GTK_CONTAINER(cOrderRecvScrollableWindow), cOrderRecvTextView);
        gtk_container_add(GTK_CONTAINER(cOrderSendScrollableWindow), cOrderSendTextView);
        gtk_container_add(GTK_CONTAINER(cOrderWindow), cOrderPane);
        gtk_widget_show_all(cOrderWindow);
        gtk_widget_hide(cWindow);
        gtk_widget_set_sensitive(GTK_WIDGET(btConnect), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(ipEntry), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(portEntry), FALSE);

        //Close Event
        g_signal_connect(G_OBJECT(cOrderWindow), "destroy",G_CALLBACK(cOrderWindowCloser), NULL);



    }

void btClientClick(GtkWidget *widget, gpointer data)
    {
        printf("Client Selected\n");
        
        
        //Definition
        cWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        cPane = gtk_fixed_new();
        btConnect = gtk_button_new_with_label("Connect");
        ipEntry = gtk_entry_new();
        portEntry = gtk_entry_new();
        //Pane-Set
        gtk_fixed_put(GTK_FIXED(cPane), btConnect, 300, 10);
        gtk_fixed_put(GTK_FIXED(cPane), ipEntry, 20, 50);
        gtk_fixed_put(GTK_FIXED(cPane), portEntry, 200, 50);

        
        //Customization
            //Window
        gtk_window_set_position(GTK_WINDOW(cWindow), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size(GTK_WINDOW(cWindow), 400, 100);
        gtk_window_set_title(GTK_WINDOW(cWindow), "Tahinli's Client");
        gtk_window_set_resizable(GTK_WINDOW(cWindow), FALSE);
            //Entry
        gtk_widget_set_size_request(GTK_WIDGET(ipEntry), 50, 10);
        gtk_widget_set_size_request(GTK_WIDGET(portEntry), 40, 10);
        gtk_entry_set_max_length(GTK_ENTRY(ipEntry), 15);
        gtk_entry_set_max_length(GTK_ENTRY(portEntry), 5);
        gtk_entry_set_placeholder_text(GTK_ENTRY(ipEntry), "IP Address");
        gtk_entry_set_placeholder_text(GTK_ENTRY(portEntry), "Port Number");
            //Button
        gtk_widget_set_can_focus(GTK_WIDGET(btConnect), TRUE);
        gtk_widget_grab_focus(GTK_WIDGET(btConnect));

            //Button-Event
        g_signal_connect(btConnect,"clicked", G_CALLBACK(btConnectClick), NULL);

        //Epilogue
        gtk_container_add(GTK_CONTAINER(cWindow), cPane);
        gtk_widget_show_all(cWindow);
    }
void btServerClick(GtkWidget *widget, gpointer data)
    {
        printf("Server Selected\n");
        //Decleration
        GtkWidget *sWindow;
        GtkWidget *sPane;
        //Definition
        sWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        sPane = gtk_fixed_new();
        //Epilogue
        gtk_container_add(GTK_CONTAINER(sWindow), sPane);
        gtk_widget_show_all(sWindow);
    }



int main(int argc, char **argv)
    {
        printf("Hello World\n");


        gtk_init(&argc,&argv);
        //


        //Declerations------------
        GtkWidget *window;
        GtkWidget *pane;
        GtkWidget *btClient;
        GtkWidget *btServer;

        //Definitions-------------
        window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
        pane = gtk_fixed_new();
        btClient = gtk_button_new_with_label("Client");
        btServer = gtk_button_new_with_label("Server");

        //Pane Set----------------
        gtk_fixed_put(GTK_FIXED(pane), btClient, 65, 60);
        gtk_fixed_put(GTK_FIXED(pane), btServer, 65, 100);


        //Customization----------

            //Window
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
        gtk_window_set_title(GTK_WINDOW(window), "Tahinli's Network");
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

            //Buttons
        gtk_widget_set_size_request(GTK_WIDGET(btClient), 80, 30);
        gtk_widget_set_size_request(GTK_WIDGET(btServer), 80, 30);


        //Click-Event_Button------
        g_signal_connect(btClient, "clicked", G_CALLBACK(btClientClick), NULL);
        g_signal_connect(btServer, "clicked", G_CALLBACK(btServerClick), NULL);






        //Epilogue----------------
        g_signal_connect(window, "destroy",G_CALLBACK(gtk_main_quit), NULL);
        gtk_container_add(GTK_CONTAINER(window), pane);
        gtk_widget_show_all(window);
        gtk_main();




        return 0;
    }


int connectToServer(char ipAddress[], char portNumber[])
    {
        return 0;
    }
int socketCloser(int aloneSocket)
    {
        if(0>close(aloneSocket))
            {
                perror("Socket Closing Error");
                return -1;
            }
        return 0;
    }
int cCleaner(int aloneSocket)
    {
        cConnection = 0;
        cRecv = 0;
        cSend = 0;
        printf("cRecv is = %d, cSend is = %d\n",cRecv, cSend);
        socketCloser(aloneSocket);
        gtk_widget_show(cWindow);
        gtk_widget_set_sensitive(GTK_WIDGET(btConnect), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ipEntry), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(portEntry), TRUE);
        pthread_cancel(sendThreadID);
        return 0;
    }