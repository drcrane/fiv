project('fiv', 'c')

pixman_dep = dependency('pixman-1')

wl_scanner = dependency('wayland-scanner', native : true)
wl_scanner_prog = find_program(wl_scanner.get_variable('wayland_scanner'))

wl_protocol_dep = dependency('wayland-protocols', version : '>= 1.24')
wl_protocol_dep_dir_base = wl_protocol_dep.get_variable('pkgdatadir')

# xdg output is not actually used yet
generated_protocols = [
  [ 'xdg-decoration', 'v1' ],
  [ 'xdg-output', 'v1' ],
  [ 'xdg-shell', 'stable' ],
  [ 'presentation-time', 'stable' ]
]

generated_protocols_sources = []

foreach proto: generated_protocols
  proto_name = proto[0]
  if proto[1] == 'stable'
    base_file = proto_name
    xml_path = '@0@/stable/@1@/@2@.xml'.format(wl_protocol_dep_dir_base, proto_name, base_file)
  else
    base_file = '@0@-unstable-@1@'.format(proto_name, proto[1])
    xml_path = '@0@/unstable/@1@/@2@.xml'.format(wl_protocol_dep_dir_base, proto_name, base_file)
  endif

  foreach output_type: [ 'client-header', 'private-code' ]
    if output_type == 'client-header'
      output_file = '@0@-client-protocol.h'.format(base_file)
    else
      output_file = '@0@-protocol.c'.format(base_file)
      if wl_scanner.version().version_compare('< 1.14.91')
        output_type = 'code'
      endif
    endif

    target = custom_target(
      '@0@ @1@'.format(base_file, output_type),
      command: [ wl_scanner_prog, output_type, '@INPUT@', '@OUTPUT@' ],
      input: xml_path,
      output: output_file,
    )
    generated_protocols_sources += target
  endforeach
endforeach

wayland_client_dep = dependency('wayland-client', version : '>= 1.20.0')
xkbcommon_dep = dependency('xkbcommon')

libfiv_sources = files(
'src/wlif.c',
'src/lodepng.c',
'src/imageloader.c',
'src/shm.c',
'src/imgterm.c',
'src/widgets.c',
)
libfiv_include_dirs = include_directories('include', 'src')
libfiv_lib = static_library('green', libfiv_sources + generated_protocols_sources, dependencies : [ wayland_client_dep, pixman_dep, xkbcommon_dep ], include_directories : libfiv_include_dirs, install : false)
libfiv_dep = declare_dependency(include_directories : libfiv_include_dirs, link_with : libfiv_lib)


executable('fiv', 'appsrc/fiv.c', dependencies : [ libfiv_dep, wayland_client_dep, pixman_dep, xkbcommon_dep ])

