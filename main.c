//==============================================================================================

/*

TODO
Split screen.

IN PROGRESS

Controller support.
Level number
Points gained i.e. (+100)

DONE

Tue Nov 14 12:52:33 EET 2017

* Bonus tochki se davat za ednovremenno unidhtojeni mnojestvo redove.
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

#define BOARD_SIZE_X 12
#define BOARD_SIZE_Y 20
const c2_t x_boardSize = { .x = BOARD_SIZE_X, .y = BOARD_SIZE_Y };
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
static v2_t ASCIISymbolSize;
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
    bool_t gameOver;
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
    char board[BOARD_SIZE_X * BOARD_SIZE_Y];
} playerSeat_t;

static playerSeat_t x_pls;

static void DrawAtlas( void ) {
    v2_t windowSize = R_GetWindowSize();
    v2_t position = v2xy( windowSize.x - ASCIITextureSize.x, 0 );
    R_ColorC( colRed );
    R_BlendPicV2( position, ASCIITextureSize, v2zero, v2one, ASCIITexture );
}

static void DrawTileKern( c2_t position, int symbol, color_t color, float partx, int shadow ) {
    v2_t st0 = v2xy( ( symbol & 15 ) * ASCIISymbolSize.x, ( symbol / 16 ) * ASCIISymbolSize.y );
    v2_t st1 = v2Add( st0, ASCIISymbolSize );
    st0 = v2xy( st0.x / ASCIITextureSize.x, st0.y / ASCIITextureSize.y );
    st1 = v2xy( st1.x / ASCIITextureSize.x, st1.y / ASCIITextureSize.y );
    v2_t scale = v2Scale( ASCIISymbolSize, PixelSize );
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
    pls->currentShape = x_pls.nextShape;
    pls->nextShape = GetRandomShape();
    pls->currentPos = c2IntToFixed( c2xy( x_boardSize.x / 2 - 2, -x_shapeSize.y + 1 ) );
}

static const char* GetAssetPath( const char *name ) {
    return va( "%sdata/%s", SYS_BaseDir(), name );
}

static void StartNewGame( playerSeat_t *pls ) {
    pls->gameOver = false;
    pls->speed = INITIAL_SPEED;
    pls->numErasedLines = 0;
    pls->nextShape = GetRandomShape();
    pls->score = 0;
    memcpy( pls->board, x_board, sizeof( x_board ) );
    PickShapeAndReset( pls );
}

static void UpdateMusicVolume( void ) {
    float vol = Clampf( VAR_Num( x_musicVolume ), 0, 1 );
    Mix_VolumeMusic( vol * MIX_MAX_VOLUME / 4 );
}

static void Init( void ) {
    x_music = Mix_LoadMUS( GetAssetPath( "ievan_polkka_8bit.ogg" ) );
    UpdateMusicVolume();
    x_soundPop = Mix_LoadWAV( GetAssetPath( "pop.ogg" ) );
    Mix_VolumeChunk( x_soundPop, MIX_MAX_VOLUME / 4 );
    x_soundThud = Mix_LoadWAV( GetAssetPath( "thud.ogg" ) );
    x_soundShift = Mix_LoadWAV( GetAssetPath( "shift.ogg" ) );
    Mix_VolumeChunk( x_soundShift, MIX_MAX_VOLUME / 2 );
    ASCIITexture = R_LoadStaticTextureEx( "cp437_12x12.png", &ASCIITextureSize );
    ASCIISymbolSize = v2Scale( ASCIITextureSize, 1 / 16. );
    x_prevTime = SYS_RealTime();
    Mix_PlayMusic( x_music, -1 );
    StartNewGame( &x_pls );
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

static void DrawBitmap( c2_t screenPos, const char *bitmap, c2_t razmerNaKarta, color_t color ) {
    static const int remap[256] = {
        ['@'] = 1,
        ['#'] = 1 + 11 * 16,
    };
    for ( int i = 0, y = 0; y < razmerNaKarta.y; y++ ) {
        for ( int x = 0; x < razmerNaKarta.x; x++ ) {
            int tile = bitmap[i++];
            DrawTile( c2Add( c2xy( x, y ), screenPos ), remap[tile], color );
        }
    }
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

static const char* GetCurrentBitmap( void ) {
    shape_t *curShape = &x_shapes[x_pls.currentShape];
    return curShape->bitmaps[x_pls.currentBitmap];
}

static bool_t IsCurrentBitmapClipping( const char *board, c2_t fixedPointPos ) {
    return IsBitmapClipping( board, c2FixedToInt( fixedPointPos ), GetCurrentBitmap() );
}

static bool_t TryMove( const char *board, c2_t nextPos ) {
    if ( IsCurrentBitmapClipping( board, nextPos ) ) {
        return false;
    }
    x_pls.currentPos = nextPos;
    return true;
}

static bool_t TryMoveDown( const char *board, int deltaTime ) {
    int butMove = x_pls.butMoveDown == BS_PRESSED;
    int y = x_pls.currentPos.y + deltaTime * x_pls.speed * ( 1 + butMove * 6 ) / 100;
    c2_t nextPos = c2xy( x_pls.currentPos.x, y );
    return TryMove( board, nextPos );
}

static bool_t Drop( playerSeat_t *pls ) {
    CopyBitmap( GetCurrentBitmap(), x_shapeSize, pls->board, x_boardSize, c2FixedToInt( pls->currentPos ) );
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

static void TryMoveHorz( const char *board, int deltaTime ) {
    int dir = ( x_pls.butMoveRight == BS_PRESSED ) 
            - ( x_pls.butMoveLeft == BS_PRESSED );
    if ( dir == 0 ) {
        return;
    }
    int off = dir > 0 ? deltaTime : -deltaTime;
    c2_t nextPos = c2Add( x_pls.currentPos, c2xy( off, 0 ) );
    TryMove( board, nextPos );
}

static void GameUpdate( void ) {
    int now = SYS_RealTime();
    int deltaTime = now - x_prevTime;
    if ( x_pls.gameOver ) {
        if ( VAR_Num( x_hiscore ) < x_pls.score ) {
            VAR_Set( x_hiscore, va( "%d", x_pls.score ), false );
        }
        DrawBitmap( c2zero, x_pls.board, x_boardSize, colWhite );
        Print( c2xy( x_boardSize.x / 2 - 3, x_boardSize.y / 2 - 1 ), "GAME OVER", colRed );
        if ( now & 256 ) { 
            Print( c2xy( x_boardSize.x / 2 - 4, x_boardSize.y / 2 ), "Press 'Space'", colRed );
        }
    } else {
        TryMoveHorz( x_pls.board, deltaTime );
        if ( ! TryMoveDown( x_pls.board, deltaTime ) ) {
            if ( ! Drop( &x_pls ) ) {
                x_pls.gameOver = true;
                Mix_HaltMusic();
            } else {
                EraseFilledLines( &x_pls );
                PickShapeAndReset( &x_pls );
                LatchButtons( &x_pls );
                if ( x_pls.numErasedLines >= 10 ) {
                    CON_Printf( "SPEED UP!\n" );
                    x_pls.numErasedLines = 0;
                    x_pls.speed += x_pls.speed / 4;
                }
            }
        } else {
            DrawBitmap( c2FixedToInt( x_pls.currentPos ), GetCurrentBitmap(), x_shapeSize, colGreen );
        }
        DrawBitmap( c2zero, x_pls.board, x_boardSize, colWhite );
    }
    Print( c2xy( x_boardSize.x + 1, 1 ), "NEXT", colCyan );
    shape_t *nextShape = &x_shapes[x_pls.nextShape];
    DrawBitmap( c2xy( x_boardSize.x + 1, 2 ), nextShape->bitmaps[0], x_shapeSize, colGreen );
    Print( c2xy( x_boardSize.x + 1, 7 ), "SCORE", colCyan );
    Print( c2xy( x_boardSize.x + 1, 8 ), va( "%d", x_pls.score ), colWhite );
    Print( c2xy( x_boardSize.x + 1, 10 ), "HISCORE", colCyan );
    Print( c2xy( x_boardSize.x + 1, 11 ), va( "%d", ( int )VAR_Num( x_hiscore ) ), colWhite );
    x_prevTime = now;
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
    TryMove( pls->board, nextPos );
}

static void TryMoveLeft( playerSeat_t *pls ) {
    c2_t nextPos = c2xy( ( pls->currentPos.x & ~255 ), pls->currentPos.y );
    TryMove( pls->board, nextPos );
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
    DoRightButton( &x_pls, CMD_Engaged() );
}

static void MoveLeft_f( void ) {
    DoLeftButton( &x_pls, CMD_Engaged() );
}

static void HorzAxis_f( void ) {
    int as = CMD_ArgvAxisSign();
    DoRightButton( &x_pls, as > 0 );
    DoLeftButton( &x_pls, as < 0 );
}

static void Rotate( void ) {
    shape_t *curShape = &x_shapes[x_pls.currentShape];
    int bmpIdx = ( x_pls.currentBitmap + 1 ) % curShape->numBitmaps;
    if ( ! IsBitmapClipping( x_pls.board, c2FixedToInt( x_pls.currentPos ), curShape->bitmaps[bmpIdx] ) ) {
        x_pls.currentBitmap = bmpIdx;
    }
}

static void Rotate_f( void ) {
    if ( DoButton( CMD_Engaged(), &x_pls.butRotate ) ) {
        Rotate();
    }
}

static void MoveDown_f( void ) {
    DoButton( CMD_Engaged(), &x_pls.butMoveDown );
}

static void Restart_f( void ) {
    if ( x_pls.gameOver ) {
        Mix_PlayMusic( x_music, -1 );
        StartNewGame( &x_pls );
    }
}

static void RegisterVars( void ) {
    x_showAtlas = VAR_Register( "showAtlas", "0" );
    x_hiscore = VAR_Register( "hiscore", "0" );
    x_musicVolume = VAR_Register( "musicVolume", "1" );
    CMD_Register( "moveLeft", MoveLeft_f );
    CMD_Register( "moveRight", MoveRight_f );
    CMD_Register( "moveDown", MoveDown_f );
    CMD_Register( "rotate", Rotate_f );
    CMD_Register( "restartGame", Restart_f );
    CMD_Register( "horizontalMove", HorzAxis_f );
    I_Bind( "Left", "moveLeft" );
    I_Bind( "Right", "moveRight" );
    I_Bind( "Down", "moveDown" );
    I_Bind( "Up", "rotate" );
    I_Bind( "Space", "!restartGame" );
    I_Bind( "joystick 0 axis 0", "horizontalMove" );
    I_Bind( "joystick 0 axis 1", "-rotate ; +moveDown" );
    //I_Bind( "joystick 0 axis 1", "+moveDown" );
}

static void AppFrame( void ) {
    PixelSize = Maxi( R_GetWindowSize().y / ( ASCIISymbolSize.y * x_boardSize.y ), 1 );
    GameUpdate();
    if ( VAR_Num( x_showAtlas ) ) {
        DrawAtlas();
    }
    if ( VAR_Changed( x_musicVolume ) ) {
        UpdateMusicVolume();
    }
    SDL_Delay( 10 );
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "tetris", RegisterVars, Init, AppFrame, NULL, 0 );
    return 0;
}
