#define DBG_PRINT CON_Printf
#include "zhost.h"

static rImage_t *ASCIITexture;
static v2_t ASCIITextureSize;
static v2_t ASCIISymbolSize;
static int PixelSize;

static void DrawASCIITexture( v2_t position ) {
    R_ColorC( colRed );
    R_BlendPicV2( position, ASCIITextureSize, v2zero, v2one, ASCIITexture );
}

static void NarisuvaiSimvol( c2_t position, int symbol ) {
    v2_t st0 = v2xy( ( symbol & 15 ) * ASCIISymbolSize.x, ( symbol / 16 ) * ASCIISymbolSize.y );
    v2_t st1 = v2Add( st0, ASCIISymbolSize );
    st0 = v2xy( st0.x / ASCIITextureSize.x, st0.y / ASCIITextureSize.y );
    st1 = v2xy( st1.x / ASCIITextureSize.x, st1.y / ASCIITextureSize.y );
    v2_t scale = v2Scale( ASCIISymbolSize, PixelSize );
    R_BlendPicV2( v2xy( position.x * scale.x, position.y * scale.y ), scale, st0, st1, ASCIITexture );
}

static void InicializiraiAktivnaFigura( void );

static void Init( void ) {
    ASCIITexture = R_LoadStaticTextureEx( "cp437_12x12.png", &ASCIITextureSize );
    ASCIISymbolSize = v2Scale( ASCIITextureSize, 1 / 16. );
    InicializiraiAktivnaFigura();
}

//==============================================================================================

/*

TODO
Kogato figurite padnat na danoto na poleto i ochertaiat linia s ostatacite na drugite figuri, zapalneniat red se unishtojava.
Igrachat moje da varti aktivnata figura.
    Izpolzvaiki gorna strelka
Igrata stava po-barza/vdiga se nivoto sled n na broi unishtojeni redove.
Ako chast ot figura dokosne gornia krai na igralnoto pole, igrata e zagubena.
Tochki se davat za vseki iztrit red.
Sledvashtata figura koiato shte bade aktivna sled tekushtata e pokazana na ekrana kato chast ot potrebitelskia interfeis.
Bonus tochki se davat za ednovremenno unidhtojeni mnojestvo redove.
Izobraziavane na tochkite kato chast ot potrebitelskia interfeis.

IN PROGRESS
Kogato figurite padnat na danoto na poleto, nova (proizvolna) figura se dava na igracha za manipulacia.
    * Proverka dali figura e na danoto (ne moje da se manipulira poveche).
    * Kopirane na starata figura varhu igralnoto pole. 
    * Sazdavene na nova figura.
    Figurite sa s razlichna forma.

DONE

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

// VNIMANIE! promeni tezi razmeri, ako promenish razmera na poleto!
const c2_t IgralnoPoleRazmer = { .x = 12, .y = 20 };
char IgralnoPole[] =
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
" @  "
" @  "
" @  "
" @  "

"    "
"@@@@"
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

static c2_t x_poziciaNaAktivnataFigura;
static int x_predishnoVreme;
static int x_skorostNaAktivnaFigura = 35;
static int x_butonNadolu;

static int ProchetiSimvolOtKarta( c2_t poziciaVavFigura, c2_t razmerNaFigura, const char *figura ) {
    if ( poziciaVavFigura.x >= 0 && poziciaVavFigura.x < razmerNaFigura.x
        && poziciaVavFigura.y >= 0 && poziciaVavFigura.y < razmerNaFigura.y ) {
        return figura[poziciaVavFigura.x + poziciaVavFigura.y * razmerNaFigura.x];
    }
    return ' ';
}

static int ProchetiSimvolOtIgralnoPole( c2_t pozicia ) {
    return ProchetiSimvolOtKarta( pozicia, IgralnoPoleRazmer, IgralnoPole );
}

static void ZapishiSimvolVKarta( int simvol, c2_t pozicia, c2_t razmerNaFigura, char *figura )
{
    if ( pozicia.x >= 0 && pozicia.x < razmerNaFigura.x
        && pozicia.y >= 0 && pozicia.y < razmerNaFigura.y ) {
        figura[pozicia.x + pozicia.y * razmerNaFigura.x] = simvol;
    }
}

static bool_t EProzrachenSimvol( int simvol ) {
    return simvol == ' ';
}

static void KopiraiKartaVDrugaKarta( const char *iztochnik, c2_t razmerIztochnik, 
                                        char *cel, c2_t razmerCel, c2_t poziciaVCelta ) {
    for ( int y = 0; y < razmerIztochnik.y; y++ ) {
        for ( int x = 0; x < razmerIztochnik.x; x++ ) {
            c2_t xy = c2xy( x, y );
            int simvol = ProchetiSimvolOtKarta( xy, razmerIztochnik, iztochnik );
            if ( ! EProzrachenSimvol( simvol ) ) {
                c2_t krainaPozicia = c2Add( xy, poziciaVCelta );
                ZapishiSimvolVKarta( simvol, krainaPozicia, razmerCel, cel );
            }
        }
    }
}

static void NarisuvaiKartaOtSimvoli( c2_t poziciaNaEkrana, const char *karta, c2_t razmerNaKarta, color_t cviat ) {
    static const int saotv[256] = {
        ['@'] = 1,
        ['#'] = 1 + 11 * 16,
    };
    R_ColorC( cviat );
    for ( int i = 0, y = 0; y < razmerNaKarta.y; y++ ) {
        for ( int x = 0; x < razmerNaKarta.x; x++ ) {
            int simvol = karta[i++];
            NarisuvaiSimvol( c2Add( c2xy( x, y ), poziciaNaEkrana ), saotv[simvol] );
        }
    }
}

static bool_t SekaLiIgralnoPole( c2_t poziciaNaFiguraNaEkrana, const char *figura, c2_t razmerNaFigura ) {
    for ( int y = 0; y < razmerNaFigura.y; y++ ) {
        for ( int x = 0; x < razmerNaFigura.x; x++ ) {
            // tekushtia simvol ot figurata e na tazi pozicia
            c2_t poziciaVavFigura = c2xy( x, y );
            int simvolOtFigura = ProchetiSimvolOtKarta( poziciaVavFigura, razmerNaFigura, figura );
            // otmesti s poziciaNaFiguraNaEkrana za da namerish poziciata na ekrana na tozi simvol
            c2_t poziciaNaEkrana = c2Add( poziciaVavFigura, poziciaNaFiguraNaEkrana );
            int simvolOtIgralnoPole = ProchetiSimvolOtIgralnoPole( poziciaNaEkrana );
            // simvolat ot figurata e neprozrachen i simvolat na sashtata pozicia ot igralnoto pole e neprozrachen
            // ! - otricanie
            // == - ravenstvo
            // != - neravenstvo
            // && - logichesko "i"
            // || - logichesko "ili"
            if ( ! EProzrachenSimvol( simvolOtFigura ) 
                    && ! EProzrachenSimvol( simvolOtIgralnoPole ) ) {
                return true;
            }
        }
    }
    return false;
}

static c2_t IntToFixed( c2_t c ) {
    return c2Scale( c, 100 );
}

static c2_t FixedToInt( c2_t c ) {
    return c2Divs( c, 100 );
}

static void InicializiraiAktivnaFigura( void ) {
    x_poziciaNaAktivnataFigura = IntToFixed( c2xy( IgralnoPoleRazmer.x / 2 - 2, -FigChertaRazmer.y ) );
    x_predishnoVreme = SYS_RealTime();
    x_butonNadolu = 0;
}

static bool_t ProveriIMesti( c2_t badeshtaPozicia ) {
    bool_t mogaDaMestia = ! SekaLiIgralnoPole( FixedToInt( badeshtaPozicia ), FigCherta, FigChertaRazmer );
    if ( mogaDaMestia ) {
        x_poziciaNaAktivnataFigura = badeshtaPozicia;
    }
    return mogaDaMestia;
}
    
static bool_t MestiNadolu( int deltaVreme ) {
    int y = x_poziciaNaAktivnataFigura.y + deltaVreme * x_skorostNaAktivnaFigura * ( 1 + x_butonNadolu * 3 ) / 100;
    c2_t badeshtaPozicia = c2xy( x_poziciaNaAktivnataFigura.x, y );
    return ProveriIMesti( badeshtaPozicia );
}

static void PraviKadar( void ) {
    // izchisli goleminata na simvolite/spraitove
    // simvoli i spraitove v tozi sa vzaimnozameniaemi
    PixelSize = Maxi( R_GetWindowSize().y / 320, 1 );
    int sega = SYS_RealTime();
    int deltaVreme = sega - x_predishnoVreme;
    // ako ne moga da se premestia nadolu...
    if ( ! MestiNadolu( deltaVreme ) ) {
        // ... togava kopirai aktivnata figura varhu igralnoto pole
        KopiraiKartaVDrugaKarta( FigCherta, FigChertaRazmer, IgralnoPole, IgralnoPoleRazmer, FixedToInt( x_poziciaNaAktivnataFigura ) );
        // ... i napravi nova figura
        InicializiraiAktivnaFigura();
    } else {
        NarisuvaiKartaOtSimvoli( FixedToInt( x_poziciaNaAktivnataFigura ), FigCherta, FigChertaRazmer, colGreen );
    }
    // narisuvai igralnoto pole izpolzvaiki funkciata za risuvane na karta ot simvoli
    NarisuvaiKartaOtSimvoli( c2zero, IgralnoPole, IgralnoPoleRazmer, colWhite );
    // narisuvai tablicata sas simvoli v desnia agal na ekrana
    v2_t windowSize = R_GetWindowSize();
    DrawASCIITexture( v2xy( windowSize.x - ASCIITextureSize.x, 0 ) );
    x_predishnoVreme = sega;
    SDL_Delay( 10 );
}

static void MestiNadiasno_f( void ) {
    c2_t badeshtaPozicia = c2Add( x_poziciaNaAktivnataFigura, IntToFixed( c2xy( +1, 0 ) ) );
    ProveriIMesti( badeshtaPozicia );
}

static void MestiNaliavo_f( void ) {
    c2_t badeshtaPozicia = c2Add( x_poziciaNaAktivnataFigura, IntToFixed( c2xy( -1, 0 ) ) );
    ProveriIMesti( badeshtaPozicia );
}

static void MestiNadolu_f( void ) {
    x_butonNadolu = *CMD_Argv( 0 ) == '+' ? 1 : 0;
}

static void RegistriraiKomandi( void ) {
    CMD_Register( "mestiNaliavo", MestiNaliavo_f );
    CMD_Register( "mestiNadiasno", MestiNadiasno_f );
    CMD_Register( "mestiNadolu", MestiNadolu_f );
    I_Bind( "Left", "+mestiNaliavo" );
    I_Bind( "Right", "+mestiNadiasno" );
    I_Bind( "Down", "mestiNadolu" );
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "tetris", RegistriraiKomandi, Init, PraviKadar, NULL, 0 );
    return 0;
}
