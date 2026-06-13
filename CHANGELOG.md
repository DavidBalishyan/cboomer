# v1.0.0

Zoomer application for Linux with *pan, zoom, flashlight, mirror, and live window tracking*. **C11** rewrite using **X11** and **OpenGL**.

- Screenshot capture via **XGetImage** / **XShmGetImage**
- OpenGL rendering with smooth pan/zoom inertia
- Flashlight mode *(f)* with adjustable radius
- Mirror mode *(m)*
- Config file at *$HOME/.config/cboomer/config*
- Build variants: *make dev, make live, make mitshm, make select*
- CLI flags: *--delay, --help, --new-config, --config, --version, --windowed*

Commit: [822a079](https://github.com/DavidBalishyan/cboomer/commit/822a0795fc8e2a80612b631c9d2543442f431c37)

# v1.1.0

What's new
- **Shader effects!** Press *t* to cycle through 5 modes: Normal, Invert (photographic negative), CRT (chromatic aberration + scanlines + vignette), Grayscale, and Edge (Sobel operator).
- **Shaders moved to src/shaders** - all *.glsl* files live in their own directory, embedded as C strings at build time. 
- **README overhaul** - added demo video link, full shader documentation explaining every effect, and improved build variant table.
- **Build the binary to CWD** - *cboomer* now lands in the project root instead of *build/*.

Commit: [084b84f](https://github.com/DavidBalishyan/cboomer/commit/084b84fc45f76d57353ad1b7cdf954b035bdc18a)
