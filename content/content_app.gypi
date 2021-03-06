# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'include_dirs': [
    '..',
  ],
  'dependencies': [
    '../base/base.gyp:base',
    '../base/base.gyp:base_i18n',
    '../crypto/crypto.gyp:crypto',
    '../mojo/mojo_base.gyp:mojo_environment_chromium',
    '../mojo/mojo_edk.gyp:mojo_system_impl',
    '../ui/base/ui_base.gyp:ui_base',
    '../skia/skia.gyp:skia',
    '../ui/gfx/gfx.gyp:gfx',
    '../ui/gfx/gfx.gyp:gfx_geometry',
    'content_common_mojo_bindings.gyp:content_common_mojo_bindings',
  ],
  'sources': [
    'app/android/app_jni_registrar.cc',
    'app/android/app_jni_registrar.h',
    'app/android/child_process_service.cc',
    'app/android/child_process_service.h',
    'app/android/content_main.cc',
    'app/android/content_main.h',
    'app/android/library_loader_hooks.cc',
    'app/content_main.cc',
    'app/content_main_runner.cc',
    'app/mojo/mojo_init.cc',
    'app/mojo/mojo_init.h',
    'public/app/android_library_loader_hooks.h',
    'public/app/content_main.h',
    'public/app/content_main_delegate.cc',
    'public/app/content_main_delegate.h',
    'public/app/content_main_runner.h',
  ],
  'conditions': [
    ['((OS=="linux" and os_posix==1 and use_aura==1) or OS=="android") and use_allocator!="none"', {
      'dependencies': [
        # This is needed by app/content_main_runner.cc
        '../base/allocator/allocator.gyp:allocator',
      ],
    }],
    ['OS=="android"', {
      'sources!': [
        'app/content_main.cc',
      ],
      'dependencies': [
        'content.gyp:content_jni_headers',
        '../skia/skia.gyp:skia',
        '../ui/android/ui_android.gyp:ui_android',
      ],
      'includes': [
        '../build/android/cpufeatures.gypi',
      ],
    }],
    ['OS=="win"', {
      'dependencies': [
        'content_startup_helper_win',
      ],
    }],
    ['OS=="ios"', {
      'sources!': [
        'app/content_main.cc',
        'app/mojo/mojo_init.cc',
        'app/mojo/mojo_init.h',
      ],
    }],
  ],
}
