import yaml


with open(".clobber/fuse-parameters.yaml", "r") as f:
  src=f.read()

obj=yaml.load(src, Loader=yaml.FullLoader)
print(str(obj))
#print("'"+obj['iTime']['Create']+"'")