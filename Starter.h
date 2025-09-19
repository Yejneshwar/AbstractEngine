// OpenGL Starter.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <AbstractApplication.h>

namespace GUI {

    class TestGUI : public AbstractApplication {
    private:
    public:
        TestGUI(const ApplicationSpecification& spec, void* nativeWindow);
        ~TestGUI();
    };

    TestGUI* CreateApplication(ApplicationCommandLineArgs args = ApplicationCommandLineArgs(), void* nativeWindow = nullptr);
}
