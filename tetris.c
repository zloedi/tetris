#include "host.h"

const char *field =
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
                      
const char *cherta =
" #  "
" #  "
" #  "
" #  "

"    "
"####"
"    "
"    "
;

static timestamp_t figureBirthTime;
static c2_t figureOrigin;

static void TTS_DrawTile( int x, int y ) {
    v2_t tileSize = v2xy( r_info.screenWidth / 32, r_info.screenHeight / 32 );
    v2_t position = v2xy( ( float )x * tileSize.x, ( float )y * tileSize.y );
    R2D_DrawPicV2( position, 
            tileSize,
            v2zero,
            v2xy( 1, 1 ),
            0 );
}                     

static void TTS_DrawFigure( c2_t pos, const char *fig ) {
    for ( int y = 0; y < 4; y++ ) {
        for ( int x = 0; x < 4; x++ ) {
            if ( fig[x + y * 4] != ' ' ) {
                TTS_DrawTile( pos.x + x, pos.y + y );
            }
        }
    }
}
                      
void TTS_Init( void ) {
    figureBirthTime = SYS_RealTime();
    figureOrigin = c2xy( 6, 0 );
}                     
                      
void TTS_Frame( void ) {
    //int milisekundi = SYS_RealTime();
    //int sekundi = milisekundi / 1000;
    //int minuti = sekundi / 60;
    //int chasove = minuti / 60;
    //CON_Printf( "ive iztekoha %.2d:%.2d:%.2d:%.4d otkakto pusnahme igrata\n", chasove % 24, minuti % 60, sekundi % 60, milisekundi % 1000 );
    //R2D_DrawPic
    R2D_Color( 1, 1, 1, 1 );
    for ( int y = 0; y < 20; y++ ) {
        for ( int x = 0; x < 12; x++ ) {
            if ( field[x + y * 12] == '#' ) {
                TTS_DrawTile( x, y );
            }
        }
    }
    timestamp_t figureAge = SYS_RealTime() - figureBirthTime;
    figureOrigin.y = figureAge / 250;
    TTS_DrawFigure( figureOrigin, cherta );
}
