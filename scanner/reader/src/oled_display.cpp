// oled_display.cpp — Renderizado de páginas OBD2 en SSD1306 128x64
#include "oled_display.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

OLEDDisplay::OLEDDisplay(SSD1306& oled, VehicleData& data)
    : oled_(oled), data_(data)
{
    lastPageChange_ = std::chrono::steady_clock::now();
}

OLEDDisplay::~OLEDDisplay() { stop(); }

// ─── Control del hilo ────────────────────────────────────────────────────────

void OLEDDisplay::start(int refreshMs)
{
    if (running_) return;
    refreshMs_ = refreshMs;
    running_ = true;
    thread_ = std::thread(&OLEDDisplay::threadLoop, this);
}

void OLEDDisplay::stop()
{
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void OLEDDisplay::threadLoop()
{
    while (running_) {
        auto t0 = std::chrono::steady_clock::now();

        // Auto-rotación de páginas
        if (autoRotate_ && data_.connected) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                t0 - lastPageChange_).count();
            if (elapsed >= autoIntervalSec_) {
                int next = (static_cast<int>(page_) + 1) % static_cast<int>(OLEDPage::COUNT);
                if (next == static_cast<int>(OLEDPage::SPLASH)) next = 1; // skip splash
                page_ = static_cast<OLEDPage>(next);
                lastPageChange_ = t0;
            }
        }

        render();

        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();
        int wait = refreshMs_ - (int)dt;
        if (wait > 0) std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
}

void OLEDDisplay::nextPage() {
    int next = (static_cast<int>(page_) + 1) % static_cast<int>(OLEDPage::COUNT);
    if (next == 0) next = 1;
    page_ = static_cast<OLEDPage>(next);
    lastPageChange_ = std::chrono::steady_clock::now();
}
void OLEDDisplay::prevPage() {
    int prev = static_cast<int>(page_) - 1;
    if (prev <= 0) prev = static_cast<int>(OLEDPage::COUNT) - 1;
    page_ = static_cast<OLEDPage>(prev);
    lastPageChange_ = std::chrono::steady_clock::now();
}
void OLEDDisplay::setPage(OLEDPage page) {
    page_ = page;
    lastPageChange_ = std::chrono::steady_clock::now();
}
void OLEDDisplay::setAutoRotate(bool enabled, int intervalSec) {
    autoRotate_ = enabled;
    autoIntervalSec_ = intervalSec;
}

// ─── Render principal ────────────────────────────────────────────────────────

void OLEDDisplay::render()
{
    oled_.clear();

    if (!data_.connected) {
        renderSplash();
        oled_.display();
        return;
    }

    switch (page_) {
        case OLEDPage::SPLASH:  renderSplash();  break;
        case OLEDPage::MAIN:    renderMain();    break;
        case OLEDPage::ENGINE:  renderEngine();  break;
        case OLEDPage::FUEL:    renderFuel();    break;
        case OLEDPage::O2:      renderO2();      break;
        case OLEDPage::GM:      renderGM();      break;
        case OLEDPage::DTC:     renderDTC();     break;
        case OLEDPage::DEBUG:   renderDebug();   break;
        default:                renderMain();    break;
    }

    oled_.display();
}

// ─── Helpers ────────────────────────────────────────────────────────────────

std::string OLEDDisplay::fmtDouble(double v, int decimals)
{
    if (v <= -999.0 || v < -90.0) return "--";
    std::stringstream ss;
    ss << std::fixed << std::setprecision(decimals) << v;
    return ss.str();
}

std::string OLEDDisplay::fmtInt(int v)
{
    if (v < -90) return "--";
    return std::to_string(v);
}

bool OLEDDisplay::dataFresh() const {
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - data_.lastUpdate).count();
    return age < 5;
}

// Cabecera: título en línea 0 + indicador de página
void OLEDDisplay::drawHeader(const std::string& title, int pageNum)
{
    // Barra superior sólida
    oled_.fillRect(0, 0, OLED_WIDTH, 9, true);
    // Título en negro (invertido)
    int tx = (OLED_WIDTH - (int)title.length()*6) / 2;
    if (tx < 1) tx = 1;
    oled_.drawString(tx, 1, title, 1, false);
    // Indicador de página
    std::string pg = std::to_string(pageNum)+"/7";
    oled_.drawString(OLED_WIDTH - (int)pg.length()*6 - 1, 1, pg, 1, false);
}

// Footer: línea separadora + número de página como puntos
void OLEDDisplay::drawFooter()
{
    oled_.drawLine(0, 62, OLED_WIDTH-1, 62, true);
    // Puntos indicadores de página
    int total = static_cast<int>(OLEDPage::COUNT) - 1; // sin SPLASH
    int cur   = static_cast<int>(page_) - 1;
    int startX = (OLED_WIDTH - total*5) / 2;
    for (int i=0; i<total; i++) {
        int px = startX + i*5;
        if (i == cur) oled_.fillRect(px, 63, 3, 1, true);
        else          oled_.drawPixel(px+1, 63, true);
    }
}

// ─── PÁGINA SPLASH ───────────────────────────────────────────────────────────
// Sin conexión: logo ASCII + estado BT
void OLEDDisplay::renderSplash()
{
    // Marco exterior
    oled_.drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT, true);

    // Título grande
    oled_.drawStringCentered(4,  "OBD2", 2);
    oled_.drawStringCentered(22, "DIAGNOSTIC", 1);

    // Línea separadora
    oled_.drawLine(10, 32, 118, 32, true);

    if (data_.connected) {
        oled_.drawStringCentered(36, "CONECTADO", 1);
        // Protocolo (truncar a 20 chars)
        std::string proto = data_.protocol;
        if (proto.length() > 20) proto = proto.substr(0,20);
        oled_.drawStringCentered(48, proto, 1);
    } else {
        oled_.drawStringCentered(36, "Buscando BT...", 1);
        // Animación: puntos parpadeantes
        static int dots = 0;
        std::string anim(dots % 4, '.');
        oled_.drawStringCentered(50, anim, 1);
        dots++;
    }
}

// ─── PÁGINA MAIN ─────────────────────────────────────────────────────────────
// RPM grande (escala, barra) + velocidad + temperatura
void OLEDDisplay::renderMain()
{
    std::lock_guard<std::mutex> lock(data_.mtx);

    drawHeader("MOTOR", 1);

    // RPM — número grande
    std::string rpmStr = std::to_string(data_.rpm);
    int rpmX = 2;
    oled_.drawString(rpmX, 11, rpmStr, 2);
    oled_.drawString(rpmX + (int)rpmStr.length()*12 + 2, 17, "rpm", 1);

    // Barra de RPM (0-6000)
    float rpmPct = std::min(100.0f, data_.rpm / 60.0f);
    oled_.drawProgressBar(2, 27, 80, 6, rpmPct);

    // Zona roja (>80%)
    if (rpmPct > 80.0f) {
        // Parpadeo
        static bool blink = false; blink = !blink;
        if (blink) oled_.drawRect(2, 27, 80, 6, true);
    }

    // Velocidad
    oled_.drawString(85, 11, "VEL", 1);
    oled_.drawString(84, 20, std::to_string(data_.speed), 2);
    oled_.drawString(84, 34, "km/h", 1);

    // Línea vertical separadora
    oled_.drawLine(80, 10, 80, 41, true);

    // Temperatura y carga en fila inferior
    oled_.drawLine(0, 43, OLED_WIDTH-1, 43, true);

    // Temperatura
    std::string tStr = fmtInt(data_.coolantTemp) + "C";
    oled_.drawString(2, 46, "TEMP:", 1);
    oled_.drawString(34, 46, tStr, 1);
    // Alerta temperatura alta
    if (data_.coolantTemp > 100) oled_.drawString(75, 46, "!HOT!", 1);

    // Carga
    oled_.drawString(2, 55, "CARGA:", 1);
    oled_.drawString(40, 55, std::to_string(data_.engineLoad)+"%", 1);
    // Barra carga
    oled_.drawProgressBar(85, 47, 40, 5, (float)data_.engineLoad);
    oled_.drawString(85, 55, "MAF:", 1);
    oled_.drawString(108, 55, fmtDouble(data_.maf,1), 1);
}

// ─── PÁGINA ENGINE ───────────────────────────────────────────────────────────
// Admisión: MAP, Acelerador, Timing, Temp admisión
void OLEDDisplay::renderEngine()
{
    std::lock_guard<std::mutex> lock(data_.mtx);
    drawHeader("ADMISION", 2);

    // 4 campos en 2 columnas
    struct Field { const char* label; std::string value; const char* unit; };
    std::vector<Field> fields = {
        {"MAP",  fmtDouble(data_.intakePressure,0), "kPa"},
        {"THRT", fmtDouble(data_.throttle,1),       "%"  },
        {"TIMG", fmtDouble(data_.timingAdvance,1),  "deg"},
        {"ITAK", fmtInt(data_.intakeTemp),          "C"  },
        {"MAF",  fmtDouble(data_.maf,2),            "g/s"},
        {"BARO", fmtDouble(data_.baroPressure,0),   "kPa"},
    };

    int row=0, col=0;
    for (auto& f : fields) {
        int x = col==0 ? 2 : 66;
        int y = 11 + row*17;
        // Label
        oled_.drawString(x, y,   f.label, 1);
        // Valor + unidad
        std::string val = f.value + f.unit;
        oled_.drawString(x, y+8, val, 1);
        // Separador vertical
        if (col==0) oled_.drawLine(63, 10, 63, 62, true);
        col = 1-col;
        if (col==0) row++;
    }
    drawFooter();
}

// ─── PÁGINA FUEL TRIM ────────────────────────────────────────────────────────
// STFT B1/B2, LTFT B1/B2, Nivel combustible
void OLEDDisplay::renderFuel()
{
    std::lock_guard<std::mutex> lock(data_.mtx);
    drawHeader("FUEL TRIM", 3);

    // STFT
    oled_.drawString(2, 11, "STFT B1:", 1);
    std::string s1 = fmtDouble(data_.stftBank1,2)+"%";
    oled_.drawStringRight(11, s1);
    // Color de advertencia (valor > 10%)
    if (std::abs(data_.stftBank1) > 10.0)
        oled_.drawRect(OLED_WIDTH-(int)s1.length()*6-3, 10, (int)s1.length()*6+2, 9, true);

    oled_.drawString(2, 21, "STFT B2:", 1);
    std::string s2 = fmtDouble(data_.stftBank2,2)+"%";
    oled_.drawStringRight(21, s2);

    oled_.drawLine(0, 30, OLED_WIDTH-1, 30, true);

    // LTFT
    oled_.drawString(2, 32, "LTFT B1:", 1);
    oled_.drawStringRight(32, fmtDouble(data_.ltftBank1,2)+"%");

    oled_.drawString(2, 42, "LTFT B2:", 1);
    oled_.drawStringRight(42, fmtDouble(data_.ltftBank2,2)+"%");

    oled_.drawLine(0, 51, OLED_WIDTH-1, 51, true);

    // Nivel combustible con barra
    oled_.drawString(2, 53, "COMB:", 1);
    std::string fuelStr = fmtDouble(data_.fuelLevel,1)+"%";
    oled_.drawString(36, 53, fuelStr, 1);
    oled_.drawProgressBar(80, 54, 46, 7, (float)data_.fuelLevel);
    drawFooter();
}

// ─── PÁGINA O2 ───────────────────────────────────────────────────────────────
// Voltaje y trim de sensor O2 B1S1 y B2S1
void OLEDDisplay::renderO2()
{
    std::lock_guard<std::mutex> lock(data_.mtx);
    drawHeader("SENSOR O2", 4);

    // B1S1
    oled_.drawString(2, 11, "BANCO 1  S1", 1);
    oled_.drawString(2, 21, "Volt:", 1);
    oled_.drawString(34, 21, fmtDouble(data_.o2B1S1V,3)+"V", 1);
    // Barra voltaje 0-1.275V
    oled_.drawProgressBar(2, 30, 124, 5, (float)(data_.o2B1S1V/1.275*100.0));
    // Estado mezcla
    const char* mix1 = data_.o2B1S1V < 0.1 ? "POBRE" :
                       data_.o2B1S1V > 0.9 ? "RICO"  : "OK";
    oled_.drawString(80, 21, mix1, 1);

    oled_.drawLine(0, 37, OLED_WIDTH-1, 37, true);

    // B2S1
    oled_.drawString(2, 39, "BANCO 2  S1", 1);
    oled_.drawString(2, 49, "Volt:", 1);
    oled_.drawString(34, 49, fmtDouble(data_.o2B2S1V,3)+"V", 1);
    oled_.drawProgressBar(2, 58, 124, 5, (float)(data_.o2B2S1V/1.275*100.0));
    const char* mix2 = data_.o2B2S1V < 0.1 ? "POBRE" :
                       data_.o2B2S1V > 0.9 ? "RICO"  : "OK";
    oled_.drawString(80, 49, mix2, 1);
}

// ─── PÁGINA GM ───────────────────────────────────────────────────────────────
// Odómetro, Voltaje batería, Torque, Presión combustible
void OLEDDisplay::renderGM()
{
    std::lock_guard<std::mutex> lock(data_.mtx);
    drawHeader("DATOS GM", 5);

    // Odómetro — más prominente
    oled_.drawString(2, 11, "ODO:", 1);
    // Truncar a 15 chars
    std::string km = data_.gmKm;
    if (km.length() > 14) km = km.substr(0,14);
    oled_.drawString(28, 10, km, 1);

    oled_.drawLine(0, 21, OLED_WIDTH-1, 21, true);

    // Voltaje batería
    oled_.drawString(2, 23, "BATT:", 1);
    std::string volt = data_.gmVoltage;
    if (volt.length() > 10) volt = volt.substr(0,10);
    oled_.drawString(34, 23, volt, 1);

    // Torque
    oled_.drawString(2, 33, "TORQ:", 1);
    std::string torq = data_.gmTorque;
    if (torq.length() > 10) torq = torq.substr(0,10);
    oled_.drawString(34, 33, torq, 1);

    // Presión combustible
    oled_.drawString(2, 43, "FUEL:", 1);
    std::string fp = data_.gmFuelPress;
    if (fp.length() > 13) fp = fp.substr(0,13);
    oled_.drawString(34, 43, fp, 1);

    // Temperatura catalizador
    oled_.drawString(2, 53, "CAT:", 1);
    std::string cat = data_.gmCatTemp;
    if (cat.length() > 12) cat = cat.substr(0,12);
    oled_.drawString(28, 53, cat, 1);

    drawFooter();
}

// ─── PÁGINA DTC ──────────────────────────────────────────────────────────────
// Lista de códigos de error
void OLEDDisplay::renderDTC()
{
    std::lock_guard<std::mutex> lock(data_.mtx);
    drawHeader("CODIGOS DTC", 6);

    if (data_.dtcCodes.empty() ||
        (data_.dtcCodes.size()==1 && data_.dtcCodes[0]=="NONE")) {
        oled_.drawStringCentered(25, "Sin codigos", 1);
        oled_.drawStringCentered(38, "activos", 1);
        // Checkmark
        oled_.drawString(58, 50, "OK", 2);
        return;
    }

    // MIL activa
    if (data_.milActive) {
        oled_.drawString(2, 11, "MIL: ACTIVA", 1);
        oled_.drawRect(0, 10, 80, 10, true);
    }

    int y = data_.milActive ? 23 : 12;
    int shown = 0;
    for (auto& code : data_.dtcCodes) {
        if (y + 9 > 62) {
            oled_.drawString(2, y, "...", 1);
            break;
        }
        oled_.drawString(2, y, code, 1);
        y += 10;
        shown++;
    }
    drawFooter();
}

// ─── PÁGINA DEBUG ────────────────────────────────────────────────────────────
// Última línea TX y RX en tiempo real
void OLEDDisplay::renderDebug()
{
    std::lock_guard<std::mutex> lock(data_.mtx);
    drawHeader("DEBUG BT", 7);

    // TX — azul en terminal, aquí solo texto
    oled_.drawString(2, 11, "TX:", 1);
    std::string tx = data_.lastTX;
    if (tx.length() > 18) tx = tx.substr(tx.length()-18); // últimos 18 chars
    oled_.drawString(22, 11, tx, 1);

    oled_.drawLine(0, 21, OLED_WIDTH-1, 21, true);

    // RX
    oled_.drawString(2, 23, "RX:", 1);
    std::string rx = data_.lastRX;
    if (rx.length() > 18) rx = rx.substr(rx.length()-18);
    oled_.drawString(22, 23, rx, 1);

    oled_.drawLine(0, 33, OLED_WIDTH-1, 33, true);

    // Protocolo
    oled_.drawString(2, 35, "PROTO:", 1);
    std::string proto = data_.protocol;
    if (proto.length() > 14) proto = proto.substr(0,14);
    oled_.drawString(40, 35, proto, 1);

    // Estado y timestamp
    oled_.drawString(2, 45, data_.connected ? "ONLINE" : "OFFLINE", 1);

    // Barra de actividad
    static int actBar = 0;
    actBar = (actBar + 1) % OLED_WIDTH;
    oled_.drawPixel(actBar, 55, true);
    oled_.drawPixel((actBar+1)%OLED_WIDTH, 55, false);
    oled_.drawPixel((actBar+2)%OLED_WIDTH, 55, false);
}
