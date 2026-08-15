#include "elm327.hpp"
#include "gm_commands.hpp"
#include "logger.hpp"
#include "ssd1306.hpp"
#include "oled_display.hpp"
#include "obd2_rpi/config.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>
#include <memory>
#include <sstream>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static std::atomic<bool> g_running(true);
static void sigHandler(int) { g_running = false; }

struct TermRaw {
    struct termios old_tio;
    TermRaw() {
        tcgetattr(STDIN_FILENO, &old_tio);
        struct termios tio = old_tio;
        tio.c_lflag &= ~(ICANON | ECHO);
        tio.c_cc[VMIN]  = 0;
        tio.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &tio);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    }
    ~TermRaw() {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
        fcntl(STDIN_FILENO, F_SETFL, 0);
    }
};

static void printControls() {
    std::cout << "Controles:\n";
    std::cout << "  n/p  = siguiente/anterior pagina OLED\n";
    std::cout << "  a    = toggle auto-rotacion\n";
    std::cout << "  l    = iniciar/detener log CSV\n";
    std::cout << "  g    = forzar lectura GM ahora\n";
    std::cout << "  d    = leer DTCs\n";
    std::cout << "  c    = borrar DTCs (con confirmacion)\n";
    std::cout << "  h    = ayuda\n";
    std::cout << "  q    = salir\n\n";
}

void obdPollThread(ELM327* elm, GMCommands* gm, VehicleData* vd,
                   std::atomic<bool>& running, const OBD2Config& cfg)
{
    int cycle = 0;
    while (running) {
        auto t0 = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(vd->mtx);
            vd->connected = elm->isConnected();
        }
        if (!elm->isConnected()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        int rpm    = elm->getRPM();
        int speed  = elm->getSpeed();
        int temp   = elm->getCoolantTemp();
        int load   = elm->getEngineLoad();
        double thr = elm->getThrottlePosition();
        double map = elm->getIntakePressure();

        double maf=0, fuel=0, baro=0, timing=0;
        double stft1=0, stft2=0, ltft1=0, ltft2=0;
        double o2v1=0, o2v2=0;
        int intakeT=0;
        if (cycle % 3 == 0) {
            maf    = elm->getMAF();
            fuel   = elm->getFuelLevel();
            baro   = elm->getBarometricPressure();
            timing = elm->getTimingAdvance();
            intakeT= elm->getIntakeTemp();
            stft1  = elm->getShortTermTrimBank1();
            stft2  = elm->getShortTermTrimBank2();
            ltft1  = elm->getLongTermTrimBank1();
            ltft2  = elm->getLongTermTrimBank2();
            auto o2b1 = elm->getO2Sensor(1,1);
            auto o2b2 = elm->getO2Sensor(2,1);
            o2v1 = o2b1.voltage;
            o2v2 = o2b2.voltage;
        }

        int gmCycle = cfg.gmCycleInterval;
        int gmSlow  = cfg.gmSlowInterval;
        int dtcCycle = cfg.dtcCycleInterval;

        std::string gmKm="--", gmV="--", gmTq="--", gmCat="--", gmFP="--";
        if (cfg.enableGM && cycle % gmCycle == 0) {
            gmKm  = gm->getKilometers();
            gmV   = gm->getECUVoltage();
            gmTq  = gm->getEngineTorque();
        }
        if (cfg.enableGM && cycle % gmSlow == 0) {
            gmCat = gm->getCatalystTemp();
            gmFP  = gm->getFuelPressure();
        }

        std::vector<std::string> dtcList;
        bool milActive = false;
        if (cycle % dtcCycle == 0) {
            milActive = elm->checkMIL();
            auto dtcs = elm->getDTCs();
            for (auto& d : dtcs) {
                if (d.code != "NONE") dtcList.push_back(d.code);
            }
        }

        {
            std::lock_guard<std::mutex> lock(vd->mtx);
            if (rpm > 0)    vd->rpm = rpm;
            if (speed >= 0) vd->speed = speed;
            if (temp > -90) vd->coolantTemp = temp;
            if (load >= 0)  vd->engineLoad = load;
            if (thr >= 0)   vd->throttle = thr;
            if (map >= 0)   vd->intakePressure = map;

            if (cycle % 3 == 0) {
                if (maf  >= 0)   vd->maf = maf;
                if (fuel >= 0)   vd->fuelLevel = fuel;
                if (baro >= 0)   vd->baroPressure = baro;
                if (timing > -90) vd->timingAdvance = timing;
                if (intakeT > -90) vd->intakeTemp = intakeT;
                if (stft1 > -999) vd->stftBank1 = stft1;
                if (stft2 > -999) vd->stftBank2 = stft2;
                if (ltft1 > -999) vd->ltftBank1 = ltft1;
                if (ltft2 > -999) vd->ltftBank2 = ltft2;
                if (o2v1 >= 0)   vd->o2B1S1V = o2v1;
                if (o2v2 >= 0)   vd->o2B2S1V = o2v2;
            }
            if (cfg.enableGM) {
                if (cycle % gmCycle == 0) {
                    if (gmKm != "--") vd->gmKm = gmKm;
                    if (gmV  != "--") vd->gmVoltage = gmV;
                    if (gmTq != "--") vd->gmTorque = gmTq;
                }
                if (cycle % gmSlow == 0) {
                    if (gmCat != "--") vd->gmCatTemp = gmCat;
                    if (gmFP  != "--") vd->gmFuelPress = gmFP;
                }
            }
            if (cycle % dtcCycle == 0) {
                vd->milActive = milActive;
                if (!dtcList.empty()) vd->dtcCodes = dtcList;
            }
            vd->lastUpdate = std::chrono::steady_clock::now();
        }

        cycle++;
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();
        int wait = cfg.obdPollIntervalMs - (int)dt;
        if (wait > 0) std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    }
}

int main(int argc, char* argv[])
{
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    // Cargar configuracion
    OBD2Config cfg = OBD2Config::fromArgs(argc, argv);

    // Intentar cargar archivo de config si existe
    std::string configPath = "config/obd2_rpi.conf";
    if (argc > 5 && std::string(argv[5]) != "-") {
        configPath = argv[5];
    }

    std::cout << "+--------------------------------------+\n";
    std::cout << "|   OBD2 RPi 2W + SSD1306 SPI v2.0    |\n";
    std::cout << "+--------------------------------------+\n\n";
    cfg.print();
    std::cout << "\n";

    // Inicializar OLED
    SSD1306::Config oledCfg;
    oledCfg.spiDevice   = cfg.spiDevice;
    oledCfg.pinDC       = cfg.pinDC;
    oledCfg.pinReset    = cfg.pinReset;
    oledCfg.spiSpeedHz  = cfg.spiSpeedHz;

    SSD1306 oled(oledCfg);
    if (!oled.begin()) {
        std::cerr << "[ERROR] No se pudo inicializar la pantalla OLED.\n";
        std::cerr << "  sudo raspi-config -> Interface Options -> SPI -> Enable\n";
        std::cerr << "  DC->GPIO" << cfg.pinDC << "  RES->GPIO" << cfg.pinReset << "\n";
        return 1;
    }

    VehicleData vehicleData;

    OLEDDisplay display(oled, vehicleData);
    display.setAutoRotate(cfg.autoRotate, cfg.autoRotateIntervalSec);
    display.setPage(OLEDPage::SPLASH);
    display.start(cfg.displayRefreshMs);

    auto elm = std::make_unique<ELM327>(cfg.btMac, cfg.btChannel);
    auto gm  = std::make_unique<GMCommands>(elm.get());

    std::cout << "[INFO] Conectando al ELM327 (" << cfg.btMac << ")...\n";
    if (!elm->connectBT()) {
        std::cerr << "[ERROR] No se pudo conectar via Bluetooth.\n";
        std::cerr << "  bluetoothctl -> pair " << cfg.btMac << "\n";
        std::cerr << "  Esperando " << cfg.btConnectTimeoutSec << "s y reintentando...\n";
        for (int i=0; i<cfg.btConnectTimeoutSec && g_running; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (!elm->connectBT()) {
            display.stop();
            return 1;
        }
    }

    std::cout << "[OK] Conectado al ELM327\n";
    std::cout << "Protocolo: " << elm->getProtocol() << "\n\n";

    {
        std::lock_guard<std::mutex> lock(vehicleData.mtx);
        vehicleData.connected = true;
        vehicleData.protocol  = elm->getProtocol();
    }
    display.setPage(OLEDPage::MAIN);

    Logger logger;
    std::thread pollThread(obdPollThread, elm.get(), gm.get(),
                           &vehicleData, std::ref(g_running), std::ref(cfg));

    TermRaw termRaw;
    printControls();

    bool logOpen = false;
    static bool autoRot = cfg.autoRotate;

    while (g_running) {
        char key = 0;
        if (read(STDIN_FILENO, &key, 1) == 1) {
            switch (key) {
                case 'n': case 'N':
                    display.nextPage();
                    break;
                case 'p': case 'P':
                    display.prevPage();
                    break;
                case 'a': case 'A':
                    autoRot = !autoRot;
                    display.setAutoRotate(autoRot, cfg.autoRotateIntervalSec);
                    std::cout << "\r[OLED] Auto-rotacion: " << (autoRot?"ON":"OFF") << "    \n";
                    break;
                case 'l': case 'L':
                    if (!logOpen) {
                        if (logger.open()) {
                            logOpen = true;
                            std::cout << "\r[LOG] Iniciado\n";
                        }
                    } else {
                        logger.close();
                        logOpen = false;
                        std::cout << "\r[LOG] Detenido\n";
                    }
                    break;
                case 'g': case 'G': {
                    std::cout << "\r[GM] Leyendo datos GM...\n";
                    std::string km = gm->getKilometers();
                    std::string v  = gm->getECUVoltage();
                    std::string tq = gm->getEngineTorque();
                    std::string ct = gm->getCatalystTemp();
                    std::string fp = gm->getFuelPressure();
                    std::cout << "  Km: " << km << "\n";
                    std::cout << "  V:  " << v  << "\n";
                    std::cout << "  Tq: " << tq << "\n";
                    std::cout << "  Cat:" << ct << "\n";
                    std::cout << "  FP: " << fp << "\n";
                    std::lock_guard<std::mutex> lock(vehicleData.mtx);
                    vehicleData.gmKm        = km;
                    vehicleData.gmVoltage   = v;
                    vehicleData.gmTorque    = tq;
                    vehicleData.gmCatTemp   = ct;
                    vehicleData.gmFuelPress = fp;
                    display.setPage(OLEDPage::GM);
                    break;
                }
                case 'd': case 'D': {
                    auto dtcs = elm->getDTCs();
                    std::cout << "\r[DTC] " << dtcs.size() << " codigos:\n";
                    std::lock_guard<std::mutex> lock(vehicleData.mtx);
                    vehicleData.dtcCodes.clear();
                    for (auto& d : dtcs) {
                        std::cout << "  " << d.code << "\n";
                        if (d.code != "NONE") vehicleData.dtcCodes.push_back(d.code);
                    }
                    display.setPage(OLEDPage::DTC);
                    break;
                }
                case 'c': case 'C': {
                    std::cout << "\r[DTC] Borrar DTCs? (s/N): ";
                    char confirm = 'n';
                    read(STDIN_FILENO, &confirm, 1);
                    if (confirm == 's' || confirm == 'S') {
                        bool ok = elm->clearDTCs();
                        std::cout << (ok ? "  Borrados OK\n" : "  Error\n");
                    }
                    break;
                }
                case 'h': case 'H':
                    printControls();
                    break;
                case 'q': case 'Q':
                    g_running = false;
                    break;
            }
        }

        if (logOpen) {
            std::lock_guard<std::mutex> lock(vehicleData.mtx);
            logger.log("rpm",       vehicleData.rpm);
            logger.log("speed",     vehicleData.speed);
            logger.log("coolant",   vehicleData.coolantTemp);
            logger.log("load",      vehicleData.engineLoad);
            logger.log("throttle",  vehicleData.throttle);
            logger.log("stft_b1",   vehicleData.stftBank1);
            logger.log("ltft_b1",   vehicleData.ltftBank1);
            logger.log("o2_b1s1",   vehicleData.o2B1S1V);
        }

        {
            std::lock_guard<std::mutex> lock(vehicleData.mtx);
            std::cout << "\rRPM:" << std::setw(5) << vehicleData.rpm
                      << " VEL:" << std::setw(3) << vehicleData.speed
                      << " TEMP:" << std::setw(3) << vehicleData.coolantTemp
                      << "C CARG:" << std::setw(2) << vehicleData.engineLoad << "%"
                      << " O2:" << std::fixed << std::setprecision(3) << vehicleData.o2B1S1V << "V"
                      << "  " << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::cout << "\n\n[INFO] Deteniendo...\n";
    display.stop();
    if (pollThread.joinable()) pollThread.join();
    if (logOpen) logger.close();
    elm->disconnect();

    oled.clear();
    oled.drawStringCentered(22, "APAGANDO", 1);
    oled.drawStringCentered(36, "OBD2 RPi", 1);
    oled.display();
    usleep(1500000);
    oled.clear();
    oled.display();

    std::cout << "[OK] Bye!\n";
    return 0;
}
