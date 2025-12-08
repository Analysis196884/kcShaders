#include "gui/app.h"

#include <iostream>
#include <exception>

int main() {
    try {
        // Create and run the application
        gui::App app("kcShaders", 1280, 720);
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return -1;
    }

    return 0;
}
