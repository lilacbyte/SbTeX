# SbTeX

```sh
./sbtex -i 'hello stratish' -o hello.png
./sbtex -i 'hello' --columns 8 > hello.png
./sbtex --help
```

fonts load automatically. to use another font folder:

```sh
./sbtex -i 'hello' --fonts-dir /path/to/fonts -f sans -o hello.png
```
