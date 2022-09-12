#include "common.c"

public int
main(int Argc, char** Argv)
{
    rectf Rect;
    Rect.TopLeft.X = 7502362.01665722f;
    Rect.TopLeft.Y = 5788883.998146315f;
    Rect.BottomRight.X = 7502501.187490554f;
    Rect.BottomRight.Y = 5789013.9085629815f;

    unsigned Width = 256;
    unsigned Height = 256;

    persistent char FoiData[1024 * 1024];
    int FoiLength = GetFoi(&Rect, Width, Height, FoiData, sizeof(FoiData));
    if(FoiLength == -1)
    {
        fprintf(stderr, "Failed to get FOI\n");
        return EXIT_FAILURE;
    }

    printf("%.*s", FoiLength, FoiData);

    return EXIT_SUCCESS;
}
