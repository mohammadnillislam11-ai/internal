// Define enum types first

/////// RJ PROFESSOR ROOT INJECTOR PROJECT LEKA///////


////// RJ PROFESSOR SABKA PAPA HAI ///////


////// ENJOY MY KIDS PROJECT///////






enum BoxType {
    BOX_STROKE = 0,
    BOX_FILLED = 1,
    BOX_CORNER = 2,
    BOX_ROUNDED = 3
};

enum LineType {
    LINE_TOP = 0,
    LINE_CENTER = 1,
    LINE_BOTTOM = 2
};

struct {
    bool aimbot = false;
    bool fovawm = false;
    bool SpeedHackV2 = false;
    bool selectTarget = false;
    float AimFov = 60;
    float AimDistance = 30;
    bool NoRecoil = false;
    bool aimbotlock = false;
    bool undercam = false;
    bool PlayerToMe = false;
    bool silen4a = false;      // Silent aim (matches variable name)
    int aimbotType = 0;
} pPlayer;

struct {
    bool Bypass = false;
    bool activar = false;
} Actived;

struct {
    bool espLinha = false;
    bool espCaixa = false;
    bool espHealth = false;
    bool espDistance = false;
    bool espName = false;
    bool espSkeleton = false;
    bool espIden360 = false;
    float textSize = 15.0f;    
    Color espColor = Color::White();
    BoxType boxType = BOX_STROKE;
    LineType lineType = LINE_TOP;
} pEspPlayer;