project('segapi', 'c', version: '1.0')

add_project_arguments(
    '-DCOBJMACROS',
    '-DDIRECTINPUT_VERSION=0x0800',
    '-DWIN32_LEAN_AND_MEAN',
    '-D_WIN32_WINNT=_WIN32_WINNT_WIN8',
    '-DMINGW_HAS_SECURE_API=1',
    language: 'c',
)

cc = meson.get_compiler('c')
if cc.get_id() != 'msvc'
    add_project_arguments(
        '-ffunction-sections',
        '-fdata-sections',
        '-Wno-unknown-pragmas',
        '-fdiagnostics-color=never',
        language: 'c',
    )

    add_project_link_arguments(
        '-Wl,--enable-stdcall-fixup',
        '-Wl,--exclude-all-symbols',
        '-Wl,--gc-sections',
        language: 'c',
    )
endif

cc = meson.get_compiler('c')

inc = include_directories('.')
ws2_lib = cc.find_library('ws2_32')

subdir('api')
