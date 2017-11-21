//==============================================================================================

/*

TODO
Any key/button handler exposed from events.c
Controller support.
    * rotate with any up axis
    * rotate with any button
    * move down with any down axis
    * move horizontal any horizontal axis
    * move using the hat switch
    hotplug and controllers.
Level number
Points gained i.e. (+100)

IN PROGRESS

Split screen.
    * encode controller id in the command
    * draw the board with an offset
    * remove all x_pls
    * get player by command device
    don't kill off a seat before both seats are done.

DONE

Tue Nov 14 12:52:33 EET 2017

* Bonus tochki se davat za ednovremenno unishtojeni mnojestvo redove.
* Tochki se davat za vseki iztrit red.
* Izobraziavane na tochkite kato chast ot potrebitelskia interfeis.
* Hiscore

* Printirane sas tailove
* Sledvashtata figura koiato shte bade aktivna sled tekushtata e pokazana na ekrana kato chast ot potrebitelskia interfeis.

* Igrata stava po-barza/vdiga se nivoto sled n na broi unishtojeni redove.
* Ako figurata ne moje da bade mestena dokato e chastichno izvan poleto, igrata e zagubena
*     Restart na igrata s interval

Mon Nov 13 16:10:20 EET 2017

* Kogato figurite padnat na danoto na poleto i ochertaiat linia s ostatacite na drugite figuri, zapalneniat red se unishtojava.

* Igrachat moje da varti aktivnata figura sas strelka nagore.
*     Izpolzvaiki gorna strelka

* Kogato figurite padnat na danoto na poleto, nova (proizvolna) figura se dava na igracha za manipulacia.
    * Proverka dali figura e na danoto (ne moje da se manipulira poveche).
    * Kopirane na starata figura varhu igralnoto pole. 
    * Sazdavene na nova figura.
    * Figurite sa s razlichna forma.

Tue Sep 12 16:16:19 EEST 2017

* Igrachat moje da mesti aktivnata figura na ekrana.
*    Liava strelka - mesti figurata naliavo s edno pole.
*    Diasna strelka - mesti figurata nadiasno.
*    Dolna strelka - mesti figurata nadolu
* Risuvane na igralnoto pole.
* Risuvane na figura.
* Animirane na figurata.
* Proverka za zastapvane s igralnoto pole.

*/

#define DBG_PRINT CON_Printf
#include "zhost.h"

const c2_t x_boardSize = { .x = 12, .y = 21 };
#define SEAT_SIZE (x_boardSize.x+6)
char x_board[] =
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"############"
;

const c2_t x_shapeSize = { .x = 4, .y = 4 };

typedef struct {
    int numBitmaps;
    const char *bitmaps[4];
} shape_t;

static shape_t x_shapes[] = {
    {
        .numBitmaps = 2,
        .bitmaps = {
            " @  "
            " @  "
            " @  "
            " @  "
            ,
            "    "
            "@@@@"
            "    "
            "    "
        },
    },
    {
        .numBitmaps = 2,
        .bitmaps = {
            "    "
            "@@  "
            " @@ "
            "    "
            ,
            "    "
            "  @ "
            " @@ "
            " @  "
        },
    },
    {
        .numBitmaps = 2,
        .bitmaps = {
            "    "
            "  @@"
            " @@ "
            "    "
            , 
            "    "
            " @  "
            " @@ "
            "  @ "
        },
    },
    {
        .numBitmaps = 1,
        .bitmaps = {
            "    "
            "    "
            " @@ "
            " @@ "
        },
    },
    {
        .numBitmaps = 4,
        .bitmaps = {
            "    "
            "    "
            " @@@"
            "  @ "
            , 
            "    "
            "  @ "
            "  @@"
            "  @ "
            , 
            "    "
            "  @ "
            " @@@"
            "    "
            , 
            "    "
            "  @ "
            " @@ "
            "  @ "
        },
    },
    {
        .numBitmaps = 4,
        .bitmaps = {
            "    "
            " @@ "
            " @  "
            " @  "
            , 
            "    "
            "@   "
            "@@@ "
            "    "
            , 
            "    "
            " @  "
            " @  "
            "@@  "
            , 
            "    "
            "    "
            "@@@ "
            "  @ "
        },
    },
    {
        .numBitmaps = 4,
        .bitmaps = {
            "    "
            " @@ "
            "  @ "
            "  @ "
            ,
            "    "
            "    "
            " @@@"
            " @  "
            ,
            "    "
            "  @ "
            "  @ "
            "  @@"
            ,
            "    "
            "   @"
            " @@@"
            "    "
        },
    },
};

static const int x_numShapes = sizeof( x_shapes ) / sizeof( *x_shapes );
static Mix_Music *x_music;
static Mix_Chunk *x_soundPop;
static Mix_Chunk *x_soundThud;
static Mix_Chunk *x_soundShift;
static rImage_t *ASCIITexture;
static v2_t ASCIITextureSize;
static v2_t TileSize;
static int PixelSize;
static var_t *x_showAtlas;
static var_t *x_hiscore;
static var_t *x_musicVolume;
static int x_prevTime;

#define INITIAL_SPEED 40

typedef enum {
    BS_RELEASED,
    BS_PRESSED,
    BS_LATCHED,
} butState_t;

typedef struct {
    int deviceType;
    int deviceId;
    bool_t activated;
    int nextShape;
    int currentShape;
    int currentBitmap;
    c2_t currentPos;
    int speed;
    butState_t butMoveDown;
    butState_t butMoveLeft;
    butState_t butMoveRight;
    butState_t butRotate;
    int numErasedLines;
    int score;
    char board[sizeof( x_board )];
} playerSeat_t;

static playerSeat_t x_pls[2];

static void DrawAtlas( void ) {
    v2_t windowSize = R_GetWindowSize();
    v2_t position = v2xy( windowSize.x - ASCIITextureSize.x, 0 );
    R_ColorC( colRed );
    R_BlendPicV2( position, ASCIITextureSize, v2zero, v2one, ASCIITexture );
}

static void DrawTileKern( c2_t position, int symbol, color_t color, float partx, int shadow ) {
    v2_t st0 = v2xy( ( symbol & 15 ) * TileSize.x, ( symbol / 16 ) * TileSize.y );
    v2_t st1 = v2Add( st0, TileSize );
    st0 = v2xy( st0.x / ASCIITextureSize.x, st0.y / ASCIITextureSize.y );
    st1 = v2xy( st1.x / ASCIITextureSize.x, st1.y / ASCIITextureSize.y );
    v2_t scale = v2Scale( TileSize, PixelSize );
    partx *= scale.x;
    if ( shadow > 0 ) {
        R_ColorC( colBlack );
        R_BlendPicV2( v2xy( position.x * partx + shadow * PixelSize, position.y * scale.y + shadow * PixelSize), scale, st0, st1, ASCIITexture );
    }
    R_ColorC( color );
    R_BlendPicV2( v2xy( position.x * partx, position.y * scale.y ), scale, st0, st1, ASCIITexture );
}

static void DrawTile( c2_t position, int symbol, color_t color ) {
    DrawTileKern( position, symbol, color, 1, 0 );
}

static int GetRandomShape( void ) {
    return COM_Rand() % x_numShapes;
}

static c2_t c2IntToFixed( c2_t c ) {
    return c2LShifts( c, 8 );
}

static c2_t c2FixedToInt( c2_t c ) {
    return c2RShifts( c, 8 );
}

static void LatchButton( butState_t *button ) {
    *button = *button == BS_PRESSED ? BS_LATCHED : BS_RELEASED;
}

static void LatchButtons( playerSeat_t *pls ) {
    LatchButton( &pls->butMoveDown );
    LatchButton( &pls->butMoveLeft );
    LatchButton( &pls->butMoveRight );
    LatchButton( &pls->butRotate );
}

static void PickShapeAndReset( playerSeat_t *pls ) {
    pls->currentBitmap = 0;
    pls->currentShape = pls->nextShape;
    pls->nextShape = GetRandomShape();
    pls->currentPos = c2IntToFixed( c2xy( x_boardSize.x / 2 - 2, -x_shapeSize.y + 1 ) );
}

static const char* GetAssetPath( const char *name ) {
    return va( "%sdata/%s", SYS_BaseDir(), name );
}

static void StartNewGame( int deviceType, int deviceId, playerSeat_t *pls ) {
    memset( pls, 0, sizeof( *pls ) );
    pls->activated = true;
    pls->deviceType = deviceType;
    pls->deviceId = deviceId;
    pls->speed = INITIAL_SPEED;
    pls->nextShape = GetRandomShape();
    memcpy( pls->board, x_board, sizeof( pls->board ) );
    PickShapeAndReset( pls );
    if ( ! Mix_PlayingMusic() ) {
        Mix_PlayMusic( x_music, -1 );
    }
}

static void UpdateMusicVolume( void ) {
    float vol = Clampf( VAR_Num( x_musicVolume ), 0, 1 );
    Mix_VolumeMusic( vol * MIX_MAX_VOLUME / 4 );
}

static playerSeat_t* ArgvSeat( void ) {
    int type = CMD_ArgvDeviceType();
    int id = CMD_ArgvDeviceId();
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *pls = &x_pls[i];
        if ( pls->deviceType == type && pls->deviceId == id ) {
            return pls;
        }
    }
    return x_pls;
}

static bool_t IsAnySeatActive( void ) {
    for ( int i = 0; i < 2; i++ ) {
        if ( x_pls[i].activated ) {
            return true;
        }
    }
    return false;
}

static playerSeat_t* GetFreeSeat( void ) {
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *pls = &x_pls[i];
        if ( ! pls->activated ) {
            return pls;
        }
    }
    return NULL;
}

static bool_t IsControllerTaken( int type, int id ) {
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *pls = &x_pls[i];
        if ( pls->deviceType == type && pls->deviceId == id ) {
            return true;
        }
    }
    return false;
}

static bool_t OnAnyButton_f( int code, bool_t down ) {
    if ( down ) {
        playerSeat_t *pls = GetFreeSeat();
        if ( pls ) {
            int type = I_IsJoystickCode( code ) ? 'j' : 'k';
            int id = '0' + I_DeviceOfCode( code );
            if ( ! IsControllerTaken( type, id ) ) {
                StartNewGame( type, id, pls );
                return true;
            }
        }
    }
    return false;
}

static void GameOver( playerSeat_t *pls ) {
    pls->activated = false;
    pls->deviceType = 0;
    pls->deviceId = 0;
    if ( ! IsAnySeatActive() ) {
        Mix_HaltMusic();
    }
}

static void Init( void ) {
    E_SetButtonOverride( OnAnyButton_f );
    x_music = Mix_LoadMUS( GetAssetPath( "ievan_polkka_8bit.ogg" ) );
    x_soundPop = Mix_LoadWAV( GetAssetPath( "pop.ogg" ) );
    x_soundThud = Mix_LoadWAV( GetAssetPath( "thud.ogg" ) );
    x_soundShift = Mix_LoadWAV( GetAssetPath( "shift.ogg" ) );

    UpdateMusicVolume();
    Mix_VolumeChunk( x_soundPop, MIX_MAX_VOLUME / 4 );
    Mix_VolumeChunk( x_soundShift, MIX_MAX_VOLUME / 2 );

    ASCIITexture = R_LoadStaticTextureEx( "cp437_12x12.png", &ASCIITextureSize );
    TileSize = v2Scale( ASCIITextureSize, 1 / 16. );

    x_prevTime = SYS_RealTime();
    for ( int i = 0; i < 2; i++ ) {
        GameOver( &x_pls[i] );
    }
}

static int ReadTile( c2_t pos, const char *bmp, c2_t bmpSz ) {
    if ( pos.x >= 0 && pos.x < bmpSz.x
        && pos.y >= 0 && pos.y < bmpSz.y ) {
        return bmp[pos.x + pos.y * bmpSz.x];
    }
    return ' ';
}

static int ReadTileOnBoard( const char *board, c2_t pos ) {
    if ( pos.x <= 0 || pos.x >= x_boardSize.x - 1 ) {
        return '#';
    }
    return ReadTile( pos, board, x_boardSize );
}

static void CopyTile( int tile, c2_t pos, char *bmp, c2_t bmpSz )
{
    if ( pos.x >= 0 && pos.x < bmpSz.x
        && pos.y >= 0 && pos.y < bmpSz.y ) {
        bmp[pos.x + pos.y * bmpSz.x] = tile;
    }
}

static bool_t IsBlank( int tile ) {
    return tile == ' ';
}

static void CopyBitmap( const char *src, c2_t srcSz, 
                            char *dst, c2_t dstSz, c2_t posInDst ) {
    for ( int y = 0; y < srcSz.y; y++ ) {
        for ( int x = 0; x < srcSz.x; x++ ) {
            c2_t xy = c2xy( x, y );
            int tile = ReadTile( xy, src, srcSz );
            if ( ! IsBlank( tile ) ) {
                c2_t final = c2Add( xy, posInDst );
                CopyTile( tile, final, dst, dstSz );
            }
        }
    }
}

static void DrawBitmap( c2_t screenPos, const char *bitmap, c2_t sz, color_t color ) {
    static const int remap[256] = {
        ['@'] = 1,
        ['#'] = 1 + 11 * 16,
    };
    for ( int i = 0, y = 0; y < sz.y; y++ ) {
        for ( int x = 0; x < sz.x; x++ ) {
            int tile = bitmap[i++];
            DrawTile( c2Add( c2xy( x, y ), screenPos ), remap[tile], color );
        }
    }
}

static void DrawBitmapOff( c2_t off, c2_t screenPos, const char *bitmap, c2_t sz, color_t color ) {
    DrawBitmap( c2Add( off, screenPos ), bitmap, sz, color );
}

static void Print( c2_t pos, const char *string, color_t color ) {
    R_ColorC( color );
    float part = 0.65f;
    pos.x /= part;
    for ( const char *p = string; *p; p++ ) {
        DrawTileKern( pos, *p, color, part, 1 );
        pos.x++;
    }
}

static void PrintOff( c2_t off, c2_t pos, const char *string, color_t color ) {
    Print( c2Add( pos, off ), string, color );
}

static bool_t IsBitmapClipping( const char *board, c2_t posOnBoard, const char *bmp ) {
    for ( int y = 0; y < x_shapeSize.y; y++ ) {
        for ( int x = 0; x < x_shapeSize.x; x++ ) {
            c2_t pos = c2xy( x, y );
            int tile = ReadTile( pos, bmp, x_shapeSize );
            if ( ! IsBlank( tile ) ) {
                int tileOnBoard = ReadTileOnBoard( board, c2Add( pos, posOnBoard ) );
                if ( ! IsBlank( tileOnBoard ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

static const char* GetCurrentBitmap( playerSeat_t *pls ) {
    shape_t *curShape = &x_shapes[pls->currentShape];
    return curShape->bitmaps[pls->currentBitmap];
}

static bool_t IsCurrentBitmapClipping( playerSeat_t *pls, c2_t fixedPointPos ) {
    return IsBitmapClipping( pls->board, c2FixedToInt( fixedPointPos ), GetCurrentBitmap( pls ) );
}

static bool_t TryMove( playerSeat_t *pls, c2_t nextPos ) {
    if ( IsCurrentBitmapClipping( pls, nextPos ) ) {
        return false;
    }
    pls->currentPos = nextPos;
    return true;
}

static int GetHoldSpeed( bool_t butDown, int baseSpeed, int deltaTime ) {
    return deltaTime * baseSpeed * ( 1 + butDown * 6 ) / 100;
}

static bool_t TryMoveDown( playerSeat_t *pls, int deltaTime ) {
    int speed = GetHoldSpeed( pls->butMoveDown == BS_PRESSED, pls->speed, deltaTime );
    c2_t nextPos = c2xy( pls->currentPos.x, pls->currentPos.y + speed );
    return TryMove( pls, nextPos );
}

static bool_t Drop( playerSeat_t *pls ) {
    CopyBitmap( GetCurrentBitmap( pls ), x_shapeSize, pls->board, x_boardSize, c2FixedToInt( pls->currentPos ) );
    Mix_PlayChannel( -1, x_soundThud, 0 );
    pls->score += pls->speed / 2;
    return pls->currentPos.y + x_shapeSize.y >= 0;
}

static void EraseFilledLines( playerSeat_t *pls ) {
    int bonus = 1;
    bool_t result = false;
    for ( int y = x_boardSize.y - 2; y >= 1; ) {
        int numFull = 0;
        for ( int x = 0; x < x_boardSize.x; x++ ) {
            int tile = ReadTile( c2xy( x, y ), pls->board, x_boardSize );
            if ( ! IsBlank( tile ) ) {
                numFull++;
            }
        }
        if ( numFull == x_boardSize.x ) {
            result = true;
            pls->numErasedLines++;
            bonus *= 2;
            CON_Printf( "Pop a line. TOTAL LINES: %d\n", pls->numErasedLines );
            for ( int i = y; i >= 1; i-- ) {
                char *dst = &pls->board[( i - 0 ) * x_boardSize.x];
                char *src = &pls->board[( i - 1 ) * x_boardSize.x];
                memcpy( dst, src, x_boardSize.x );
            }
        } else {
            y--;
        }
    }
    if ( result ) {
        pls->score += bonus * pls->speed * 4;
        Mix_PlayChannel( -1, x_soundPop, 0 );
    }
}

static void TryMoveHorz( playerSeat_t *pls, int deltaTime ) {
    int dir = ( pls->butMoveRight == BS_PRESSED ) 
            - ( pls->butMoveLeft == BS_PRESSED );
    if ( dir != 0 ) {
        int speed = GetHoldSpeed( true, INITIAL_SPEED, deltaTime ) / 2;
        int off = dir > 0 ? speed : -speed;
        c2_t nextPos = c2xy( pls->currentPos.x + off, pls->currentPos.y );
        TryMove( pls, nextPos );
    }
}

static void UpdateHiscore( playerSeat_t *pls ) {
    if ( VAR_Num( x_hiscore ) < pls->score ) {
        VAR_Set( x_hiscore, va( "%d", pls->score ), false );
    }
}

static void GameUpdate( c2_t boffset, playerSeat_t *pls, int deltaTime ) {
    if ( ! pls->activated ) {
        DrawBitmapOff( boffset, c2zero, pls->board, x_boardSize, colWhite );
        //PrintOff( boffset, c2xy( x_boardSize.x / 2 - 3, x_boardSize.y / 2 - 1 ), 
        //        "GAME OVER", colRed );
        if ( SYS_RealTime() & 256 ) { 
            PrintOff( boffset, c2xy( x_boardSize.x / 2 - 5, x_boardSize.y / 2 ), 
                    "Press Any Button", colMagenta );
        }
    } else {
        TryMoveHorz( pls, deltaTime );
        if ( ! TryMoveDown( pls, deltaTime ) ) {
            if ( ! Drop( pls ) ) {
                GameOver( pls );
            } else {
                EraseFilledLines( pls );
                PickShapeAndReset( pls );
                LatchButtons( pls );
                if ( pls->numErasedLines >= 10 ) {
                    CON_Printf( "SPEED UP!\n" );
                    pls->numErasedLines = 0;
                    pls->speed += pls->speed / 4;
                }
            }
        } else {
            DrawBitmapOff( boffset, c2FixedToInt( pls->currentPos ), GetCurrentBitmap( pls ), x_shapeSize, colGreen );
        }
        DrawBitmap( boffset, pls->board, x_boardSize, colWhite );
    }
    PrintOff( boffset, c2xy( x_boardSize.x + 1, 1 ), "NEXT", colCyan );
    shape_t *nextShape = &x_shapes[pls->nextShape];
    DrawBitmapOff( boffset, c2xy( x_boardSize.x + 1, 2 ), 
                    nextShape->bitmaps[0], x_shapeSize, colGreen );
    PrintOff( boffset, c2xy( x_boardSize.x + 1, 7 ), "SCORE", colCyan );
    PrintOff( boffset, c2xy( x_boardSize.x + 1, 8 ), va( "%d", pls->score ), colWhite );
    PrintOff( boffset, c2xy( x_boardSize.x + 1, 10 ), "HISCORE", colCyan );
    PrintOff( boffset, c2xy( x_boardSize.x + 1, 11 ), va( "%d", ( int )VAR_Num( x_hiscore ) ), colWhite );
    UpdateHiscore( pls );
}

static bool_t DoButton( bool_t down, butState_t *button ) {
    bool_t result = false;
    if ( down ) {
        if ( *button != BS_PRESSED && *button != BS_LATCHED ) {
            Mix_PlayChannel( -1, x_soundShift, 0 );
            *button = BS_PRESSED;
            result = true;
        }
    } else {
        *button = BS_RELEASED;
    }
    return result;
}

//static bool_t DoLatchButton( bool_t down, butState_t *button ) {
//    if ( DoButton( down, button ) ) {
//        LatchButton( button );
//        return true;
//    }
//    return false;
//}

static void TryMoveRight( playerSeat_t *pls ) {
    c2_t nextPos = c2xy( ( pls->currentPos.x & ~255 ) + 256, pls->currentPos.y );
    TryMove( pls, nextPos );
}

static void TryMoveLeft( playerSeat_t *pls ) {
    c2_t nextPos = c2xy( ( pls->currentPos.x & ~255 ), pls->currentPos.y );
    TryMove( pls, nextPos );
}

static void DoRightButton( playerSeat_t *pls, bool_t down ) {
    if ( DoButton( down, &pls->butMoveRight ) ) {
        TryMoveRight( pls );
    }
}

static void DoLeftButton( playerSeat_t *pls, bool_t down ) {
    if ( DoButton( down, &pls->butMoveLeft ) ) {
        TryMoveLeft( pls );
    }
}

static void MoveRight_f( void ) {
    DoRightButton( ArgvSeat(), CMD_ArgvEngaged() );
}

static void MoveLeft_f( void ) {
    DoLeftButton( ArgvSeat(), CMD_ArgvEngaged() );
}

static void HorzAxis_f( void ) {
    int as = CMD_ArgvAxisSign();
    DoRightButton( ArgvSeat(), as > 0 );
    DoLeftButton( ArgvSeat(), as < 0 );
}

static void Rotate( playerSeat_t *pls ) {
    shape_t *curShape = &x_shapes[pls->currentShape];
    int bmpIdx = ( pls->currentBitmap + 1 ) % curShape->numBitmaps;
    if ( ! IsBitmapClipping( pls->board, c2FixedToInt( pls->currentPos ), curShape->bitmaps[bmpIdx] ) ) {
        pls->currentBitmap = bmpIdx;
    }
}

static void Rotate_f( void ) {
    playerSeat_t *pls = ArgvSeat();
    if ( DoButton( CMD_ArgvEngaged(), &pls->butRotate ) ) {
        Rotate( pls );
    }
}

static void MoveDown_f( void ) {
    DoButton( CMD_ArgvEngaged(), &ArgvSeat()->butMoveDown );
}

static void RegisterVars( void ) {
    x_showAtlas = VAR_Register( "showAtlas", "0" );
    x_hiscore = VAR_Register( "hiscore", "0" );
    x_musicVolume = VAR_Register( "musicVolume", "1" );
    CMD_Register( "moveLeft", MoveLeft_f );
    CMD_Register( "moveRight", MoveRight_f );
    CMD_Register( "moveDown", MoveDown_f );
    CMD_Register( "rotate", Rotate_f );
    CMD_Register( "horizontalMove", HorzAxis_f );
    I_Bind( "Left", "moveLeft" );
    I_Bind( "Right", "moveRight" );
    I_Bind( "Down", "moveDown" );
    I_Bind( "Up", "rotate" );
    for ( int joy = 0; joy < I_MAX_JOYSTICKS; joy++ ) {
        I_Bind( va( "joystick %d axis 0", joy ), "horizontalMove" );
        I_Bind( va( "joystick %d axis 1", joy ), "-rotate ; +moveDown" );
        for ( int button = 0; button < I_MAX_BUTTONS; button++ ) {
            const char *str = va( "joystick %d button %d", joy, button );
            I_Bind( str, "rotate" );
        }
        I_Bind( va( "joystick %d hat horizontal", joy ), "horizontalMove" );
        I_Bind( va( "joystick %d hat vertical", joy ), "-rotate ; +moveDown" );
    }
}

static void AppFrame( void ) {
    v2_t ws = R_GetWindowSize();
    int szx = Maxi( ws.x / ( TileSize.x * SEAT_SIZE * 2 ), 1 );
    int szy = Maxi( ws.y / ( TileSize.y * x_boardSize.y ), 1 );
    PixelSize = Mini( szx, szy );
    c2_t tileScreen = c2Divs( c2Div( c2v2( ws ), c2v2( TileSize ) ), PixelSize );
    int now = SYS_RealTime();
    int deltaTime = now - x_prevTime;
    GameUpdate( c2xy( 0, 0 ), &x_pls[0], deltaTime );
    GameUpdate( c2xy( tileScreen.x - SEAT_SIZE, 0 ), &x_pls[1], deltaTime );
    if ( VAR_Num( x_showAtlas ) ) {
        DrawAtlas();
    }
    if ( VAR_Changed( x_musicVolume ) ) {
        UpdateMusicVolume();
    }
    SDL_Delay( 10 );
    x_prevTime = now;
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "tetris", RegisterVars, Init, AppFrame, NULL, 0 );
    return 0;
}
