#include "common.c"

public int
main(int Argc, char** Argv)
{
    int CemeteryFromId = 1;
    int CemeteryToId = 50;

    if(Argc >= 2)
    {
        CemeteryFromId = atoi(Argv[1]);
    }

    if(Argc >= 3)
    {
        CemeteryToId = atoi(Argv[2]);
    }

    const char* Host = "mapa.um.warszawa.pl";

    int Result;

    WSADATA WSA;
    Result = WSAStartup(MAKEWORD(2, 2), &WSA);
    if(Result)
    {
        fprintf(stderr, "Failed to initialize WSA: %d\n", Result);
        return EXIT_FAILURE;
    }

    ADDRINFOA* AddressInfo = 0;
    Result = getaddrinfo(Host, "80", 0, &AddressInfo);
    if(Result)
    {
        fprintf(stderr, "Could not get address info: %d\n", Result);
        return EXIT_FAILURE;
    }

    for(int CemeteryId = CemeteryFromId;
        CemeteryId < CemeteryToId;
        CemeteryId++)
    {
        SOCKET Socket = socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
        if(Socket == INVALID_SOCKET)
        {
            fprintf(stderr, "Could not create socket: %d\n", WSAGetLastError());
            return EXIT_FAILURE;
        }

        Result = connect(Socket, AddressInfo->ai_addr, (int)AddressInfo->ai_addrlen);
        if(Result)
        {
            fprintf(stderr, "Could not connect socket: %d\n", WSAGetLastError());
            return EXIT_FAILURE;
        }

        persistent char TxData[1024];
        memset(TxData, 0, sizeof(TxData));

        char* TxAt = TxData;
        TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "GET /mapaApp1/GetCmentarzeById?id=%d HTTP/1.1\r\n", CemeteryId);
        TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "Host: %s\r\n", Host);
        TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "Connection: close\r\n");
        TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "\r\n");
        int TxTotal = (int)(TxAt - TxData);

        persistent char RxData[1024 * 1024];
        memset(RxData, 0, sizeof(RxData));

        Result = send(Socket, TxData, TxTotal, 0);
        if(Result == SOCKET_ERROR)
        {
            fprintf(stderr, "Could not send to socket: %d\n", WSAGetLastError());
            return EXIT_FAILURE;
        }

        int RxTotal = 0;
        while(1)
        {
            char* RxAt = RxData + RxTotal;
            int RxElapsed = sizeof(RxData) - RxTotal;
            Result = recv(Socket, RxAt, RxElapsed, 0);
            if(Result == SOCKET_ERROR)
            {
                fprintf(stderr, "Could not read from socket: %d\n", WSAGetLastError());
                return EXIT_FAILURE;
            }
            else if(Result == 0)
            {
                break;
            }

            RxTotal += Result;
        }

        // printf("(%d) ----> %.*s", CemeteryId, RxTotal, RxData);

        if(!strstr(RxData, "HTTP/1.1 200 OK"))
        {
            fprintf(stderr, "Invalid response\n");
            return EXIT_FAILURE;
        }

        char* ContentData = strstr(RxData, "\r\n\r\n");
        if(!ContentData)
        {
            fprintf(stderr, "End of headers not found\n");
            return EXIT_FAILURE;
        }

        ContentData += 4;

        int ContentLength = RxTotal - (int)(ContentData - RxData);

        printf("%.*s", ContentLength, ContentData);

        Result = closesocket(Socket);
        if(Result == SOCKET_ERROR)
        {
            fprintf(stderr, "Failed to close socket\n");
            return EXIT_FAILURE;
        }

        char OutputFileName[256];
        snprintf(OutputFileName, sizeof(OutputFileName), "cemetery_%d.xml", CemeteryId);
        if(!WriteEntireFile(OutputFileName, ContentData, ContentLength))
        {
            fprintf(stderr, "Failed to write output file: %d\n", GetLastError());
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
