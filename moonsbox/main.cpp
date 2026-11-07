#include "gameapp.hpp"
#include "soundsystem.hpp"
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_error.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <new>
#include <stdexcept>
#include <exception>
#include <cstdio>
#include <string>

static std::string format_exception(std::string_view prolog, std::string_view exc_type,
                                    std::string_view exc_what)
{
    return std::format(
        "{} Moonsbox will terminate.\n\nTechnical info:\nException: {}\nWhat: {}",
        prolog, exc_type, exc_what
    );
}

static int show_error(const char *title, const char *message)
{
    // Not std::cout or std::println because they might allocate which is dangerous on std::bad_alloc
    printf("======== %s =========\n%s\n", title, message);
    if(!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, nullptr))
        puts("(failed to display a message box)");
    return 1;
}

int main(int, char *[])
{
    SDL_SetAppMetadata("moonsbox", "2.0.0", "io.github.klasterk.moonsbox");

    if(!SDL_Init(SDL_INIT_VIDEO))
        return show_error(
            "SDL failed to init",
            std::format("Could not init SDL.\n\nTechnical info:\n{}", SDL_GetError()).c_str()
        );

    if(!TTF_Init())
        return show_error(
            "SDL_ttf failed to init",
            std::format("Could not init SDL_ttf.\n\nTechnical info:\n{}", SDL_GetError()).c_str()
        );

    try
    {
        sfx::init();
        GameApp app;
        app.run();
    }
    catch(const std::bad_alloc &e)
    {
        return show_error(
            "moonsbox Out of Memory",
            "A memory allocation error has occured. Moonsbox will terminate."
                "\n\nTechnical info:"
                "\nException: std::bad_alloc"
        );
    }
    catch(const std::runtime_error &e)
    {
        return show_error(
            "moonsbox Runtime Error",
            (
                format_exception("A runtime error has occured.", "std::runtime_error", e.what())
                + "\n\nSDL_GetError (should be useless and might be contradictory if it's not an SDL error):\n"
                + SDL_GetError()
            ).c_str()
        );
    }
    catch(const std::logic_error &e)
    {
        return show_error(
            "moonsbox Logic Error",
            format_exception(
                "A logic error has occured. It's a bug, contact devs.",
                "std::logic_error",
                e.what()
            ).c_str()
        );
    }
    catch(const std::exception &e)
    {
        return show_error(
            "moonsbox Unidentified Exception Error",
            format_exception(
                "An unidentified exception was thrown.",
                std::string(typeid(e).name()) + " (mangled)",
                e.what()
            ).c_str()
        );
    }
    catch(...)
    {
        return show_error(
            "moonsbox Unknown Error",
            format_exception(
                "Some non-exception object was thrown. It might be a bug, contact devs.",
                "(catched as ...)",
                "-"
            ).c_str()
        );
    }
    return 0;
}
