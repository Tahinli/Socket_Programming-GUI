//Libraries
    #include <stdio.h>
    #include <stdlib.h>
    #include <gtk/gtk.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <netinet/ip.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <string.h>
    #include <unistd.h>
    #include <errno.h>
    #include <pthread.h>
    #include <sys/epoll.h>
    #include <fcntl.h>

//Global Variables
    //Client
        int cSendLength;
        int cRecvLength;
        int cRecv;
        int cRecvCommunicate;
        char recvBuffer[512];
        int cSendCheck = -1;
    //Server
        struct sockaddr_in serverAddr;
        struct sockaddr_in clientAddr;
        int sClientSocket;
        int sRecv;
        int maxClient = 10;
        int socketLen;
        int messageLoop = 1;
        int epoll_fd;
        

//Thread Values
pthread_mutex_t cMutex;
pthread_mutex_t sMutex;


//Server Window Decleration
        GtkWidget *sWindow;
        GtkWidget *sPane;
        GtkWidget *btStart;
        GtkWidget *sPortEntry;

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
        GtkWidget *btcOrderMessageSend;
//Socket Decleration
    int clientSocket;
    int serverSocket;
//Funcs
    int connectToServer(void);
    int socketCloser(int aloneSocket);
    int cCleaner(int aloneSocket);
    int sCleaner(void);
    int cSendMessageF(int clientSocket, char cMessage[]);
    int startServer(void);
    int setnonblocking(int sockfd);

//Threads

void *cRecvThread (void *vargp)
    {
        
        printf("cRecvThread\n");
        GtkTextIter cRIter;
        GtkTextMark *cRMark;
        GtkAdjustment *cRHeight;
        char cLine = '\n';
        while(cRecv)
            {
                memset(recvBuffer, ' ', 511);
                recvBuffer[512] = '\0';
                cRecvLength = recv(clientSocket, recvBuffer, strlen(recvBuffer), MSG_DONTWAIT);
                recvBuffer[cRecvLength] = '\0';
                if(cRecvLength > 0)
                    {
                                //Thread Lock
                            pthread_mutex_lock(&cMutex);
                        GtkTextBuffer *cOMessageRecvBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(cOrderRecvTextView));
                        cRMark = gtk_text_buffer_get_insert(cOMessageRecvBuffer);
                        gtk_text_buffer_get_iter_at_mark(cOMessageRecvBuffer, &cRIter, cRMark);
                        gtk_text_buffer_insert(cOMessageRecvBuffer, &cRIter, recvBuffer, -1);
                        //gtk_text_buffer_insert(cOMessageRecvBuffer, &cRIter, &cLine, 1);
                        //gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(cOrderRecvTextView), cRMark, 0., FALSE, 0., 0.);
                              //Thread Unlock.
                            pthread_mutex_unlock(&cMutex);
                        g_print("%s", recvBuffer);
                    }
                
            }
        printf("\nRecv Thread will be closed\n");
        pthread_exit(cRecvThread);
    }

void *sRecvThread (void *vargp)
    {
        printf("\nsRecvThread\n");

        char sMessage[513];
        int sMessageCount = 0;
        int sClientCount = 0;
        char sClientAddr[NI_MAXHOST];
        char sClientServ[NI_MAXSERV];
        

        epoll_fd = epoll_create1(0);
        if(0>epoll_fd)
            {
                perror("Epoll Creation\n");
            }

        struct epoll_event ev[maxClient+1];
        ev[0].events = EPOLLIN | EPOLLOUT | EPOLLET;
        ev[0].data.fd = serverSocket;
        sClientCount++;

        if(0>epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &ev[0]))
            {
                perror("Epoll_CTL\n");
            }
        int nfds = 0;
        
        while(sRecv)
            {
                nfds = epoll_wait(epoll_fd, ev, maxClient, 0);
                for(int i = 0;i<nfds;i++)
                    {
                        if(ev[i].data.fd == serverSocket)
                            {
                                sClientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &socketLen);
                                if(0>getnameinfo((struct sockaddr*)&clientAddr, socketLen, sClientAddr, sizeof(sClientAddr), sClientServ, sizeof(sClientServ), NI_NUMERICHOST | NI_NUMERICSERV))
                                    {
                                        perror("Getnameinfo\n");
                                    }
                                else
                                    {
                                        printf("Client Info = %s:%s\n", sClientAddr, sClientServ);
                                        sClientCount++;
                                    }
                                setnonblocking(sClientSocket);
                                ev[sClientCount].events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP;
                                ev[sClientCount].data.fd = sClientSocket;
                                if(0>epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sClientSocket, &ev[sClientCount]))
                                    {
                                        perror("EPOLL_CTL-Client\n");
                                    }
                                else
                                    {
                                        printf("New Client Added\n");
                                    }
                            }
                        else if (ev[i].events && EPOLLIN)
                            {
                                int sCompletedMessage = 1;
                                while(sCompletedMessage)
                                    {
                                        bzero(sMessage, sizeof(sMessage));
                                        sMessageCount = read(ev[i].data.fd, sMessage, sizeof(sMessage)-1);
                                        sMessage[513] = '\0';
                                        if(0>sMessageCount)
                                            {
                                                //perror("MessageCount\n");
                                                /*printf("Closing Connection Requested\n");
                                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev[i].data.fd, NULL);
                                                close(ev[i].data.fd);
                                                sClientCount--;
                                                continue;*/
                                            }
                                        else if(0==sMessageCount)
                                            {
                                                sCompletedMessage = 0;
                                                printf("0 Closing Connection Requested\n");
                                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev[i].data.fd, NULL);
                                                close(ev[i].data.fd);
                                                sClientCount--;
                                                continue;
                                            }
                                        else
                                            {
                                                printf("Message = %s\n", sMessage);
                                                for(int j = 0;j<=sClientCount;j++)
                                                    {
                                                        write(ev[j].data.fd, sMessage, sizeof(sMessage));
                                                    }
                                                sCompletedMessage = 0;
                                            }
                                    }

                            }
                        else
                            {
                                printf("Bir Şey Oldu\n");
                            }
                    }
            }
            

        if(0>close(epoll_fd))
            {
                perror("Epoll Close\n");
            }        
        printf("\nsRecv is Closing\n");
    }

//Time Based Events

gboolean cRecvScrollRefresh(gpointer data)
    {
        if(cRecv)
            {
                GtkTextMark *cRMark;
                //Thread Lock
                            pthread_mutex_lock(&cMutex);
                        GtkTextBuffer *cOMessageRecvBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(cOrderRecvTextView));
                        cRMark = gtk_text_buffer_get_insert(cOMessageRecvBuffer);
                        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(cOrderRecvTextView), cRMark, 0., FALSE, 0., 0.);
                              //Thread Unlock.
                            pthread_mutex_unlock(&cMutex);

                return 1;
            }
        else
            {
                printf("\nAuto Scroll's Disabled\n");
                return 0;
            }
    }

//Key Pressed Events
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);




//Button Events
void sWindowCloser(GtkWidget *widget, gpointer data)
    {
        printf("\nClosing Server\n");
        gtk_widget_destroy(GTK_WIDGET(sWindow));
        sCleaner();
    }

void btStartClick(GtkWidget *widget, gpointer data)
    {
        g_print("\nServer Start Order\n");
        
        if(!startServer())
            {
                printf("Threading's Starting\n");
                //Multithread
                sRecv = 1;
                pthread_mutex_init(&sMutex, NULL);
                pthread_t sRecvThreadID;
                pthread_create(&sRecvThreadID, NULL, sRecvThread, NULL);
                gtk_widget_set_sensitive(GTK_WIDGET(btStart), FALSE);
            }
        else
            {
                perror("Server couldn't start");
            }
    }


gboolean cOrderSendMessage(GtkWidget *widget, gpointer data)
    {
        GtkTextIter cSFirst, cSLast;
        GtkTextBuffer *cOMessageSendBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(cOrderSendTextView));
        gtk_text_buffer_get_bounds(cOMessageSendBuffer, &cSFirst, &cSLast);
        gchar *cSendMessage;
        cSendMessage = gtk_text_buffer_get_text(cOMessageSendBuffer, &cSFirst, &cSLast, FALSE);
        if(cSendMessageF(clientSocket, cSendMessage))
            {
                perror("Send Failed");
                cSendCheck = 0;
                return -1;
            }
        else
            {
                gtk_text_buffer_set_text(cOMessageSendBuffer, "", 0);
                cSendCheck = 1;
                return 0;
            }
    }
void cOrderWindowCloser(GtkWidget *widget, gpointer data)
    {
        gtk_widget_destroy(GTK_WIDGET(cOrderWindow));
        cCleaner(clientSocket);
    }
void btConnectClick(GtkWidget *widget, gpointer data)
    {
        g_print("Connection Order\n");
        //Connection Call
        if(!connectToServer())
            {
                printf("Client Side Threading's Starting\n");
                //Multithread
                cRecv = 1;
                pthread_mutex_init(&cMutex, NULL);
                pthread_t cRecvThreadID;
                pthread_create(&cRecvThreadID, NULL, cRecvThread, NULL);

            }
        else
            {
                perror("Connection Failed\n");
                cCleaner(clientSocket);
            }

        
        //Time Based Refresh
        g_timeout_add_seconds(1, cRecvScrollRefresh, NULL);



        //Chat Window Definition
        cOrderWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        cOrderPane = gtk_fixed_new();
        cOrderRecvScrollableWindow = gtk_scrolled_window_new(NULL, NULL);
        cOrderSendScrollableWindow = gtk_scrolled_window_new(NULL, NULL);
        cOrderRecvTextView = gtk_text_view_new();
        cOrderSendTextView = gtk_text_view_new();
        btcOrderMessageSend = gtk_button_new_with_label("Send");

        //Customization
            //Window
        gtk_window_set_position(GTK_WINDOW(cOrderWindow), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size(GTK_WINDOW(cOrderWindow), 520, 500);
        gtk_window_set_title(GTK_WINDOW(cOrderWindow), "Tahinli's Client");
        gtk_window_set_resizable(GTK_WINDOW(cOrderWindow), FALSE);
            //TextView
        gtk_widget_set_size_request(GTK_WIDGET(cOrderRecvScrollableWindow), 500, 320);
        gtk_widget_set_size_request(GTK_WIDGET(cOrderSendScrollableWindow), 500, 100);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(cOrderRecvTextView), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(cOrderRecvTextView), FALSE);
            //Auto Line Visual Fix
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(cOrderSendTextView), GTK_WRAP_WORD);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(cOrderRecvTextView), GTK_WRAP_WORD);

                //Catching Key Events in TextView
        g_signal_connect(G_OBJECT(cOrderSendTextView), "key_press_event", G_CALLBACK(on_key_press), NULL);

        
        

            

        //Pane Set
        gtk_fixed_put(GTK_FIXED(cOrderPane), cOrderRecvScrollableWindow, 10, 10);
        gtk_fixed_put(GTK_FIXED(cOrderPane), cOrderSendScrollableWindow, 10, 350);
        gtk_fixed_put(GTK_FIXED(cOrderPane), btcOrderMessageSend, 420, 450);

        //Button-Event
        g_signal_connect(btcOrderMessageSend, "clicked", G_CALLBACK(cOrderSendMessage), NULL);

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
        //Definition
        sWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        sPane = gtk_fixed_new();
        btStart = gtk_button_new_with_label("Start Server");
        sPortEntry = gtk_entry_new();

        //Pane-Set
        gtk_fixed_put(GTK_FIXED(sPane), btStart, 150, 45);
        gtk_fixed_put(GTK_FIXED(sPane), sPortEntry, 125, 10);

        //Customization
            //Window
        gtk_window_set_position(GTK_WINDOW(sWindow), GTK_WIN_POS_CENTER);
        gtk_window_set_default_size(GTK_WINDOW(sWindow), 400, 100);
        gtk_window_set_title(GTK_WINDOW(sWindow), "Tahinli's Server");
        gtk_window_set_resizable(GTK_WINDOW(sWindow), FALSE);
            //Entry
        gtk_entry_set_placeholder_text(GTK_ENTRY(sPortEntry), "Port Number");
        gtk_entry_set_max_length(GTK_ENTRY(sPortEntry), 5);
            //Button
        gtk_widget_set_can_focus(GTK_WIDGET(btStart), TRUE);
        gtk_widget_grab_focus(GTK_WIDGET(btStart));

        //Button-Event
        g_signal_connect(btStart,"clicked", G_CALLBACK(btStartClick), NULL);


        //Epilogue
        gtk_container_add(GTK_CONTAINER(sWindow), sPane);
        gtk_widget_show_all(sWindow);

        //CloseEvent
        g_signal_connect(G_OBJECT(sWindow), "destroy",G_CALLBACK(sWindowCloser), NULL);
        
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

//FUNCS
int connectToServer(void)
    {
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
                return -1;
            }
        ipAddress[strlen(ipAddress)+1]='\0';
        portNumber[strlen(portNumber)+1]='\0';
        struct sockaddr_in server_Socket;
        server_Socket.sin_family = AF_INET;
        server_Socket.sin_addr.s_addr = inet_aton(ipAddress, (struct in_addr*)&server_Socket.sin_addr.s_addr);
        server_Socket.sin_port = htons(atoi(portNumber));
        socklen_t server_Socket_len = sizeof(server_Socket);
        
        if(0>inet_pton(AF_INET, ipAddress, &server_Socket.sin_addr.s_addr))
            {
                perror("Undefined IP Address");
                socketCloser(clientSocket);
                return -1;
            }
        printf("IP is = %s and len = [%d]\n", ipAddress, strlen(ipAddress));
        printf("Port is = %s and len = [%d]\n", portNumber, strlen(portNumber));
        if(0!=connect(clientSocket, (struct sockaddr*)&server_Socket, server_Socket_len))
            {
                perror("Connection Error");
                socketCloser(clientSocket);
                return -1;
            }
        else
            {
                printf("Connected\n");
            }
        
        return 0;
    }
int socketCloser(int aloneSocket)
    {
        if(0!=close(aloneSocket))
            {
                perror("Socket Closing Error");
                return -1;
            }
        else
            {
                printf("\nSocket is Closed\n");
                return 0; 
            }
        
    }
int cCleaner(int aloneSocket)
    {
        cRecv = 0;
        socketCloser(aloneSocket);
        pthread_mutex_destroy(&cMutex);
        gtk_widget_show(cWindow);
        gtk_widget_set_sensitive(GTK_WIDGET(btConnect), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(ipEntry), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(portEntry), TRUE);
        return 0;
    }
int sCleaner(void)
    {
        sRecv = 0;
        socketCloser(serverSocket);
        socketCloser(sClientSocket);
        return 0;
    }
int cSendMessageF(int clientSocket, char cMessage[])
    {
        
        if(strlen(cMessage)<512)
            {
                int isEmpyty = -1;
                int lastLetter = -1;
                //is Empty
                for(int i = 0; i < strlen(cMessage); i++)
                    {
                        if(isEmpyty == -1 && cMessage[i] != 10 && cMessage[i] != 32 && cMessage[i] != 9)
                            {
                                isEmpyty = i;
                                printf("\nBOŞ\n");
                            }
                        if(isEmpyty != -1 && cMessage[i] != 10 && cMessage[i] != 32 && cMessage[i] != 9)
                            {
                                lastLetter = i;
                                printf("DEGIL\n");
                            }
                    }
                if(isEmpyty != -1)
                    {
                        char cSendBuffer[512];
                        for(int i = 0; i < lastLetter-isEmpyty+1; i++)
                            {
                                cSendBuffer[i] = cMessage[isEmpyty+i];
                            }
                        cSendBuffer[lastLetter+1] = '\0';
                        for(int i = 0; i < strlen(cSendBuffer); i++)
                            {
                                printf("%c--%d\n", cSendBuffer[i], cSendBuffer[i]);
                            }
                        cSendLength=send(clientSocket, cSendBuffer, strlen(cSendBuffer), 0);
                        if(cSendLength != strlen(cSendBuffer))
                            {
                                perror("Send Failed");
                                return -1;
                            }
                    }
                else
                    {
                        printf("\nEmpyt\n");
                        return -1;
                    }
            }
        else
            {
                printf("\nToo Long\n");
                return -1;
            }
        printf("\nEverything is OK\n");
        return 0;

    }
int startServer(void)
    {
        printf("\nServer is Starting\n");
        char sPortNumber[] = "00000";
        strncpy(sPortNumber, gtk_entry_get_text(GTK_ENTRY(sPortEntry)),5);
        sPortNumber[5] = '\n';
        printf("Port is = %s\n", sPortNumber);
        socketLen = sizeof(clientAddr);

        bzero((char *)&serverAddr, sizeof(serverAddr));
        bzero((char *)&clientAddr, sizeof(clientAddr));
        

        //Socket Setup
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        printf("Server Socket is = %d\n", serverSocket);
        if(0>serverSocket)
            {   
                printf("ERRNO = %d:%s\n", errno, strerror(errno));
                printf("Error ID = %d\n", serverSocket);
                perror("Socket Setup Failed");
                socketCloser(serverSocket);
                return -1;
            }
        
        
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(atoi(sPortNumber));
        socklen_t serverAddr_len = sizeof(serverAddr);
        if(0>bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)))
            {
                printf("ERRNO = %d:%s\n", errno, strerror(errno));
                perror("Bind Failed");
                socketCloser(serverSocket);
                return -1;
            }
        setnonblocking(serverSocket);
        if(0>listen(serverSocket, 0))
            {
                printf("ERRNO = %d:%s\n", errno, strerror(errno));
                perror("Listen Failed");
                socketCloser(serverSocket);
                return -1;
            }

        return 0;
    }

int setnonblocking(int sockfd)
    {
        if(0>fcntl(sockfd, F_SETFD, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK))
            {
                return -1;
            }
        return 0;
    }


//Key Pressed Events
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
    {
        switch(event->keyval)
            {
                case GDK_KEY_Return:
                cOrderSendMessage(widget, data);
                return 1;
                break;

                default:
                NULL;
            }
        return 0;
    }

