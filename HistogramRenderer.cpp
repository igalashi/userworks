#include "HistogramRenderer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

HistogramRenderer::HistogramRenderer(const std::string& windowTitle, int width, int height)
    : m_window(nullptr), m_renderer(nullptr), m_font(nullptr)
    , m_dataMinX(0), m_dataMaxX(1)
    , m_histogramColor({0, 100, 200, 255})
    , m_title("Histogram"), m_xlabel("X"), m_ylabel("Y")
    , m_running(true), m_logScale(false), m_dragging(false)
    , m_lastMouseX(0), m_lastMouseY(0)
    , m_viewMinX(0), m_viewMaxX(1), m_viewMinY(0), m_viewMaxY(1)
    , m_marginLeft(80), m_marginRight(20), m_marginTop(40), m_marginBottom(60)
{
    initSDL();
    
    int numDrivers = SDL_GetNumVideoDrivers();
    std::cout << "Available video drivers (" << numDrivers << "): ";
    for (int i = 0; i < numDrivers; ++i) {
        std::cout << SDL_GetVideoDriver(i);
        if (i < numDrivers - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    std::cout << "Current video driver: " << SDL_GetCurrentVideoDriver() << std::endl;
    
    m_window = SDL_CreateWindow(windowTitle.c_str(),
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    if (!m_window) {
        throw std::runtime_error("Failed to create SDL window: " + std::string(SDL_GetError()));
    }
    
    int numRenderDrivers = SDL_GetNumRenderDrivers();
    std::cout << "Available render drivers (" << numRenderDrivers << "): " << std::endl;
    for (int i = 0; i < numRenderDrivers; ++i) {
        SDL_RendererInfo info;
        if (SDL_GetRenderDriverInfo(i, &info) == 0) {
            std::cout << "  " << i << ": " << info.name;
            std::cout << " (flags: ";
            if (info.flags & SDL_RENDERER_SOFTWARE) std::cout << "SOFTWARE ";
            if (info.flags & SDL_RENDERER_ACCELERATED) std::cout << "ACCELERATED ";
            if (info.flags & SDL_RENDERER_PRESENTVSYNC) std::cout << "VSYNC ";
            if (info.flags & SDL_RENDERER_TARGETTEXTURE) std::cout << "TARGETTEXTURE ";
            std::cout << ")" << std::endl;
        }
    }
    
    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);
    
    if (m_renderer) {
        SDL_RendererInfo info;
        if (SDL_GetRendererInfo(m_renderer, &info) == 0) {
            std::cout << "Created accelerated renderer: " << info.name << std::endl;
        }
    } else {
        std::cout << "Hardware accelerated renderer failed: " << SDL_GetError() << std::endl;
        
        rendererFlags = SDL_RENDERER_SOFTWARE;
        m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);
        
        if (m_renderer) {
            SDL_RendererInfo info;
            if (SDL_GetRendererInfo(m_renderer, &info) == 0) {
                std::cout << "Created software renderer: " << info.name << std::endl;
            }
        } else {
            std::cout << "Software renderer also failed: " << SDL_GetError() << std::endl;
            
            m_renderer = SDL_CreateRenderer(m_window, -1, 0);
            if (m_renderer) {
                SDL_RendererInfo info;
                if (SDL_GetRendererInfo(m_renderer, &info) == 0) {
                    std::cout << "Created fallback renderer: " << info.name << std::endl;
                }
            } else {
                throw std::runtime_error("Failed to create any SDL renderer: " + std::string(SDL_GetError()));
            }
        }
    }
    
    m_font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
    if (!m_font) {
        std::cerr << "Warning: Could not load font: " << TTF_GetError() << std::endl;
    }
}

HistogramRenderer::~HistogramRenderer() {
    cleanup();
}

void HistogramRenderer::initSDL() {
    static bool sdlInitialized = false;
    if (!sdlInitialized) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::string error = SDL_GetError();
            std::cerr << "SDL video initialization failed: " << error << std::endl;
            
            if (SDL_SetHint("SDL_VIDEODRIVER", "dummy") == SDL_FALSE) {
                std::cerr << "Failed to set dummy video driver hint" << std::endl;
            }
            
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                throw std::runtime_error("Failed to initialize SDL even with dummy driver: " + std::string(SDL_GetError()));
            }
            
            std::cout << "SDL initialized with dummy video driver for headless environment" << std::endl;
        }
        
        if (TTF_Init() == -1) {
            throw std::runtime_error("Failed to initialize SDL_ttf: " + std::string(TTF_GetError()));
        }
        
        sdlInitialized = true;
    }
}

void HistogramRenderer::cleanup() {
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

void HistogramRenderer::setBins(const std::vector<double>& binCounts, double dataMinX, double dataMaxX) {
    m_binCounts = binCounts;
    m_dataMinX = dataMinX;
    m_dataMaxX = dataMaxX;
    
    m_viewMinX = dataMinX;
    m_viewMaxX = dataMaxX;
    
    calculateYRange();
}

void HistogramRenderer::setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    m_histogramColor = {r, g, b, a};
}

void HistogramRenderer::setTitle(const std::string& title) {
    m_title = title;
}

void HistogramRenderer::setXLabel(const std::string& xlabel) {
    m_xlabel = xlabel;
}

void HistogramRenderer::setYLabel(const std::string& ylabel) {
    m_ylabel = ylabel;
}

void HistogramRenderer::setLogScale(bool logScale) {
    m_logScale = logScale;
    calculateYRange();
}

void HistogramRenderer::calculateYRange() {
    if (m_binCounts.empty()) {
        m_viewMinY = 0;
        m_viewMaxY = 1;
        return;
    }
    
    double maxVal = *std::max_element(m_binCounts.begin(), m_binCounts.end());
    
    if (m_logScale) {
        m_viewMinY = 0.1;
        m_viewMaxY = std::max(1.0, maxVal * 2.0);
    } else {
        m_viewMinY = 0;
        m_viewMaxY = maxVal * 1.1;
    }
}

std::vector<double> HistogramRenderer::calculateYTicks() {
    std::vector<double> ticks;
    
    if (m_logScale) {
        double logMin = std::log10(std::max(0.1, m_viewMinY));
        double logMax = std::log10(m_viewMaxY);
        
        for (int i = static_cast<int>(std::floor(logMin)); i <= static_cast<int>(std::ceil(logMax)); ++i) {
            double val = std::pow(10.0, i);
            if (val >= m_viewMinY && val <= m_viewMaxY) {
                ticks.push_back(val);
            }
        }
    } else {
        double range = m_viewMaxY - m_viewMinY;
        double step = range / 10.0;
        
        double magnitude = std::pow(10.0, std::floor(std::log10(step)));
        double normalized = step / magnitude;
        
        if (normalized <= 1.0) step = magnitude;
        else if (normalized <= 2.0) step = 2.0 * magnitude;
        else if (normalized <= 5.0) step = 5.0 * magnitude;
        else step = 10.0 * magnitude;
        
        double start = std::ceil(m_viewMinY / step) * step;
        for (double val = start; val <= m_viewMaxY; val += step) {
            ticks.push_back(val);
        }
    }
    
    return ticks;
}

std::vector<double> HistogramRenderer::calculateXTicks() {
    std::vector<double> ticks;
    double range = m_viewMaxX - m_viewMinX;
    double step = range / 10.0;
    
    double magnitude = std::pow(10.0, std::floor(std::log10(step)));
    double normalized = step / magnitude;
    
    if (normalized <= 1.0) step = magnitude;
    else if (normalized <= 2.0) step = 2.0 * magnitude;
    else if (normalized <= 5.0) step = 5.0 * magnitude;
    else step = 10.0 * magnitude;
    
    double start = std::ceil(m_viewMinX / step) * step;
    for (double val = start; val <= m_viewMaxX; val += step) {
        ticks.push_back(val);
    }
    
    return ticks;
}

void HistogramRenderer::worldToScreen(double worldX, double worldY, int& screenX, int& screenY) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);
    
    int plotWidth = windowWidth - m_marginLeft - m_marginRight;
    int plotHeight = windowHeight - m_marginTop - m_marginBottom;
    
    screenX = m_marginLeft + static_cast<int>((worldX - m_viewMinX) / (m_viewMaxX - m_viewMinX) * plotWidth);
    
    double yVal = worldY;
    if (m_logScale && worldY > 0) {
        yVal = logTransform(worldY);
        double logMin = logTransform(m_viewMinY);
        double logMax = logTransform(m_viewMaxY);
        screenY = m_marginTop + plotHeight - static_cast<int>((yVal - logMin) / (logMax - logMin) * plotHeight);
    } else {
        screenY = m_marginTop + plotHeight - static_cast<int>((worldY - m_viewMinY) / (m_viewMaxY - m_viewMinY) * plotHeight);
    }
}

void HistogramRenderer::screenToWorld(int screenX, int screenY, double& worldX, double& worldY) {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);
    
    int plotWidth = windowWidth - m_marginLeft - m_marginRight;
    int plotHeight = windowHeight - m_marginTop - m_marginBottom;
    
    worldX = m_viewMinX + (screenX - m_marginLeft) * (m_viewMaxX - m_viewMinX) / plotWidth;
    
    if (m_logScale) {
        double logMin = logTransform(m_viewMinY);
        double logMax = logTransform(m_viewMaxY);
        double logY = logMin + (plotHeight - (screenY - m_marginTop)) * (logMax - logMin) / plotHeight;
        worldY = invLogTransform(logY);
    } else {
        worldY = m_viewMinY + (plotHeight - (screenY - m_marginTop)) * (m_viewMaxY - m_viewMinY) / plotHeight;
    }
}

double HistogramRenderer::logTransform(double value) {
    return std::log10(std::max(0.1, value));
}

double HistogramRenderer::invLogTransform(double value) {
    return std::pow(10.0, value);
}

int HistogramRenderer::getBinAtScreenX(int screenX) {
    double worldX, worldY;
    screenToWorld(screenX, 0, worldX, worldY);
    
    if (m_binCounts.empty()) return -1;
    
    double binWidth = (m_dataMaxX - m_dataMinX) / m_binCounts.size();
    int binIndex = static_cast<int>((worldX - m_dataMinX) / binWidth);
    
    if (binIndex >= 0 && binIndex < static_cast<int>(m_binCounts.size())) {
        return binIndex;
    }
    
    return -1;
}

void HistogramRenderer::renderText(const std::string& text, int x, int y, SDL_Color color, bool vertical) {
    if (!m_font || text.empty()) return;
    
    SDL_Surface* textSurface = TTF_RenderText_Solid(m_font, text.c_str(), color);
    if (!textSurface) return;
    
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(m_renderer, textSurface);
    if (!textTexture) {
        SDL_FreeSurface(textSurface);
        return;
    }
    
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_FreeSurface(textSurface);
    
    SDL_Rect destRect;
    if (vertical) {
        destRect = {x - textWidth/2, y - textHeight/2, textHeight, textWidth};
        SDL_RenderCopyEx(m_renderer, textTexture, nullptr, &destRect, -90.0, nullptr, SDL_FLIP_NONE);
    } else {
        destRect = {x - textWidth/2, y - textHeight/2, textWidth, textHeight};
        SDL_RenderCopy(m_renderer, textTexture, nullptr, &destRect);
    }
    
    SDL_DestroyTexture(textTexture);
}

void HistogramRenderer::drawHistogram() {
    if (m_binCounts.empty()) return;
    
    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);
    
    double binWidth = (m_dataMaxX - m_dataMinX) / m_binCounts.size();
    
    SDL_SetRenderDrawColor(m_renderer, m_histogramColor.r, m_histogramColor.g, m_histogramColor.b, m_histogramColor.a);
    
    for (size_t i = 0; i < m_binCounts.size(); ++i) {
        double binLeft = m_dataMinX + i * binWidth;
        double binRight = m_dataMinX + (i + 1) * binWidth;
        
        if (binRight < m_viewMinX || binLeft > m_viewMaxX) continue;
        
        double binHeight = m_binCounts[i];
        if (binHeight <= 0 && m_logScale) continue;
        
        int x1, y1, x2, y2;
        worldToScreen(binLeft, 0, x1, y1);
        worldToScreen(binRight, binHeight, x2, y2);
        
        SDL_Rect rect = {x1, y2, x2 - x1, y1 - y2};
        SDL_RenderFillRect(m_renderer, &rect);
    }
}

void HistogramRenderer::drawAxes() {
    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);
    
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    
    int x1, y1, x2, y2;
    worldToScreen(m_viewMinX, 0, x1, y1);
    worldToScreen(m_viewMaxX, 0, x2, y2);
    SDL_RenderDrawLine(m_renderer, x1, y1, x2, y1);
    
    worldToScreen(m_viewMinX, m_viewMinY, x1, y1);
    worldToScreen(m_viewMinX, m_viewMaxY, x2, y2);
    SDL_RenderDrawLine(m_renderer, x1, y1, x1, y2);
}

void HistogramRenderer::drawGrid() {
    SDL_SetRenderDrawColor(m_renderer, 128, 128, 128, 255);
    
    auto yTicks = calculateYTicks();
    for (double tick : yTicks) {
        int x1, y1, x2, y2;
        worldToScreen(m_viewMinX, tick, x1, y1);
        worldToScreen(m_viewMaxX, tick, x2, y2);
        
        for (int x = x1; x <= x2; x += 4) {
            SDL_RenderDrawPoint(m_renderer, x, y1);
        }
    }
    
    auto xTicks = calculateXTicks();
    for (double tick : xTicks) {
        int x1, y1, x2, y2;
        worldToScreen(tick, m_viewMinY, x1, y1);
        worldToScreen(tick, m_viewMaxY, x2, y2);
        
        for (int y = y2; y <= y1; y += 4) {
            SDL_RenderDrawPoint(m_renderer, x1, y);
        }
    }
}

void HistogramRenderer::drawLabels() {
    SDL_Color black = {0, 0, 0, 255};
    
    renderText(m_title, 0, 0, black);
    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);
    renderText(m_title, windowWidth/2, 20, black);
    
    renderText(m_xlabel, windowWidth/2, windowHeight - 20, black);
    
    renderText(m_ylabel, 20, windowHeight/2, black, true);
    
    auto yTicks = calculateYTicks();
    for (double tick : yTicks) {
        int x, y;
        worldToScreen(m_viewMinX, tick, x, y);
        
        std::ostringstream oss;
        if (m_logScale) {
            oss << std::scientific << std::setprecision(0) << tick;
        } else {
            oss << std::fixed << std::setprecision(1) << tick;
        }
        
        renderText(oss.str(), m_marginLeft - 10, y, black);
    }
    
    auto xTicks = calculateXTicks();
    for (double tick : xTicks) {
        int x, y;
        worldToScreen(tick, 0, x, y);
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << tick;
        
        renderText(oss.str(), x, windowHeight - m_marginBottom + 15, black);
    }
}

void HistogramRenderer::drawMouseOverInfo(int mouseX, int mouseY) {
    int binIndex = getBinAtScreenX(mouseX);
    if (binIndex >= 0 && binIndex < static_cast<int>(m_binCounts.size())) {
        double binHeight = m_binCounts[binIndex];
        
        std::ostringstream oss;
        oss << "Bin " << binIndex << ": " << std::fixed << std::setprecision(2) << binHeight;
        
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 150);
        SDL_Rect bgRect = {mouseX + 10, mouseY - 20, 120, 20};
        SDL_RenderFillRect(m_renderer, &bgRect);
        
        SDL_Color black = {0, 0, 0, 255};
        renderText(oss.str(), mouseX + 70, mouseY - 10, black);
    }
}

void HistogramRenderer::zoom(double factor, int mouseX, int mouseY) {
    double worldX, worldY;
    screenToWorld(mouseX, mouseY, worldX, worldY);
    
    double rangeX = m_viewMaxX - m_viewMinX;
    double rangeY = m_viewMaxY - m_viewMinY;
    
    double newRangeX = rangeX / factor;
    double newRangeY = rangeY / factor;
    
    m_viewMinX = worldX - newRangeX * (worldX - m_viewMinX) / rangeX;
    m_viewMaxX = worldX + newRangeX * (m_viewMaxX - worldX) / rangeX;
    
    if (m_logScale) {
        double logWorldY = logTransform(worldY);
        double logMinY = logTransform(m_viewMinY);
        double logMaxY = logTransform(m_viewMaxY);
        double logRange = logMaxY - logMinY;
        double newLogRange = logRange / factor;
        
        double newLogMinY = logWorldY - newLogRange * (logWorldY - logMinY) / logRange;
        double newLogMaxY = logWorldY + newLogRange * (logMaxY - logWorldY) / logRange;
        
        m_viewMinY = invLogTransform(newLogMinY);
        m_viewMaxY = invLogTransform(newLogMaxY);
    } else {
        m_viewMinY = worldY - newRangeY * (worldY - m_viewMinY) / rangeY;
        m_viewMaxY = worldY + newRangeY * (m_viewMaxY - worldY) / rangeY;
    }
    
    m_viewMinY = std::max(0.0, m_viewMinY);
}

void HistogramRenderer::pan(int deltaX, int deltaY) {
    double worldDeltaX = deltaX * (m_viewMaxX - m_viewMinX) / 800.0;
    double worldDeltaY = deltaY * (m_viewMaxY - m_viewMinY) / 600.0;
    
    m_viewMinX -= worldDeltaX;
    m_viewMaxX -= worldDeltaX;
    
    if (m_logScale) {
        double logMinY = logTransform(m_viewMinY);
        double logMaxY = logTransform(m_viewMaxY);
        double logDeltaY = worldDeltaY * (logMaxY - logMinY) / (m_viewMaxY - m_viewMinY);
        
        logMinY += logDeltaY;
        logMaxY += logDeltaY;
        
        m_viewMinY = invLogTransform(logMinY);
        m_viewMaxY = invLogTransform(logMaxY);
    } else {
        m_viewMinY += worldDeltaY;
        m_viewMaxY += worldDeltaY;
    }
    
    m_viewMinY = std::max(0.0, m_viewMinY);
}

bool HistogramRenderer::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_running = false;
                return false;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    m_running = false;
                    return false;
                }
                break;
                
            case SDL_MOUSEWHEEL:
                {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    double factor = (event.wheel.y > 0) ? 1.2 : 0.8;
                    zoom(factor, mouseX, mouseY);
                }
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (event.button.clicks == 2 && event.button.x < m_marginLeft) {
                        setLogScale(!m_logScale);
                    } else {
                        m_dragging = true;
                        m_lastMouseX = event.button.x;
                        m_lastMouseY = event.button.y;
                    }
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_dragging = false;
                }
                break;
                
            case SDL_MOUSEMOTION:
                if (m_dragging) {
                    int deltaX = event.motion.x - m_lastMouseX;
                    int deltaY = event.motion.y - m_lastMouseY;
                    pan(deltaX, deltaY);
                    m_lastMouseX = event.motion.x;
                    m_lastMouseY = event.motion.y;
                }
                break;
        }
    }
    return true;
}

void HistogramRenderer::render() {
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderClear(m_renderer);
    
    drawGrid();
    drawHistogram();
    drawAxes();
    drawLabels();
    
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (mouseX > m_marginLeft && mouseX < 800 - m_marginRight &&
        mouseY > m_marginTop && mouseY < 600 - m_marginBottom) {
        drawMouseOverInfo(mouseX, mouseY);
    }
    
    SDL_RenderPresent(m_renderer);
}
