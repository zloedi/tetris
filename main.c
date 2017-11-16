//==============================================================================================

/*

TODO
Controller support.
Split screen.

IN PROGRESS

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
static bool_t x_gameOver;
static rImage_t *ASCIITexture;
static v2_t ASCIITextureSize;
static v2_t ASCIISymbolSize;
static int PixelSize;
static int x_currentShape;
static int x_nextShape;
static var_t *x_showAtlas;
static var_t *x_hiscore;

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

static void PickShapeAndReset( void );

static const char* GetAssetPath( const char *name ) {
    return va( "%sdata/%s", SYS_BaseDir(), name );
}

static int GetRandomShape( void ) {
    return COM_Rand() % x_numShapes;
}

static void Init( void ) {
    x_music = Mix_LoadMUS( GetAssetPath( "ievan_polkka_8bit.ogg" ) );
    Mix_VolumeMusic( MIX_MAX_VOLUME / 4 );
    x_soundPop = Mix_LoadWAV( GetAssetPath( "pop.ogg" ) );
    Mix_VolumeChunk( x_soundPop, MIX_MAX_VOLUME / 4 );
    x_soundThud = Mix_LoadWAV( GetAssetPath( "thud.ogg" ) );
    x_soundShift = Mix_LoadWAV( GetAssetPath( "shift.ogg" ) );
    Mix_VolumeChunk( x_soundShift, MIX_MAX_VOLUME / 2 );
    ASCIITexture = R_LoadStaticTextureEx( "cp437_12x12.png", &ASCIITextureSize );
    ASCIISymbolSize = v2Scale( ASCIITextureSize, 1 / 16. );
    x_nextShape = GetRandomShape();
    PickShapeAndReset();
    Mix_PlayMusic( x_music, -1 );
}

static int x_currentBitmap;
static c2_t x_currentPos;
static int x_prevTime;
#define INITIAL_SPEED 40
static int x_speed = INITIAL_SPEED;
static int x_buttonDown;
static int x_numErasedLines;
static int x_score;

static int ReadTile( c2_t pos, const char *bmp, c2_t bmpSz ) {
    if ( pos.x >= 0 && pos.x < bmpSz.x
        && pos.y >= 0 && pos.y < bmpSz.y ) {
        return bmp[pos.x + pos.y * bmpSz.x];
    }
    return ' ';
}

static int ReadTileOnBoard( c2_t pos ) {
    if ( pos.x <= 0 || pos.x >= x_boardSize.x - 1 ) {
        return '#';
    }
    return ReadTile( pos, x_board, x_boardSize );
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

static bool_t IsBitmapClipping( c2_t posOnBoard, const char *bmp ) {
    for ( int y = 0; y < x_shapeSize.y; y++ ) {
        for ( int x = 0; x < x_shapeSize.x; x++ ) {
            c2_t pos = c2xy( x, y );
            int tile = ReadTile( pos, bmp, x_shapeSize );
            if ( ! IsBlank( tile ) ) {
                int tileOnBoard = ReadTileOnBoard( c2Add( pos, posOnBoard ) );
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

static const char* GetCurrentBitmap( void ) {
    shape_t *curShape = &x_shapes[x_currentShape];
    return curShape->bitmaps[x_currentBitmap];
}

static bool_t IsCurrentBitmapClipping( c2_t fixedPointPos ) {
    return IsBitmapClipping( FixedToInt( fixedPointPos ), GetCurrentBitmap() );
}

static void PickShapeAndReset( void ) {
    x_currentBitmap = 0;
    x_currentShape = x_nextShape;
    x_nextShape = GetRandomShape();
    x_currentPos = IntToFixed( c2xy( x_boardSize.x / 2 - 2, -x_shapeSize.y + 1 ) );
    x_prevTime = SYS_RealTime();
    x_buttonDown = x_buttonDown ? -1 : 0;
}

static bool_t TryMove( c2_t nextPos ) {
    if ( IsCurrentBitmapClipping( nextPos ) ) {
        return false;
    }
    x_currentPos = nextPos;
    return true;
}

static bool_t TryMoveDown( int deltaTime ) {
    int y = x_currentPos.y + deltaTime * x_speed * ( 1 + Maxi( x_buttonDown, 0 ) * 6 ) / 100;
    c2_t nextPos = c2xy( x_currentPos.x, y );
    return TryMove( nextPos );
}

static bool_t Drop( void ) {
    CopyBitmap( GetCurrentBitmap(), x_shapeSize, x_board, x_boardSize, FixedToInt( x_currentPos ) );
    Mix_PlayChannel( -1, x_soundThud, 0 );
    x_score += x_speed / 2;
    return x_currentPos.y + x_shapeSize.y >= 0;
}

static void EraseFilledLines( void ) {
    int bonus = 1;
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
            bonus *= 2;
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
        x_score += bonus * x_speed * 4;
        Mix_PlayChannel( -1, x_soundPop, 0 );
    }
}

static void GameUpdate( void ) {
    int now = SYS_RealTime();
    int deltaTime = now - x_prevTime;
    if ( x_gameOver ) {
        if ( VAR_Num( x_hiscore ) < x_score ) {
            VAR_Set( x_hiscore, va( "%d", x_score ), false );
        }
        DrawBitmap( c2zero, x_board, x_boardSize, colWhite );
        Print( c2xy( x_boardSize.x / 2 - 3, x_boardSize.y / 2 - 1 ), "GAME OVER", colRed );
        if ( now & 256 ) { 
            Print( c2xy( x_boardSize.x / 2 - 4, x_boardSize.y / 2 ), "Press 'Space'", colRed );
        }
    } else {
        if ( ! TryMoveDown( deltaTime ) ) {
            if ( ! Drop() ) {
                x_gameOver = true;
                Mix_HaltMusic();
            } else {
                EraseFilledLines();
                PickShapeAndReset();
                if ( x_numErasedLines >= 10 ) {
                    CON_Printf( "SPEED UP!\n" );
                    x_numErasedLines = 0;
                    x_speed += x_speed / 4;
                }
            }
        } else {
            DrawBitmap( FixedToInt( x_currentPos ), GetCurrentBitmap(), x_shapeSize, colGreen );
        }
        DrawBitmap( c2zero, x_board, x_boardSize, colWhite );
    }
    Print( c2xy( x_boardSize.x + 1, 1 ), "NEXT", colCyan );
    shape_t *nextShape = &x_shapes[x_nextShape];
    DrawBitmap( c2xy( x_boardSize.x + 1, 2 ), nextShape->bitmaps[0], x_shapeSize, colGreen );
    Print( c2xy( x_boardSize.x + 1, 7 ), "SCORE", colCyan );
    Print( c2xy( x_boardSize.x + 1, 8 ), va( "%d", x_score ), colWhite );
    Print( c2xy( x_boardSize.x + 1, 10 ), "HISCORE", colCyan );
    Print( c2xy( x_boardSize.x + 1, 11 ), va( "%d", ( int )VAR_Num( x_hiscore ) ), colWhite );
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
    shape_t *curShape = &x_shapes[x_currentShape];
    int bmpIdx = ( x_currentBitmap + 1 ) % curShape->numBitmaps;
    if ( ! IsBitmapClipping( FixedToInt( x_currentPos ), curShape->bitmaps[bmpIdx] ) ) {
        x_currentBitmap = bmpIdx;
    }
}

static void MoveDown_f( void ) {
    int down = *CMD_Argv( 0 ) == '+';
    if ( down == x_buttonDown ) {
        return;
    }
    if ( x_buttonDown == -1 && down ) {
        // waiting for release
        return;
    }
    if ( down ) {
        Mix_PlayChannel( -1, x_soundShift, 0 );
    }
    x_buttonDown = down;
}

static void ClearBoard( void ) {
    for ( int y = 0; y < x_boardSize.y - 1; y++ ) {
        memset( &x_board[1 + y * x_boardSize.x], ' ', x_boardSize.x - 2 );
    }
}

static void Restart_f( void ) {
    if ( x_gameOver ) {
        x_gameOver = false;
        Mix_PlayMusic( x_music, -1 );
        x_speed = INITIAL_SPEED;
        x_numErasedLines = 0;
        x_nextShape = GetRandomShape();
        x_score = 0;
        ClearBoard();
        PickShapeAndReset();
    }
}

static void RegisterVars( void ) {
    x_showAtlas = VAR_Register( "showAtlas", "0" );
    x_hiscore = VAR_Register( "hiscore", "0" );
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
    PixelSize = Maxi( R_GetWindowSize().y / ( ASCIISymbolSize.y * x_boardSize.y ), 1 );
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
