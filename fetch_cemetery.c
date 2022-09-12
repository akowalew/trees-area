#include "common.c"

public int
main(int Argc, char** Argv)
{
    unsigned Id = 1;

    if(!InitializeNetworking())
    {
        fprintf(stderr, "Failed to initialize networking\n");
        return EXIT_FAILURE;
    }

    cemetery_info CemeteryInfo = {0};
    if(!GetCemeteryInfo(Id, &CemeteryInfo))
    {
        fprintf(stderr, "Failed to get cemetery info\n");
        return EXIT_FAILURE;
    }

    rectf BoundingBox = {0};
    if(!CalculateBoundingBox(CemeteryInfo.OrdinatesCount, CemeteryInfo.OrdinatesData, &BoundingBox))
    {
        fprintf(stderr, "Failed to calculate bounding box\n");
        return EXIT_FAILURE;
    }

    trees_stats Stats = {0};

    float MetersPerPixel = 0.1f;
    unsigned BitmapWidth = 1024;
    unsigned BitmapHeight = 1024;
    float DX = BitmapWidth * MetersPerPixel;
    float DY = BitmapHeight * MetersPerPixel;

    persistent char FoiData[1024 * 1024];

    unsigned IdxX = 0;
    unsigned IdxY = 0;

    rectf Rect;
    for(Rect.TopLeft.Y = BoundingBox.TopLeft.Y;
        Rect.TopLeft.Y <= BoundingBox.BottomRight.Y;
        Rect.TopLeft.Y += DY)
    {
        IdxX = 0;

        for(Rect.TopLeft.X = BoundingBox.TopLeft.X;
            Rect.TopLeft.X <= BoundingBox.BottomRight.X;
            Rect.TopLeft.X += DX)
        {
            Rect.BottomRight.X = Rect.TopLeft.X + DX;
            Rect.BottomRight.Y = Rect.TopLeft.Y + DY;

            char FileName[256];
            snprintf(FileName, sizeof(FileName), "%d_%d_%d.json", Id, IdxY, IdxX);

            // printf("Processing bitmap (%u, %u)\n", IdxY, IdxX);

#if 0
            int FoiLength = GetFoi(&Rect, BitmapWidth, BitmapHeight, FoiData, sizeof(FoiData));
            if(FoiLength == -1)
            {
                fprintf(stderr, "Failed to get FOI\n");
                return EXIT_FAILURE;
            }

            if(!WriteEntireFile(FileName, FoiData, FoiLength))
            {
                fprintf(stderr, "Failed to write foi file\n");
                return EXIT_FAILURE;
            }
#else
            int FoiLength = ReadEntireFileIntoBufferAndNullTerminate(FileName, FoiData, sizeof(FoiData));
            if(FoiLength == -1)
            {
                fprintf(stderr, "Failed to read FOI data file\n");
                return EXIT_FAILURE;
            }
#endif

            if(!CalculateTreesArea(FoiData,
                                   BitmapWidth,
                                   BitmapHeight,
                                   &Rect,
                                   CemeteryInfo.OrdinatesData,
                                   CemeteryInfo.OrdinatesCount,
                                   MetersPerPixel,
                                   &Stats))
            {
                fprintf(stderr, "Failed to parse FOI data\n");
                return EXIT_FAILURE;
            }

            IdxX++;
        }

        IdxY++;
    }

    double CemeteryArea = CalculatePolygonArea(CemeteryInfo.OrdinatesData, CemeteryInfo.OrdinatesCount);

    int TotalTrees = 0;
    double TotalTreesArea = 0.0;

    printf("Cemetery id: %d\n", Id);
    printf("Cemetery name: \"%.*s\"\n", CemeteryInfo.DescriptionLength, CemeteryInfo.DescriptionData);
    printf("Cemetery area: %10.2lfm^2\n", CemeteryArea);

    printf("Trees statistics:\n");
    for(int CategoryIdx = 0;
        CategoryIdx < ArrayLength(SpeciesCategories);
        CategoryIdx++)
    {
        int Trees = Stats.TreesPerCategory[CategoryIdx];
        double TreesArea = Stats.TreesAreasPerCategory[CategoryIdx];
        double AreaPercent = (TreesArea / CemeteryArea) * 100;
        printf("%2d) %6.2lf%% (%10.2lfm^2, %6d trees) ", CategoryIdx, AreaPercent, TreesArea, Trees);

        printf("[%s", SpeciesCategories[CategoryIdx][0]);
        for(int SpecieIdx = 1;
            SpecieIdx < SPECIES_MAX;
            SpecieIdx++)
        {
            const char* Specie = SpeciesCategories[CategoryIdx][SpecieIdx];
            if(!Specie)
            {
                break;
            }

            printf(", %s", Specie);
        }
        printf("]");

        printf("\n");

        TotalTrees += Trees;
        TotalTreesArea += TreesArea;
    }

    double TotalAreaPercent = (TotalTreesArea / CemeteryArea) * 100;

    printf("---------------\n");
    printf("Total trees area: %6.2lf%% (%10.2lfm^2, %d trees)\n", TotalAreaPercent, TotalTreesArea, TotalTrees);

    return EXIT_SUCCESS;
}
