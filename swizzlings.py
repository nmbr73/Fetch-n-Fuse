

# Erzeugt alle Defines fuer alle Swizzlings.
# Quick and dirty - braucht nicht schön sein.
# Der komische Kommentar vor jedem Define ist dazu da,
# daß das 'fuse' nachher nur die tatsächlich benötigten
# defines in die fertige Fuse übernimmt.

chars=['x','y','z','w']

print("#if defined(USE_NATIVE_METAL_IMPL)")

for c0 in chars:
  for c1 in chars:
    print("  /*| swi"+c0+c1+"   |*/ #define swi"+c0+c1+"(A) (A)."+c0+c1)

for c0 in chars:
  for c1 in chars:
    for c2 in chars:
      print("  /*| swi"+c0+c1+c2+"  |*/ #define swi"+c0+c1+c2+"(A) (A)."+c0+c1+c2)

for c0 in chars:
  for c1 in chars:
    for c2 in chars:
      for c3 in chars:
        print("  /*| swi"+c0+c1+c2+c3+" |*/ #define swi"+c0+c1+c2+c3+"(A) (A)."+c0+c1+c2+c3)

print("#else")

for c0 in chars:
  for c1 in chars:
    print("  /*| swi"+c0+c1+"   |*/ #define swi"+c0+c1+"(A) to_float2((A)."+c0+",(A)."+c1+")")

for c0 in chars:
  for c1 in chars:
    for c2 in chars:
      print("  /*| swi"+c0+c1+c2+"  |*/ #define swi"+c0+c1+c2+"(A) to_float3((A)."+c0+",(A)."+c1+",(A)."+c2+")")

for c0 in chars:
  for c1 in chars:
    for c2 in chars:
      for c3 in chars:
        print("  /*| swi"+c0+c1+c2+c3+" |*/ #define swi"+c0+c1+c2+c3+"(A) to_float4((A)."+c0+",(A)."+c1+",(A)."+c2+",(A)."+c3+")")

print("#endif")