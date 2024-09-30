// Copyright Epic Games, Inc. All Rights Reserved.

#include "Warp.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FWarpGameModule, Warp, "Warp" );

void FWarpGameModule::StartupModule()
{

	UE_LOG(LogUnrealMath, Log, TEXT("Warp startup"));

    SetTileTypeW(5);
    
    GenerateTileMap(8, false, 6);
    LoadTileMap();

}

// Geometry tiles functions

//Try spawn tile
bool TrySpawn(vector<Tile> *tiles, string coord, GyroVectorD gv) {
    for (int i = 0; i < tiles->size(); ++i) {
        GyroVectorD gv2 = tiles->at(i).gv;
        if (sqrMagnitude(sub(gv, gv2).vec) < 1e-10) {
            return false;
        }
    }
    tiles->push_back(Tile(coord, gv));
    return true;
}

void SplitAndAdd(FBufferArchive* dataArchive, float v) {
    char* cv = (char*)&v;
    for (int i = 0; i < 4; ++i) {
        dataArchive->Add(cv[i]);
    }
}

void Add(string coord, GyroVectorD gv, FString fname) {
    FBufferArchive dataArchive;
    FFileHelper::LoadFileToArray(dataArchive, *fname);
    dataArchive.Add((uint8)coord.length() & 0x0000FF);
    for (int i = 0; i < coord.length(); ++i) {
        dataArchive.Add((uint8)coord[i]);
    }
    SplitAndAdd(&dataArchive, gv.vec.X);
    SplitAndAdd(&dataArchive, gv.vec.Y);
    SplitAndAdd(&dataArchive, gv.vec.Z);
    SplitAndAdd(&dataArchive, gv.gyr.X);
    SplitAndAdd(&dataArchive, gv.gyr.Y);
    SplitAndAdd(&dataArchive, gv.gyr.Z);
    SplitAndAdd(&dataArchive, gv.gyr.W);
    FFileHelper::SaveArrayToFile(dataArchive, *fname);
}

float GetAndUnite(TArray<uint8> dataArchive, int32 *it) {
    float f;
    char b[] = { (char)dataArchive[*it], (char)dataArchive[*it + 1], (char)dataArchive[*it + 2], (char)dataArchive[*it + 3] };
    memcpy(&f, &b, sizeof(f));
    *it = *it + 4;
    return f;
}

// Load tilemap of 2D area
void FWarpGameModule::LoadTileMap() {

    curr_tilemap.Empty();
    TArray<uint8> dataArchive;
    FFileHelper::LoadFileToArray(dataArchive, *curr_map);
    int32 n = dataArchive.Num();
    int32 it = 0;
    TMap<char, FVector2D> conv = {
        {'L', FVector2D(-CELL_WIDTH, 0)},
        {'R', FVector2D(CELL_WIDTH, 0)},
        {'D', FVector2D(0, -CELL_WIDTH)},
        {'U', FVector2D(0, CELL_WIDTH)},
        {'C', FVector2D(0, 0)},
    };

    FString Output;
	
	// Create non-euqlidean tiles
    while (it < n) {
        
        int32 len = (int32)dataArchive[it];
        it++;
        len = len + it;
        string coord;
        FVector2D xz = FVector2D(0, 0);
        for (;it < len; ++it) {
            char c = (char)dataArchive[it];
            xz += conv[c];
        }

        FVector vec;
        vec.X = GetAndUnite(dataArchive, &it);
        vec.Y = GetAndUnite(dataArchive, &it);
        vec.Z = GetAndUnite(dataArchive, &it);

        FQuat quat;
        quat.X = GetAndUnite(dataArchive, &it);
        quat.Y = GetAndUnite(dataArchive, &it);
        quat.Z = GetAndUnite(dataArchive, &it);
        quat.W = GetAndUnite(dataArchive, &it);

        GyroVectorD gv = GyroVectorD(vec, quat);
        WorldTile tile = WorldTile(xz, gv);
        curr_tilemap.Add(tile);
    }
    
}

//Generate 2D tilemap
void FWarpGameModule::GenerateTileMap(int type, bool lattice3D, int max_expand) 
{

    FString Output = "";

    SetTileType(type);
    vector<Tile> tiles;
    tiles.push_back(Tile("C", GyroVectorD()));

    FFileManagerGeneric *GFileManager = new FFileManagerGeneric();

    if (!GFileManager->DirectoryExists(*MAP_DIR)) GFileManager->MakeDirectory(*MAP_DIR);

    FString mapName = "Map" + FString::FromInt(type) + (lattice3D ? "L" : "") + ".bin";
    curr_map = MAP_DIR + "/" + mapName;

    FBufferArchive dataArchive;
    FFileHelper::SaveArrayToFile(dataArchive, *curr_map);
	
	
	//Each type of geometry has its own number of tiles
	if (N == 2) {
	   tiles.push_back(Tile("R", GyroVectorD(CELL_WIDTH, 0.0, 0.0)));
	}
	else if (N == 3) {
	   ExpandMap(&tiles, 0, lattice3D);
	   tiles.push_back(Tile("RR", add(tiles.at(1).gv, FVector4(CELL_WIDTH, 0.0, 0.0, 0.0))));
	}
	else {
	   for (int i = 0; i < max_expand; ++i) {
		   ExpandMap(&tiles, i, lattice3D);
	   }
	}

	for (int i = 0; i < tiles.size(); ++i) {
	   Add(tiles[i].coord, tiles[i].gv, curr_map);
	}

}

// Shift tiles by direction
FVector FWarpGameModule::MakeShift(char c) {
    switch (c) {
		case 'L': return FVector(-CELL_WIDTH, 0.0, 0.0);
		case 'R': return FVector(CELL_WIDTH, 0.0, 0.0);
		case 'F': return FVector(0.0, -CELL_WIDTH, 0.0);
		case 'B': return FVector(0.0, CELL_WIDTH, 0.0);
		case 'D': return FVector(0.0, 0.0, -CELL_WIDTH);
		case 'U': return FVector(0.0, 0.0, CELL_WIDTH);
		default: return FVector(0, 0, 0);
    }
}

// Expand 2D tilemap
void FWarpGameModule::ExpandMap(vector<Tile> *tiles, int len, bool lattice3D) {
    for (int i = 0; i < tiles->size(); ++i) {
        string coord = tiles->at(i).coord;
        GyroVectorD gv = tiles->at(i).gv;

        if (coord.length() == len) {
            char last = (coord.length() > 0 ? coord[coord.length() - 1] : '\0');
            if (last != 'L') {
                TrySpawn(tiles, coord + "R", add(gv, MakeShift('R')));
            }
            if (last != 'R') {
                TrySpawn(tiles, coord + "L", add(gv, MakeShift('L')));
            }
            if (last != 'D') {
                TrySpawn(tiles, coord + "U", add(gv, MakeShift('U')));
            }
            if (last != 'U') {
                TrySpawn(tiles, coord + "D", add(gv, MakeShift('D')));
            }
            if (lattice3D) {
                if (last != 'F') {
                    TrySpawn(tiles, coord + "B", add(gv, MakeShift('B')));
                }
                if (last != 'B') {
                    TrySpawn(tiles, coord + "F", add(gv, MakeShift('F')));
                }
            }
        }
    }
}

unsigned char FWarpGameModule::NearbyAfterShift(vector<Tile> tiles, int ix, char c) {
    GyroVectorD gv = add(tiles[ix].gv, MakeShift(c));
    for (int i = 0; i < tiles.size(); ++i) {
        GyroVectorD gv2 = tiles[i].gv;
        if (sqrMagnitude(sub(gv, gv2).vec) < 1e-10) {
            return 1;
        }
    }
    return 0;
}