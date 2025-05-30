api_lib = static_library(
    'segapi',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    dependencies : [
        ws2_lib
    ],
    sources : [
        'api.c',
        'api.h',
        'config.c',
        'config.h',
        'dprintf.c',
        'dprintf.h',
        'main.c',
        'util.c',
        'util.h',
    ],
)

api_dep = declare_dependency(
    link_with : api_lib,
    include_directories : inc,
)