#!/usr/bin/env python3

import os
import sys
import pathlib
import io
import re
import requests
import json
import yaml
import argparse
from dotenv import load_dotenv


CONVERSIONS_PATH  = './Conversions/'
VERBOSE           = False
NOASSETS          = False


MEDIAMAP = {
  "/media/a/52d2a8f514c4fd2d9866587f4d7b2a5bfa1a11a0e772077d7682deb8b3b517e5.jpg" : { "type": "Texture", "folder":"Textures", "name":"Abstract 1"       },
  "/media/a/bd6464771e47eed832c5eb2cd85cdc0bfc697786b903bfd30f890f9d4fc36657.jpg" : { "type": "Texture", "folder":"Textures", "name":"Abstract 2"       },
  "/media/a/8979352a182bde7c3c651ba2b2f4e0615de819585cc37b7175bcefbca15a6683.jpg" : { "type": "Texture", "folder":"Textures", "name":"Abstract 3"       },
  "/media/a/85a6d68622b36995ccb98a89bbb119edf167c914660e4450d313de049320005c.png" : { "type": "Texture", "folder":"Textures", "name":"Bayer"            },
  "/media/a/cb49c003b454385aa9975733aff4571c62182ccdda480aaba9a8d250014f00ec.png" : { "type": "Texture", "folder":"Textures", "name":"Blue Noise"       },
  "/media/a/08b42b43ae9d3c0605da11d0eac86618ea888e62cdd9518ee8b9097488b31560.png" : { "type": "Texture", "folder":"Textures", "name":"Font 1"           },
  "/media/a/0c7bf5fe9462d5bffbd11126e82908e39be3ce56220d900f633d58fb432e56f5.png" : { "type": "Texture", "folder":"Textures", "name":"Gray Noise Medium"},
  "/media/a/0a40562379b63dfb89227e6d172f39fdce9022cba76623f1054a2c83d6c0ba5d.png" : { "type": "Texture", "folder":"Textures", "name":"Gray Noise Small" },
  "/media/a/fb918796edc3d2221218db0811e240e72e340350008338b0c07a52bd353666a6.jpg" : { "type": "Texture", "folder":"Textures", "name":"Lichen"           },
  "/media/a/8de3a3924cb95bd0e95a443fff0326c869f9d4979cd1d5b6e94e2a01f5be53e9.jpg" : { "type": "Texture", "folder":"Textures", "name":"London"           },
  "/media/a/cbcbb5a6cfb55c36f8f021fbb0e3f69ac96339a39fa85cd96f2017a2192821b5.png" : { "type": "Texture", "folder":"Textures", "name":"Nyancat"          },
  "/media/a/cd4c518bc6ef165c39d4405b347b51ba40f8d7a065ab0e8d2e4f422cbc1e8a43.jpg" : { "type": "Texture", "folder":"Textures", "name":"Organic 1"        },
  "/media/a/92d7758c402f0927011ca8d0a7e40251439fba3a1dac26f5b8b62026323501aa.jpg" : { "type": "Texture", "folder":"Textures", "name":"Organic 2"        },
  "/media/a/79520a3d3a0f4d3caa440802ef4362e99d54e12b1392973e4ea321840970a88a.jpg" : { "type": "Texture", "folder":"Textures", "name":"Organic 3"        },
  "/media/a/3871e838723dd6b166e490664eead8ec60aedd6b8d95bc8e2fe3f882f0fd90f0.jpg" : { "type": "Texture", "folder":"Textures", "name":"Organic 4"        },
  "/media/a/ad56fba948dfba9ae698198c109e71f118a54d209c0ea50d77ea546abad89c57.png" : { "type": "Texture", "folder":"Textures", "name":"Pebbles"          },
  "/media/a/f735bee5b64ef98879dc618b016ecf7939a5756040c2cde21ccb15e69a6e1cfb.png" : { "type": "Texture", "folder":"Textures", "name":"RGBA Noise Medium"},
  "/media/a/3083c722c0c738cad0f468383167a0d246f91af2bfa373e9c5c094fb8c8413e0.png" : { "type": "Texture", "folder":"Textures", "name":"RGBA Noise Small" },
  "/media/a/10eb4fe0ac8a7dc348a2cc282ca5df1759ab8bf680117e4047728100969e7b43.jpg" : { "type": "Texture", "folder":"Textures", "name":"Rock Tiles"       },
  "/media/a/95b90082f799f48677b4f206d856ad572f1d178c676269eac6347631d4447258.jpg" : { "type": "Texture", "folder":"Textures", "name":"Rusty Metal"      },
  "/media/a/e6e5631ce1237ae4c05b3563eda686400a401df4548d0f9fad40ecac1659c46c.jpg" : { "type": "Texture", "folder":"Textures", "name":"Stars"            },
  "/media/a/1f7dca9c22f324751f2a5a59c9b181dfe3b5564a04b724c657732d0bf09c99db.jpg" : { "type": "Texture", "folder":"Textures", "name":"Wood"             },

  "/media/previz/buffer00.png" : { "type": "Previsualization", "folder":"Misc", "name":"Buffer A"             },
  "/media/previz/buffer01.png" : { "type": "Previsualization", "folder":"Misc", "name":"Buffer B"             },
  "/media/previz/buffer02.png" : { "type": "Previsualization", "folder":"Misc", "name":"Buffer C"             },
  "/media/previz/buffer03.png" : { "type": "Previsualization", "folder":"Misc", "name":"Buffer D"             },

  "/media/a/94284d43be78f00eb6b298e6d78656a1b34e2b91b34940d02f1ca8b22310e8a0.png" : { "type": "Cubemap", "folder":"Cubemaps", "name":"Forest_0"                     },
  "/media/a/0681c014f6c88c356cf9c0394ffe015acc94ec1474924855f45d22c3e70b5785.png" : { "type": "Cubemap", "folder":"Cubemaps", "name":"Forest Blurred_0"             },
  "/media/a/488bd40303a2e2b9a71987e48c66ef41f5e937174bf316d3ed0e86410784b919.jpg" : { "type": "Cubemap", "folder":"Cubemaps", "name":"St Peters Basilica_0"         },
  "/media/a/550a8cce1bf403869fde66dddf6028dd171f1852f4a704a465e1b80d23955663.png" : { "type": "Cubemap", "folder":"Cubemaps", "name":"St Peters Basilica Blurred_0" },
  "/media/a/585f9546c092f53ded45332b343144396c0b2d70d9965f585ebc172080d8aa58.jpg" : { "type": "Cubemap", "folder":"Cubemaps", "name":"Uffizi Gallery_0"             },
  "/media/a/793a105653fbdadabdc1325ca08675e1ce48ae5f12e37973829c87bea4be3232.png" : { "type": "Cubemap", "folder":"Cubemaps", "name":"Uffizi Gallery Blurred_0"     },

  "/presets/tex00.jpg" : { "type": "Preset", "folder":"Misc", "name":"Keyboard"  },
  # /presets/tex00.jpg does not work, or does it?!?
  # thumbnail says it's previz/keyboard.png ?!?
  }

for k,v in MEDIAMAP.items():
  v["suffix"]=k[-3:]

# ---------------------------------------------------------------------------------------------------------------------

def verbose(msg):
  if VERBOSE:
    print(msg)

# ---------------------------------------------------------------------------------------------------------------------

def patch_webgl(code,fuse_name,buffer_name):
  """
  Do simple text replacement to make some WebGL to DCTL conversions.
  """

  code = code.replace("\t", "  ")


  # --- dimensions

  code=re.sub(r'([^\w])(fragCoord)\.xy([^\w])',r'\1\2\3', code)
  code=re.sub(r'([^\w])(iResolution)\.xy([^\w])',r'\1\2\3', code)

  #rgba noch verbesserbar ").rgb"
  code=re.sub(r'(\w+\.)(rgba)',r'\1xyzw', code)
  code=re.sub(r'(\w+\.)(rgb)',r'\1xyz', code)
  code=re.sub(r'(\w+\.)(rbg)',r'\1xzy', code)
  code=re.sub(r'(\w+\.)(rg)',r'\1xy', code)
  code=re.sub(r'(\w+\.)(r)([\s\)\,\;\*\\\-\+/])',r'\1x\3', code)
  code=re.sub(r'(\w+\.)(g)([\s\)\,\;\*\\\-\+/])',r'\1y\3', code)
  code=re.sub(r'(\w+\.)(b)([\s\)\,\;\*\\\-\+/])',r'\1z\3', code)
  code=re.sub(r'(\w+\.)(a)([\s\)\,\;\*\\\-\+/])',r'\1w\3', code)

  #code=re.sub(r'(\w+)\.([xyzw]{2,4})',r'swi\2(\1)', code)
  code=re.sub(r'(\w+)\.([xyzw])([xyzw])([\s\)\,\;\*\\\-\+])',r'swi2(\1,\2,\3)\4', code)
  code=re.sub(r'(\w+)\.([xyzw])([xyzw])([xyzw])([\s\)\,\;\*\\\-\+])',r'swi3(\1,\2,\3,\4)\5', code)
  code=re.sub(r'(\w+)\.([xyzw])([xyzw])([xyzw])([xyzw])([\s\)\,\;\*\\\-\+])',r'swi4(\1,\2,\3,\4,\5)\6', code)


  # --- arrays

  code=re.sub(r'(^\s?)(\w+)(\[\s?\])\s*(\w+)',r'\1\2 \4\3', code) #Deklaration

  #code=re.sub(r'(^\w+)texture(\s*)\(',r'\g<1>_tex2DVecN\2(',code)
  code=re.sub(r'(.*)texture(\s*\(\s*)(\w+)\s*\,\s*(\w+)\s*\)','\g<1>_tex2DVecN\g<2>\g<3>,\g<4>.x,\g<4>.y,15)',code)
  #code=re.sub(r'(sampler2D)(\s*\w+)','__Texture2D__\2',code) #kollidiert noch mit Kernelaufruf - muss dort dann geaendert werden


  # --- math functions

  for s in [
      # prefix, suffix, functions
      [ '' ,   '_f', 'mod'], # '_f' suffix to redirect to own implementation
      [ '_',   'f' , 'pow|log2|log10|log|copysign|saturate|sqrt|trunc|hypot|cos|sin|cospi|sinpi|tan|acos|asinh|atanh|cosh|sinh|tanh|cbrt|lgamma|tgamma|rsqrt|exp|exp2'],
      [ '_',   '2f', 'atan'],
      [ '_f',  'f' , 'max|min|dim'],
      [ '_' ,  ''  , 'ceil|floor|mix'],
      [ '_f',  ''  , 'maf|divide|recip|abs|remainder']
    ]:
    code=re.sub(r'([^\w])('+s[2]+r')(\s*)\(',r'\g<1>'+s[0]+r'\g<2>'+s[1]+r'\3(', code)


  # --- float literals

  code=re.sub(r'([ \(,\+\-\*=/<>]+)(\.[0-9]+f{0,1})([ \),\+\-\*=;/<>]+)',r'\g<1>0\2\3', code)
  code=re.sub(r'([ \(,\+\-\*=/<>]+)(\.[0-9]+f{0,1})([ \),\+\-\*=;/<>]+)',r'\g<1>0\2\3', code)
  code=re.sub(r'([ \(,\+\-\*=/<>]+)([0-9]+\.)([ \),\+\-\*=;/<>]+)',r'\1\g<2>0\3', code)
  code=re.sub(r'([ \(,\+\-\*=/<>]+)([0-9]+\.)([ \),\+\-\*=;/<>]+)',r'\1\g<2>0\3', code)
  code=re.sub(r'([ \(,\+\-\*=/<>]+)([0-9]+\.[0-9]+)([ \),\+\-\*=;/<>]+)',r'\1\2f\3', code)
  code=re.sub(r'([ \(,\+\-\*=/<>]+)([0-9]+\.[0-9]+)([ \),\+\-\*=;/<>]+)',r'\1\2f\3', code)


  # --- vector types

  # vecN ... =  -> floatN ... =
  code=re.sub(r'\n(\s*)vec([234])(\s+[_A-Za-z][_A-Za-z0-9]*\s*=)',r'\n\1float\2\3', code)
  code=re.sub(r'\n(\s*)const(\s+)vec([234])(\s+[_A-Za-z][_A-Za-z0-9]*\s*=)',r'\n\1const\2float\3\4', code)

  # ivecN ... =  -> intN ... =
  code=re.sub(r'\n(\s*)ivec([234])(\s+[_A-Za-z][_A-Za-z0-9]*\s*=)',r'\n\1int\2\3', code)
  code=re.sub(r'\n(\s*)const(\s+)ivec([234])(\s+[_A-Za-z][_A-Za-z0-9]*\s*=)',r'\n\1const\2int\3\4', code)

  # vecN(float) -> to_floatN_s(float)
  code=re.sub(r'vec([234])(\s*\(\s*[0-9]+\.[0-9]+f\s*\))',r'to_float\1_s\2', code)

  # versuche floatN_aw zu erwischen - geht aber natuerlich nur sehr bedingt
  code=re.sub(r'vec([34])(\s*\([^,]+,[^,\)]+\))',r'to_float\1_aw\2', code)

  code=re.sub(r'ivec([234])(\s*\()',r'to_int\1\2', code)
  code=re.sub(r'vec([234])(\s*\()',r'to_float\1\2', code)

  # am Schluss alle verbleibenden 'vecN' weghauen:
  code=re.sub(r'([\s\(\)\*\+\-;,=])ivec([234])(\s)',r'\1int\2\3', code)
  code=re.sub(r'([\s\(\)\*\+\-;,=])vec([234])(\s)',r'\1float\2\3', code)


  # --- kernel function

  kernel_name=fuse_name+'Fuse'

  if buffer_name!="Image":
    kernel_name=fuse_name+'Fuse__'+buffer_name.replace(" ","_")

  kernel_parameters=""

  for e in [
      ['iTime','float iTime'],
      ['iResolution','float2 iResolution'],
      ['iMouse','float4 iMouse'],
      ['iTimeDelta' , 'float iTimeDelta'],
      ['iFrame' , 'int iFrame'],
      ['iChannelTime' , 'float iChannelTime[]'],
      ['iChannelResolution' , 'float3 iChannelResolution[]'],
      ['iDate' , 'float4 iDate'],
      ['iSampleRate' , 'float iSampleRate'],
      ['iChannel0', 'sampler2D iChannel0'],
      ['iChannel1', 'sampler2D iChannel1'],
      ['iChannel2', 'sampler2D iChannel2'],
      ['iChannel3', 'sampler2D iChannel3'],
    ]:
    if code.find(e[0])!=-1: # okay, ein find() ist hier arg grob - aber fuer's erste soll's reichen
      kernel_parameters=kernel_parameters+", "+e[1]


  if buffer_name!='Common':
    # Kernelaufruf
    #   Ich erwische dabei nur die mit fragColor und fragCoord? Gibt's bei Shadertoy auch Aufrufe ohne diese beiden,
    #   gar oder mit mehr Parametern?!?
    #
    match_kernel=r'void\s+mainImage\s*\(\s*out\s+float4\s+([A-Za-z_]\w*)\s*,\s*in\s+float2\s+([A-Za-z_]\w*)\s*\)\s*{'

    m = re.search(match_kernel,code)

    if not m:
      # schnellschuss und keine wirkliche loesung; hatte einen shader ohne 'in' an dem vec2
      match_kernel=r'void\s+mainImage\s*\(\s*out\s+float4\s+([A-Za-z_]\w*)\s*,\s*float2\s+([A-Za-z_]\w*)\s*\)\s*{'
      m = re.search(match_kernel,code)

      if not m:
        # schnellschuss und keine wirkliche loesung; hatte einen shader mit kommentar hinter dem kernelnamen
        match_kernel=r'void\s+mainImage\s*\(\s*out\s+float4\s+([A-Za-z_]\w*)\s*,\s*float2\s+([A-Za-z_]\w*)\s*\)\s*//[^\n]*\n{'
        m = re.search(match_kernel,code)


    if m:
      fragColor=m.group(1)

      code = re.sub(match_kernel,
        '__KERNEL__ void '+kernel_name+'(float4 \\1, float2 \\2'+kernel_parameters+')\n{\n'
        , code)

      # Versuche jetzt am Ende noch etwas reinzuschmieren:

      p=code.rfind("}")

      if p!=-1:
        code=code[0:p] + "\n\n  SetFragmentShaderComputedColor("+ fragColor +");\n" +code[p]
    else:
      print("attention: no kernel found in "+buffer_name)


  # Mal versuchen Funktionen zu finden:

  code=re.sub(r'(\n\s*)(mat[2-4]|float[1-4]{0,1}|int|void|bool)(\s+[A-Za-z_]\w*\s*\([^\)]*\)\s*{)',r'\g<1>__DEVICE__ \g<2>\g<3>',code)


  return code

# ---------------------------------------------------------------------------------------------------------------------

def as_fuse_id(shader_name,shader_id):
  """
  Derive an identifier from shader_name.

  Remove whitespace, leading digits, special characters, etc to make something that can be used as an identifier out of `shader_name`.
  Such an identifier is in particular what's needed as the first parametert to `FuRegisterClass()`.
  """

  name = shader_name

  # Example: "Fork Who cares? nmbr73 321" -> "Who cares"
  name = re.sub(r'^Fork (.+) ([^ ]+) \d+$',r'\1',name)

  # Replace all invalid characters with a ' '
  name = re.sub(r'[^A-Za-z0-9 ]+',' ', name)

  # Put 'D' in front if first character is a digit
  name = re.sub(r'^(\d.*)$',r'D\1', name)

  # Transform leading characters to upper case
  name = name.title()

  # Eliminate all spaces
  name = ''.join(x for x in name if not x.isspace())

  return name

# ---------------------------------------------------------------------------------------------------------------------

def as_fuse_name(shader_name,shader_id):
  """
  Derive a fuse name from shader_name.

  The fuse name is what is passed as `REGS_Name` to the `FuRegisterClass()` call.
  This name is shown in the 'Add Tool' context menu, in the 'Select Tool' dialiog,
  in the 'Efects' panel, in the 'About' dialog, etc.

  When inserting a node into a composition it's also the fuse name what get's
  used - but DaFusion makes it an identifier (to be able to use it in expressions
  or export it to settings files) by elimiating whitespace and leading digits.
  """

  return as_fuse_id(shader_name,shader_id)

# ---------------------------------------------------------------------------------------------------------------------

def as_kernel_name(shader_name,shader_id):
  """
  Derive a kernel function name from shader_name.
  """

  return as_fuse_id(shader_name,shader_id)

# ---------------------------------------------------------------------------------------------------------------------

def as_file_name(shader_name,shader_id):
  """
  Derive a filename (without suffix) from shader_name.

  This filename is used for alle the temporary '.json', '.yaml', '.c', etc. files
  generated by `fetch` and it can be used as the filename for the '.md' and '.fuse'
  file created by `fuse`.
  """

  return as_fuse_id(shader_name,shader_id)


# ---------------------------------------------------------------------------------------------------------------------

def create_json(shader_id):
  """
  See if there is a JSON file with the given `shader_id`.

  This checks `CONVERSIONS_PATH` and the environments `DOWNLOADS`
  folder for a JSON file containing the shader `shader_id`.

  @return (string) filename (without '.<ID>.json').
  """

  files=[entry for entry in os.scandir(CONVERSIONS_PATH) if entry.is_file() and entry.name.endswith(f".{shader_id}.json")]

  if len(files) > 1:
    raise Exception(f"multiple files matching '{CONVERSIONS_PATH}*.{shader_id}.json'")

  if len(files) == 1:
    conv_name = files[0].name
    conv_name = conv_name[0:len(conv_name)-len(f".{shader_id}.json")]
    return conv_name


  # then we have to read and parse the json to get a filename

  json_text   = None
  remove_file = None


  # try to read from shader_ID.json file

  if json_text == None:

    fname = f"shader_{shader_id}.json"

    if os.path.isfile(CONVERSIONS_PATH + fname):

      print("read {fname} file")

      with open(CONVERSIONS_PATH+fname, "r") as f:
        json_text=f.read()

      remove_file = CONVERSIONS_PATH+fname


  # try to read shader_ID.json from downloads

  if json_text == None:

    fname = f"shader_{shader_id}.json"

    downloads=os.getenv('DOWNLOADS')

    if downloads!=None and downloads!="" and os.path.isfile(downloads + fname):

      print("read {fname} from downloads")

      with open(downloads+fname, "r") as f:
        json_text=f.read()


  # try to fetch from shadertoy.com

  if json_text == None:

    print("fetch JSON from shadertoy.com")
    response = requests.get("https://www.shadertoy.com/api/v1/shaders/" +shader_id+"?key="+os.getenv('APIKEY'),
      headers={"user-agent": "Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:59.0) Gecko/20100101 Firefox/59.0"}
      )

    if response.status_code != 200:
      raise Exception("HTTP Error "+str(response.status_code))

    json_text = response.text


  # no options left if we still got no json (unreachable)

  if json_text == None:
    raise Exception(f"unable to get json data for {shader_id}")


  # extract filename from json data

  json_data = json.loads(json_text)

  error = json_data.get('Error',None)

  if error != None:
    raise Exception(error)

  if 'Shader' in json_data:
     json_data = json_data['Shader']

  shader_name = json_data['info']['name']

  conv_name = as_file_name(shader_name,shader_id)


  # write file

  outfilepath = f"{CONVERSIONS_PATH}{conv_name}.{shader_id}.json"
  print(f"write to {outfilepath}")

  with io.open(outfilepath, 'w') as f:
    f.write(json_text)


  # delete shader_ID.json if in our folder

  if remove_file!=None:
    print("delete {remove_file}")
    os.remove(remove_file)

  return conv_name

# ---------------------------------------------------------------------------------------------------------------------

def read_json(conv_name, shader_id):
  """
  Get all the Shadertoy information as a JSON structure.
  """

  json_text = None

  # --- read file

  infilepath = f"{CONVERSIONS_PATH}{conv_name}.{shader_id}.json"

  if not os.path.isfile(infilepath):
    raise Exception(f"{infilepath} does not exists")

  verbose(f"read from {infilepath}")

  with open(infilepath, "r") as f:
    json_text=f.read()


  # --- parse json

  json_data = json.loads(json_text)

  error = json_data.get('Error',None)

  if error != None:
    raise Exception(error)

  if 'Shader' in json_data:
     json_data = json_data['Shader']


  # --- fix json data

  if not ('username' in json_data['info']):
    json_data['info']['username']="N.N."

  for entry in json_data['renderpass']:

    for input in entry.get('inputs',{}):

      if not 'ctype' in input:
        input['ctype']=input['type']

      if not 'src' in input:
        input['src']=input['filepath']

      if 'name' in input:
        raise Exception("input already has a name?!?")

      media=MEDIAMAP.get(input['src'],None)

      if media==None:
        input['name']=input['src']
      else:
        input['name']=media['type']+': '+media['name']

  return json_data

# ---------------------------------------------------------------------------------------------------------------------

def create_yaml(conv_name, shader_id, json_data):

  yaml_filename = f"{conv_name}.{shader_id}.yaml"

  if os.path.isfile(CONVERSIONS_PATH+yaml_filename):
    return

  info = json_data['info']

  shader_name = json_data['info']['name']

  yaml_data={
        'shader':{
          'id'          : shader_id,
          'name'        : shader_name,
          'author'      : info['username'],
          'url'         : 'https://www.shadertoy.com/view/'+shader_id,
          'description' : info['description'],
          'tags'        : info['tags'],
        },
        'fuse':{
          'id'      : as_fuse_id(shader_name,shader_id),
          'name'    : as_fuse_name(shader_name,shader_id),
          'file'    : as_file_name(shader_name,shader_id),
          'kernel'  : as_kernel_name(shader_name,shader_id),
          'author'  : os.getenv('AUTHOR'),
        }
      }

  if "parentid" in info and "parentname" in info:
    yaml_data['shader']['parent']={
      'id'    : info['parentid'],
      'name'  : info['parentname'],
      'url'   : 'https://www.shadertoy.com/view/'+info['parentid'],
      }


  with io.open(CONVERSIONS_PATH+yaml_filename, 'w', encoding='utf8') as outfile:
      yaml.dump(yaml_data,outfile, default_flow_style=False, allow_unicode=True)

# # ---------------------------------------------------------------------------------------------------------------------

# def read_yaml(conv_name, shader_id):
#   """
#   Get infortmation stored in the YAML file.
#   """

#   yaml_text = None

#   # --- read file

#   infilepath = f"{CONVERSIONS_PATH}{conv_name}.{shader_id}.yaml"

#   if not os.path.isfile(infilepath):
#     raise Exception(f"{infilepath} does not exists")

#   verbose(f"read from {infilepath}")

#   with open(infilepath, "r") as f:
#     yaml_text=f.read()

#   yaml_data = yaml.load(yaml_text, Loader=yaml.FullLoader)

#   if yaml_data['shader']['id'] != shader_id:
#     raise Exception(f"shader id missmatch in yaml file")

#   # for old yaml files - can be removed later:
#   if not 'kernel' in yaml_data['fuse']:
#     shader_name=yaml_data['shader']['name']
#     yaml_data['fuse']['kernel'] = as_kernel_name(shader_name,shader_id)

#   return yaml_data

# ---------------------------------------------------------------------------------------------------------------------

def create_glsl(conv_name, shader_id, json_data):

  glsl_filename = f"{conv_name}.{shader_id}.glsl"

  if os.path.isfile(CONVERSIONS_PATH+glsl_filename):
    return

  glsl_text='''

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //

'''

  for entry in json_data['renderpass']:

    name = entry['name']
    code = entry['code']

    glsl_text += '\n// >>> ___ GLSL:['+name+']' + ' '+('_'*(73-len(name))) + ' <<<\n'
    glsl_text += code


  with io.open(CONVERSIONS_PATH+glsl_filename, 'w') as f:
    f.write(glsl_text)

# ---------------------------------------------------------------------------------------------------------------------

def read_glsl(conv_name, shader_id):
  """
  Get GLSL code from file.
  """

  glsl_text = None

  # --- read file

  infilepath = f"{CONVERSIONS_PATH}{conv_name}.{shader_id}.glsl"

  if not os.path.isfile(infilepath):
    raise Exception(f"{infilepath} does not exists")

  verbose(f"read from {infilepath}")

  with open(infilepath, "r") as f:
    glsl_text=f.read()

  glsl_data={}

  parts =  glsl_text.split("\n// >>> ___ GLSL:[")[1:]

  for p in parts:
    m = re.match(r'([^\]]+)\] _{30,} <<< *\n',p)

    if not m:
      raise Exception("broken marker 'GLSL: ["+p[0:10]+"...'")

    name = m.group(1)

    if name in glsl_data:
      raise Exception("mulltiple occurences of 'GLSL: ["+name+"]' marker")

    code = p[p.find('\n')+1:]

    glsl_data[name]=code

  return glsl_data

# ---------------------------------------------------------------------------------------------------------------------

def create_assets(json_data):

  if NOASSETS:
    return

  for entry in json_data['renderpass']:

    inputs=entry.get('inputs',None)

    if inputs==None:
      continue

    for input in inputs:

      src=input['src']

      if src==None:
        continue

      media=MEDIAMAP.get(src,None)

      if media != None:

        # known asset
        filename=selfpath+"./Assets/"+media['folder']+"/"+media['name']+"."+media['suffix']

      else:

        # unknown media
        filename=selfpath+"./Assets/UNKNOWN/"+re.sub(r'/','_',src)


      # do nothing if already downloaded

      if os.path.exists(filename):
        verbose(f"asset '{filename}' already downloaded")
        continue


      # http get and write file

      verbose(f"download asset '{filename}'")

      data = requests.get("https://www.shadertoy.com"+src)

      with open(filename, 'wb') as f:
          f.write(data.content)

# ---------------------------------------------------------------------------------------------------------------------

def create_dctl(conv_name, shader_id, json_data, glsl_data):

  dctl_filename = f"{conv_name}.{shader_id}.c"

  if os.path.isfile(CONVERSIONS_PATH+dctl_filename):
    return

  shader_name = json_data['info']['name']
  kernel_name = as_kernel_name(shader_name,shader_id)

  known_code_parts=['Common','Buffer A','Buffer B','Buffer C','Buffer D','Image','Sound']

  code_parts={}

  for part in known_code_parts:
    code_parts[part]={ 'code' : ''}

  for entry in json_data['renderpass']:

    name = entry['name']

    if not name in known_code_parts:
      raise Exception("unknown code section '"+name+"'")

    if not name in glsl_data:
      raise Exception(f"missing part '{name}' in GLSL")

    code = entry['code']

    if code != glsl_data[name]:
      verbose(f"glsl code changed for '{name}'")
      # with io.open(name+'.glsl.txt', 'w') as f:
      #   f.write(glsl_data[name])
      # with io.open(name+'.json.txt', 'w') as f:
      #   f.write(code)
      code = glsl_data[name]


    header= "\n" \
            "// ----------------------------------------------------------------------------------\n" \
            "// - "+name+(" "*(79-len(name)))+"-\n" \
            "// ----------------------------------------------------------------------------------\n"

    inputs=entry.get('inputs',None)

    if inputs!=None and len(inputs)>0:
      for input in inputs:
          #header=header + "// Connect '"+input['name']+"' to iChannel"+str(input['channel'])+"\n"
          header=header + "// Connect "+name+" '"+input['name']+"' to iChannel"+str(input['channel'])+"\n"

    code_parts[name]['code'] = header + "\n\n" + patch_webgl(code,kernel_name,name)


  code=""

  for part in known_code_parts:
    code=code + code_parts[part]['code']

  with open(CONVERSIONS_PATH+dctl_filename, 'w') as f:
     f.write(code)


# ---------------------------------------------------------------------------------------------------------------------

def do_fetch(shader_id):

  conv_name   = create_json(shader_id)
  json_data   = read_json(conv_name, shader_id)
  create_glsl(conv_name, shader_id, json_data)
  glsl_data   = read_glsl(conv_name, shader_id)
  create_dctl(conv_name, shader_id, json_data, glsl_data)
  create_assets(json_data)
  create_yaml(conv_name, shader_id, json_data)
  #yaml_data   = read_yaml(conv_name, shader_id)

# =====================================================================================================================

selfpath = ""
folder = ""

if sys.argv[0] != "fetch.py" and sys.argv[0] != "./fetch.py":

    # I guess this is where we get if started from within Fusion?
    # print("##Argv2##",id,param,txt)
    # id, folder, etc. are passed in by the incubator Fuse?!?

    print("\n#################### Fetch Script ###################")

    selfpath = os.path.dirname(sys.argv[0])+os.sep

    print("# SELFPATH: ",selfpath)
    print("# ARGV:     ",sys.argv, len(sys.argv))
    print("# Folder:  ",folder)
    print("# ID:      ",id)

    CONVERSIONS_PATH = selfpath + "Conversions"+os.sep
      

    NOASSETS  = False
    VERBOSE   = False # verbose
    ID        = id


else:

    #parser.add_argument('-f','--force',action='store_true',help='overwrite code if it already exists')
    #parser.add_argument('-a','--assets',action='store_true',help='fetch assets (even if they exist)') # TODO
    #parser.add_argument('-nc','--no-cache',action='store_true',help='re-fetch the .json file (assets are not fetched if they exist localy)')
    #parser.add_argument('-np','--no-patch',action='store_true',help='do not patch the code for DCTÖL - see normal WebGL in the .c file')

    parser = argparse.ArgumentParser(description='Fetch fuse source code.')
    parser.add_argument('-i','--id', help='shadertoy id as used in the URL', required=True)
    parser.add_argument('-f','--folder', help='subfolder for conversions', required=False)
    parser.add_argument('-na','--no-assets',action='store_true',help='do not try to download the assets, even if they are not yet existing')
    parser.add_argument('-v','--verbose',action='store_true',help='verbose output')
    args = parser.parse_args()

    NOASSETS  = args.no_assets
    VERBOSE   = args.verbose
    ID        = args.id
    folder    = args.folder

if not(os.path.isfile(selfpath+".env")):
  with open(".env", 'w') as f:
    f.write(  "AUTHOR=\"\"\n"
              "APIKEY=\"\"\n"
              "DOWNLOADS=\"\"\n"
              "FUSEPATH=\"\"\n"
              "REPOPATH=\"\"\n"
            )
  print(".env file created - please enter your credentials to use")


load_dotenv(selfpath+".env")

if folder:
  CONVERSIONS_PATH = CONVERSIONS_PATH + folder + os.sep

# print("\n##PATH##",CONVERSIONS_PATH)
# print("\nENVIRIONMENT ",os.getenv('APIKEY'))




#try:
#  do_fetch(args.id,force=args.force,nocache=args.no_cache,noassets=args.no_assets,nopatch=args.no_patch)
#except Exception as e:
#  print("ERROR: "+str(e))
do_fetch(ID)
#do_fetch(id)