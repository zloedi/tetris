//==============================================================================================

/*

TODO
Fix joystick locks and other crap on windows
Controller support.
    * rotate with any up axis
    * rotate with any button
    * move down with any down axis
    * move horizontal any horizontal axis
    * move using the hat switch
    hotplug and controllers.
More than 2 players
Wait a bit after game over

IN PROGRESS

Wake up cpu earlier when start playing

DONE

Fri Nov 24 10:48:46 EET 2017

* No dead zone in input code, handle it in app
* Rotate even at edges of board
* Show where the figure will fall as silhouette
* The lines counter should be common to both and 20 in a VS game
. Points gained i.e. (+100)
* Don't get the grids too separated on wide screens
* Set controller dead zone from app, store var in app.
* Both players increase speed each 10 lines

Wed Nov 22 19:30:30 EET 2017

* Draw vertical lines 
* Any key/button handler exposed from events.c

. Level number
* Split screen.
    * encode controller id in the command
    * draw the board with an offset
    * remove all x_pls
    * get player by command device
    * don't kill off a seat before both seats are done.
    * both players finished, then on press button should start in the same field
    * optional common speed
    * optional game over only when both are over
    * announce winner on end game
    * colorize score of the leader

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

#define MAX_PLAYERS 2
#define TIME_TO_CPU_KICK_IN 30000

const c2_t x_boardSize = { .x = 12, .y = 21 };
#define SEAT_WIDTH (x_boardSize.x+6)
#define SEAT_HEIGHT (x_boardSize.y-1)
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
            "AA  "
            " AA "
            "    "
            ,
            "    "
            "  A "
            " AA "
            " A  "
        },
    },
    {
        .numBitmaps = 2,
        .bitmaps = {
            "    "
            "  BB"
            " BB "
            "    "
            , 
            "    "
            " B  "
            " BB "
            "  B "
        },
    },
    {
        .numBitmaps = 1,
        .bitmaps = {
            "    "
            "    "
            " CC "
            " CC "
        },
    },
    {
        .numBitmaps = 4,
        .bitmaps = {
            "    "
            "    "
            " DDD"
            "  D "
            , 
            "    "
            "  D "
            "  DD"
            "  D "
            , 
            "    "
            "  D "
            " DDD"
            "    "
            , 
            "    "
            "  D "
            " DD "
            "  D "
        },
    },
    {
        .numBitmaps = 4,
        .bitmaps = {
            "    "
            " EE "
            " E  "
            " E  "
            , 
            "    "
            "E   "
            "EEE "
            "    "
            , 
            "    "
            " E  "
            " E  "
            "EE  "
            , 
            "    "
            "    "
            "EEE "
            "  E "
        },
    },
    {
        .numBitmaps = 4,
        .bitmaps = {
            "    "
            " FF "
            "  F "
            "  F "
            ,
            "    "
            "    "
            " FFF"
            " F  "
            ,
            "    "
            "  F "
            "  F "
            "  FF"
            ,
            "    "
            "   F"
            " FFF"
            "    "
        },
    },
};

static const int x_numShapes = sizeof( x_shapes ) / sizeof( *x_shapes );
static Mix_Music *x_music;
static Mix_Chunk *x_soundPop;
static Mix_Chunk *x_soundThud;
static Mix_Chunk *x_soundShift;
static SDL_Texture *x_tileset;
static c2_t x_tilesetSize;
static c2_t x_tileSize;
static int x_glyphLeft;
static int x_glyphWidth;
static rImage_t *ASCIITexture;
static v2_t ASCIITextureSize;
static v2_t TileSize;
static int x_pixelSize;
static var_t *x_showAtlas;
static var_t *x_hiscore;
static var_t *x_musicVolume;
static var_t *x_numLinesPerLevel;
static var_t *x_skipBoards;
static var_t *x_skipGhostShape;
static var_t *x_showSpeedFunc;
static var_t *x_speedFuncMax;
static var_t *x_speedCoefA;
static var_t *x_speedCoefB;
static var_t *x_speedCoefC;
static var_t *x_joystickDeadZone;
static var_t *x_nextShape;
static int x_timeSinceGameOver;
static int x_prevTime;

typedef enum {
    BS_RELEASED,
    BS_PRESSED,
    BS_LATCHED,
} butState_t;

typedef struct {
    bool_t active;
    int gameDuration;
    int matchEndTime;
    int deviceType;
    int deviceId;
    int nextShape;
    int currentShape;
    int currentBitmap;
    c2_t currentPos;
    int speed;
    int rotateClip;
    butState_t butMoveDown;
    butState_t butMoveLeft;
    butState_t butMoveRight;
    butState_t butRotate;
    int numErasedLines;
    int lastScoreGainAlpha;
    int lastScoreGain;
    int score;
    int numButtonActions;
    int humanIdleTime;
    int cpuIdleTime;
    char **cpuCommands;
    int cpuCurrentCommand;
    char board[sizeof( x_board )];
} playerSeat_t;

static playerSeat_t x_pls[2];

static void DrawAtlas( void ) {
    v2_t windowSize = R_GetWindowSize();
    v2_t position = v2xy( windowSize.x - ASCIITextureSize.x, 0 );
    R_ColorC( colRed );
    R_BlendPicV2( position, ASCIITextureSize, v2zero, v2one, ASCIITexture );
}

static void DrawTile( SDL_Rect *dst, int index, color_t color ) {
    SDL_SetTextureColorMod( x_tileset,
            ( Uint8 )( color.r * 255 ), 
            ( Uint8 )( color.g * 255 ),
            ( Uint8 )( color.b * 255 ) );
    SDL_SetTextureAlphaMod( x_tileset, ( Uint8 )( color.alpha * 255 ) );
    c2_t st = c2xy( index & 15, index >> 4 );
    SDL_Rect src = {
        .x = st.x * x_tileSize.x,
        .y = st.y * x_tileSize.y,
        .w = x_tileSize.x,
        .h = x_tileSize.y,
    };
    SDL_RenderCopy( r_renderer, x_tileset, &src, dst );
}

static void DrawTileInScaledPixels( c2_t spCoord, int index, color_t color ) {
    SDL_Rect dst = {
        .x = spCoord.x * x_pixelSize,
        .y = spCoord.y * x_pixelSize,
        .w = x_tileSize.x * x_pixelSize,
        .h = x_tileSize.y * x_pixelSize,
    };
    DrawTile( &dst, index, color );
}

static int DrawChar( c2_t spCoord, int glyph, color_t color ) {
    DrawTileInScaledPixels( c2xy( spCoord.x - x_glyphLeft, spCoord.y ), glyph, color );
    return x_glyphWidth;
}

static void DrawTileInGrid( c2_t gridCoord, int index, color_t color ) {
    DrawTileInScaledPixels( c2Mul( gridCoord, x_tileSize ), index, color );
}

static int GetRandomShape( void ) {
    int ns = VAR_Num( x_nextShape );
    return ( ns == 0 ? COM_Rand() : ns ) % x_numShapes;
}

static c2_t c2IntToFixed( c2_t c ) {
    return c2LShifts( c, 8 );
}

static c2_t c2FixedToInt( c2_t c ) {
    return c2RShifts( c, 8 );
}

static void UnlatchButton( butState_t *button ) {
    *button = BS_RELEASED;
}

static void LatchButton( butState_t *button ) {
    *button = *button == BS_PRESSED ? BS_LATCHED : BS_RELEASED;
}

static void UnlatchButtons( playerSeat_t *pls ) {
    UnlatchButton( &pls->butMoveDown );
    UnlatchButton( &pls->butMoveLeft );
    UnlatchButton( &pls->butMoveRight );
    UnlatchButton( &pls->butRotate );
}

static void LatchButtons( playerSeat_t *pls ) {
    LatchButton( &pls->butMoveDown );
    LatchButton( &pls->butMoveLeft );
    LatchButton( &pls->butMoveRight );
    LatchButton( &pls->butRotate );
}

static bool_t IsBlank( int tile ) {
    return tile == ' ';
}

static const char* GetCurrentBitmap( playerSeat_t *pls ) {
    shape_t *curShape = &x_shapes[pls->currentShape];
    return curShape->bitmaps[pls->currentBitmap];
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

static bool_t IsBitmapPartiallyOff( const char *board, c2_t posOnBoard, const char *bmp ) {
    for ( int y = 0; y < x_shapeSize.y; y++ ) {
        for ( int x = 0; x < x_shapeSize.x; x++ ) {
            c2_t pos = c2xy( x, y );
            int tile = ReadTile( pos, bmp, x_shapeSize );
            if ( ! IsBlank( tile ) ) {
                c2_t tilePos = c2Add( pos, posOnBoard );
                if ( tilePos.y < 0 ) {
                    return true;
                }
            }
        }
    }
    return false;
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

static bool_t IsCurrentBitmapClipping( playerSeat_t *pls, c2_t fixedPointPos ) {
    return IsBitmapClipping( pls->board, c2FixedToInt( fixedPointPos ), GetCurrentBitmap( pls ) );
}

static bool_t TraceDownOnBoard( const char *board, c2_t originOnBoard, const char *bmp, c2_t *outResult ) {
    for ( c2_t res = originOnBoard; res.y < x_boardSize.y - 1; res.y++ ) {
        if ( IsBitmapClipping( board, c2xy( res.x, res.y + 1 ), bmp ) ) {
            *outResult = res;
            return true;
        }
    }
    return false;
}

static int EvalScore( const char *board, c2_t posOnBoard, const char *bitmap ) {
    int score = 0;
    for ( int y = 0; y < x_shapeSize.y; y++ ) {
        for ( int x = 0; x < x_shapeSize.x; x++ ) {
            c2_t p[4] = {
                c2xy( x - 1, y ),
                c2xy( x + 1, y ),
                c2xy( x, y + 1 ),
                c2xy( x, y - 1 ),
            };
            int yScore = ( 1 + posOnBoard.y + y ) * 10;
            if ( ! IsBlank( ReadTile( c2xy( x, y ), bitmap, x_shapeSize ) ) ) {
                for ( int i = 0; i < 4; i++ ) {
                    c2_t pb = c2Add( p[i], posOnBoard );
                    int blank = IsBlank( ReadTileOnBoard( board, pb ) );
                    score += yScore * ! blank;
                }
            }
        }
    }
    return score;
}

static int EvalDrop( playerSeat_t *pls, c2_t posOnBoard, const char *bitmap ) {
    int score = 0;
    c2_t proj;
    if ( TraceDownOnBoard( pls->board, posOnBoard, bitmap, &proj ) ) {
        score = EvalScore( pls->board, proj, bitmap );
    }
    return score;
}

static char* CPUGetCommand( const char *bind, int deviceType, int deviceId, bool_t doRelease ) {
    char engage[32];
    char release[32];
    CMD_FromBindBuf( bind, deviceType == 'j', deviceId - '0', true, I_AXIS_MAX_VALUE, engage, 32 );
    if ( doRelease ) {
        CMD_FromBindBuf( bind, deviceType == 'j', deviceId - '0', false, I_AXIS_MAX_VALUE, release, 32 );
        return A_StrDup( va( "%s; %s", engage, release ) );
    }
    return A_StrDup( engage );
}

static void CPUPushCommand( playerSeat_t *pls, const char *bind, bool_t engage ) {
    char *cmd = CPUGetCommand( bind, pls->deviceType, pls->deviceId, engage );
    //PrintString( cmd );
    stb_sb_push( pls->cpuCommands, cmd );
}

static void CPUClearCommands( playerSeat_t *pls ) {
    for ( int i = 0; i < stb_sb_count( pls->cpuCommands ); i++ ) {
        A_Free( pls->cpuCommands[i] );
    }
    stb_sb_free( pls->cpuCommands );
    pls->cpuCommands = NULL;
    //CON_Printf( "clear commands\n" );
}

void CPUEvaluate( playerSeat_t *pls ) {
    CPUClearCommands( pls );
    UnlatchButtons( pls );
    pls->cpuCurrentCommand = 0;
    int maxScore = 0;
    int numRotates = 0;
    c2_t bestDrop;
    c2_t origin = c2FixedToInt( pls->currentPos );
    c2_t pos = origin;
    for ( pos.x = -2; pos.x < x_boardSize.x + 2; pos.x++ ) {
        shape_t *curShape = &x_shapes[pls->currentShape];
        for ( int i = 0; i < curShape->numBitmaps; i++ ) {
            int nextBitmap = ( pls->currentBitmap + i ) % curShape->numBitmaps;
            const char *bmp = curShape->bitmaps[nextBitmap];
            if ( ! IsBitmapClipping( pls->board, pos, bmp ) ) {
                int s = EvalDrop( pls, pos, bmp );
                if ( s > maxScore ) {
                    numRotates = i;
                    bestDrop = pos;
                    maxScore = s;
                }
            }
        }
    }
    if ( maxScore > 0 ) {
        int d = bestDrop.x - origin.x;
        int numMoves = abs( d );
        for ( int i = 0; i < numRotates; i++ ) {
            CPUPushCommand( pls, "rotate", true );
        }
        for ( int i = 0; i < numMoves; i++ ) {
            const char *bind = d > 0 ? "moveRight" : "moveLeft";
            CPUPushCommand( pls, bind, true );
        }
        CPUPushCommand( pls, "moveDown", true );
        CPUPushCommand( pls, "moveDown", false );
    }
}

static bool_t IsCPU( playerSeat_t *pls ) {
    return pls->humanIdleTime >= TIME_TO_CPU_KICK_IN;
}

static int NumCPUPlayers( void ) {
    int n = 0;
    for ( int i = 0; i < MAX_PLAYERS; i++ ) {
        n += IsCPU( &x_pls[i] );
    }
    return n;
}

static int NumActiveHumanPlayers( void ) {
    int n = 0;
    for ( int i = 0; i < MAX_PLAYERS; i++ ) {
        n += x_pls[i].active && ! IsCPU( &x_pls[i] );
    }
    return n;
}

//static int NumActiveCPUPlayers( void ) {
//    int n = 0;
//    for ( int i = 0; i < MAX_PLAYERS; i++ ) {
//        n += x_pls[i].active && IsCPU( &x_pls[i] );
//    }
//    return n;
//}

static int NumHumanPlayers( void ) {
    return MAX_PLAYERS - NumCPUPlayers();
}

static bool_t IsDemo( void ) {
    return ! NumHumanPlayers();
}

//static bool_t AllSeatsActive( void ) {
//    return x_pls[0].active && x_pls[1].active;
//}

static bool_t AnySeatActive( void ) {
    return x_pls[0].active || x_pls[1].active;
}

static void UpdateMusicPlayback( void ) {
    if ( IsDemo() ) {
        Mix_HaltMusic();
    } else if ( ! AnySeatActive() ) {
        Mix_FadeOutMusic( 300 );
    } else {
        if ( ! Mix_PlayingMusic() ) {
            Mix_PlayMusic( x_music, -1 );
        }
        Mix_VolumeMusic( Clampf( VAR_Num( x_musicVolume ), 0, 1 ) * MIX_MAX_VOLUME / 4 );
    }
}

static void CPUAcquire( playerSeat_t *pls ) {
    pls->humanIdleTime = TIME_TO_CPU_KICK_IN;
    UpdateMusicPlayback();
}

static void CPUHandOver( playerSeat_t *pls ) {
    CPUClearCommands( pls );
    UnlatchButtons( pls );
    pls->humanIdleTime = 0;
    UpdateMusicPlayback();
}

static int APM( playerSeat_t *pls  ) {
    if ( ! pls->gameDuration ) {
        return 666;
    }
    return pls->numButtonActions * 60000 / pls->gameDuration;
}

static int CPUGetAPM( void ) {
    if ( IsDemo() ) {
        return 666;
    }
    int result = 100;
    for ( int i = 0; i < MAX_PLAYERS; i++ ) {
        playerSeat_t *p = &x_pls[i];
        if ( ! IsCPU( p ) ) {
            int cpuAPM = APM( p ) * 2;
            result = Maxi( result, cpuAPM );
        }
    }
    return result;
}

static void CPUUpdate( playerSeat_t *pls, int deltaTime ) {
    pls->cpuIdleTime += deltaTime;
    if ( pls->cpuIdleTime > 4000 ) {
        CPUEvaluate( pls );
    }
    if ( pls->cpuCommands ) {
        int delay = 60000 / CPUGetAPM();
        if ( pls->cpuIdleTime >= delay ) {
            char *cmd = pls->cpuCommands[pls->cpuCurrentCommand];
            CMD_ExecuteString( cmd );
            pls->cpuIdleTime = 0;
            pls->cpuCurrentCommand++;
            if ( pls->cpuCurrentCommand == stb_sb_count( pls->cpuCommands ) ) {
                CPUClearCommands( pls );
            }
        } 
    }
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

static void ClearBoard( playerSeat_t *pls ) {
    memcpy( pls->board, x_board, sizeof( pls->board ) );
}

static inline int InitialSpeed( void ) {
    return VAR_Num( x_speedCoefC );
}

static void StartNewGame( playerSeat_t *pls, int deviceType, int deviceId ) {
    CPUClearCommands( pls );
    memset( pls, 0, sizeof( *pls ) );
    pls->active = true;
    pls->deviceType = deviceType;
    pls->deviceId = deviceId;
    pls->speed = InitialSpeed();
    pls->nextShape = GetRandomShape();
    ClearBoard( pls );
    PickShapeAndReset( pls );
    UpdateMusicPlayback();
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

static playerSeat_t* GetSeat( int type, int id ) {
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *p = &x_pls[i];
        if ( p->deviceType == type && p->deviceId == id ) {
            return p;
        }
    }
    return NULL;
}

static playerSeat_t* GetFreeSeat( int hintType, int hintId ) {
    playerSeat_t *p = GetSeat( hintType, hintId );
    if ( p && ( ! p->active || IsCPU( p ) ) ) {
        return p;
    }
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *p = &x_pls[i];
        if ( ! p->active ) {
            return p;
        }
    }
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *p = &x_pls[i];
        if ( IsCPU( p ) ) {
            return p;
        }
    }
    return NULL;
}

//static bool_t IsControllerTaken( int type, int id ) {
//    for ( int i = 0; i < 2; i++ ) {
//        playerSeat_t *pls = &x_pls[i];
//        if ( pls->active && pls->deviceType == type && pls->deviceId == id ) {
//            return true;
//        }
//    }
//    return false;
//}

static void Restart_f( void ) {
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *pls = &x_pls[i];
        if ( pls->active ) {
            StartNewGame( pls, pls->deviceType, pls->deviceId );
        }
    }
}

static void CPUStartGame( playerSeat_t *pls, int id ) {
    // a non-existing joystick for an AI until another human picks it up
    StartNewGame( pls, 'j', id );
    // make sure the cpu has control even if no eval initially
    CPUAcquire( pls );
}

static bool_t OnAnyButton_f( int code, bool_t down ) {
    if ( ! code ) {
        return false;
    }
    // consume the input a bit after game over
    if ( x_timeSinceGameOver < 2000 ) {
        return true;
    }
    int type = I_IsJoystickCode( code ) ? 'j' : 'k';
    int id = '0' + I_DeviceOfCode( code );
    playerSeat_t *pls = GetSeat( type, id );
    bool_t result = false;
    if ( down ) {
        if ( IsDemo() || ! AnySeatActive() ) {
            int sel = pls ? pls - x_pls : 0;
            StartNewGame( &x_pls[sel], type, id );
            x_pls[sel].humanIdleTime = 0;
            CPUStartGame( &x_pls[( sel + 1 ) & 1], '9' );
            CON_Printf( "Start new game: %c%c\n", type, id ); 
            result = true;
        } else {
            if ( pls ) {
                if ( ! pls->active ) {
                    StartNewGame( pls, type, id );
                    result = true;
                } else if ( IsCPU( pls ) ) {
                    CPUHandOver( pls );
                    CON_Printf( "Rejoined the game: %c%c\n", type, id ); 
                    result = true;
                } 
            } else {
                playerSeat_t *freeSeat = GetFreeSeat( type, id );
                if ( freeSeat ) {
                    CPUHandOver( freeSeat );
                    freeSeat->deviceType = type;
                    freeSeat->deviceId = id;
                    CON_Printf( "Joined the game: %c%c\n", type, id ); 
                    result = true;
                }
            }
        }
    }
    if ( pls ) {
        pls->numButtonActions += ! down;
        pls->humanIdleTime = 0;
    }
    return result;
}

static void StartDemoGame( void ) {
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *p = &x_pls[i];
        CPUStartGame( p, '8' + i );
        CPUEvaluate( p );
    }
}

static void Init( void ) {
    R_SetClearColor( colorrgb( 0.1, 0.1, 0.1 ) );
    E_SetButtonOverride( OnAnyButton_f );
    I_SetJoystickDeadZone( VAR_Num( x_joystickDeadZone ) * I_AXIS_MAX_VALUE );
    x_music = Mix_LoadMUS( GetAssetPath( "ievan_polkka_8bit.ogg" ) );
    x_soundPop = Mix_LoadWAV( GetAssetPath( "pop.ogg" ) );
    x_soundThud = Mix_LoadWAV( GetAssetPath( "thud.ogg" ) );
    x_soundShift = Mix_LoadWAV( GetAssetPath( "shift.ogg" ) );

    Mix_VolumeChunk( x_soundPop, MIX_MAX_VOLUME / 4 );
    Mix_VolumeChunk( x_soundShift, MIX_MAX_VOLUME / 2 );

    const char *tilesetImgName = "cp437_12x12.png";
    int bytesPerPixel;
    byte *bits = R_LoadImageRaw( tilesetImgName, &x_tilesetSize, &bytesPerPixel, 0 );
    if ( ! bits ) {
        static const byte dummy[4] = {
            0xff, 0xff,
            0xff, 0xff,
        };
        x_tilesetSize = c2xy( 2, 2 );
        x_tileset = R_CreateStaticTexFromBitmap( dummy, x_tilesetSize, 1 );
        CON_Printf( "Failed to load %s\n", tilesetImgName );
    } else {
        x_tileset = R_CreateStaticTexFromBitmap( bits, x_tilesetSize, bytesPerPixel );
        A_Free( bits );
    }
    SDL_SetTextureBlendMode( x_tileset, SDL_BLENDMODE_BLEND );
    ASCIITexture = R_LoadStaticTextureEx( "cp437_12x12.png", &ASCIITextureSize );
    TileSize = v2Scale( ASCIITextureSize, 1 / 16. );
    x_tileSize = c2Divs( x_tilesetSize, 16 );
    x_glyphLeft = x_tileSize.x % 6 == 0 ? x_tileSize.x / 6 : x_tileSize.x / 10;
    x_glyphWidth = x_tileSize.x - x_glyphLeft * 2;

    x_prevTime = SYS_RealTime();
    StartDemoGame();
}

static void CopyTile( int tile, c2_t pos, char *bmp, c2_t bmpSz )
{
    if ( pos.x >= 0 && pos.x < bmpSz.x
        && pos.y >= 0 && pos.y < bmpSz.y ) {
        bmp[pos.x + pos.y * bmpSz.x] = tile;
    }
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
        ['A'] = 2,
        ['B'] = 3,
        ['C'] = 4,
        ['D'] = 5,
        ['E'] = 6,
        ['F'] = 7,
        ['#'] = 1 + 11 * 16,
    };
    for ( int i = 0, y = 0; y < sz.y; y++ ) {
        for ( int x = 0; x < sz.x; x++ ) {
            int tile = bitmap[i++];
            DrawTileInGrid( c2Add( c2xy( x, y ), screenPos ), remap[tile], color );
        }
    }
}

static void DrawBitmapOff( c2_t off, c2_t screenPos, const char *bitmap, c2_t sz, color_t color ) {
    DrawBitmap( c2Add( off, screenPos ), bitmap, sz, color );
}

static void Print( c2_t scaledPixelsPos, const char *string, color_t color ) {
    for ( const char *p = string; *p; p++ ) {
        scaledPixelsPos.x += DrawChar( scaledPixelsPos, *p, color );
    }
}

static c2_t CenterBoxInGrid( c2_t gridCoord, c2_t gridSize, c2_t boxSize ) {
    c2_t pos = c2Mul( gridCoord, x_tileSize ); 
    c2_t size = c2Mul( gridSize, x_tileSize ); 
    return c2Add( pos, c2Divs( c2Sub( size, boxSize ), 2 ) );
}

static void DrawBox( c2_t scaledPixelsPos, c2_t scaledPixelsSize, color_t color ) {
    SDL_SetRenderDrawColor( r_renderer,
            ( Uint8 )( color.r * 255 ), 
            ( Uint8 )( color.g * 255 ),
            ( Uint8 )( color.b * 255 ),
            ( Uint8 )( color.alpha * 255 ) );
    SDL_SetRenderDrawBlendMode( r_renderer, SDL_BLENDMODE_BLEND );
    SDL_Rect rect = {
        .x = x_pixelSize * scaledPixelsPos.x,
        .y = x_pixelSize * scaledPixelsPos.y,
        .w = x_pixelSize * scaledPixelsSize.x,
        .h = x_pixelSize * scaledPixelsSize.y,
    };
    SDL_RenderFillRect( r_renderer, &rect );
}

//static void DrawBoxCenteredInGrid( c2_t gridCoord, c2_t gridSize, c2_t box, color_t color ) {
//    DrawBox( CenterBoxInGrid( gridCoord, gridSize, box ), box, color );
//}

static c2_t MeasurePrint( const char *string ) {
    return c2xy( x_glyphWidth * COM_StrLen( string ), x_tileSize.y );
}

static void PrintCenteredInGridSz( c2_t gridCoord, c2_t gridSize, c2_t strSize, const char *string, color_t color ) {
    Print( CenterBoxInGrid( gridCoord, gridSize, strSize ), string, color );
}

static void PrintCenteredInGrid( c2_t gridCoord, c2_t gridSize, const char *string, color_t color ) {
    c2_t strSize = MeasurePrint( string );
    PrintCenteredInGridSz( gridCoord, gridSize, strSize, string, color );
}

//static void PrintBoxedCenteredInGrid( c2_t gridCoord, c2_t gridSize, const char *string, color_t strColor, c2_t boxOutline, color_t boxColor ) {
//    c2_t strSize = MeasurePrint( string );
//    c2_t strPos = CenterBoxInGrid( gridCoord, gridSize, strSize );
//    c2_t boxPos = c2Sub( strPos, boxOutline );
//    c2_t boxSize = c2Add( strSize, c2Scale( boxOutline, 2 ) );
//    DrawBox( boxPos, boxSize, boxColor );
//    Print( strPos, string, strColor );
//}

static void PrintInGrid( c2_t offset, int x, int y, const char *string, 
                            color_t color ) {
    c2_t xy = c2Mul( c2Add( c2xy( x, y ), offset ), x_tileSize );
    Print( xy, string, color );
} 

static bool_t TryMove( playerSeat_t *pls, c2_t nextPosFP ) {
    if ( IsCurrentBitmapClipping( pls, nextPosFP ) ) {
        return false;
    }
    pls->currentPos = nextPosFP;
    return true;
}

static int GetHoldSpeed( bool_t butDown, int baseSpeed, int deltaTime ) {
    int v = Mini( baseSpeed * ( 1 + butDown * 6 ), 1000 );
    return deltaTime * v / 100;
}

static bool_t TryMoveDown( playerSeat_t *pls, int deltaTime ) {
    int speed = GetHoldSpeed( pls->butMoveDown == BS_PRESSED, pls->speed, deltaTime );
    c2_t nextPos = c2xy( pls->currentPos.x, pls->currentPos.y + speed );
    bool_t result = TryMove( pls, nextPos );
    if ( ! result ) {
        //nextPos.y &= ~255;
        //while ( ! TryMove( pls, nextPos ) ) {
        //    nextPos.y -= 256;
        //}
        //pls->currentPos.y += 255;
    }
    return result;
}

static bool_t Drop( playerSeat_t *pls, int *outDropScore ) {
    c2_t pos = c2FixedToInt( pls->currentPos );
    const char *bitmap = GetCurrentBitmap( pls );
    if ( IsBitmapPartiallyOff( pls->board, pos, bitmap ) ) {
        return false;
    }
    *outDropScore = EvalScore( pls->board, pos, bitmap );
    CopyBitmap( bitmap, x_shapeSize, pls->board, x_boardSize, pos );
    Mix_PlayChannel( -1, x_soundThud, 0 );
    return true;
}

static void EraseFilledLines( playerSeat_t *pls, int *outLinesScore ) {
    int bonus = 50;
    bool_t result = false;
    for ( int y = x_boardSize.y - 2; y >= 1; ) {
        int numBlanks = x_boardSize.x - 2;
        for ( int x = 1; x < x_boardSize.x - 1; x++ ) {
            int tile = ReadTile( c2xy( x, y ), pls->board, x_boardSize );
            numBlanks -= ! IsBlank( tile );
        }
        if ( ! numBlanks ) {
            result = true;
            pls->numErasedLines++;
            bonus *= 2;
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
        *outLinesScore = bonus * 100;
        Mix_PlayChannel( -1, x_soundPop, 0 );
    }
}

static void TryMoveSide( playerSeat_t *pls, int nextFPX ) {
    c2_t nextPos = c2xy( nextFPX, pls->currentPos.y );
    if ( TryMove( pls, nextPos ) ) {
        pls->rotateClip = 0;
    }
}

static void TryMoveSideTime( playerSeat_t *pls, int deltaTime ) {
    int dir = ( pls->butMoveRight == BS_PRESSED ) 
            - ( pls->butMoveLeft == BS_PRESSED );
    if ( dir != 0 ) {
        int speed = GetHoldSpeed( true, 40, deltaTime ) / 2;
        int off = dir > 0 ? speed : -speed;
        TryMoveSide( pls, pls->currentPos.x + off );
    }
}

static bool_t UpdateHiscore( playerSeat_t *pls ) {
    if ( VAR_Num( x_hiscore ) <= pls->score ) {
        VAR_Set( x_hiscore, va( "%d", pls->score ), false );
        return true;
    }
    return false;
}

static void DrawStripes( c2_t offset ) {
    for ( int i = 0; i < x_boardSize.x; i++ ) {
        c2_t pos = c2Mul( c2Add( offset, c2xy( i, 0 ) ), x_tileSize );
        DrawBox( c2xy( pos.x - 1, pos.y ), c2xy( 2, x_boardSize.y * x_tileSize.y ), colorrgb( 0.15, 0.15, 0.15 ) );
    }
}

static inline int Square8bitFP( int x, int scale ) {
    return ( ( x * x * scale ) >> 8 ) >> 8;
}

static int GetSpeedParam( int i, int fpStep, int coefA, int coefB, int coefC ) {
    int x = i * fpStep;
    int y = Square8bitFP( x, coefA );
    y += ( coefB * x ) >> 8;
    y += coefC;
    return y;
}

static int GetSpeed( int i ) {
    int step = 256 * VAR_Num( x_speedFuncMax ) / 10;
    int coefA = VAR_Num( x_speedCoefA );
    int coefB = VAR_Num( x_speedCoefB );
    int coefC = VAR_Num( x_speedCoefC );
    return GetSpeedParam( i, step, coefA, coefB, coefC );
}

static bool_t UpdateSeat( c2_t boffset, playerSeat_t *pls, int deltaTime, int level, int scoreCompare ) {
    bool_t keepPlaying = pls->active;

    pls->humanIdleTime += deltaTime;

    if ( pls->active ) {
        if ( IsCPU( pls ) ) {
            CPUUpdate( pls, deltaTime );
        }
        pls->gameDuration += deltaTime;
        pls->speed = GetSpeed( level );
        TryMoveSideTime( pls, deltaTime );
        if ( ! TryMoveDown( pls, deltaTime ) ) {
            int dropScore = 0;
            int linesScore = 0;
            keepPlaying = Drop( pls, &dropScore );
            if ( keepPlaying ) {
                EraseFilledLines( pls, &linesScore );
                PickShapeAndReset( pls );
                LatchButtons( pls );
                if ( IsCPU( pls ) ) {
                    CPUEvaluate( pls );
                }
            }
            pls->lastScoreGain = dropScore + linesScore;
            if ( pls->lastScoreGain > 0 ) {
                pls->lastScoreGainAlpha = 256;
                pls->score += pls->lastScoreGain;
            }
        }
    }

    DrawStripes( boffset );

    if ( pls->active || pls->gameDuration > 0 ) {
        c2_t shapePos = c2FixedToInt( pls->currentPos );
        const char *shape = GetCurrentBitmap( pls );
        if ( ! VAR_Num( x_skipGhostShape ) ) {
            c2_t proj;
            if ( TraceDownOnBoard( pls->board, shapePos, shape, &proj ) ) {
                DrawBitmapOff( boffset, proj, shape, x_shapeSize, colorrgba( 0.8, 0.8, 0.8, 0.3 + 0.1 * sin( SYS_RealTime() * 0.01 ) ) );
            }
        }
        DrawBitmapOff( boffset, shapePos, shape, x_shapeSize, colWhite );
        DrawBitmap( boffset, pls->board, x_boardSize, colWhite );
        int x = x_boardSize.x + 1;
        PrintInGrid( boffset, x, 1, "NEXT", colCyan );
        shape_t *nextShape = &x_shapes[pls->nextShape];
        DrawBitmapOff( boffset, c2xy( x, 2 ), 
                        nextShape->bitmaps[0], x_shapeSize, colWhite );
        color_t scoreCol = colWhite;
        color_t hiscoreCol = colWhite;
        if ( UpdateHiscore( pls ) ) {
            hiscoreCol = colMagenta;
            scoreCol = colMagenta;
        } else if ( scoreCompare > 0 ) {
            scoreCol = colGreen;
        } else if ( scoreCompare < 0 ) {
            scoreCol = colRed;
        }
        PrintInGrid( boffset, x, 7, "SCORE", colCyan );
        PrintInGrid( boffset, x, 8, va( "%d", pls->score ), scoreCol );
        float alpha = Maxi( pls->lastScoreGainAlpha, 0 ) / 256.0f;
        pls->lastScoreGainAlpha -= deltaTime / 4;
        PrintInGrid( boffset, x, 9, va( "+%d", pls->lastScoreGain ), colorrgba( colOrange.r, colOrange.g, colOrange.b, alpha ) );
        PrintInGrid( boffset, x, 11, "HISCORE", colCyan );
        PrintInGrid( boffset, x, 12, va( "%d", ( int )VAR_Num( x_hiscore ) ), hiscoreCol );
        PrintInGrid( boffset, x, 14, "LINES", colCyan );
        PrintInGrid( boffset, x, 15, va( "%d", pls->numErasedLines ), colWhite );
        if ( pls->speed > InitialSpeed() ) {
            PrintInGrid( boffset, x, 17, "SPEED", colCyan );
            PrintInGrid( boffset, x, 18, va( "x%.2f", pls->speed / ( float )InitialSpeed() ), colWhite );
        }
    } else {
        DrawBitmap( boffset, pls->board, x_boardSize, colWhite );
    }
    if ( pls->active ) {
        if ( IsCPU( pls ) ) {
            float a = 0.2 + 0.1 * sin( SYS_RealTime() * 0.01 );
            PrintCenteredInGrid( boffset, x_boardSize, "Press a button", colorrgba( 0, 1, 1, a ) );
        }
    } else {
        const char *string = "Press a Button";
        c2_t strSize = MeasurePrint( string );
        c2_t rowPos = c2xy( boffset.x, boffset.y + x_boardSize.y / 2 ); 
        c2_t rowSize = c2xy( x_boardSize.x, 1 ); 
        c2_t strPos = CenterBoxInGrid( rowPos, rowSize, strSize );
        c2_t boxOutline = x_tileSize;
        c2_t boxPos = c2Sub( strPos, boxOutline );
        c2_t boxSize = c2Add( strSize, c2Scale( boxOutline, 2 ) );
        if ( ! IsCPU( pls ) && scoreCompare > 0 ) {
            boxPos.y -= x_tileSize.y * 2;
            boxSize.y += x_tileSize.y * 2;
        }
        DrawBox( boxPos, boxSize, colBlack );
        if ( ! IsCPU( pls ) && scoreCompare > 0 ) {
            rowPos.y -= 2;
            PrintCenteredInGrid( rowPos, rowSize, "YOU WIN!", colOrange );
        }
        if ( SYS_RealTime() & 512 ) { 
            Print( strPos, string, colWhite );
        }
    }
    return keepPlaying;
}

static bool_t DoButton( bool_t down, butState_t *button, bool_t doSound ) {
    bool_t result = false;
    if ( down ) {
        if ( *button != BS_PRESSED && *button != BS_LATCHED ) {
            if ( doSound ) {
                Mix_PlayChannel( -1, x_soundShift, 0 );
            }
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

static void DoRightButton( playerSeat_t *pls, bool_t down ) {
    if ( DoButton( down, &pls->butMoveRight, ! IsCPU( pls ) ) ) {
        TryMoveSide( pls, ( pls->currentPos.x & ~255 ) + 256 );
    }
}

static void DoLeftButton( playerSeat_t *pls, bool_t down ) {
    if ( DoButton( down, &pls->butMoveLeft, ! IsCPU( pls ) ) ) {
        TryMoveSide( pls, ( pls->currentPos.x & ~255 ) - 1 );
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
    int nextIdx = ( pls->currentBitmap + 1 ) % curShape->numBitmaps;
    const char *bitmap = curShape->bitmaps[nextIdx];
    pls->currentPos.x -= pls->rotateClip << 8;
    pls->rotateClip = 0;
    c2_t origin = c2FixedToInt( pls->currentPos );
    if ( IsBitmapClipping( pls->board, origin, bitmap ) ) {
        for ( int dir = -1; dir <= 1; dir += 2 ) {
            for ( int i = 1; i <= 2; i++ ) {
                c2_t c = c2xy( origin.x + i * dir, origin.y );
                if ( ! IsBitmapClipping( pls->board, c, bitmap ) ) {
                    pls->rotateClip = i * dir;
                    pls->currentPos.x = c.x << 8;
                    pls->currentBitmap = nextIdx;
                    break;
                }
            }
        }
    } else {
        pls->currentBitmap = nextIdx;
    }
}

static void Rotate_f( void ) {
    playerSeat_t *pls = ArgvSeat();
    if ( DoButton( CMD_ArgvEngaged(), &pls->butRotate, ! IsCPU( pls ) ) ) {
        Rotate( pls );
    }
}

static void MoveDown_f( void ) {
    DoButton( CMD_ArgvEngaged(), &ArgvSeat()->butMoveDown, ! IsCPU( ArgvSeat() ) );
}

static void RegisterVars( void ) {
    x_showAtlas = VAR_Register( "showAtlas", "0" );
    x_hiscore = VAR_Register( "hiscore", "0" );
    x_musicVolume = VAR_Register( "musicVolume", "1" );
    x_numLinesPerLevel = VAR_Register( "numLinesPerLevel", "10" );
    x_skipBoards = VAR_Register( "skipBoards", "0" );
    x_skipGhostShape = VAR_Register( "skipGhostShape", "0" );
    x_showSpeedFunc = VAR_Register( "showSpeedFunc", "0" );
    x_speedFuncMax = VAR_Register( "speedFuncMax", "0.75" );
    x_speedCoefA = VAR_Register( "speedCoefA", "400" );
    x_speedCoefB = VAR_Register( "speedCoefB", "50" );
    x_speedCoefC = VAR_Register( "speedCoefC", "40" );
    x_joystickDeadZone = VAR_Register( "joystickDeadZone", "0.9" );
    x_nextShape = VAR_Register( "nextShape", "0" );
    CMD_Register( "moveLeft", MoveLeft_f );
    CMD_Register( "moveRight", MoveRight_f );
    CMD_Register( "moveDown", MoveDown_f );
    CMD_Register( "rotate", Rotate_f );
    CMD_Register( "horizontalMove", HorzAxis_f );
    CMD_Register( "restart", Restart_f );
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

static void DrawSpeedFunc( float showVal ) {
    R_ColorC( colWhite );
    v2_t start = v2xy( 0, R_GetWindowSize().y );
    R_ColorC( colWhite );
    R_DBGLineBegin( start );
    float step = VAR_Num( x_speedFuncMax ) / 10;
    float coefA = VAR_Num( x_speedCoefA );
    float coefB = VAR_Num( x_speedCoefB );
    float coefC = VAR_Num( x_speedCoefC );
    bool_t changed = VAR_Changed( x_speedFuncMax ) 
        || VAR_Changed( x_speedCoefA )
        || VAR_Changed( x_speedCoefB )
        || VAR_Changed( x_speedCoefC );
    for ( int i = 0; i < 10; i++ ) {
        int x = i * step * coefA;
        int y = GetSpeedParam( i, step * 256, coefA , coefB, coefC );
        R_DBGLineTo( v2Add( start, v2xy( x, -y ) ) );
        if ( changed ) {
            PrintC2( c2xy( x, y ) );
        }
    }
    for ( int i = 0; i < 10; i++ ) {
        int x = i * step * coefA;
        int y = GetSpeedParam( i, step * 256, coefA , coefB, coefC );
        v2_t p0 = v2xy( x, start.y );
        v2_t p1 = v2xy( x, start.y - y );
        R_DBGLine( p0, p1 );
    }
}

static int GetTotalErasedLines( void ) {
    int numErasedLines = 0;
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *p = &x_pls[i];
        if ( p->active ) {
            numErasedLines += p->numErasedLines;
        }
    }
    return numErasedLines;
}

static bool_t IsVSGame( void ) {
    return NumActiveHumanPlayers() > 1 || IsDemo(); 
}

static void UpdateAllSeats( int time, int deltaTime ) {
    v2_t ws = R_GetWindowSize();
    int szx = Maxi( ws.x / ( x_tileSize.x * SEAT_WIDTH * 2 ), 1 );
    int szy = Maxi( ws.y / ( x_tileSize.y * SEAT_HEIGHT ), 1 );
    x_pixelSize = Mini( szx, szy );
    c2_t tileScreen = c2Divs( c2Div( c2v2( ws ), x_tileSize ), x_pixelSize );
    int scoreLead = 0;
    if ( IsVSGame() ) {
        if ( x_pls[0].score > x_pls[1].score ) {
            scoreLead = 1;
        } else if ( x_pls[0].score < x_pls[1].score ) {
            scoreLead = -1;
        }
    }
    int level = GetTotalErasedLines() / VAR_Num( x_numLinesPerLevel );
    int gap = tileScreen.x - SEAT_WIDTH * 2; 
    int keepPlaying[2];
    c2_t offset[] =  {
        c2xy( gap / 3, 0 ),
        c2xy( tileScreen.x - SEAT_WIDTH - gap / 3, 0 ),
    };
    for ( int i = 0; i < 2; i++ ) {
        playerSeat_t *p = &x_pls[i];
        keepPlaying[i] = UpdateSeat( offset[i], p, deltaTime, level, i == 0 ? scoreLead : -scoreLead );
    }

    if ( IsDemo() ) { 
        if ( ! keepPlaying[0] || ! keepPlaying[1] ) {
            StartDemoGame();
        }
    } else if ( AnySeatActive() ) {
        if ( ( IsCPU( &x_pls[0] ) && ! keepPlaying[1] )
            || ( IsCPU( &x_pls[1] ) && ! keepPlaying[0] )
            || ( ! keepPlaying[0] && ! keepPlaying[1] ) ) {
            for ( int i = 0; i < 2; i++ ) {
                playerSeat_t *p = &x_pls[i];
                if ( p->active ) {
                    CPUClearCommands( p );
                    UnlatchButtons( p );
                    p->matchEndTime = time;
                    p->active = false;
                    x_timeSinceGameOver = 0;
                    CON_Printf( "Game Over\n" );
                }
            }
            UpdateMusicPlayback();
        }
    }
}

static void AppFrame( void ) {
    int now = SYS_RealTime();
    int deltaTime = now - x_prevTime;
    if ( ! VAR_Num( x_skipBoards ) ) {
        UpdateAllSeats( now, deltaTime );
    }
    if ( VAR_Num( x_showAtlas ) ) {
        DrawAtlas();
    }
    if ( VAR_Changed( x_musicVolume ) ) {
        UpdateMusicPlayback();
    }
    float showFunc = VAR_Num( x_showSpeedFunc );
    if ( showFunc ) {
        DrawSpeedFunc( showFunc );
    }
    SDL_Delay( 10 );
    x_timeSinceGameOver  += deltaTime;
    x_prevTime = now;
}

static void AppDone( void ) {
    CPUClearCommands( &x_pls[0] );
    CPUClearCommands( &x_pls[1] );
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "tetris", RegisterVars, Init, AppFrame, AppDone, 0 );
    return 0;
}
