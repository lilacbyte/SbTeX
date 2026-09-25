# SbTeX

```sh
./sbtex -i 'hello stratish' -o hello.png
./sbtex -i 'hello' --columns 8 > hello.png
./sbtex -i 'hello' -bg '#222' -color '#ff88cc' -o pink.png
./sbtex --help
```

`-bg` sets the background and `-color` sets the glyph colour. Both accept
`#rgb` or `#rrggbb`. Quote the hex codes so the shell doesn't treat `#` as a comment.
Defaults are white background and black glyphs.

fonts load automatically. to use another font folder:

```sh
./sbtex -i 'hello' --fonts-dir /path/to/fonts -f sans -o hello.png
```
