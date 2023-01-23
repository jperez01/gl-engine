add_library(glad STATIC)
target_sources(glad PRIVATE
    glad/glad.c)

target_include_directories(glad PUBLIC ${PROJECT_SOURCE_DIR}/include)