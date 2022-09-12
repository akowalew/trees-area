#include "common.c"

public int
main(int Argc, char** Argv)
{
    if(Argc == 1)
    {
        fprintf(stderr, "Syntax: %s <FileName>\n", Argv[0]);
        return EXIT_FAILURE;
    }

    const char* FileName = Argv[1];
    char* FileData = ReadEntireFileAndNullTerminate(FileName);
    if(!FileData)
    {
        fprintf(stderr, "Failed to read file\n");
        return EXIT_FAILURE;
    }

    BitmapWidth = 1053;
    BitmapHeight = 983;
    BitmapArea = BitmapWidth * BitmapHeight;
    BitmapData = malloc(BitmapWidth * BitmapHeight);

    double X1 = 7502362.01665722;
    double Y1 = 5788883.998146315;
    double X2 = 7502501.187490554;
    double Y2 = 5789013.9085629815;
    double RealWidth = (X2 - X1);
    double RealHeight = (Y2 - Y1);
    double RealArea = (RealWidth * RealHeight);

    AreaCoefficient = (RealArea / BitmapArea);

    parser Parser = {0};
    Parser.At = FileData;
    if(!ParseFoi(&Parser))
    {
        fprintf(stderr, "Foi parsing failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
