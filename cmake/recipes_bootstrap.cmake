find_program(CONAN_COMMAND "conan" REQUIRED
    HINTS
        "${CMAKE_SOURCE_DIR}/.venv/bin"
        "/opt/homebrew/bin"
)

file(GLOB_RECURSE LOCAL_RECIPES "${CMAKE_SOURCE_DIR}/recipes/*/conanfile.py")
list(REMOVE_ITEM LOCAL_RECIPES "${CMAKE_SOURCE_DIR}/recipes/go/conanfile.py")
foreach(RECIPE ${LOCAL_RECIPES})
    get_filename_component(RECIPE_DIR ${RECIPE} DIRECTORY)
    execute_process(
        COMMAND ${CONAN_COMMAND} export ${RECIPE_DIR}
    )
endforeach()

# FIXME(ygurov): export all versions declared on recipies_bootstrap call
execute_process(
    COMMAND ${CONAN_COMMAND} export "${CMAKE_SOURCE_DIR}/recipes/go" --version 1.26.0
)
execute_process(
    COMMAND ${CONAN_COMMAND} export "${CMAKE_SOURCE_DIR}/recipes/go" --version 1.23.12
)

execute_process(
    COMMAND ${CONAN_COMMAND} remote add amnezia "https://artifactory.amnezia.org/artifactory/api/conan/client-prebuilts" --force
)
