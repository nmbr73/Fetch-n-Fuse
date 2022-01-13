These are the 96x96px thumbnails for all the files under [`Assets//media/a/`](../a/). They are used in particular by the `Fuses/Keyboard_nmbr73.fuse` for the preview image.


To create such a thumbnail: Opening the dialog to choose an input on a shadertoy you can right click the previev to open the image in new tab to then save it. But entering the URL directly does for whatever reason not work. Before deriving the base64 all the JPEGs must be converted to PNGs! So all in all this procedure is really a pain.


You should end up with the following files:

```
08b42b43ae9d3c0605da11d0eac86618ea888e62cdd9518ee8b9097488b31560.png
0a40562379b63dfb89227e6d172f39fdce9022cba76623f1054a2c83d6c0ba5d.png
0c7bf5fe9462d5bffbd11126e82908e39be3ce56220d900f633d58fb432e56f5.png
10eb4fe0ac8a7dc348a2cc282ca5df1759ab8bf680117e4047728100969e7b43.jpg
1f7dca9c22f324751f2a5a59c9b181dfe3b5564a04b724c657732d0bf09c99db.jpg
3083c722c0c738cad0f468383167a0d246f91af2bfa373e9c5c094fb8c8413e0.png
3871e838723dd6b166e490664eead8ec60aedd6b8d95bc8e2fe3f882f0fd90f0.jpg
52d2a8f514c4fd2d9866587f4d7b2a5bfa1a11a0e772077d7682deb8b3b517e5.jpg
79520a3d3a0f4d3caa440802ef4362e99d54e12b1392973e4ea321840970a88a.jpg
85a6d68622b36995ccb98a89bbb119edf167c914660e4450d313de049320005c.png
8979352a182bde7c3c651ba2b2f4e0615de819585cc37b7175bcefbca15a6683.jpg
8de3a3924cb95bd0e95a443fff0326c869f9d4979cd1d5b6e94e2a01f5be53e9.jpg
92d7758c402f0927011ca8d0a7e40251439fba3a1dac26f5b8b62026323501aa.jpg
95b90082f799f48677b4f206d856ad572f1d178c676269eac6347631d4447258.jpg
ad56fba948dfba9ae698198c109e71f118a54d209c0ea50d77ea546abad89c57.png
bd6464771e47eed832c5eb2cd85cdc0bfc697786b903bfd30f890f9d4fc36657.jpg
cb49c003b454385aa9975733aff4571c62182ccdda480aaba9a8d250014f00ec.png
cbcbb5a6cfb55c36f8f021fbb0e3f69ac96339a39fa85cd96f2017a2192821b5.png
cd4c518bc6ef165c39d4405b347b51ba40f8d7a065ab0e8d2e4f422cbc1e8a43.jpg
e6e5631ce1237ae4c05b3563eda686400a401df4548d0f9fad40ecac1659c46c.jpg
f735bee5b64ef98879dc618b016ecf7939a5756040c2cde21ccb15e69a6e1cfb.png
fb918796edc3d2221218db0811e240e72e340350008338b0c07a52bd353666a6.jpg
```

Then create a PNG for every JPEG:

```
10eb4fe0ac8a7dc348a2cc282ca5df1759ab8bf680117e4047728100969e7b43.png
1f7dca9c22f324751f2a5a59c9b181dfe3b5564a04b724c657732d0bf09c99db.png
3871e838723dd6b166e490664eead8ec60aedd6b8d95bc8e2fe3f882f0fd90f0.png
52d2a8f514c4fd2d9866587f4d7b2a5bfa1a11a0e772077d7682deb8b3b517e5.png
79520a3d3a0f4d3caa440802ef4362e99d54e12b1392973e4ea321840970a88a.png
8979352a182bde7c3c651ba2b2f4e0615de819585cc37b7175bcefbca15a6683.png
8de3a3924cb95bd0e95a443fff0326c869f9d4979cd1d5b6e94e2a01f5be53e9.png
92d7758c402f0927011ca8d0a7e40251439fba3a1dac26f5b8b62026323501aa.png
95b90082f799f48677b4f206d856ad572f1d178c676269eac6347631d4447258.png
bd6464771e47eed832c5eb2cd85cdc0bfc697786b903bfd30f890f9d4fc36657.png
cd4c518bc6ef165c39d4405b347b51ba40f8d7a065ab0e8d2e4f422cbc1e8a43.png
e6e5631ce1237ae4c05b3563eda686400a401df4548d0f9fad40ecac1659c46c.png
fb918796edc3d2221218db0811e240e72e340350008338b0c07a52bd353666a6.png
```

And finaly base64 encode every PNG:

```
08b42b43ae9d3c0605da11d0eac86618ea888e62cdd9518ee8b9097488b31560.base64
0a40562379b63dfb89227e6d172f39fdce9022cba76623f1054a2c83d6c0ba5d.base64
0c7bf5fe9462d5bffbd11126e82908e39be3ce56220d900f633d58fb432e56f5.base64
10eb4fe0ac8a7dc348a2cc282ca5df1759ab8bf680117e4047728100969e7b43.base64
1f7dca9c22f324751f2a5a59c9b181dfe3b5564a04b724c657732d0bf09c99db.base64
3083c722c0c738cad0f468383167a0d246f91af2bfa373e9c5c094fb8c8413e0.base64
3871e838723dd6b166e490664eead8ec60aedd6b8d95bc8e2fe3f882f0fd90f0.base64
52d2a8f514c4fd2d9866587f4d7b2a5bfa1a11a0e772077d7682deb8b3b517e5.base64
79520a3d3a0f4d3caa440802ef4362e99d54e12b1392973e4ea321840970a88a.base64
85a6d68622b36995ccb98a89bbb119edf167c914660e4450d313de049320005c.base64
8979352a182bde7c3c651ba2b2f4e0615de819585cc37b7175bcefbca15a6683.base64
8de3a3924cb95bd0e95a443fff0326c869f9d4979cd1d5b6e94e2a01f5be53e9.base64
92d7758c402f0927011ca8d0a7e40251439fba3a1dac26f5b8b62026323501aa.base64
95b90082f799f48677b4f206d856ad572f1d178c676269eac6347631d4447258.base64
ad56fba948dfba9ae698198c109e71f118a54d209c0ea50d77ea546abad89c57.base64
bd6464771e47eed832c5eb2cd85cdc0bfc697786b903bfd30f890f9d4fc36657.base64
cb49c003b454385aa9975733aff4571c62182ccdda480aaba9a8d250014f00ec.base64
cbcbb5a6cfb55c36f8f021fbb0e3f69ac96339a39fa85cd96f2017a2192821b5.base64
cd4c518bc6ef165c39d4405b347b51ba40f8d7a065ab0e8d2e4f422cbc1e8a43.base64
e6e5631ce1237ae4c05b3563eda686400a401df4548d0f9fad40ecac1659c46c.base64
f735bee5b64ef98879dc618b016ecf7939a5756040c2cde21ccb15e69a6e1cfb.base64
fb918796edc3d2221218db0811e240e72e340350008338b0c07a52bd353666a6.base64
```

... good luck! :tired_face: