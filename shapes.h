static const c2_t x_boardSize = { .x = 12, .y = 21 };
static const char x_board[] =
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

static const c2_t x_shapeSize = { .x = 4, .y = 4 };

typedef struct {
    const int numBitmaps;
    const char *bitmaps[4];
} shape_t;

static const shape_t x_shapes[] = {
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

#define NUM_SHAPES (sizeof(x_shapes)/sizeof(*x_shapes))
