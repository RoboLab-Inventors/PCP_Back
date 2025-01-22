{
  "targets": [
    {
      "target_name": "myAddon",
      "sources": [ "addon.cpp" ],
      "include_dirs": [
        "<!(node -p \"require('node-addon-api').include\")"

      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "libraries": [
          "<(module_root_dir)/hidapi/hidapi.lib"
      ],
      "include_dirs": [
        "<(module_root_dir)/hidapi/include"
      ],
      "defines": [
        "NODE_ADDON_API_DISABLE_CPP_EXCEPTIONS"
      ]
    }
  ]
}
