#pragma once

extern GrEngine::Engine* GrEngine::Init();

int main(int argc, char** argv)
{
    Logger::Out("Starting the engine", OutputColor::Gray);
    auto app = GrEngine::Init();
    app->Run();
    delete app;
    return 0;
}
