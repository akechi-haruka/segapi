api_lib = shared_library(
    'segapi',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    vs_module_defs : 'exports.def',
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
