Vermutlich arg drüber, für jede kleine Auffälligkeit ein "Issue" zu erzeugen. Daher der Vorschlag in dieser Markdown-Datei kurz zu notieren, wenn einem etwas bei Basteleien an anderen Dingen etwas auffällt. Da man dann vermutlich eh gerade im Repo arbeitet, sollte es einfach sein soetwas dann schnell hier reinzukritzeln.

# Incubator.fuse

...

# fetch

- [ ] https://www.shadertoy.com/view/XtlSD7 ... bringt 'fetch' aus dem Tritt wegen der Sound-Section.
- [ ] to_float2(-1.0f) ... nicht erkannt
- [ ] Floats in defines werden nicht ersetzt; siehe [#6](https://github.com/nmbr73/Fetch-n-Fuse/issues/6)
- [x] <del>.g wird durch .yg oder so ersetzt</del>
- [ ] noch offen: "return to_float4(color.r/255.0f,color.g/255.0f,color.b/255.0f,1.0f);"
- [ ] Ceil() für OpenCL
- [ ] sign_f Generic: sign_f neccessary for all sign()

# fuse

- [ ] CONNECT-Makros bleiben drin, falls bspw. Kommetare in der Zeile sind (sieht man, wenn man SimpleDCTL generiert) ... [#18](https://github.com/nmbr73/Fetch-n-Fuse/issues/18)
- [ ] sign_f wird nicht übernommen (sign_f wird für die weiteren Dimensionen benötigt !)