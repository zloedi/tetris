#include "zhost.h"

static rImage_t *ASCIITexture;
static v2_t ASCIITextureSize;
static v2_t ASCIISymbolSize;

static void DrawASCIITexture( v2_t position )
{
    R_ColorC( colRed );
    R_DrawPicV2( position, ASCIITextureSize, v2zero, v2one, ASCIITexture );
}

static void NarisuvaiSimvol( v2_t position, int symbol ) {
    v2_t symbolSize = ASCIISymbolSize;
    v2_t st0 = v2xy( ( symbol % 16 ) * symbolSize.x, ( symbol / 16 ) * symbolSize.y );
    v2_t st1 = v2Add( st0, symbolSize );
    st0 = v2xy( st0.x / ASCIITextureSize.x, st0.y / ASCIITextureSize.y );
    st1 = v2xy( st1.x / ASCIITextureSize.x, st1.y / ASCIITextureSize.y );
    R_DrawPicV2( position, symbolSize, st0, st1, ASCIITexture );
}

static void Init( void ) {
    ASCIITexture = R_LoadStaticTextureEx( "cp437_12x12.png", &ASCIITextureSize );
    ASCIISymbolSize = v2Scale( ASCIITextureSize, 1 / 16. );
}

static void NarisuvaiKartaOtSimvoli( v2_t poziciaNaEkrana, const char *karta, c2_t razmerNaKarta, color_t cviat )
{
    R_ColorC( cviat );
    for ( int y = 0; y < razmerNaKarta.y; y++ ) {
        for ( int x = 0; x < razmerNaKarta.x; x++ ) {
            v2_t poziciaNaSimvol = v2xy( x * ASCIISymbolSize.x, y * ASCIISymbolSize.y );
            int simvol = karta[x + y * razmerNaKarta.x];
            NarisuvaiSimvol( v2Add( poziciaNaSimvol, poziciaNaEkrana ), simvol );
        }
    }
}

//==============================================================================================

// VNIMANIE! promeni tezi razmeri, ako promenish razmera na poleto!
const c2_t IgralnoPoleRazmer = { .x = 12, .y = 20 };
const char *IgralnoPole =
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
                      
const c2_t FigChertaRazmer = { .x = 4, .y = 4 };
const char *FigCherta =
" #  "
" #  "
" #  "
" #  "

"    "
"####"
"    "
"    "
;

const c2_t FigZRazmer = { .x = 3, .y = 3 };
const char *FigZ =
"## "
" ##"
"   "

" # "
"## "
"#  "
;

static void Frame( void ) {
    v2_t windowSize = R_GetWindowSize();
    NarisuvaiKartaOtSimvoli( v2zero, IgralnoPole, IgralnoPoleRazmer, colWhite );
    NarisuvaiKartaOtSimvoli( v2xy( 20, 20 ), FigCherta, FigChertaRazmer, colGreen );
    DrawASCIITexture( v2xy( windowSize.x - ASCIITextureSize.x, 0 ) );
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "tetris", NULL, Init, Frame, NULL );
    return 0;
}
