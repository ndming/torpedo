include(FetchContent)

# For Window management
FetchContent_Declare(
    glfw GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4 GIT_SHALLOW ON)

# For scene composition
FetchContent_Declare(
    entt GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.15.0 GIT_SHALLOW ON)

# For debug logging
FetchContent_Declare(
    plog GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
    GIT_TAG 495a54de43e21aaf74b7f2704297aeeae16da421)

FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(entt)
FetchContent_MakeAvailable(plog)