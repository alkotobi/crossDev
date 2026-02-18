#ifndef APPLICATION_H
#define APPLICATION_H

// Application lifecycle management
class Application {
public:
    static Application& getInstance();
    
    void run();
    void quit();
    
private:
    Application() = default;
    ~Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
};

#endif // APPLICATION_H
