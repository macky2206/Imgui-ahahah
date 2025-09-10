## Known Issues

- **Keyboard Input Not Detected:** ImGui does not currently read keyboard input.
- **Surface View Support:** Uncertainty regarding Surface View integration.

```smali
    const-string v0, "native-lib"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
```