#include "SDL.h"
#include "gameapp.hpp"
#include "soundsystem.hpp"
#include <SDL2pp/SDL.hh>
#include <SDL2pp/SDL2pp.hh>
#include <SDL2pp/SDLTTF.hh>
#include <stdexcept>

static std::string _format_error(std::string_view prolog, std::string_view exc_type, 
                                 std::string_view exc_what)
{
    return std::format(
        "{} Moonsbox will terminate.\n\nTechnical info:\nException: {}\nWhat: {}",
        prolog, exc_type, exc_what
    );
}

int main()
{
    try
    {
        SDL2pp::SDL sdl(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);
        SDL2pp::SDLTTF ttf;
        sfx::init();

        GameApp app;
        app.run();
    }
    catch(const SDL2pp::Exception &e)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, 
            "moonsbox SDL2 Error", 
            _format_error("An SDL2 error has occured.", "SDL2pp::Exception", e.what()).c_str(), 
            nullptr
        );
        return 1;
    }
    catch(const std::bad_alloc &e)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, 
            "moonsbox Out of Memory", 
            "A memory allocation error has occured. Moonsbox will terminate."
            "\n\nTechnical info:"
            "\nException: std::bad_alloc",
            nullptr
        );
        return 1;
    }
    catch(const std::runtime_error &e)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, 
            "moonsbox Runtime Error", 
            _format_error("A runtime error has occured.", "std::runtime_error", e.what()).c_str(), 
            nullptr
        );
        return 1;
    }
    catch(const std::logic_error &e)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, 
            "moonsbox Logic Error", 
            _format_error(
                "A logic error has occured. It's a bug, contact devs.", 
                "std::logic_error", 
                e.what()
            ).c_str(), 
            nullptr
        );
        return 1;
    }
    catch(const std::exception &e)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, 
            "moonsbox Unidentified Exception Error", 
            _format_error(
                "An unidentified exception was thrown.", 
                std::string(typeid(e).name()) + " (mangled)",
                e.what()
            ).c_str(), 
            nullptr
        );
        return 1;
    }
    catch(...)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, 
            "moonsbox Unknown Error", 
            _format_error(
                "Some non-exception object was thrown. It may be a bug, contact devs.", 
                "(catched as ...)",
                "-"
            ).c_str(), 
            nullptr
        );
        return 1;
    }
    return 0;
}
