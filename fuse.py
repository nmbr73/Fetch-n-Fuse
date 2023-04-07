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
from datetime import date



VERBOSE=False

# ---------------------------------------------------------------------------------------------------------------------

def verbose(msg):
  if VERBOSE:
    print(msg)

# ---------------------------------------------------------------------------------------------------------------------

def get_compatibility_code(conversionCode):
  """
  Load the CompatibilityCode and adapt it.
  """

  # copatibility code laden

  with open(selfpath+"./.clobber/CompatibilityCode.c", "r") as f:
    compatibilityCode=f.read()


  # gucken, welche keys es im compatibility code gibt

  keys = {}

  for line in compatibilityCode.split("\n"):
    match=re.match(r'^\s*\/\*\|([^\|]*)\|\*\/(.*)$',line)
    if match != None:
      key=match.group(1).strip()
      if key != '' and not key in keys:
        keys[key]=False

  # gucken, welche keys im conversion code vorkommen

  for key in keys:
    if re.search(r'[^\w]'+key+r'[^\w]',conversionCode) != None:
      keys[key]=True


  # zeilen rausfiltern, deren key nicht verwendet wird

  abridgedCompatibilityCode=""

  for line in compatibilityCode.split("\n"):
    match=re.match(r'^\s*\/\*\|([^\|]*)\|\*\/(.*)$',line)
    if match==None:
      abridgedCompatibilityCode = abridgedCompatibilityCode+line+"\n"
    else:
      key  = match.group(1).strip()
      code = match.group(2)
      if key=='' or keys[key]:
        abridgedCompatibilityCode = abridgedCompatibilityCode+code+"\n"


  # ueberfluessegen whitespace eliminieren

  abridgedCompatibilityCode=re.sub(r'\t','  ',abridgedCompatibilityCode)
  abridgedCompatibilityCode=re.sub(r' +\n','\n',abridgedCompatibilityCode)
  abridgedCompatibilityCode=re.sub(r'\n +\n','\n\n',abridgedCompatibilityCode)
  abridgedCompatibilityCode=re.sub(r'\n{3,}','\n\n',abridgedCompatibilityCode)


  # und damit ist der compatibility code entsprechend angepasst

  return abridgedCompatibilityCode

# ---------------------------------------------------------------------------------------------------------------------

def get_basefilename(path,id):
  """
  Get the name preceding the '.<ID>.c' suffix.
  """

  files=[entry for entry in os.scandir(path) if entry.is_file() and entry.name.endswith(f".{id}.c")]

  if len(files) < 1:
    raise Exception(f"no such file '{path}*.{id}.c'")

  if len(files) > 1:
    raise Exception(f"there are multiple files matching '{path}*.{id}.c'")

  name = re.sub(r'^(.+)\.'+id+'\.c$',r'\1',files[0].name)

  # print(f"fuse conversion '{path}{name}.{id}.c'")

  return name


# ---------------------------------------------------------------------------------------------------------------------

def write_markdown_file(info,target_file,force=False):

  fuse_name     = info['fuse']['name']
  fuse_author   = info['fuse']['author']
  shader_author = info['shader']['author']
  shader_id     = info['shader']['id']
  shader_name   = info['shader']['name']
  description   = info['shader'].get('description','')

  if os.path.isfile(target_file) and not force:
    print("file '"+target_file+"' already exists - use force option to overwrite")
    return

  dlbtn = f"[![Download Installer](https://img.shields.io/static/v1?label=Download&message={fuse_name}-Installer.lua&color=blue)]({fuse_name}-Installer.lua \"Installer\")"
  with open(target_file, 'w') as f:
    f.write( "" \
      f"# Here insert a description of the shader! And maybe a gif\n\n" \
      f"# Description of the Shader in Shadertoy: \n{description}\n\n" \
      f"[![Thumbnail]({fuse_name}_screenshoot.png)]({fuse_name}.fuse)\n\n"
      )

    verbose("wrote file '"+target_file+"'")

# ---------------------------------------------------------------------------------------------------------------------

def write_sfi_file(info,target_file,force=False):

  fuse_name     = info['fuse']['name']
  fuse_author   = info['fuse']['author']
  shader_author = info['shader']['author']
  shader_id     = info['shader']['id']
  shader_name   = info['shader']['name']
  description   = info['shader'].get('description','')
  creationdate  = date.today().strftime("%Y-%m-%d")

  if os.path.isfile(target_file) and not force:
    print("file '"+target_file+"' already exists - use force option to overwrite")
    return

  with open(target_file, 'w') as f:
    f.write( "" \
      f"info = {{\n\n   -- https://www.shadertoy.com/view/{shader_id}\n\n   Shadertoy = {{\n      Name = \"{shader_name}\",\n      Author = \"{shader_author}\",\n      ID = \"{shader_id}\",\n   }},\n\n" \
      f"   Fuse = {{\n      Author = \"{fuse_author}\",\n      Date = \"{creationdate}\",\n   }},\n\n" \
      f"   Compatibility = {{\n      macOS_Metal = false,\n      macOS_OpenCL = false,\n      Windows_CUDA = false,\n      Windows_OpenCL = false,\n   }},\n}}"
      )

    verbose("wrote file '"+target_file+"'")

# ---------------------------------------------------------------------------------------------------------------------

def patch_kernel(conversionCode,kernelName,has_param,part=""):


  conversionKernelName=kernelName+"Fuse"+part

  verbose("Patch Kernel '"+conversionKernelName+"'")

  m = re.search(r'__KERNEL__\s+void\s+'+conversionKernelName+r'\s*\(([^\)]*)\)\s*\{',conversionCode)

  if not m:
    if part=="":
      raise Exception("no image kernel found!")
    return conversionCode


  kernel_parameters=m.group(1)


  fragColor_name = None # float4
  fragCoord_name = None # float2


  # ja, okay, jetzt wird's echt bescheuert ...

  i=-1 # iteration to allow fragColor and Coord only as the first two parameters

  kernel_parameters=kernel_parameters.strip()
  verbose("kernel parameters='"+kernel_parameters+"'")

  for p in kernel_parameters.split(','):

    i+=1

    t = None
    n = None
    m = re.search(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z_0-9]*)\s*$', p) #match "type name"
    if m:
        t = m.group(1)
        n = m.group(2)
    else:
      m = re.search(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s*\*\s*([A-Za-z_][A-Za-z_0-9]*)\s*$', p) # match "type * name" (yes, type*name matches, so what)
      if m:
        t = m.group(1)+'*'
        n = m.group(2)
      else:
        m= re.search(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z_0-9]*)\s*\[\s*\]\s*$', p) # match "type name [ ]"
        if m:
          t = m.group(1)+'*'
          n = m.group(2)

    if not m:
      raise Exception("parameter='"+p+"' has bad format")

    verbose("parameter"+str(i)+": type='"+t+"', name='"+n+"'")
    if has_param.get(n,None)==None:
      if i==0 and t=='float4' and fragColor_name==None:
        fragColor_name=n # first and only the first parameter can be the color (yes, stupid idea to call that color iDate)
      elif i<2 and t=='float2' and fragCoord_name==None:
        fragCoord_name=n # first or second parameter can be the coord
      else:
        raise Exception("unknown parameter '"+t+" "+n+"'")
    else:
      if t != has_param[n]['Type']:
        raise Exception("bad type '"+t+"' for '"+n+"' ('"+has_param[n]['Type']+"' expected)")

      if has_param[n]['Present']:
        print("multiple occurences of '"+t+" "+n+"'")
      else:
        has_param[n]['Present']=True

  # even 'gaps' in the channels (unused channels) must be counted
  # so if we have iChannel3 and only iChannel3, then the number
  # of channels is 4 (iChannel0 to iChannel3).

  NUM_INPUT_CHANNELS=0
  kernel_parameters = "__CONSTANTREF__ Params*  params, "
  for i in range(16):
    if has_param["iChannel"+str(i)]['Present']:
      kernel_parameters += " __TEXTURE2D__ iChannel"+str(i)+", "
      NUM_INPUT_CHANNELS=i+1

  kernel_parameters += "__TEXTURE2D_WRITE__ destinationTexture"

  if NUM_INPUT_CHANNELS==0:
    has_param['iChannelResolution']['Code'] = '/* no iChannels!?! */\n'
    has_param['iChannelTime']['Code']       = '/* no iChannels!?! */\n'
  else:

    # iChannelResolution[]
    # iChannelTime[]
    has_param['iChannelResolution']['Code']='  float2 iChannelResolution['+ str(NUM_INPUT_CHANNELS) +'];\n'
    has_param['iChannelTime']['Code']='  float iChannelTime['+ str(NUM_INPUT_CHANNELS) +'];\n'
    for i in range(NUM_INPUT_CHANNELS):
      if has_param["iChannel"+str(i)]['Present']:
        has_param['iChannelResolution']['Code'] += "  iChannelResolution["+str(i)+"] = to_float2(params->iChannelResolution["+str(i)+"][0], params->iChannelResolution["+str(i)+"][1]);\n"
        has_param['iChannelTime']['Code']       += "  iChannelTime["+str(i)+"] = params->iChannelTime["+str(i)+"];\n"
      else:
        has_param['iChannelResolution']['Code'] += "  iChannelResolution["+str(i)+"] = to_float2(0.0f,0.0f);\n"
        has_param['iChannelTime']['Code']       += "  iChannelTime["+str(i)+"] = 0.0f;\n"


  variable_declarations = ""

  for n,p in has_param.items():
    if p['Present'] and p['Code']!='':
      variable_declarations += p['Code']

  if fragColor_name != None:
    variable_declarations+="  float4 "+fragColor_name+"   = to_float4_s(0.0f);\n"

  if fragCoord_name != None:
    variable_declarations+="  float2 "+fragCoord_name+"   = to_float2(fusion_x,fusion_y);\n"


  m=r'__KERNEL__\s+void\s+'+conversionKernelName+r'\s*\([^\)]*\)\s*\{'

  r=  "__KERNEL__ void "+conversionKernelName+"("+kernel_parameters+")\n" \
      "{\n" \
      "   DEFINE_KERNEL_ITERATORS_XY(fusion_x, fusion_y);\n\n" \
      "   if (fusion_x >= params->width || fusion_y >= params->height)\n" \
      "     return;\n\n"+variable_declarations+"\n\n<<CODEFROMCONTROLS>>  // --------\n\n"

  conversionCode  = re.sub( m,r,conversionCode)

  conversionCode  = re.sub(
      r'(\s+)SetFragmentShaderComputedColor\s*\(\s*',
      r'\1_tex2DVec4Write(destinationTexture, fusion_x, fusion_y, ',
      conversionCode)


  return conversionCode

# ---------------------------------------------------------------------------------------------------------------------

def check_controls(conversionCode):

  controls = re.findall(r'\s+CONNECT_([A-Z]+)(\d+)\s*\(\s*([A-Za-z_][\w]*)\s*,\s*([^\)]+)\)\s*;',conversionCode)

  fullCodeCreate=''
  fullCodeProcess=''
  fullCodeKernel=''
  fullCodeParams=''

  for ctrl in controls:

    ctrlType    = ctrl[0]
    ctrlIdx     = ctrl[1]
    ctrlVar     = ctrl[2]
    ctrlParams  = ctrl[3] # ctrl[3].split(",")

    # print(f"create {ctrlType} for {ctrlVar} with parameters '{ctrlParams}'")

    if ctrlType == 'SLIDER' or ctrlType == 'SCREW':

      m = re.match(r'^\s*(-?\d+\.\d+)f\s*,\s*(-?\d+\.\d+)f\s*,\s*(-?\d+\.\d+)f\s*$',ctrlParams)

      if m == None:
        raise Exception (f"failed to match '{ctrlParams}' as {ctrlType} parameters")

      min, max, default = m.group(1), m.group(2), m.group(3)

      codeParams  = f"float  {ctrlVar};"
      codeKernel  = f"float  {ctrlVar} = params->{ctrlVar};"
      codeProcess = f"params.{ctrlVar} = In{ctrlVar}Slider:GetValue(req).Value"
      if ctrlType == 'SLIDER':
          codeCreate  = f"""
              In{ctrlVar}Slider = self:AddInput("{ctrlVar}", "{ctrlVar}", {{
                  LINKID_DataType    = "Number",
                  INPID_InputControl = "SliderControl",
                  INP_MinScale       = {min},
                  INP_MaxScale       = {max},
                  INP_Default        = {default},
              }})"""
      else:
          codeCreate  = f"""
              In{ctrlVar}Slider = self:AddInput("{ctrlVar}", "{ctrlVar}", {{
                  LINKID_DataType    = "Number",
                  INPID_InputControl = "ScrewControl",
                  INP_MinScale       = {min},
                  INP_MaxScale       = {max},
                  INP_Default        = {default},
              }})"""

    elif ctrlType == 'INTSLIDER':

      m = re.match(r'^\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*$',ctrlParams)

      if m == None:
        raise Exception (f"failed to match '{ctrlParams}' as {ctrlType} parameters")

      min, max, default = m.group(1), m.group(2), m.group(3)

      codeParams  = f"int    {ctrlVar};"
      codeKernel  = f"int    {ctrlVar} = params->{ctrlVar};"
      codeProcess = f"params.{ctrlVar} = In{ctrlVar}Slider:GetValue(req).Value"
      codeCreate  = f"""
          In{ctrlVar}Slider = self:AddInput("{ctrlVar}", "{ctrlVar}", {{
              LINKID_DataType    = "Number",
              INPID_InputControl = "SliderControl",
              INP_MinScale       = {min},
              INP_MaxScale       = {max},
              INP_Default        = {default},
              INP_Integer        = true,
          }})"""

    elif ctrlType == 'POINT':

      m = re.match(r'^\s*(-?\d+\.\d+)f\s*,\s*(-?\d+\.\d+)f\s*$',ctrlParams)

      if m == None:
        raise Exception (f"failed to match '{ctrlParams}' as {ctrlType} parameters")

      x, y = m.group(1), m.group(2)

      codeParams  = f"float  {ctrlVar}[2];"
      codeKernel  = f"float2 {ctrlVar} = to_float2(params->{ctrlVar}[0], params->{ctrlVar}[1]);"
      codeProcess = f"params.{ctrlVar} = {{In{ctrlVar}Point:GetValue(req).X,In{ctrlVar}Point:GetValue(req).Y}}"
      codeCreate  = f"""
          In{ctrlVar}Point = self:AddInput("{ctrlVar}", "{ctrlVar}", {{
              LINKID_DataType    = "Point",
              INPID_InputControl = "OffsetControl",
              INPID_PreviewControl  = "CrosshairControl",
              INP_DefaultX          = {x},
              INP_DefaultY          = {y},
          }})"""

    elif ctrlType == 'CHECKBOX':

      m = re.match(r'^\s*(1|true|0|false)\s*$',ctrlParams)

      if m == None:
        raise Exception (f"failed to match '{ctrlParams}' as {ctrlType} parameters")

      default = m.group(1)
      if default=="1" or default=="true":
        default=1
      else:
        default=0

      codeParams  = f"bool   {ctrlVar};"
      codeKernel  = f"bool   {ctrlVar} = params->{ctrlVar};"
      codeProcess = f"params.{ctrlVar} = In{ctrlVar}Checkbox:GetValue(req).Value"
      codeCreate  = f"""
          In{ctrlVar}Checkbox = self:AddInput("{ctrlVar}", "{ctrlVar}", {{
            LINKID_DataType     = "Number",
            INPID_InputControl  = "CheckboxControl",
            INP_Integer         = true,
            INP_Default         = {default},
          }})"""

    elif ctrlType == 'COLOR':

      m = re.match(r'^\s*(-?\d+\.\d+)f\s*,\s*(-?\d+\.\d+)f\s*,\s*(-?\d+\.\d+)f\s*,\s*(-?\d+\.\d+)f\s*$',ctrlParams)

      if m == None:
        raise Exception (f"failed to match '{ctrlParams}' as {ctrlType} parameters")

      r, g, b, a = m.group(1), m.group(2), m.group(3), m.group(4)

      codeParams  = f"float  {ctrlVar}[4];"
      codeKernel  = f"float4 {ctrlVar} = to_float4(params->{ctrlVar}[0], params->{ctrlVar}[1], params->{ctrlVar}[2], params->{ctrlVar}[3]);"
      codeProcess = f"params.{ctrlVar} = {{\n    In{ctrlVar}ColorR:GetValue(req).Value,\n    In{ctrlVar}ColorG:GetValue(req).Value,\n    In{ctrlVar}ColorB:GetValue(req).Value,In{ctrlVar}ColorA:GetValue(req).Value\n  }}"
      codeCreate  = f"""
          self:BeginControlNest("{ctrlVar}", "{ctrlVar}", true, {{}})

            ctrl_grp_cnt = (ctrl_grp_cnt==nil) and 1 or (ctrl_grp_cnt+1)

            attrs = {{
              ICS_Name = "{ctrlVar}",
              LINKID_DataType = "Number",
              INPID_InputControl = "ColorControl",
              INP_MinScale = 0.0,
              INP_MaxScale = 1.0,
              IC_ControlGroup = ctrl_grp_cnt,
            }}

            In{ctrlVar}ColorR = self:AddInput("Red",   "{ctrlVar}Red",   {{ INP_Default  = {r}, IC_ControlID = 0, attrs}})
            In{ctrlVar}ColorG = self:AddInput("Green", "{ctrlVar}Green", {{ INP_Default  = {g}, IC_ControlID = 1, attrs}})
            In{ctrlVar}ColorB = self:AddInput("Blue",  "{ctrlVar}Blue",  {{ INP_Default  = {b}, IC_ControlID = 2, attrs}})
            In{ctrlVar}ColorA = self:AddInput("Alpha", "{ctrlVar}Alpha", {{ INP_Default  = {a}, IC_ControlID = 3, attrs}})

          self:EndControlNest()"""

    elif ctrlType == 'BUTTON':
      #print(f"create {ctrlType} for {ctrlVar} with parameters '{ctrlParams}'")

      m=ctrlParams.split(",")

      if m == None:
        raise Exception (f"failed to match '{ctrlParams}' as {ctrlType} parameters")

      #print("m",m, m[0])

      if (m[0] == "0"):
        _type = ""
      if (m[0] == "1"):
        _type = f""" MBTNC_Type         = "Toggle", """

      buttons = ""
      m.pop(0)
      for i in m:
        buttons += f""" {{ MBTNC_AddButton  = "{i}", }}, """

      codeParams  = f"float  {ctrlVar};"
      codeKernel  = f"float  {ctrlVar} = params->{ctrlVar};"
      codeProcess = f"params.{ctrlVar} = In{ctrlVar}Button:GetValue(req).Value"
      codeCreate  = f"""
          In{ctrlVar}Button = self:AddInput("{ctrlVar}", "{ctrlVar}", {{
              LINKID_DataType    = "Number",
              INPID_InputControl = "MultiButtonControl",
              MBTNC_ForceButtons = true,
        {_type}
              MBTNC_ShowName     = false,
        {buttons}
		      MBTNC_StretchToFit = true,
              IC_NoLabel         = true,
		      INP_Default        = 0,
              IC_Visible         = true,
          }})"""


    else:
      raise Exception(f"unknow resp. not yet implemented contol {ctrlType} for {ctrlVar}")

    codeCreate  = re.sub(r'\n        ',  '\n', codeCreate)
    codeCreate  = re.sub(r'^\s+',        '  ', codeCreate)
    codeParams  = "  "+codeParams
    codeKernel  = "  "+codeKernel
    codeProcess = "  "+codeProcess

    fullCodeCreate  += codeCreate  + "\n\n"
    fullCodeParams  += codeParams  + "\n"
    fullCodeKernel  += codeKernel  + "\n"
    fullCodeProcess += codeProcess + "\n"

  return fullCodeCreate, "\n"+fullCodeProcess, fullCodeKernel+"\n",  fullCodeParams



# ---------------------------------------------------------------------------------------------------------------------

def fuse_it(id,force=False):

  source_path=selfpath+'./Conversions/'+folder+'/'
  target_path=selfpath+'./Converted/'

  conversion_file = get_basefilename(source_path,id)

  verbose(f"process '{conversion_file}'")



  # read .yaml file

  info=None
  with open(f"{source_path}{conversion_file}.{id}.yaml", "r") as f:
    info=yaml.safe_load(f)


  fuse_name     = info['fuse']['name']  # FuReg name ... this is what's displayed in the tool selection menu
  fuse_kernel   = conversion_file       # name of the fragment shader kernel function
  fuse_id       = info['fuse'].get('id',conversion_file) # FuReg ID (must follow identifier syntax)
  fuse_file     = info['fuse'].get('file',fuse_id)  # filename to save the fuse as

  if info['shader']['id'] != id:
    raise Exception("inconsistency in your source and yaml file")



  # write .md file

  write_markdown_file(info, target_file = target_path+fuse_file+".md", force = force )

  # write .sfi file

  write_sfi_file(info, target_file = target_path+fuse_file+".sfi", force = force )

  # read conversion code

  with open(f"{source_path}{conversion_file}.{id}.c", "r") as f:
    conversionCode=f.read()

  conversionCode = re.sub(r'\t','  ',conversionCode)
  conversionCode = re.sub(r' +\n','\n',conversionCode)



  # create compatibility code for this conversion

  compatibilityCode = get_compatibility_code(conversionCode)




  # load the data from yaml file

  with open(selfpath+".clobber/fuse-parameters.yaml", "r") as f:
    src=f.read()

  has_param=yaml.load(src, Loader=yaml.FullLoader)


  # iChannel<0..15> with 15 as the maximum number of input channels supported for the time being

  for i in range(16):
    has_param['iChannel'+str(i)]={ 'Type':'sampler2D', 'Code': '' }


  # do some code indention

  for k,v in has_param.items():
    for f in ['Code','Proc','Create','Param']:
      if v.get(f,None)==None:
        v[f]=''
      elif v[f]!='':
        v[f]='  '+v[f].replace('\n','\n  ')+'\n'



  # patch all kernel signatures

  channelsSeen    = {}
  channelsSeenNum = 0

  for i in range(16):
    channelsSeen['iChannel'+str(i)]=False


  for part in ['__Buffer_A','__Buffer_B','__Buffer_C','__Buffer_D','']:

    for k,v in has_param.items():
      v['Present'] = False

    conversionCode = patch_kernel(conversionCode,fuse_kernel,has_param,part)

    for i in range(16):
      channel='iChannel'+str(i)
      if has_param[channel]['Present']:
        channelsSeen[channel]=True
        if i+1>channelsSeenNum:
          channelsSeenNum=i+1


  ichan_create    = ''
  ichan_process1  = ''
  ichan_process2  = ''
  ichan_process2_time  = ''
  ichan_process2_res   = ''
  ichan_process3  = ''
  j=0 # how to handle gaps when it comes to LINK_Main?!?
  for i in range(channelsSeenNum):
    channel='iChannel'+str(i)
    if channelsSeen[channel]:
      j+=1
      ichan_create   += f'  InChannel{i} = self:AddInput( "iChannel{i}",  "iChannel{i}",  {{ LINKID_DataType = "Image", LINK_Main = {j}, INP_Required = false  }})\n'
      ichan_process1 += f'  iChannel{i} = InChannel{i}:GetValue(req)\n\n  if iChannel{i}==nil then\n    iChannel{i} = Image(imgattrs)\n    iChannel{i}:Fill(black)\n  end\n\n'
      ichan_process2_time += f'  params.iChannelTime[{i}] = 0\n'
      ichan_process2_res  += f'  params.iChannelResolution[{i}][0] = iChannel{i}.DataWindow:Width()    -- or maybe: iChannel{i}.Width\n  params.iChannelResolution[{i}][1] = iChannel{i}.DataWindow:Height()   -- or maybe: iChannel{i}.Height\n'
      ichan_process3 += f'  node:AddInput("iChannel{i}",iChannel{i}) -- TODO: add a better channel name\n'
    else:
      ichan_process2_time += f'  params.iChannelTime[{i}] = 0\n'
      ichan_process2_res  += f'  params.iChannelResolution[{i}][0] = 0\n  params.iChannelResolution[{i}][1] = 0\n'

  # if channelsSeenNum==0:
  #   ichan_process2 = f'  params.iChannelTime[0] = 0\n  params.iChannelResolution[0][0] = 0\n  params.iChannelResolution[0][1] = 0\n'
  # else:
  #   ichan_process2 = ichan_process2_time + "\n" + ichan_process2_res



  fuse_param    = ''
  fuse_create   = ''
  fuse_process  = ''




  for n,p in has_param.items():
    if p['Present'] and p['Code']!='':
      fuse_param   +=  p['Param']
      fuse_create  +=  p['Create']
      fuse_process +=  p['Proc']


  with open(selfpath+".clobber/fuse-template.lua", "r") as f:
    fuse=f.read()


  # now we need to equip the fuse with all the controls

  create, process, kernel, params = check_controls(conversionCode)

  # remove all controls from source

  conversionCode = re.sub(r' +CONNECT_[A-Z]+\d+\s*\(\s*[A-Za-z_][\w]*\s*,\s*[^\)]+\)\s*; *\n', '', conversionCode)


  fuse_param   += params
  fuse_create  += create
  fuse_process += process

  #print(create)
  #print(params)
  #print(kernel)
  #print(process)

  conversionCode = re.sub(r'<<CODEFROMCONTROLS>>',kernel,conversionCode)


  # load some author information from yaml file

  fuse_author_url   = ''
  fuse_author_logo  = ''

  with open(selfpath+".clobber/fuse-authors.yaml", "r") as f:
    tmp=f.read()

  tmp=yaml.load(tmp, Loader=yaml.FullLoader)

  for author in tmp:
    if author['Name']==info['fuse']['author']:
      fuse_author_url   = author['URL']
      fuse_author_logo  = author['Logo']




  fuse=re.sub(r'<<<SHADER_ID>>>',info['shader']['id'],fuse)
  fuse=re.sub(r'<<<SHADER_AUTHOR>>>',info['shader']['author'],fuse)
  fuse=re.sub(r'<<<SHADER_NAME>>>',info['shader']['name'],fuse)
  fuse=re.sub(r'<<<FUSE_NAME>>>',info['fuse']['name'],fuse)
  fuse=re.sub(r'<<<FUSE_AUTHOR>>>',info['fuse']['author'],fuse)
  fuse=re.sub(r'<<<FUSE_CREATION_DATE>>>',date.today().strftime("%B %d, %Y"),fuse)
  fuse=re.sub(r'<<<FUSE_AUTHOR_URL>>>',fuse_author_url,fuse)
  fuse=re.sub(r'<<<FUSE_AUTHOR_LOGO>>>',fuse_author_logo,fuse)

  fuse=re.sub(r'<<<CODE_COMPATIBILITY>>>',compatibilityCode,fuse)
  fuse=re.sub(r'<<<CODE_KERNEL>>>',conversionCode,fuse)
  fuse=re.sub(r'<<<CODE_PARAMETERS>>>',"\n"+fuse_param,fuse)
  fuse=re.sub(r'<<<CODE_CREATE>>>',fuse_create,fuse)
  fuse=re.sub(r'<<<CODE_PROCESS>>> *\n',fuse_process,fuse)

  fuse=re.sub(r'<<<ICHANNELS_CREATE>>>',ichan_create,fuse)
  fuse=re.sub(r'<<<ICHANNELS_PROCESS1>>> *\n',ichan_process1,fuse)
  fuse=re.sub(r'<<<ICHANNELS_PROCESS2>>> *\n',ichan_process2,fuse)
  fuse=re.sub(r'<<<ICHANNELS_PROCESS3>>> *\n',ichan_process3,fuse)



  # nur zum mal gucken wo wir stehen:
  with open(target_path+fuse_file+".fuse", 'w') as f:
    f.write(fuse)



print("\n#################### Fuse Script ###################")
#print("Aufruf :",sys.argv[0]) #ok

if (sys.argv[0] != "fuse.py" and sys.argv[0] != "./fuse.py" ):
 selfpath = os.path.dirname(sys.argv[0])+"\\"

 print("#SELFPATH:",selfpath)
 print(sys.argv, len(sys.argv))
 #print("##Argv2##",id,param,txt)

 print("Folder: ",folder)

 CONVERSIONS_PATH = selfpath+"\Conversions\\"+folder+"\\"

 #VERBOSE   = verbose
 ID = id
 FORCE = pforce
else:
 selfpath = ""
 folder   = ""

 parser = argparse.ArgumentParser(description='Encapsulate conversion in a Fuse.')
 parser.add_argument('-f','--force',action='store_true',help='overwrite fuse and/or markdown file even if it already exists')
 parser.add_argument('-v','--verbose',action='store_true',help="tell what's happening")
 parser.add_argument('-i','--id', help="shadertoy id as it can be found in the conversion's filename right before the'.c' suffix", required=True)
 args = parser.parse_args()

 VERBOSE=args.verbose
 FORCE  =args.force
 ID     =args.id


load_dotenv(selfpath+".env")

#fuse_it(args.id,force=args.force)
fuse_it(ID,force=FORCE)

# try:
#   fuse_it(args.id)
# except Exception as e:
#   print("outch: "+str(e))
