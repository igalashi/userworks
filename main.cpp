#include "HistogramRenderer.h"
#include "GaussianGenerator.h"
#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

struct HistogramWindow {
    std::unique_ptr<HistogramRenderer> renderer;
    std::string title;
    double mean, stddev;
    int windowOffset;
    bool initialized;
    
    HistogramWindow(const std::string& t, double m, double s, int offset) 
        : title(t), mean(m), stddev(s), windowOffset(offset), initialized(false) {}
};

int main() {
    try {
        std::vector<HistogramWindow> windows;
        windows.emplace_back("Gaussian μ=0, σ=1", 0.0, 1.0, 0);
        windows.emplace_back("Gaussian μ=2, σ=0.5", 2.0, 0.5, 1);
        windows.emplace_back("Gaussian μ=-1, σ=1.5", -1.0, 1.5, 2);
        
        for (auto& window : windows) {
            try {
                std::cout << "Creating window: " << window.title << std::endl;
                
                window.renderer = std::make_unique<HistogramRenderer>(window.title, 800, 600);
                
                SDL_Window* sdlWindow = SDL_GetWindowFromID(SDL_GetWindowID(SDL_GL_GetCurrentWindow()));
                if (sdlWindow) {
                    SDL_SetWindowPosition(sdlWindow, 100 + window.windowOffset * 250, 100 + window.windowOffset * 50);
                }
                
                GaussianGenerator generator(window.mean, window.stddev, window.windowOffset + 1);
                
                double minX = window.mean - 4 * window.stddev;
                double maxX = window.mean + 4 * window.stddev;
                auto binCounts = generator.generateHistogramData(50, 10000, minX, maxX);
                
                window.renderer->setBins(binCounts, minX, maxX);
                window.renderer->setTitle(window.title);
                window.renderer->setXLabel("Value");
                window.renderer->setYLabel("Frequency");
                
                switch (window.windowOffset % 3) {
                    case 0: window.renderer->setColor(0, 100, 200); break;
                    case 1: window.renderer->setColor(200, 100, 0); break;
                    case 2: window.renderer->setColor(100, 200, 0); break;
                }
                
                window.initialized = true;
                std::cout << "Successfully created window: " << window.title << std::endl;
                
            } catch (const std::exception& e) {
                std::cerr << "Error creating window " << window.title << ": " << e.what() << std::endl;
                window.initialized = false;
            }
        }
        
        bool anyWindowRunning = true;
        while (anyWindowRunning) {
            anyWindowRunning = false;
            
            for (auto& window : windows) {
                if (window.initialized && window.renderer && window.renderer->isRunning()) {
                    anyWindowRunning = true;
                    
                    if (!window.renderer->handleEvents()) {
                        continue;
                    }
                    
                    window.renderer->render();
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        std::cout << "All windows closed, shutting down..." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    SDL_Quit();
    TTF_Quit();
    
    return 0;
}
