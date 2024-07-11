project(
  'tts',
  'c',
  default_options: [
    'c_std=c89'
  ]
)

tts = executable(
  meson.project_name(),
  'main.c',
  install: true
)

if meson.is_subproject()
  meson.override_find_program('tts', tts)
endif
