#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <memory>

class HistogramRenderer {
public:
    HistogramRenderer(const std::string& windowTitle, int width = 800, int height = 600);
    ~HistogramRenderer();

    void setBins(const std::vector<double>& binCounts, double dataMinX, double dataMaxX);
    void setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void setTitle(const std::string& title);
    void setXLabel(const std::string& xlabel);
    void setYLabel(const std::string& ylabel);
    
    void setLogScale(bool logScale);
    bool isLogScale() const { return m_logScale; }
    
    void render();
    bool handleEvents();
    bool isRunning() const { return m_running; }
    
    void zoom(double factor, int mouseX, int mouseY);
    void pan(int deltaX, int deltaY);

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    
    std::vector<double> m_binCounts;
    double m_dataMinX, m_dataMaxX;
    
    SDL_Color m_histogramColor;
    std::string m_title, m_xlabel, m_ylabel;
    
    bool m_running;
    bool m_logScale;
    bool m_dragging;
    int m_lastMouseX, m_lastMouseY;
    
    double m_viewMinX, m_viewMaxX;
    double m_viewMinY, m_viewMaxY;
    
    int m_marginLeft, m_marginRight, m_marginTop, m_marginBottom;
    
    void initSDL();
    void cleanup();
    
    void drawHistogram();
    void drawAxes();
    void drawGrid();
    void drawLabels();
    void drawMouseOverInfo(int mouseX, int mouseY);
    
    void calculateYRange();
    std::vector<double> calculateYTicks();
    std::vector<double> calculateXTicks();
    
    void worldToScreen(double worldX, double worldY, int& screenX, int& screenY);
    void screenToWorld(int screenX, int screenY, double& worldX, double& worldY);
    
    int getBinAtScreenX(int screenX);
    void renderText(const std::string& text, int x, int y, SDL_Color color, bool vertical = false);
    
    double logTransform(double value);
    double invLogTransform(double value);
};
