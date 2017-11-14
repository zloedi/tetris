#define DBG_PRINT CON_Printf
#include "zhost.h"

const c2_t x_boardSize = { .x = 12, .y = 20 };
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

typedef struct {
    c2_t size;
    int numBitmaps;
    const char *bitmaps[4];
} figure_t;

static figure_t x_figures[] = {
    {
        .size = { .x = 4, .y = 4 },
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
        .size = { .x = 3, .y = 3 },
        .numBitmaps = 2,
        .bitmaps = {
            "   "
            "@@ "
            " @@"
            ,
            " @ "
            "@@ "
            "@  "
        },
    },
    {
        .size = { .x = 3, .y = 3 },
        .numBitmaps = 2,
        .bitmaps = {
            "   "
            " @@"
            "@@ "
            ,
            "@  "
            "@@ "
            " @ "
        },
    },
    {
        .size = { .x = 2, .y = 2 },
        .numBitmaps = 1,
        .bitmaps = {
            "@@"
            "@@"
        },
    },
    {
        .size = { .x = 3, .y = 3 },
        .numBitmaps = 4,
        .bitmaps = {
            "   "
            "@@@"
            " @ "
            ,
            " @ "
            " @@"
            " @ "
            ,
            " @ "
            "@@@"
            "   "
            ,
            " @ "
            "@@ "
            " @ "
        },
    },
    {
        .size = { .x = 3, .y = 3 },
        .numBitmaps = 4,
        .bitmaps = {
            " @@"
            " @ "
            " @ "
            ,
            "@  "
            "@@@"
            "   "
            ,
            " @ "
            " @ "
            "@@ "
            ,
            "   "
            "@@@"
            "  @"
        },
    },
    {
        .size = { .x = 3, .y = 3 },
        .numBitmaps = 4,
        .bitmaps = {
            "@@ "
            " @ "
            " @ "
            ,
            "   "
            "@@@"
            "@  "
            ,
            " @ "
            " @ "
            " @@"
            ,
            "  @"
            "@@@"
            "   "
        },
    },
};

static const int x_numFigures = sizeof( x_figures ) / sizeof( *x_figures );
static Mix_Chunk *x_soundPop;
static Mix_Chunk *x_soundThud;
static Mix_Chunk *x_soundShift;
static bool_t x_gameOver;
static rImage_t *ASCIITexture;
static v2_t ASCIITextureSize;
static v2_t ASCIISymbolSize;
static int PixelSize;
static int x_currentFigure;
static int x_nextFigure;
static var_t *x_showAtlas;

static void DrawAtlas( void ) {
    v2_t windowSize = R_GetWindowSize();
    v2_t position = v2xy( windowSize.x - ASCIITextureSize.x, 0 );
    R_ColorC( colRed );
    R_BlendPicV2( position, ASCIITextureSize, v2zero, v2one, ASCIITexture );
}

static void DrawTileKern( c2_t position, int symbol, float partx ) {
    v2_t st0 = v2xy( ( symbol & 15 ) * ASCIISymbolSize.x, ( symbol / 16 ) * ASCIISymbolSize.y );
    v2_t st1 = v2Add( st0, ASCIISymbolSize );
    st0 = v2xy( st0.x / ASCIITextureSize.x, st0.y / ASCIITextureSize.y );
    st1 = v2xy( st1.x / ASCIITextureSize.x, st1.y / ASCIITextureSize.y );
    v2_t scale = v2Scale( ASCIISymbolSize, PixelSize );
    partx *= scale.x;
    R_BlendPicV2( v2xy( position.x * partx, position.y * scale.y ), scale, st0, st1, ASCIITexture );
}

static void DrawTile( c2_t position, int symbol ) {
    DrawTileKern( position, symbol, 1 );
}

static bool_t PickFigureAndReset( void );

static const char* GetAssetPath( const char *name ) {
    return va( "%sdata/%s", SYS_BaseDir(), name );
}

static int GetRandomFigure( void ) {
    return COM_Rand() % x_numFigures;
}

static void Init( void ) {
    x_soundPop = Mix_LoadWAV( GetAssetPath( "pop.ogg" ) );
    Mix_VolumeChunk( x_soundPop, MIX_MAX_VOLUME / 4 );
    x_soundThud = Mix_LoadWAV( GetAssetPath( "thud.ogg" ) );
    x_soundShift = Mix_LoadWAV( GetAssetPath( "shift.ogg" ) );
    Mix_VolumeChunk( x_soundShift, MIX_MAX_VOLUME / 2 );
    ASCIITexture = R_LoadStaticTextureEx( "cp437_12x12.png", &ASCIITextureSize );
    ASCIISymbolSize = v2Scale( ASCIITextureSize, 1 / 16. );
    x_nextFigure = GetRandomFigure();
    PickFigureAndReset();
}

//==============================================================================================

/*

TODO
Bonus tochki se davat za ednovremenno unidhtojeni mnojestvo redove.

IN PROGRESS

Tochki se davat za vseki iztrit red.
Izobraziavane na tochkite kato chast ot potrebitelskia interfeis.

DONE

Tue Nov 14 12:52:33 EET 2017

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

static int x_currentBitmap;
static c2_t x_currentPos;
static int x_prevTime;
#define INITIAL_SPEED 64
static int x_speed = INITIAL_SPEED;
static int x_buttonDown;
static int x_numErasedLines;

static int ReadTile( c2_t pos, const char *bmp, c2_t bmpSz ) {
    if ( pos.x >= 0 && pos.x < bmpSz.x
        && pos.y >= 0 && pos.y < bmpSz.y ) {
        return bmp[pos.x + pos.y * bmpSz.x];
    }
    return ' ';
}

static int ReadTileOnBoard( c2_t pozicia ) {
    return ReadTile( pozicia, x_board, x_boardSize );
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
    R_ColorC( color );
    for ( int i = 0, y = 0; y < razmerNaKarta.y; y++ ) {
        for ( int x = 0; x < razmerNaKarta.x; x++ ) {
            int tile = bitmap[i++];
            DrawTile( c2Add( c2xy( x, y ), screenPos ), remap[tile] );
        }
    }
}

static void Print( c2_t pos, const char *string, color_t color ) {
    R_ColorC( color );
    float part = 0.65f;
    pos.x /= part;
    for ( const char *p = string; *p; p++ ) {
        DrawTileKern( pos, *p, part );
        pos.x++;
    }
}

static bool_t IsBitmapClipping( c2_t boardPos, const char *bmp, c2_t bmpSize ) {
    for ( int y = 0; y < bmpSize.y; y++ ) {
        for ( int x = 0; x < bmpSize.x; x++ ) {
            c2_t pos = c2xy( x, y );
            int tile = ReadTile( pos, bmp, bmpSize );
            if ( ! IsBlank( tile ) ) {
                int tileOnBoard = ReadTileOnBoard( c2Add( pos, boardPos ) );
                if ( ! IsBlank( tileOnBoard ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

static c2_t IntToFixed( c2_t c ) {
    return c2LShifts( c, 8 );
}

static c2_t FixedToInt( c2_t c ) {
    return c2RShifts( c, 8 );
}

static void GetCurrentBitmap( const char **outBmp, c2_t *outBmpSz ) {
    figure_t *curFigure = &x_figures[x_currentFigure];
    *outBmp = curFigure->bitmaps[x_currentBitmap];
    *outBmpSz = curFigure->size;
}

static bool_t IsCurrentBitmapClipping( c2_t fixedPointPos ) {
    const char *bmp;
    c2_t bmpSz;
    GetCurrentBitmap( &bmp, &bmpSz );
    return IsBitmapClipping( FixedToInt( fixedPointPos ), bmp, bmpSz );
}

static bool_t PickFigureAndReset( void ) {
    x_currentBitmap = 0;
    x_currentFigure = x_nextFigure;
    x_nextFigure = GetRandomFigure();
    figure_t *curFig = &x_figures[x_currentFigure];
    x_currentPos = IntToFixed( c2xy( x_boardSize.x / 2 - 2, -curFig->size.y + 1 ) );
    x_prevTime = SYS_RealTime();
    x_buttonDown = 0;
    return ! IsCurrentBitmapClipping( x_currentPos );
}

static bool_t TryMove( c2_t nextPos ) {
    if ( IsCurrentBitmapClipping( nextPos ) ) {
        return false;
    }
    x_currentPos = nextPos;
    return true;
}

static bool_t TryMoveDown( int deltaTime ) {
    int y = x_currentPos.y + deltaTime * x_speed * ( 1 + x_buttonDown * 3 ) / 100;
    c2_t nextPos = c2xy( x_currentPos.x, y );
    return TryMove( nextPos );
}

static void Drop( void ) {
    const char *bmp;
    c2_t bmpSz;
    GetCurrentBitmap( &bmp, &bmpSz );
    CopyBitmap( bmp, bmpSz, x_board, x_boardSize, FixedToInt( x_currentPos ) );
    Mix_PlayChannel( -1, x_soundThud, 0 );
}

static void EraseFilledLines( void ) {
    bool_t result = false;
    for ( int y = x_boardSize.y - 2; y >= 1; ) {
        int numFull = 0;
        for ( int x = 0; x < x_boardSize.x; x++ ) {
            int tile = ReadTile( c2xy( x, y ), x_board, x_boardSize );
            if ( ! IsBlank( tile ) ) {
                numFull++;
            }
        }
        if ( numFull == x_boardSize.x ) {
            result = true;
            x_numErasedLines++;
            CON_Printf( "Pop a line. TOTAL LINES: %d\n", x_numErasedLines );
            for ( int i = y; i >= 1; i-- ) {
                char *dst = &x_board[( i - 0 ) * x_boardSize.x];
                char *src = &x_board[( i - 1 ) * x_boardSize.x];
                memcpy( dst, src, x_boardSize.x );
            }
        } else {
            y--;
        }
    }
    if ( result ) {
        Mix_PlayChannel( -1, x_soundPop, 0 );
    }
}

static void GameUpdate( void ) {
    int now = SYS_RealTime();
    int deltaTime = now - x_prevTime;
    if ( x_gameOver ) {
    } else {
        if ( ! TryMoveDown( deltaTime ) ) {
            Drop();
            EraseFilledLines();
            if ( ! PickFigureAndReset() ) {
                CON_Printf( "GAME OVER!\n" );
                x_gameOver = true;
            } else if ( x_numErasedLines >= 10 ) {
                CON_Printf( "SPEED UP!\n" );
                x_numErasedLines = 0;
                x_speed += x_speed / 4;
            }
        } else {
            const char *bmp;
            c2_t bmpSz;
            GetCurrentBitmap( &bmp, &bmpSz );
            DrawBitmap( FixedToInt( x_currentPos ), bmp, bmpSz, colGreen );
        }
    }
    Print( c2xy( x_boardSize.x + 1, 1 ), "Next:", colCyan );
    figure_t *nextFigure = &x_figures[x_nextFigure];
    DrawBitmap( c2xy( x_boardSize.x + 1, 3 ), nextFigure->bitmaps[0], nextFigure->size, colCyan );
    DrawBitmap( c2zero, x_board, x_boardSize, colWhite );
    x_prevTime = now;
}

static void MoveRight_f( void ) {
    Mix_PlayChannel( -1, x_soundShift, 0 );
    c2_t nextPos = c2Add( x_currentPos, IntToFixed( c2xy( +1, 0 ) ) );
    TryMove( nextPos );
}

static void MoveLeft_f( void ) {
    Mix_PlayChannel( -1, x_soundShift, 0 );
    c2_t nextPos = c2Add( x_currentPos, IntToFixed( c2xy( -1, 0 ) ) );
    TryMove( nextPos );
}

static void Rotate_f( void ) {
    Mix_PlayChannel( -1, x_soundShift, 0 );
    figure_t *curFigure = &x_figures[x_currentFigure];
    int bmpIdx = ( x_currentBitmap + 1 ) % curFigure->numBitmaps;
    const char *bmp = curFigure->bitmaps[bmpIdx];
    if ( ! IsBitmapClipping( FixedToInt( x_currentPos ), bmp, curFigure->size ) ) {
        x_currentBitmap = bmpIdx;
    }
 
}

static void MoveDown_f( void ) {
    x_buttonDown = *CMD_Argv( 0 ) == '+' ? 1 : 0;
}

static void ClearBoard( void ) {
    for ( int y = 0; y < x_boardSize.y - 1; y++ ) {
        memset( &x_board[1 + y * x_boardSize.x], ' ', x_boardSize.x - 2 );
    }
}

static void Restart_f( void ) {
    if ( x_gameOver ) {
        x_gameOver = false;
        x_speed = INITIAL_SPEED;
        ClearBoard();
        PickFigureAndReset();
    }
}

static void RegisterVars( void ) {
    x_showAtlas = VAR_Register( "showAtlas", "0" );
    CMD_Register( "moveLeft", MoveLeft_f );
    CMD_Register( "moveRight", MoveRight_f );
    CMD_Register( "moveDown", MoveDown_f );
    CMD_Register( "rotate", Rotate_f );
    CMD_Register( "restartGame", Restart_f );
    I_Bind( "Left", "+moveLeft" );
    I_Bind( "Right", "+moveRight" );
    I_Bind( "Down", "moveDown" );
    I_Bind( "Up", "+rotate" );
    I_Bind( "Space", "+restartGame" );
}

static void AppFrame( void ) {
    PixelSize = Maxi( R_GetWindowSize().y / 320, 1 );
    GameUpdate();
    if ( VAR_Num( x_showAtlas ) ) {
        DrawAtlas();
    }
    SDL_Delay( 10 );
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "tetris", RegisterVars, Init, AppFrame, NULL, 0 );
    return 0;
}
