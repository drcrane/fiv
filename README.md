# Simple Image Viewer for Wayland

## Requirements

* wayland-dev
* pixman-dev
* freetype-dev

## Building

Build with `make`

```
make
```

Build with `meson`

```
meson setup build --buildtype=debug
meson compile -C build
```

Compiling into `build/` with meson will create a compilation database that
clangd will be able to find.

## References

Cool Websites that might help this...

https://www.aleksandrhovhannisyan.com/blog/crlf-vs-lf-normalizing-line-endings-in-git/

https://bugaevc.gitbooks.io/writing-wayland-clients/content/beyond-the-black-square/xdg-shell.html
https://wayland-book.com/xdg-shell-basics.html
https://wayland.app/protocols/xdg-shell
https://github.com/oasislinux/oasis/blob/master/pkg/st/patch/0001-Port-to-wayland-using-wld.patch
https://nilsbrause.github.io/waylandpp_docs/classwayland_1_1xdg__wm__base__t.html

[Wanna do some client-side decorations?](https://github.com/christianrauch/wayland_window_decoration_example)

