#include "zhost.h"

static rImage_t *ASCIITexture;
static v2_t ASCIITextureSize;
static v2_t ASCIISymbolSize;
static int PixelSize;

static void DrawASCIITexture( v2_t position ) {
    R_ColorC( colRed );
    R_DrawPicV2( position, ASCIITextureSize, v2zero, v2one, ASCIITexture );
}

static void NarisuvaiSimvol( c2_t position, int symbol ) {
    v2_t st0 = v2xy( ( symbol & 15 ) * ASCIISymbolSize.x, ( symbol / 16 ) * ASCIISymbolSize.y );
    v2_t st1 = v2Add( st0, ASCIISymbolSize );
    st0 = v2xy( st0.x / ASCIITextureSize.x, st0.y / ASCIITextureSize.y );
    st1 = v2xy( st1.x / ASCIITextureSize.x, st1.y / ASCIITextureSize.y );
    v2_t scale = v2Scale( ASCIISymbolSize, PixelSize );
    R_DrawPicV2( v2xy( position.x * scale.x, position.y * scale.y ), scale, st0, st1, ASCIITexture );
}

static void Init( void ) {
    ASCIITexture = R_LoadStaticTextureEx( "cp437_12x12.png", &ASCIITextureSize );
    ASCIISymbolSize = v2Scale( ASCIITextureSize, 1 / 16. );
}

static void NarisuvaiKartaOtSimvoli( c2_t poziciaNaEkrana, const char *karta, c2_t razmerNaKarta, color_t cviat, int kartinkaNaNeprozrachenSimvol ) {
    R_ColorC( cviat );
    for ( int y = 0; y < razmerNaKarta.y; y++ ) {
        for ( int x = 0; x < razmerNaKarta.x; x++ ) {
            int simvol = karta[x + y * razmerNaKarta.x];
            if ( simvol != ' ' ) {
                simvol = kartinkaNaNeprozrachenSimvol;
            } 
            NarisuvaiSimvol( c2Add( c2xy( x, y ), poziciaNaEkrana ), simvol );
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

static int x_vazrastFigura;
static int x_predishnoVreme;

static int ProchetiSimvolOtIgralnoPole( c2_t pozicia ) {
    return IgralnoPole[pozicia.x + pozicia.y * IgralnoPoleRazmer.x];
}

static bool_t SekaLiNeprozrachniSimvoli( c2_t poziciaNaFigura, const char *figura, c2_t razmerNaFigura ) {
    for ( int y = 0; y < razmerNaFigura.y; y++ ) {
        for ( int x = 0; x < razmerNaFigura.x; x++ ) {
            c2_t poziciaNaSimvol = c2Add( c2xy( x, y ), poziciaNaFigura );
            int simvolOtFigura = figura[x + y * razmerNaFigura.x];
            int simvolOtIgralnoPole = ProchetiSimvolOtIgralnoPole( poziciaNaSimvol );
            // simvolat ot figurata e neprozrachen i simvolat na sashtata pozicia ot igralnoto pole e neprozrachen
            // ! - otricanie
            // == - ravenstvo
            // != - neravenstvo
            // && - logichesko "i"
            // || - logichesko "ili"
            if ( simvolOtFigura != ' ' && simvolOtIgralnoPole != ' ' ) {
                return true;
            }
        }
    }
    return false;
}

static c2_t IzchisliPoziciaSporedSkorostVreme( int skorost, int vreme ) {
    return c2xy( IgralnoPoleRazmer.x / 2 - 2, skorost * vreme / 1000 );
}

static void PraviKadar( void ) {
    PixelSize = Maxi( R_GetWindowSize().y / 320, 1 );
    NarisuvaiKartaOtSimvoli( c2zero, IgralnoPole, IgralnoPoleRazmer, colWhite, 1 + 11 * 16 );
    int sega = SYS_RealTime();
    int vreme = x_vazrastFigura + sega - x_predishnoVreme;
    int skorost = 3;
    c2_t poziciaNaFigura = IzchisliPoziciaSporedSkorostVreme( skorost, vreme );
    if ( ! SekaLiNeprozrachniSimvoli( poziciaNaFigura, FigCherta, FigChertaRazmer ) ) {
        x_vazrastFigura = vreme;
    }
    poziciaNaFigura = IzchisliPoziciaSporedSkorostVreme( skorost, x_vazrastFigura );
    NarisuvaiKartaOtSimvoli( poziciaNaFigura, FigCherta, FigChertaRazmer, colGreen, 1 );
    x_predishnoVreme = sega;

    v2_t windowSize = R_GetWindowSize();
    DrawASCIITexture( v2xy( windowSize.x - ASCIITextureSize.x, 0 ) );
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "tetris", NULL, Init, PraviKadar, NULL );
    return 0;
}
