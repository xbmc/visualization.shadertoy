# JSON file for user defined shader lists

The file `presets.json` is used to define custom shaders.
A custom path to the shader files (GLSL) can be declared in the add-on settings.

Contents of `presets.json` should be:
1. Name of preset. Either hardcoded string or defined in `resources/language/.../strings.po`
2. GLSL shader to use. If no custom path was declared, the default `resources/` will be used.
   Set custom path to './' to use shaders from the same path as your `presets.json` file.
3. Channel value 0. Defines the texture to use (e.g. PNG file). Can also be set to "audio"
   to pass related stream data within texture. The PNG and GLSL files should be located
   in the same directory.
4. Channel value 1 definition.
5. Channel value 2 definition.
6. Channel value 3 definition.
7. *[Optional]* append string `"gl_only"` to make this preset only available
   to OpenGL.

Example:
-------------------
~~~javascript
{ "presets":[

/* Default way used by the addon itself */
[ "The Disco Tunnel by poljere",  "discotunnel.frag.glsl",      "tex02.png", "tex15.png", "audio", "" ],

/* Description defined in strings.po */
[ 30100,                          "audioeclipse.frag.glsl",     "audio", "", "", "" ],

/* Define custom file paths */
[ "My own one",                   "/path/to/glsl/my.frag.glsl", "audio", "/path/to/pngs/my_image.png", "", "" ],

/* Preset only available to OpenGL (see 7)  */
[ "Polar Beats by sauj123",       "polarbeats.frag.glsl",       "audio", "", "", "", "gl_only" ],

/* GLSL and PNG files are in the same folder as presets.json */
[ "Another one",                  "./another.frag.glsl",        "./my_01.png", "./my_02.png", "audio", "" ],

/* Select texture image by file name, use default `resources/` dir */
[ "Fractal Land by Kali",         "fractalland.frag.glsl",      "tex02.png", "tex15.png", "audio", "" ]
]}
~~~
