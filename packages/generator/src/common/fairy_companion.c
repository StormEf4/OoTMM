#include <combo.h>
#include <combo/actor.h>
#include <combo/matrix.h>
#include <combo/math.h>
#include <combo/misc.h>
#include <combo/player.h>

/*
 * Fairy Companion cosmetic (fork): restyles Navi (OoT) and Tatl (MM).
 * The generator sets FAIRY_STYLE from Cosmetics > Fairy Companion.
 * See docs/custom/fairy-cosmetics.md.
 */

#define FAIRY_STYLE_FURBY   (1 << 0) /* Draw a Furby sprite instead of the wings */
#define FAIRY_STYLE_RGB     (1 << 1) /* Glow cycles through every color */

u8 gFairyStyle;
EXPORT_SYMBOL(FAIRY_STYLE, gFairyStyle);

typedef void (*FairyDrawFunc)(Actor*, PlayState*);

static FairyDrawFunc sFairyVanillaDraw;

ALIGNED(8) static const u16 kFurbyTex[32 * 32] = {
#include "fairy_furby_tex.rgba16.inc"
};

ALIGNED(8) static const u8 kGlowTex[32 * 32] = {
#include "fairy_glow_tex.i8.inc"
};

static const Vtx kFairyQuadVtx[] = {
    VTX(-100, -100, 0,    0, 1024, 255, 255, 255, 255),
    VTX( 100, -100, 0, 1024, 1024, 255, 255, 255, 255),
    VTX( 100,  100, 0, 1024,    0, 255, 255, 255, 255),
    VTX(-100,  100, 0,    0,    0, 255, 255, 255, 255),
};

/* Glow: lerp from the outer color (env) to the inner color (prim) by the texture */
static const Gfx kFairyGlowDL[] = {
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock(kGlowTex, G_IM_FMT_I, G_IM_SIZ_8b, 32, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                         G_TX_NOMIRROR | G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetCombineLERP(PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                       0, 0, 0, COMBINED, 0, 0, 0, COMBINED),
    gsSPClearGeometryMode(G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsSPVertex(kFairyQuadVtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};

/* Furby: the texture as is, faded by the prim alpha */
static const Gfx kFairyFurbyDL[] = {
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock(kFurbyTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                         G_TX_NOMIRROR | G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetCombineLERP(0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0,
                       0, 0, 0, COMBINED, 0, 0, 0, COMBINED),
    gsSPClearGeometryMode(G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsSPVertex(kFairyQuadVtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};

static int FairyCompanion_IsCompanion(Actor_EnElf* this)
{
    /* Navi is fairy type 0 in OoT, Tatl is fairy type 0 in MM */
    return (this->base.params & 0xf) == 0;
}

/* Hue 0-1535 to a fully saturated color */
static void FairyCompanion_Hue(u32 hue, u8* r, u8* g, u8* b)
{
    u8 x;

    hue %= 1536;
    x = hue & 0xff;
    switch (hue >> 8)
    {
    case 0: *r = 255;     *g = x;       *b = 0;       break;
    case 1: *r = 255 - x; *g = 255;     *b = 0;       break;
    case 2: *r = 0;       *g = 255;     *b = x;       break;
    case 3: *r = 0;       *g = 255 - x; *b = 255;     break;
    case 4: *r = x;       *g = 0;       *b = 255;     break;
    default: *r = 255;    *g = 0;       *b = 255 - x; break;
    }
}

static void FairyCompanion_RgbColors(Actor_EnElf* this, PlayState* play)
{
    u32 hue;
    u8 r;
    u8 g;
    u8 b;

    /* One full cycle every 3 seconds */
    hue = play->gameplayFrames * 9;

    FairyCompanion_Hue(hue, &r, &g, &b);
    this->innerColor.r = r;
    this->innerColor.g = g;
    this->innerColor.b = b;

    /* Outer glow a bit ahead on the wheel, so the halo shimmers */
    FairyCompanion_Hue(hue + 256, &r, &g, &b);
    this->outerColor.r = r;
    this->outerColor.g = g;
    this->outerColor.b = b;
}

static void FairyCompanion_DrawFurby(Actor_EnElf* this, PlayState* play)
{
    GraphicsContext* gfxCtx;
    f32 alphaScale;
    f32 scale;
    s32 envAlpha;
    u8 alpha;
    int noDepth;

    gfxCtx = play->state.gfxCtx;

    /* Same fade and pulse as the vanilla glow */
    alphaScale = this->disappearTimer < 0 ? (this->disappearTimer * (7.0f / 6000.0f)) + 1.0f : 1.0f;
    envAlpha = (this->timer * 50) & 0x1ff;
    envAlpha = (envAlpha > 255) ? 511 - envAlpha : envAlpha;
    alpha = (u8)(this->innerColor.a * alphaScale);
    scale = this->base.scale.x * 125.0f;

    /* Like the vanilla draw, the companion shows through walls */
    noDepth = this->fairyFlags & 4;

    OPEN_DISPS(gfxCtx);
    Matrix_Push();

    Gfx_SetupDL25_Xlu(gfxCtx);

    /* Glow, facing the camera */
    Matrix_Translate(this->base.world.pos.x, this->base.world.pos.y, this->base.world.pos.z, MTXMODE_NEW);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Push();
    Matrix_Scale(
        ((Math_SinS(this->timer * 4096) * 0.1f) + 1.0f) * 0.14f * scale,
        ((Math_SinS(this->timer * 4096) * 0.1f) + 1.0f) * 0.14f * scale,
        1.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_Finalize(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPPipeSync(POLY_XLU_DISP++);
    if (noDepth)
    {
        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_CLD_SURF2);
    }
    else
    {
        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_ZB_CLD_SURF2);
    }
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x01, (u8)this->innerColor.r, (u8)this->innerColor.g, (u8)this->innerColor.b, alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, (u8)this->outerColor.r, (u8)this->outerColor.g, (u8)this->outerColor.b, (u8)(envAlpha * alphaScale));
    gSPDisplayList(POLY_XLU_DISP++, kFairyGlowDL);
    Matrix_Pop();

    /* Furby, in front of the glow */
    Matrix_Translate(0.0f, 0.0f, 1.0f, MTXMODE_APPLY);
    Matrix_Scale(0.07f * scale, 0.07f * scale, 1.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_Finalize(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPPipeSync(POLY_XLU_DISP++);
    if (noDepth)
    {
        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_XLU_SURF2);
    }
    else
    {
        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    }
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x01, 255, 255, 255, alpha);
    gSPDisplayList(POLY_XLU_DISP++, kFairyFurbyDL);

    Matrix_Pop();
    CLOSE_DISPS();
}

static void FairyCompanion_Draw(Actor* thisx, PlayState* play)
{
    Actor_EnElf* this;
    Gfx* start;

    this = (Actor_EnElf*)thisx;

    if (gFairyStyle & FAIRY_STYLE_RGB)
        FairyCompanion_RgbColors(this, play);

    if (!(gFairyStyle & FAIRY_STYLE_FURBY))
    {
        sFairyVanillaDraw(thisx, play);
        return;
    }

    /*
     * Let the vanilla draw decide when the fairy is visible (hidden in
     * first person, in some cutscenes, ...), then throw away what it drew
     * and draw the Furby instead.
     */
    start = play->state.gfxCtx->polyXlu.append;
    sFairyVanillaDraw(thisx, play);
    if (play->state.gfxCtx->polyXlu.append == start)
        return;
    play->state.gfxCtx->polyXlu.append = start;

    FairyCompanion_DrawFurby(this, play);
}

void FairyCompanion_Init(Actor_EnElf* this, PlayState* play)
{
    if (!gFairyStyle || !FairyCompanion_IsCompanion(this) || this->base.draw == NULL)
        return;

    sFairyVanillaDraw = this->base.draw;
    this->base.draw = FairyCompanion_Draw;
}
