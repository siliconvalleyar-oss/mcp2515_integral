#pragma once
// =============================================================================
// OLEDDisplay — Gestiona las páginas de información OBD2 en la pantalla SSD1306
//
// Páginas disponibles (rotación automática o manual):
//   PAGE_MAIN    : RPM grande + Velocidad + Temp
//   PAGE_ENGINE  : Carga motor, MAF, Avance, Presión admisión
//   PAGE_FUEL    : STFT B1/B2, LTFT B1/B2, Nivel combustible
//   PAGE_O2      : Voltaje O2 B1S1 / B2S1 + trim
//   PAGE_GM      : Odómetro GM, Voltaje batería, Torque
//   PAGE_DTC     : Códigos de error activos
//   PAGE_DEBUG   : Última línea TX/RX BT (debug raw)
//   PAGE_SPLASH  : Pantalla de inicio / sin conexión
// =============================================================================

#include "ssd1306.hpp"
#include "elm327.hpp"
#include "gm_commands.hpp"
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

enum class OLEDPage {
    SPLASH = 0,
    MAIN,
    ENGINE,
    FUEL,
    O2,
    GM,
    DTC,
    DEBUG,
    COUNT
};

// Datos compartidos entre el hilo OBD y el hilo de display
struct VehicleData {
    std::mutex mtx;

    // Estado conexión
    bool connected   = false;
    std::string protocol;
    std::string lastTX;
    std::string lastRX;

    // OBD-II estándar
    int    rpm          = 0;
    int    speed        = 0;
    int    coolantTemp  = 0;
    int    engineLoad   = 0;
    double throttle     = 0.0;
    double intakePressure = 0.0;
    int    intakeTemp   = 0;
    double timingAdvance  = 0.0;
    double maf          = 0.0;
    double fuelLevel    = 0.0;
    double baroPressure = 0.0;

    // Fuel Trim
    double stftBank1   = 0.0;
    double stftBank2   = 0.0;
    double ltftBank1   = 0.0;
    double ltftBank2   = 0.0;

    // O2
    double o2B1S1V     = 0.0;
    double o2B2S1V     = 0.0;
    double o2B1S1T     = 0.0;
    double o2B2S1T     = 0.0;

    // GM específico
    std::string gmKm        = "--";
    std::string gmVoltage   = "--";
    std::string gmTorque    = "--";
    std::string gmCatTemp   = "--";
    std::string gmFuelPress = "--";

    // DTCs
    std::vector<std::string> dtcCodes;
    bool milActive = false;

    // Timestamp última actualización
    std::chrono::steady_clock::time_point lastUpdate;
};

class OLEDDisplay {
public:
    OLEDDisplay(SSD1306& oled, VehicleData& data);
    ~OLEDDisplay();

    // Arrancar/detener el hilo de refresco
    void start(int refreshMs = 500);
    void stop();

    // Cambiar página manualmente
    void nextPage();
    void prevPage();
    void setPage(OLEDPage page);
    OLEDPage currentPage() const { return page_; }

    // Activar/desactivar rotación automática de páginas
    void setAutoRotate(bool enabled, int intervalSec = 5);

    // Dibuja la página actual al framebuffer y hace display()
    void render();

private:
    SSD1306&      oled_;
    VehicleData&  data_;
    OLEDPage      page_     = OLEDPage::SPLASH;
    bool          running_  = false;
    bool          autoRotate_ = true;
    int           autoIntervalSec_ = 5;
    int           refreshMs_  = 500;

    std::thread   thread_;
    std::chrono::steady_clock::time_point lastPageChange_;

    void threadLoop();

    // Renders por página
    void renderSplash();
    void renderMain();
    void renderEngine();
    void renderFuel();
    void renderO2();
    void renderGM();
    void renderDTC();
    void renderDebug();

    // Helpers comunes
    void drawHeader(const std::string& title, int pageNum);
    void drawFooter();
    void drawConnectionStatus();
    std::string fmtDouble(double v, int decimals = 1);
    std::string fmtInt(int v);
    bool dataFresh() const;  // true si última actualización < 5s
};
