// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/CoreMisc.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManagerGeneric.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Serialization/BufferArchive.h"

using namespace std;

struct Tile;
struct WorldTile;

class WARP_API FWarpGameModule : public IModuleInterface
{

    const FString MAP_DIR = FPaths::ProjectContentDir() + TEXT("Levels/");
    FString curr_map = "";
    TArray<WorldTile> curr_tilemap;

    int N = 1;
    float K = 1.0f;
    float KLEIN_V = 1.0f;
    float CELL_WIDTH = 1.0f;
    

public:
	virtual void StartupModule() override;

	void GenerateTileMap(int type, bool lattice3D, int max_expand);
    FVector MakeShift(char c);
    void ExpandMap(vector<Tile> *tiles, int len, bool lattice3D);
    unsigned char NearbyAfterShift(vector<Tile> tiles, int ix, char c);
    void LoadTileMap();

    int GetN() { return N; }
    float GetK() { return K; }
    float GetKlein() { return KLEIN_V; }
    float GetCellW() { return CELL_WIDTH; }
    FString GetCurrMap() { return curr_map; }
    TArray<WorldTile>* GetTilemap() { return &curr_tilemap; };

    void SetN(int v) { N = v; }
    void SetK(float v) { K = v; }
    void SetKlein(float v) { KLEIN_V = v; }
    void SetCellW(float v) { CELL_WIDTH = v; }
    
	//Set tile type and parameters by geometry type
    void SetTileTypeW(float n) {

        N = (int)n;
        float a = PI / 4;
        float b = PI / N;
        float r = cos(b) / sin(a);
		
        if (n == 4.0f) {
            K = 0.0f;
            KLEIN_V = 1.0f;
            CELL_WIDTH = 2.0;
        }
        else {
            K = (n < 4.0f ? 1.0f : -1.0f);
            a = PI / 4;
            b = PI / n;
            r = cos(b) / sin(a);
            CELL_WIDTH = sqrt(abs(r * r - 1.0)) / r;
            KLEIN_V = (float)(CELL_WIDTH + (3e-4 / n));
        }
    }

};

#include "WarpMath.h"

using namespace WarpMath;

//NQ tiles
struct Tile {
    Tile(string _coord, GyroVectorD _gv) {
        coord = _coord; gv = _gv;
    };
    GyroVectorD gv;
    string coord;
};

struct WorldTile {
    WorldTile(FVector2D _xz, GyroVectorD _gv) {
        xz = _xz; gv = _gv;
    };
    GyroVectorD gv;
    FVector2D xz;
};


