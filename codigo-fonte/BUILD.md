# Compilando o StickCombo.asi

Plugin de 32 bits, sem SDK, usando só LLVM (clang + lld + llvm-dlltool).

```
llvm-dlltool -m i386 -k -d k32.def -l kernel32.lib
clang --target=i686-pc-windows-msvc -O2 -c -ffreestanding -fno-stack-protector -fno-builtin StickCombo.c -o StickCombo.obj
lld-link /dll /nodefaultlib /machine:x86 /entry:DllMain@12 /out:StickCombo.asi StickCombo.obj kernel32.lib
```

Coloque o `StickCombo.asi` em `scripts\` na pasta do jogo.

Os endereços (`0x0234E80C`, `0x0234E7F4`, `0x00E9A5BC`, `0x00EBE860`, RVA `0x1102C8` do DFEngine.dll) valem para o SR2_pc.exe do Juiced Patch testado. Em outra versão o plugin confere o ponteiro antes de trocar e, se não bater, não faz nada.
