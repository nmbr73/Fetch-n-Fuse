Vermutlich arg drüber, für jede kleine Auffälligkeit ein "Issue" zu erzeugen. Daher der Vorschlag in dieser Markdown-Datei kurz zu notieren, wenn einem etwas bei Basteleien an anderen Dingen etwas auffällt. Da man dann vermutlich eh gerade im Repo arbeitet, sollte es einfach sein soetwas dann schnell hier reinzukritzeln.

# Incubator.fuse

...

# fetch

- [x] <del>https://www.shadertoy.com/view/XtlSD7 ... bringt 'fetch' aus dem Tritt wegen der Sound-Section. -> Sound-Section Code wird einfach übernommen
- [ ] to_float2(-1.0f) ... nicht erkannt
- [ ] Floats in defines werden nicht ersetzt; siehe [#6](https://github.com/nmbr73/Fetch-n-Fuse/issues/6)
- [x] <del>.g wird durch .yg oder so ersetzt</del>
- [x] <del>noch offen: "return to_float4(color.r/255.0f,color.g/255.0f,color.b/255.0f,1.0f);"
- [x] Ceil() für OpenCL
- [x] <del>sign_f Generic: sign_f neccessary for all sign()
- [ ] mix ohne Unterstrich:  "return _mix(mix(_mix( hash(n..."
- [ ] Es gibt Fuses mit namenlosen Codeblöcken - das führt dann im GLSL code zu einem "broken marker" (Beispiel: 4tjGW1) @nmbr73


# fuse

- [ ] CONNECT-Makros bleiben drin, falls bspw. Kommetare in der Zeile sind (sieht man, wenn man SimpleDCTL generiert) ... [#18](https://github.com/nmbr73/Fetch-n-Fuse/issues/18)
- [x] <del>sign_f wird nicht übernommen (sign_f wird für die weiteren Dimensionen benötigt !)