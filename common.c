#pragma warning(push, 0)
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <float.h>
#include <winsock2.h>
#include <math.h>
#pragma warning(pop)

#define public
#define private static
#define global static
#define persistent static

#define ArrayLength(Array) (sizeof(Array)/sizeof((Array)[0]))

typedef struct
{
    float X;
    float Y;
} point2f;

typedef struct
{
    point2f TopLeft;
    point2f BottomRight;
} rectf;

typedef struct
{
    unsigned OrdinatesCount;
    point2f OrdinatesData[1024];
    unsigned DescriptionLength;
    char* DescriptionData;
} cemetery_info;

private inline unsigned
MinUnsigned(unsigned A, unsigned B)
{
    return (A < B) ? A : B;
}

private inline unsigned
MaxUnsigned(unsigned A, unsigned B)
{
    return (A > B) ? A : B;
}

private inline float
MinFloat(float A, float B)
{
    return (A < B) ? A : B;
}

private inline float
MaxFloat(float A, float B)
{
    return (A > B) ? A : B;
}

private int
ReadEntireFileIntoBufferAndNullTerminate(const char* FileName, char* Data, int Size)
{
    int Result = -1;

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        int FileSize = GetFileSize(FileHandle, NULL);
        if(FileSize != INVALID_FILE_SIZE)
        {
            if(FileSize < Size)
            {
                if(ReadFile(FileHandle, Data, FileSize, NULL, NULL))
                {
                    CloseHandle(FileHandle);

                    Data[FileSize] = '\0';

                    Result = FileSize;
                }
            }
        }
    }

    return Result;
}

private char*
ReadEntireFileAndNullTerminate(const char* FileName)
{
    char* Result = 0;

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD FileSize = GetFileSize(FileHandle, NULL);
        if(FileSize != INVALID_FILE_SIZE)
        {
            char* FileData = VirtualAlloc(0, FileSize+1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if(FileData)
            {
                if(ReadFile(FileHandle, FileData, FileSize, NULL, NULL))
                {
                    CloseHandle(FileHandle);

                    FileData[FileSize] = '\0';

                    Result = FileData;
                }
            }
        }
    }

    return Result;
}

static int
WriteEntireFile(const char* FileName, const void* Data, int Size)
{
    int Result = 0;

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        if(WriteFile(FileHandle, Data, Size, 0, 0))
        {
            CloseHandle(FileHandle);

            Result = 1;
        }
    }

    return Result;
}

private int
InitializeNetworking(void)
{
    WSADATA WSA;
    int Result = WSAStartup(MAKEWORD(2, 2), &WSA);
    if(Result)
    {
        fprintf(stderr, "Failed to initialize WSA: %d\n", Result);
        return 0;
    }

    return 1;
}

private int
HttpRequest(const char* Host, const char* TxData, int TxTotal, char* RxData, int RxSize)
{
    int Result;

    ADDRINFOA* AddressInfo = 0;
    Result = getaddrinfo(Host, "80", 0, &AddressInfo);
    if(Result)
    {
        fprintf(stderr, "Could not get address info: %d\n", Result);
        return -1;
    }

    SOCKET Socket = socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
    if(Socket == INVALID_SOCKET)
    {
        fprintf(stderr, "Could not create socket: %d\n", WSAGetLastError());
        return -1;
    }

    Result = connect(Socket, AddressInfo->ai_addr, (int)AddressInfo->ai_addrlen);
    if(Result)
    {
        fprintf(stderr, "Could not connect socket: %d\n", WSAGetLastError());
        return -1;
    }

    Result = send(Socket, TxData, TxTotal, 0);
    if(Result == SOCKET_ERROR)
    {
        fprintf(stderr, "Could not send to socket: %d\n", WSAGetLastError());
        return -1;
    }

    int RxTotal = 0;
    while(1)
    {
        char* RxAt = RxData + RxTotal;
        int RxElapsed = RxSize - RxTotal;
        Result = recv(Socket, RxAt, RxElapsed, 0);
        if(Result == SOCKET_ERROR)
        {
            fprintf(stderr, "Could not read from socket: %d\n", WSAGetLastError());
            return -1;
        }
        else if(Result == 0)
        {
            break;
        }

        RxTotal += Result;
    }

    return RxTotal;
}

typedef struct
{
    char* At;
} parser;

private int
IsWhiteSpace(char C)
{
    if(C == ' ' ||
       C == '\t' ||
       C == '\n' ||
       C == '\r' ||
       C == '\b')
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

private int
IsDigit(char C)
{
    if(C >= '0' &&
       C <= '9')
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

private int
IsIdentifier(char C)
{
    if((C >= 'a' &&
        C <= 'z') ||
       (C >= 'A' &&
        C <= 'Z'))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

private void
EatAllWhitespace(parser* Parser)
{
    while(IsWhiteSpace(Parser->At[0]))
    {
        Parser->At++;
    }
}

private int
ExpectExactChar(parser* Parser, char C)
{
    EatAllWhitespace(Parser);

    if(Parser->At[0] == C)
    {
        Parser->At++;

        return 1;
    }
    else
    {
        return 0;
    }
}

private int
ExpectExactSingleChar(parser* Parser, char C)
{
    if(Parser->At[0] == C)
    {
        Parser->At++;

        return 1;
    }
    else
    {
        return 0;
    }
}

private int
ExpectUnsigned(parser* Parser, unsigned* Unsigned)
{
    EatAllWhitespace(Parser);

    unsigned Tmp = 0;

    if(IsDigit(Parser->At[0]))
    {
        do
        {
            unsigned NextTmp = Tmp * 10 + (Parser->At[0] - '0');
            if(NextTmp < Tmp)
            {
                return 0;
            }

            Tmp = Tmp * 10 + (Parser->At[0] - '0');
            Parser->At++;

        } while(IsDigit(Parser->At[0]));

        *Unsigned = Tmp;

        return 1;
    }
    else
    {
        return 0;
    }
}

private int
ExpectFloat(parser* Parser, float* Float)
{
    EatAllWhitespace(Parser);

    float Tmp = 0;

    if(IsDigit(Parser->At[0]))
    {
        do
        {
            float NextTmp = Tmp * 10 + (Parser->At[0] - '0');
            if(NextTmp < Tmp)
            {
                return 0;
            }

            Tmp = NextTmp;
            Parser->At++;
        } while(IsDigit(Parser->At[0]));

        if(Parser->At[0] == '.')
        {
            float Divider = 0.1F;

            Parser->At++;
            if(IsDigit(Parser->At[0]))
            {
                do
                {
                    float NextTmp = Tmp + (Parser->At[0] - '0') * Divider;
                    if(NextTmp < Tmp)
                    {
                        return 0;
                    }

                    Tmp = NextTmp;
                    Divider /= 10;
                    Parser->At++;
                } while(IsDigit(Parser->At[0]));
            }
        }

        *Float = Tmp;

        return 1;
    }
    else
    {
        return 0;
    }
}

private int
ExpectExactString(parser* Parser, const char* String)
{
    EatAllWhitespace(Parser);

    if(Parser->At[0] != '"')
    {
        return 0;
    }

    Parser->At++;

    size_t StringLength = strlen(String);

    if(strcmp(Parser->At, String) <= 0)
    {
        return 0;
    }

    Parser->At += StringLength;

    if(Parser->At[0] != '"')
    {
        return 0;
    }

    Parser->At++;

    return 1;
}

private int
ExpectString(parser* Parser, char** String)
{
    EatAllWhitespace(Parser);

    if(Parser->At[0] != '"')
    {
        return 0;
    }

    Parser->At++;

    char* StringStart = Parser->At;
    char* StringEnd;

    do
    {
        StringEnd = strchr(Parser->At, '"');
        if(!StringEnd)
        {
            return 0;
        }

        Parser->At = StringEnd + 1;
    } while(StringEnd[-1] == '\\');

    *StringEnd = '\0';
    *String = StringStart;

    return 1;
}

private int
ExpectIdentifier(parser* Parser, char** IdentifierData, int* IdentifierLength)
{
    EatAllWhitespace(Parser);

    if(IsIdentifier(Parser->At[0]))
    {
        char* Start = Parser->At;

        do
        {
            Parser->At++;
        } while(IsIdentifier(Parser->At[0]));

        *IdentifierData = Start;
        *IdentifierLength = (int)(Parser->At - Start);

        return 1;
    }
    else
    {
        return 0;
    }
}

private int
ExpectExactIdentifier(parser* Parser, const char* Identifier)
{
    EatAllWhitespace(Parser);

    size_t IdentifierLength = strlen(Identifier);

    if(strcmp(Parser->At, Identifier) <= 0)
    {
        return 0;
    }

    Parser->At += IdentifierLength;

    return 1;
}

private int
GetCemeteryInfo(unsigned Id, cemetery_info* CemeteryInfo)
{
    char FileName[256];
    snprintf(FileName, sizeof(FileName), "cemetery_%d.xml", Id);

    char* At = ReadEntireFileAndNullTerminate(FileName);
    if(!At)
    {
        fprintf(stderr, "Failed to read input file\n");
        return 0;
    }

    // printf("%s", At);

    persistent const char SdoOrdinatesStartTag[] = "<sdo_ordinates>";
    char* SdoOrdinatesStart = strstr(At, SdoOrdinatesStartTag);
    if(!SdoOrdinatesStart)
    {
        fprintf(stderr, "SdoOrdinates start tag not found\n");
        return 0;
    }

    At = SdoOrdinatesStart + sizeof(SdoOrdinatesStartTag) - 1;

    parser Parser = {0};
    Parser.At = At;

    unsigned OrdinateIdx = 0;

    do
    {
        point2f Ordinate = {0};
        if(!ExpectFloat(&Parser, &Ordinate.X))
        {
            fprintf(stderr, "Expected X oordinate\n");
            return 0;
        }

        if(!ExpectExactChar(&Parser, ','))
        {
            fprintf(stderr, "Expected comma after X oordinate\n");
            return 0;
        }

        if(!ExpectFloat(&Parser, &Ordinate.Y))
        {
            fprintf(stderr, "Expected Y oordinate\n");
            return 0;
        }

        if(OrdinateIdx >= ArrayLength(CemeteryInfo->OrdinatesData))
        {
            fprintf(stderr, "Too much oordinates\n");
            return 0;
        }

        CemeteryInfo->OrdinatesData[OrdinateIdx++] = Ordinate;
    } while(ExpectExactChar(&Parser, ','));

    CemeteryInfo->OrdinatesCount = OrdinateIdx;

    persistent const char SdoOrdinatesEndTag[] = "</sdo_ordinates>";
    char* SdoOrdinatesEnd = strstr(At, SdoOrdinatesEndTag);
    if(!SdoOrdinatesEnd ||
       SdoOrdinatesEnd != Parser.At)
    {
        fprintf(stderr, "SdoOrdinates end tag not found\n");
        return 0;
    }

    // printf("Ordinates: %.*s\n", (int)(SdoOrdinatesEnd - At), At);

    At = SdoOrdinatesEnd + sizeof(SdoOrdinatesEndTag) - 1;

    persistent const char DescriptionStartTag[] = "<OPIS>";
    char* DescriptionStart = strstr(At, DescriptionStartTag);
    if(!DescriptionStart)
    {
        fprintf(stderr, "Description start tag not found");
        return 0;
    }

    At = DescriptionStart + sizeof(DescriptionStartTag) - 1;

    persistent const char DescriptionEndTag[] = "</OPIS>";
    char* DescriptionEnd = strstr(At, DescriptionEndTag);
    if(!DescriptionEnd)
    {
        fprintf(stderr, "Description end tag not found");
        return 0;
    }

    CemeteryInfo->DescriptionLength = (unsigned)(DescriptionEnd - At);
    CemeteryInfo->DescriptionData = At;

    // printf("Description: %.*s\n", (int)(DescriptionCount), At);
    // At = DescriptionEnd + sizeof(DescriptionEndTag) - 1;

    return 1;
}

private int
GetFoi(rectf* Rect, unsigned Width, unsigned Height, char* FoiData, int FoiSize)
{
    const char* Host = "mapa.um.warszawa.pl";

    persistent char TxData[1024] = {0};
    char* TxAt = TxData;
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "GET /mapviewer/foi?");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "request=getfoi&");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "version=1.0&");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "bbox=%lf:%lf:%lf:%lf&", Rect->TopLeft.X, Rect->TopLeft.Y, Rect->BottomRight.X, Rect->BottomRight.Y);
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "width=%u&", Width);
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "height=%u&", Height);
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "theme=dane_wawa.BOS_ZIELEN_ZASIEG_KORON_TOOLTIP&");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "clickable=yes&");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "area=yes&");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "dstsrid=2178&");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "wholeimage=yes&");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "tid=51_2811795124287878500&");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "aw=no ");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "HTTP/1.1\r\n");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "Host: %s\r\n", Host);
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "Connection: close\r\n");
    TxAt += snprintf(TxAt, sizeof(TxData) - (TxAt - TxData), "\r\n");
    int TxTotal = (int)(TxAt - TxData);

    persistent char RxData[1024 * 1024] = {0};
    int RxTotal = HttpRequest(Host, TxData, TxTotal, RxData, sizeof(RxData));
    if(RxTotal == -1)
    {
        fprintf(stderr, "Http request failed\n");
        return -1;
    }

    // printf("%.*s", RxTotal, RxData);

    if(!strstr(RxData, "HTTP/1.1 200 OK"))
    {
        fprintf(stderr, "Invalid response\n");
        return -1;
    }

    if(!strstr(RxData, "Transfer-Encoding: chunked\r\n"))
    {
        fprintf(stderr, "Invalid transfer encoding\n");
        return -1;
    }

    char* At = strstr(RxData, "\r\n\r\n");
    if(!At)
    {
        fprintf(stderr, "End of headers not found\n");
        return -1;
    }

    At += 4;

    int FoiLength = 0;

    unsigned ChunkSize;
    while((ChunkSize = strtoul(At, &At, 16)) != 0)
    {
        if(ChunkSize == ULONG_MAX &&
           errno == ERANGE)
        {
            fprintf(stderr, "Failed to convert chunk size\n");
            return -1;
        }

        if(At[0] != '\r' ||
           At[1] != '\n')
        {
            fprintf(stderr, "Invalid chunk begin\n");
            return -1;
        }

        At += 2;

        // printf("%.*s", (int)ChunkSize, At);
        memcpy(&FoiData[FoiLength], At, ChunkSize);
        FoiLength += ChunkSize;

        At += ChunkSize;

        if(At[0] != '\r' ||
           At[1] != '\n')
        {
            fprintf(stderr, "Invalid chunk end\n");
            return -1;
        }

        At += 2;
    }

    return FoiLength;
}

private int
CalculateBoundingBox(unsigned Count, point2f* Data, rectf* BoundingBox)
{
    if(Count < 2)
    {
        fprintf(stderr, "Not enough ordinates provided\n");
        return 0;
    }

    point2f First = Data[0];
    BoundingBox->TopLeft.X = First.X;
    BoundingBox->TopLeft.Y = First.Y;
    BoundingBox->BottomRight.X = First.X;
    BoundingBox->BottomRight.Y = First.Y;

    point2f Ordinate = {0};
    for(unsigned Idx = 1;
        Idx < Count;
        Idx++)
    {
        Ordinate = Data[Idx];
        BoundingBox->TopLeft.X = MinFloat(BoundingBox->TopLeft.X, Ordinate.X);
        BoundingBox->TopLeft.Y = MinFloat(BoundingBox->TopLeft.Y, Ordinate.Y);
        BoundingBox->BottomRight.X = MaxFloat(BoundingBox->BottomRight.X, Ordinate.X);
        BoundingBox->BottomRight.Y = MaxFloat(BoundingBox->BottomRight.Y, Ordinate.Y);
    }

    if(First.X != Ordinate.X ||
       First.Y != Ordinate.Y)
    {
        fprintf(stderr, "First and last ordinates differ\n");
        return 0;
    }

    return 1;
}

#define PARSER_CHECK(x) if(!(x)) { fprintf(stderr, "Parsing failed at %s:%d (%s)\n", __FILE__, __LINE__, #x); return 0; }

private int
ExpectJsonStringParamWithExactIdentifier(parser* Parser, const char* Identifier, char** String)
{
    PARSER_CHECK(ExpectExactIdentifier(Parser, Identifier));
    PARSER_CHECK(ExpectExactChar(Parser, ':'));
    PARSER_CHECK(ExpectString(Parser, String));

    return 1;
}

private int
ExpectJsonNumberParamWithExactIdentifier(parser* Parser, const char* Identifier, unsigned* Number)
{
    PARSER_CHECK(ExpectExactIdentifier(Parser, Identifier));
    PARSER_CHECK(ExpectExactChar(Parser, ':'));
    PARSER_CHECK(ExpectUnsigned(Parser, Number));

    return 1;
}

private int
ExpectNextJsonStringParamWithExactIdentifier(parser* Parser, const char* Identifier, char** String)
{
    PARSER_CHECK(ExpectExactChar(Parser, ','));
    PARSER_CHECK(ExpectJsonStringParamWithExactIdentifier(Parser, Identifier, String));

    return 1;
}

private int
ExpectNextJsonNumberParamWithExactIdentifier(parser* Parser, const char* Identifier, unsigned* Number)
{
    PARSER_CHECK(ExpectExactChar(Parser, ','));
    PARSER_CHECK(ExpectJsonNumberParamWithExactIdentifier(Parser, Identifier, Number));

    return 1;
}

typedef struct
{
    rectf Rect;
    float MetersPerPixel;
    unsigned BitmapWidth;
    unsigned BitmapHeight;
    unsigned BitmapArea;
    double   AreaCoefficient;
    point2f* PolygonData;
    int PolygonSize;
} foi_config;

private int
ExpectFoiAreaPoint(parser* Parser, unsigned* X, unsigned* Y, foi_config* FoiConfig)
{
    unsigned TmpX = 0;
    unsigned TmpY = 0;

    PARSER_CHECK(ExpectUnsigned(Parser, &TmpX));
    PARSER_CHECK(ExpectExactChar(Parser, ','));
    PARSER_CHECK(ExpectUnsigned(Parser, &TmpY));
    PARSER_CHECK(TmpX <= FoiConfig->BitmapWidth &&
                 TmpY <= FoiConfig->BitmapHeight);

    *X = TmpX;
    *Y = TmpY;

    return 1;
}

enum { SPECIES_MAX = 7 };
static const char* SpeciesCategories[][SPECIES_MAX] =
{
    { "brzoza" },
    { "buk" },
    { "dąb" },
    { "dąb czerwony" },
    { "grab" },
    { "głóg" },
    { "jarząb" },
    { "jesion" },
    { "kasztanowiec" },
    { "klon" },
    { "klon jesionolistny" },
    { "lipa" },
    { "platan" },
    { "robinia" },
    { "topola" },
    { "wierzba" },
    { "wiąz" },
    { "śliwa", "wiśnia", "jabłoń", "grusza" },
    { "świerk", "sosna", "jodła", "modrzew", "daglezja", "żywotnik", "cis" },
    { "inne" },
};

private int
CategorizeTree(char* NameInfo, int* Category)
{
    const char PolishNameBeginMarker[] = "Gatunek PL: ";
    char* PolishName = strstr(NameInfo, PolishNameBeginMarker);
    PARSER_CHECK(PolishName);

    PolishName += sizeof(PolishNameBeginMarker) - 1;

    char* EndOfPolishName = strstr(PolishName, "\\n");
    PARSER_CHECK(EndOfPolishName);

    *EndOfPolishName = '\0';

    for(int CategoryIdx = ArrayLength(SpeciesCategories)-1;
        CategoryIdx >= 0;
        CategoryIdx--)
    {
        for(int SpecieIdx = 0;
            SpecieIdx < SPECIES_MAX;
            SpecieIdx++)
        {
            const char* Specie = SpeciesCategories[CategoryIdx][SpecieIdx];
            if(!Specie)
            {
                break;
            }

            if(strstr(PolishName, Specie))
            {
                *Category = CategoryIdx;
                return 1;
            }
        }
    }

    // fprintf(stderr, "UNMATCHED category for name: \"%s\"\n", PolishName);
    *Category = ArrayLength(SpeciesCategories)-1;
    return 1;
}

private int
IsPointInsidePolygon(point2f* PolygonData, int PolygonSize, point2f P)
{
    int IntersectionCount = 0;

    int LastPolygonIdx = PolygonSize - 1;
    for(int PolygonIdx = 0;
        PolygonIdx < LastPolygonIdx;
        PolygonIdx++)
    {
        point2f P1 = PolygonData[PolygonIdx];
        point2f P2 = PolygonData[PolygonIdx+1];

        if(P1.Y == P2.Y &&
           P1.Y == P.Y)
        {
            if(MinFloat(P1.X, P2.X) <= P.X &&
               MaxFloat(P1.X, P2.X) >= P.X)
            {
                IntersectionCount++;
            }
        }
        else
        {
            float X = (((P.Y - P1.Y) * (P2.X - P1.X)) / (P2.Y - P1.Y)) + P1.X;
            if(X <= P.X)
            {
                IntersectionCount++;
            }
        }
    }

    if(IntersectionCount & 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

typedef struct
{
    int TreesPerCategory[ArrayLength(SpeciesCategories)];
    double TreesAreasPerCategory[ArrayLength(SpeciesCategories)];
} trees_stats;

private int
ParseFoiArrayItem(parser* Parser, foi_config* FoiConfig, trees_stats* Stats)
{
    char* AreaId = 0;
    char* AreaName = 0;
    char* AreaGType = 0;
    char* AreaImgUrl = 0;
    float AreaX = 0;
    float AreaY = 0;
    unsigned AreaWidth = 0;
    unsigned AreaHeight = 0;
    int AreaTotal = 0;

    int TreesCount = 0;
    int PointsCount = 0;
    int PointsInside = 0;

    do
    {
        int IdentifierLength;
        char* IdentifierData;
        PARSER_CHECK(ExpectIdentifier(Parser, &IdentifierData, &IdentifierLength));
        PARSER_CHECK(ExpectExactChar(Parser, ':'));

        if(strncmp(IdentifierData, "id", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectString(Parser, &AreaId));
        }
        else if(strncmp(IdentifierData, "name", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectString(Parser, &AreaName));
        }
        else if(strncmp(IdentifierData, "gtype", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectString(Parser, &AreaGType));
        }
        else if(strncmp(IdentifierData, "imgurl", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectString(Parser, &AreaImgUrl));
        }
        else if(strncmp(IdentifierData, "x", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectFloat(Parser, &AreaX));
        }
        else if(strncmp(IdentifierData, "y", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectFloat(Parser, &AreaY));
        }
        else if(strncmp(IdentifierData, "width", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectUnsigned(Parser, &AreaWidth));
        }
        else if(strncmp(IdentifierData, "height", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectUnsigned(Parser, &AreaHeight));
        }
        else if(strncmp(IdentifierData, "area", IdentifierLength) == 0)
        {
            PARSER_CHECK(ExpectExactChar(Parser, '"'));

            PARSER_CHECK(AreaWidth != 0 &&
                         AreaWidth <= FoiConfig->BitmapWidth &&
                         AreaHeight != 0 &&
                         AreaHeight <= FoiConfig->BitmapHeight);

            do
            {
                unsigned PrevAreaX = 0;
                unsigned PrevAreaY = 0;
                PARSER_CHECK(ExpectFoiAreaPoint(Parser, &PrevAreaX, &PrevAreaY, FoiConfig));

                while(ExpectExactSingleChar(Parser, ','))
                {
                    unsigned NextAreaX = 0;
                    unsigned NextAreaY = 0;
                    PARSER_CHECK(ExpectFoiAreaPoint(Parser, &NextAreaX, &NextAreaY, FoiConfig));

                    if(PrevAreaX != NextAreaX &&
                       PrevAreaY != NextAreaY)
                    {
                        return 0;
                    }

                    point2f P = {0};
                    P.X = (NextAreaX * FoiConfig->MetersPerPixel) + FoiConfig->Rect.TopLeft.X;
                    P.Y = (NextAreaY * FoiConfig->MetersPerPixel) + FoiConfig->Rect.TopLeft.Y;

                    PointsCount++;
                    if(IsPointInsidePolygon(FoiConfig->PolygonData, FoiConfig->PolygonSize, P))
                    {
                        PointsInside++;
                    }

                    AreaTotal += (PrevAreaX + NextAreaX + 2) * (PrevAreaY - NextAreaY);

                    PrevAreaX = NextAreaX;
                    PrevAreaY = NextAreaY;
                }

                TreesCount++;
            }
            while(ExpectExactSingleChar(Parser, ' '));

            PARSER_CHECK(ExpectExactChar(Parser, '"'));

            AreaTotal /= 2;

            if(AreaTotal > (int)FoiConfig->BitmapArea)
            {
                return 0;
            }
        }
        else
        {
            fprintf(stderr, "Invalid identifier: %.*s\n", IdentifierLength, IdentifierData);
            return 0;
        }
    } while(ExpectExactChar(Parser, ','));

    double Area = AreaTotal * FoiConfig->AreaCoefficient;

    int Category = 0;
    PARSER_CHECK(CategorizeTree(AreaName, &Category));

    if(PointsInside == PointsCount)
    {
        Stats->TreesPerCategory[Category] += TreesCount;
        Stats->TreesAreasPerCategory[Category] += Area;
    }

    // printf("Name: (%d) %s\n", Category, AreaName);
    // printf("Pos: %f %f\n", AreaX, AreaY);
    // printf("Area: %lf\n", Area);
    // printf("\n");

    return 1;
}

private int
ParseFoi(parser* Parser, foi_config* FoiConfig, trees_stats* Stats)
{
    PARSER_CHECK(ExpectExactChar(Parser, '{'));
    PARSER_CHECK(ExpectExactString(Parser, "foiarray"));
    PARSER_CHECK(ExpectExactChar(Parser, ':'));
    PARSER_CHECK(ExpectExactChar(Parser, '['));

    if(!ExpectExactChar(Parser, ']'))
    {
        do
        {
            PARSER_CHECK(ExpectExactChar(Parser, '{'));
            PARSER_CHECK(ParseFoiArrayItem(Parser, FoiConfig, Stats));
            PARSER_CHECK(ExpectExactChar(Parser, '}'));
        } while(ExpectExactChar(Parser, ','));

        PARSER_CHECK(ExpectExactChar(Parser, ']'));
    }

    return 1;
}

private int
CalculateTreesArea(char* FoiData,
                   unsigned BitmapWidth,
                   unsigned BitmapHeight,
                   rectf* Rect,
                   point2f* PolygonData,
                   int PolygonSize,
                   float MetersPerPixel,
                   trees_stats* Stats)
{
    unsigned BitmapArea = BitmapWidth * BitmapHeight;

    float RealWidth = (Rect->BottomRight.X - Rect->TopLeft.X);
    float RealHeight = (Rect->BottomRight.Y - Rect->TopLeft.Y);
    float RealArea = (RealWidth * RealHeight);
    float AreaCoefficient = (RealArea / BitmapArea);

    foi_config FoiConfig = {0};
    FoiConfig.Rect = *Rect;
    FoiConfig.BitmapWidth = BitmapWidth;
    FoiConfig.BitmapHeight = BitmapHeight;
    FoiConfig.BitmapArea = BitmapArea;
    FoiConfig.AreaCoefficient = AreaCoefficient;
    FoiConfig.PolygonData = PolygonData;
    FoiConfig.PolygonSize = PolygonSize;
    FoiConfig.MetersPerPixel = MetersPerPixel;

    parser Parser = {0};
    Parser.At = FoiData;

    if(!ParseFoi(&Parser, &FoiConfig, Stats))
    {
        fprintf(stderr, "Foi parsing failed\n");
        return 0;
    }

    return 1;
}

private double
CalculatePolygonArea(point2f* Data, int Size)
{
    double Result = 0.0;

    int LastIdx = Size - 1;
    for(int Idx = 0;
        Idx < LastIdx;
        Idx++)
    {
        point2f P1 = Data[Idx + 0];
        point2f P2 = Data[Idx + 1];

        Result += (P1.X + P2.X) * (P2.Y - P1.Y);
    }

    Result /= 2;

    return Result;
}

private double
CalculatePolygonLength(point2f* Data, int Size)
{
    double Result = 0.0;

    int LastIdx = Size - 1;
    for(int Idx = 0;
        Idx < LastIdx;
        Idx++)
    {
        point2f P1 = Data[Idx + 0];
        point2f P2 = Data[Idx + 1];

        double DX = P2.X - P1.X;
        double DY = P2.Y - P1.Y;

        Result += sqrt(DX*DX + DY*DY);
    }

    return Result;
}
