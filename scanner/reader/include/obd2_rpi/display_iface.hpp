#pragma once
#include <string>

class DisplayInterface {
public:
    virtual ~DisplayInterface() = default;

    virtual bool begin() = 0;
    virtual void end() = 0;

    virtual void clear() = 0;
    virtual void display() = 0;

    virtual void drawPixel(int x, int y, bool on = true) = 0;
    virtual void drawLine(int x0, int y0, int x1, int y1, bool on = true) = 0;
    virtual void drawRect(int x, int y, int w, int h, bool on = true) = 0;
    virtual void fillRect(int x, int y, int w, int h, bool on = true) = 0;

    virtual void drawString(int x, int y, const std::string& s, int scale = 1, bool on = true) = 0;
    virtual void drawStringCentered(int y, const std::string& s, int scale = 1) = 0;
    virtual void drawStringRight(int y, const std::string& s, int scale = 1) = 0;
    virtual void drawProgressBar(int x, int y, int w, int h, float percent, bool on = true) = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;
};
